#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses/ncurses.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>
#include "clinit.h"
#include "msgq.h"

#define MAX_MSG_LEN 55

typedef struct WIN_PARAMS_struct {
	int bx, by;
	int h, w;
	int attrs_usual, attrs_highline;
} WIN_PARAMS;

#define MSGSZ (MAX_MSG_LEN + MAX_NICKNAME_LEN + 3) 

typedef struct msgbuf {
	long mtype;
	char mtext[MSGSZ];
} msgbuf_t;

#define MTYPE_REGULAR 2L
#define MTYPE_NICKNAME 1L


// For threads (same):
typedef struct msg_rcv_in_data_t_struct {
	int msgid;
        char **msgs;		// We must not modify it outside the thread, or we need mutex
	int linescount;
	int msglen;
	WINDOW* win;		// Some trouble may be, if we work with window in the thread
	WIN_PARAMS win_p;
} msg_rcv_in_data_t;

//Mutex to work wit allmsg
pthread_mutex_t msg_mutex;
//Mutex to work with nicknames
pthread_mutex_t nick_mutex;

void destroy_win(WINDOW *local_win);
void init_windows_params(WIN_PARAMS *std_win_p, WIN_PARAMS *allmsg_win_p, WIN_PARAMS *members_win_p, WIN_PARAMS *usermsg_win_p);
void init_windows(WIN_PARAMS std_win_p, WIN_PARAMS allmsg_win_p, WIN_PARAMS members_win_p, WIN_PARAMS usermsg_win_p,
                  WINDOW **allmsg_win, WINDOW **members_win, WINDOW **usermsg_win);
void clearWin(WIN_PARAMS win_p, WINDOW *win);
void refresh_windows(WIN_PARAMS std_win_p, WIN_PARAMS allmsg_win_p, WIN_PARAMS members_win_p, WIN_PARAMS usermsg_win_p,
                     WINDOW *allmsg_win, WINDOW *members_win, WINDOW *usermsg_win,
                     char **allmsg, int allmsg_lines_count,
		     char **nicknames,
                     char *usermsg);
void initNC();
int sendmessage(int msgid, const char* nickname, const char* msg);
int sendnickname(int msgid, const char* nickname);
void *regular_messages_reciever_work(void *args);
void *nickname_messages_reciever_work(void *args);

int main(int argc, char **argv)
{
	// Initialization from launch parameters "argv"
	CLINIT_PARAMS lp;
	if (clinit(argc, argv, &lp) != 0) {
		fprintf(stderr, "main(): fatal error on \"clinit()\" function\n");
		exit(EXIT_FAILURE);
	}

	// Print launch parameters
	print_clinit_params(lp);

	int msgid = (lp.i_am_server == 1) ? start_server() : start_client();
	if (msgid < 0) {
		fprintf(stderr, "Cannot start msg\n");
		exit(EXIT_FAILURE);
	}
/*
{ // Test
	sendmessage(msgid, lp.nickname, "Hello World");

	msgbuf_t rbuf;
	if (msgrcv(msgid, &rbuf, MSGSZ, 1, 0) < 0) {
        	perror("msgrcv");
	        exit(EXIT_FAILURE);
	} else {
        	printf(rbuf.mtext);
		printf("\n");
	}

	return 0;
}
*/

	// NCurses initialization
	initNC();

	// Windows defining
	WINDOW *allmsg_win, *members_win, *usermsg_win;
	WIN_PARAMS std_win_p, allmsg_win_p, members_win_p, usermsg_win_p;

	// Windows initializing
	init_windows_params(&std_win_p, &allmsg_win_p, &members_win_p, &usermsg_win_p);

	init_windows(std_win_p, allmsg_win_p, members_win_p, usermsg_win_p, &allmsg_win, &members_win, &usermsg_win);

	// Global parameters initializing
	int usermsg_length = MAX_MSG_LEN;
	char* usermsg = (char *)malloc((usermsg_length + 1) * sizeof(char));
	memset(usermsg, 0, sizeof(char) * (usermsg_length + 1));

	int allmsg_lines_count = allmsg_win_p.h;
	int allmsg_lines_length = MAX_MSG_LEN + MAX_NICKNAME_LEN + 2;

	char** allmsg = (char **)malloc(allmsg_lines_count * sizeof(char*));
	int i;
	for (i = 0; i < allmsg_lines_count; i++) {
		allmsg[i] = (char *)malloc((allmsg_lines_length + 1) * sizeof(char));
		memset(allmsg[i], (int)'-', sizeof(char) * allmsg_lines_length);
		allmsg[i][allmsg_lines_length] = '\0';
	}

	char** nicknames = (char **)malloc(allmsg_lines_count * sizeof(char*));
        for (i = 0; i < allmsg_lines_count; i++) {
                nicknames[i] = (char *)malloc((MAX_NICKNAME_LEN + 1) * sizeof(char));
                memset(nicknames[i], (int)' ', sizeof(char) * MAX_NICKNAME_LEN);
                nicknames[i][MAX_NICKNAME_LEN] = '\0';
        }

	// Thread for regular messages init
        msg_rcv_in_data_t msg_rcv_in;
	msg_rcv_in.msgid = msgid;
	msg_rcv_in.msgs = allmsg;
        msg_rcv_in.linescount = allmsg_lines_count;
        msg_rcv_in.msglen = allmsg_lines_length;
	msg_rcv_in.win = allmsg_win;       
	msg_rcv_in.win_p = allmsg_win_p;
	pthread_mutex_init(&msg_mutex, NULL);

	pthread_t msg_rcv_thread;
	pthread_attr_t msg_rcv_attr;

	if (pthread_attr_init(&msg_rcv_attr) != 0) {
                perror("pthread_attr_init");
                exit(EXIT_FAILURE);
        }
        pthread_attr_setdetachstate(&msg_rcv_attr, PTHREAD_CREATE_JOINABLE);

        if (pthread_create(&msg_rcv_thread, &msg_rcv_attr, &regular_messages_reciever_work, &msg_rcv_in) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
        }

	// Thread for nicknames init
        msg_rcv_in_data_t nick_rcv_in;
        nick_rcv_in.msgid = msgid;
        nick_rcv_in.msgs = nicknames;
        nick_rcv_in.linescount = allmsg_lines_count;
        nick_rcv_in.msglen = MAX_NICKNAME_LEN;
	nick_rcv_in.win = members_win;
        nick_rcv_in.win_p = members_win_p;
	pthread_mutex_init(&nick_mutex, NULL);

        pthread_t nick_rcv_thread;
        pthread_attr_t nick_rcv_attr;

        if (pthread_attr_init(&nick_rcv_attr) != 0) {
                perror("pthread_attr_init");
                exit(EXIT_FAILURE);
        }
        pthread_attr_setdetachstate(&nick_rcv_attr, PTHREAD_CREATE_JOINABLE);

        if (pthread_create(&nick_rcv_thread, &nick_rcv_attr, &nickname_messages_reciever_work, &nick_rcv_in) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
        }

        // Send my nickname
	sendnickname(msgid, lp.nickname);

	// Windows first refreshing to show default data
	pthread_mutex_lock(&msg_mutex);
	pthread_mutex_lock(&nick_mutex);

	refresh_windows(std_win_p, allmsg_win_p, members_win_p, usermsg_win_p, allmsg_win, members_win, usermsg_win, allmsg, allmsg_lines_count, nicknames, usermsg);

	pthread_mutex_unlock(&msg_mutex);
	pthread_mutex_unlock(&nick_mutex);

	while (1)
	{
		// Getting current working dir and files
                int ch = getch();

		pthread_mutex_lock(&msg_mutex);
		pthread_mutex_lock(&nick_mutex);

                switch(ch)
                {
                        case (int)'\n': // enter
				sendmessage(msgid, lp.nickname, usermsg);

				memset(usermsg, 0, sizeof(char) * MAX_MSG_LEN);

                                break; 
                        case 127: // backspace
                                if (strlen(usermsg) > 0) {
					usermsg[strlen(usermsg) - 1] = '\0';
				}
                                
                                break;
			case KEY_F(10):
				{
				// close thread, because it is usually blocked
			        if (pthread_cancel(msg_rcv_thread) != 0) {
                			perror("pthread_cancel");
			                exit(EXIT_FAILURE);
			        }
				/*void *thread_out_data;
				if (pthread_join(msg_rcv_thread, &thread_out_data) != 0) {
					perror("pthread_join");
                        		exit(EXIT_FAILURE);
                		}*/
                                endwin();

         			if (lp.i_am_server == 1) close_connection(msgid);

				free(usermsg);

				for (i = 0; i < allmsg_lines_count; i++) {
                			free(allmsg[i]);
				}
				free(allmsg);

				for (i = 0; i < allmsg_lines_count; i++) {
                                        free(nicknames[i]);
                                }
                                free(nicknames);

                                exit(0);
				}
                        default:
			{
				//TODO not worked properly
				if (isprint(ch) != 0) {
					if (strlen(usermsg) < MAX_MSG_LEN) {
						usermsg[strlen(usermsg)] = (char)ch;
					}
				}
			}
                }

		refresh_windows(std_win_p, allmsg_win_p, members_win_p, usermsg_win_p, allmsg_win, members_win, usermsg_win, allmsg, allmsg_lines_count, nicknames, usermsg);

		pthread_mutex_unlock(&msg_mutex);
		pthread_mutex_unlock(&nick_mutex);
	}

	// close thread, because it is usually blocked
	if (pthread_cancel(msg_rcv_thread) != 0) {
		perror("pthread_cancel");
		exit(EXIT_FAILURE);
	}
	/*void *thread_out_data;
	if (pthread_join(msg_rcv_thread, &thread_out_data) != 0) {
        	perror("pthread_join");
                exit(EXIT_FAILURE);
	}*/
	// End ncurses mode
	endwin();
	// End IPC connection
	if (lp.i_am_server == 1) close_connection(msgid);
	// Free memory
	free(usermsg);

        for (i = 0; i < allmsg_lines_count; i++) {
        	free(allmsg[i]);
	}
	free(allmsg);

	for (i = 0; i < allmsg_lines_count; i++) {
		free(nicknames[i]);
	}
        free(nicknames);

	exit(EXIT_SUCCESS);
}

void destroy_win(WINDOW *local_win)
{	
	wrefresh(local_win);
	delwin(local_win);
}

void init_windows_params(WIN_PARAMS *std_win_p, WIN_PARAMS *allmsg_win_p, WIN_PARAMS *members_win_p, WIN_PARAMS *usermsg_win_p)
{
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
        init_pair(2, COLOR_WHITE, COLOR_GREEN);
	init_pair(3, COLOR_WHITE, COLOR_CYAN);
	init_pair(4, COLOR_WHITE, COLOR_RED);

	(*std_win_p).bx = 0;
	(*std_win_p).by = 0;
	getmaxyx(stdscr, (*std_win_p).h, (*std_win_p).w);
	(*std_win_p).attrs_usual = COLOR_PAIR(3);
        (*std_win_p).attrs_highline = COLOR_PAIR(4);

	const int usermsg_win_height = 1;
	const int allmsg_win_width = (*std_win_p).w - (*std_win_p).w / 5;

	(*allmsg_win_p).bx = 1;
        (*allmsg_win_p).by = 1;
        (*allmsg_win_p).h = (*std_win_p).h - usermsg_win_height - 4;
        (*allmsg_win_p).w = allmsg_win_width;
        (*allmsg_win_p).attrs_usual = COLOR_PAIR(1);
        (*allmsg_win_p).attrs_highline = COLOR_PAIR(2);

	(*usermsg_win_p).h = usermsg_win_height;
        (*usermsg_win_p).w = (*allmsg_win_p).w;
        (*usermsg_win_p).bx = 1;
        (*usermsg_win_p).by = (*allmsg_win_p).h + 2;
        (*usermsg_win_p).attrs_usual = COLOR_PAIR(1);
        (*usermsg_win_p).attrs_highline = COLOR_PAIR(2);

        (*members_win_p).by = 1;
        (*members_win_p).h = (*std_win_p).h - 3;
        (*members_win_p).w = (*std_win_p).w - allmsg_win_width - 3;
	(*members_win_p).bx = (*allmsg_win_p).w + 2;
        (*members_win_p).attrs_usual = COLOR_PAIR(1);
        (*members_win_p).attrs_highline = COLOR_PAIR(2);
}

void init_windows(WIN_PARAMS std_win_p, WIN_PARAMS allmsg_win_p, WIN_PARAMS members_win_p, WIN_PARAMS usermsg_win_p,
                  WINDOW **allmsg_win, WINDOW **members_win, WINDOW **usermsg_win)
{
	int i, j;

	// Main window (background)
	attron(std_win_p.attrs_usual);
        for (i = 0; i < std_win_p.h; i++)
        for (j = 0; j < std_win_p.w; j++)
                mvaddch(i, j, ' ');
        move(0, 0);

	// All messages list window
	(*allmsg_win) = newwin(allmsg_win_p.h, allmsg_win_p.w, allmsg_win_p.by, allmsg_win_p.bx);

	wattron(*allmsg_win, allmsg_win_p.attrs_usual);
	for (i = 0; i < allmsg_win_p.h; i++)
        for (j = 0; j < allmsg_win_p.w; j++)
                mvwaddch(*allmsg_win, i, j, ' ');
        wmove(*allmsg_win, 0, 0);

	// Members list window
        (*members_win) = newwin(members_win_p.h, members_win_p.w, members_win_p.by, members_win_p.bx);

        wattron(*members_win, members_win_p.attrs_usual);
        for (i = 0; i < members_win_p.h; i++)
        for (j = 0; j < members_win_p.w; j++)
                mvwaddch(*members_win, i, j, ' ');
        wmove(*members_win, 0, 0);

	// Bottom window
        (*usermsg_win) = newwin(usermsg_win_p.h, usermsg_win_p.w, usermsg_win_p.by, usermsg_win_p.bx);

        wattron(*usermsg_win, usermsg_win_p.attrs_usual);
        for (i = 0; i < usermsg_win_p.h; i++)
        for (j = 0; j < usermsg_win_p.w; j++)
                mvwaddch(*usermsg_win, i, j, ' ');
        wmove(*usermsg_win, 0, 0);

	refresh();
	wrefresh(*allmsg_win);
	wrefresh(*members_win);
	wrefresh(*usermsg_win);
}

void clearWin(WIN_PARAMS win_p, WINDOW *win)
{
	int i, j;

	wattron(win, win_p.attrs_usual);
        for (i = 0; i < win_p.h; i++)
        for (j = 0; j < win_p.w; j++)
                mvwaddch(win, i, j, ' ');
        wmove(win, 0, 0);
}

void refresh_windows(WIN_PARAMS std_win_p, WIN_PARAMS allmsg_win_p, WIN_PARAMS members_win_p, WIN_PARAMS usermsg_win_p,
                     WINDOW *allmsg_win, WINDOW *members_win, WINDOW *usermsg_win,
                     char **allmsg, int allmsg_lines_count,
		     char **nicknames,
                     char *usermsg)
{
	// Some details on main window
        clearWin(std_win_p, stdscr);
        attron(std_win_p.attrs_highline);
        mvprintw(std_win_p.h - 1, 1, "SEND MESSAGE - ENTER, EXIT - F10");
        attron(std_win_p.attrs_usual);
	mvprintw(allmsg_win_p.h + 1, 1, "YOUR MESSAGE"); //TODO user name
	mvprintw(0, 1, "MQCHAT MESSAGES");
	mvprintw(0, allmsg_win_p.w + 2, "MQCHAT MEMBERS");
	refresh();

	// All messages window
	clearWin(allmsg_win_p, allmsg_win);
	attron(allmsg_win_p.attrs_usual);
	int i;
        for (i = 0; i < allmsg_lines_count; i++) {
                mvwprintw(allmsg_win, i, 0, allmsg[i]);
        }

	wrefresh(allmsg_win);

	// Members list window
	clearWin(members_win_p, members_win);
        attron(members_win_p.attrs_usual);
        for (i = 0; i < allmsg_lines_count; i++) {
                mvwprintw(members_win, i, 0, nicknames[i]);
        }
        wrefresh(members_win);

	// User message window
	clearWin(usermsg_win_p, usermsg_win);
        attron(usermsg_win_p.attrs_usual);
	mvwprintw(usermsg_win, 0, 0, usermsg);
	wmove(usermsg_win, 0, strlen(usermsg));
        curs_set(2);

        wrefresh(usermsg_win);
}
void initNC()
{
        // Start curses mode
        initscr();

        // Start the color functionality
        start_color();

        // Line buffering disabled
        cbreak();

        // Disabling echoing
        noecho();

        // Enabling F1..., arrows
        keypad(stdscr, TRUE);

        // Hide cursor
        curs_set(0);
}

int sendmessage(int msgid, const char* nickname, const char* msg)
{
	msgbuf_t buf;

	buf.mtype = MTYPE_REGULAR;

	memset(buf.mtext, (int)' ', sizeof(char) * MAX_NICKNAME_LEN);                    // for nickname
	memset(&buf.mtext[MAX_NICKNAME_LEN + 2], (int)'-', sizeof(char) * MAX_MSG_LEN);  // for message
	buf.mtext[MSGSZ - 1] = '\0';
	memcpy(buf.mtext, nickname, strlen(nickname));
	buf.mtext[MAX_NICKNAME_LEN] = ':';
	buf.mtext[MAX_NICKNAME_LEN + 1] = ' ';
	memcpy(&buf.mtext[MAX_NICKNAME_LEN + 2], msg, strlen(msg));

	if (msgsnd(msgid, &buf, MSGSZ * sizeof(char), IPC_NOWAIT) < 0) {
        	perror("msgsnd error");
        	return -1;
	}

	return 0;
}

int sendnickname(int msgid, const char* nickname)
{
        msgbuf_t buf;

        buf.mtype = MTYPE_NICKNAME;

	memset(buf.mtext, (int)' ', sizeof(char) * MAX_NICKNAME_LEN);
	buf.mtext[MAX_NICKNAME_LEN] = '\0';
	memcpy(buf.mtext, nickname, strlen(nickname));

        if (msgsnd(msgid, &buf, MSGSZ * sizeof(char), IPC_NOWAIT) < 0) {
                perror("msgsnd error");
                return -1;
        }

        return 0;
}

void *regular_messages_reciever_work(void *args)
{
	msg_rcv_in_data_t *data = (msg_rcv_in_data_t *)args;

	for ( ; ; ) {
		msgbuf_t rbuf;
        	if (msgrcv(data->msgid, &rbuf, MSGSZ, MTYPE_REGULAR, 0) < 0) {
                	perror("msgrcv");
                	exit(EXIT_FAILURE);
        	}
	
		pthread_mutex_lock(&msg_mutex);
	
		int i;
		for (i = 0; i < (data->linescount - 1); i++)
			memcpy(data->msgs[i],
                               data->msgs[i + 1],
			       sizeof(char) * data->msglen);		
		memcpy(data->msgs[data->linescount - 1], rbuf.mtext, sizeof(char) * strlen(rbuf.mtext));

		// Refresh window
        	clearWin(data->win_p, data->win);
        	attron(data->win_p.attrs_usual);
        	
        	for (i = 0; i < data->linescount; i++) {
                	mvwprintw(data->win, i, 0, data->msgs[i]);
        	}

        	wrefresh(data->win);

		pthread_mutex_unlock(&msg_mutex);
	}	

	return NULL;	
}

void *nickname_messages_reciever_work(void *args)
{
	msg_rcv_in_data_t *data = (msg_rcv_in_data_t *)args;

	for ( ; ; ) {
                msgbuf_t rbuf;
                if (msgrcv(data->msgid, &rbuf, MSGSZ, MTYPE_NICKNAME, 0) < 0) {
                        perror("msgrcv");
                        exit(EXIT_FAILURE);
                }

		pthread_mutex_lock(&nick_mutex);

		int i;
                for (i = (data->linescount - 1); i > 0; i--)
                        memcpy(data->msgs[i],
                               data->msgs[i - 1],
                               sizeof(char) * data->msglen);

                memcpy(data->msgs[0], rbuf.mtext, sizeof(char) * strlen(rbuf.mtext));

		// Refresh window
                clearWin(data->win_p, data->win);
                attron(data->win_p.attrs_usual);

                for (i = 0; i < data->linescount; i++) {
                        mvwprintw(data->win, i, 0, data->msgs[i]);
                }

                wrefresh(data->win);

		pthread_mutex_unlock(&nick_mutex);
        }

	return NULL;
}
