#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses/ncurses.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "clinit.h"

#define MAX_MSG_LEN 55
#define MAX_USER_COUNT 10
/** Shared memory segment size in bytes = mem for messages and one int - count of members */
#define SHM_MAX_LEN (((MAX_MSG_LEN + MAX_NICKNAME_LEN + 3) * sizeof(char) + sizeof(int)) * MAX_USER_COUNT + sizeof(int))

typedef struct WIN_PARAMS_struct {
	int bx, by;
	int h, w;
	int attrs_usual, attrs_highline;
} WIN_PARAMS;

// For thread msg_rcv:
typedef struct msg_rcv_in_data_t_struct {
	char *shmdata;
	int semid;
	int my_id;
        char **allmsg;
	char *usermsg;
	char **nicknames; 
	int linescount;
	int allmsg_len;
	WINDOW *allmsg_win, *members_win, *usermsg_win;
	WIN_PARAMS std_win_p, allmsg_win_p, members_win_p, usermsg_win_p;
} msg_rcv_in_data_t;

//Mutex to refresh windows, changing messages list
pthread_mutex_t main_mutex;

union semun {
	int val;    /* Value for SETVAL */
	struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
	unsigned short  *array;  /* Array for GETALL, SETALL */
	struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                   (Linux specific) */
};

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

int atomic_get_and_inc_members_count(char *shmdata);
void getMessage(char *shmdata, int curr_id, int my_id, char *buf, int semid);
void setMessage(char *shmdata, int my_id, char *buf, int semid);

void *messages_reciever_work(void *args);

int main(int argc, char **argv)
{
	// Initialization from launch parameters "argv"
	CLINIT_PARAMS lp;
	if (clinit(argc, argv, &lp) != 0) {
		fprintf(stderr, "main(): fatal error on \"clinit()\" function\n");
		goto error;
	}

	// Print launch parameters
	print_clinit_params(lp);

	// For both client and server - getting the key
	key_t key = ftok("./smchat", (int)'a');
	if (key == (key_t)-1) {
		perror("ftok");
		goto error;
	}

	// Creating shared memory segment

	// For both client and server - the shared memory segment identifier
	int shmid = (lp.i_am_server == 1) ? shmget(key, SHM_MAX_LEN, 0666 | IPC_CREAT) :
					    shmget(key, SHM_MAX_LEN, 0); // we can use IPC_CREAT for client and server
	if (shmid == -1) {
		perror("shmget");
		goto error;
        }

	// For both client and server - getting the pointer to the segment
	char *shmdata = shmat(shmid, (void *)0, 0);
	if (shmdata == (char *)(-1)) {
		perror("shmat");
                goto error;
	}

	//Creating semaphores set

	// create a semaphore set with MAX_USER_COUNT semaphores: 
	int semid = (lp.i_am_server == 1) ? semget(key, MAX_USER_COUNT, 0666 | IPC_CREAT) :
					    semget(key, MAX_USER_COUNT, 0);
	if (semid == -1) {
		perror("semget");
		goto error;
	}

	// initialize semaphores to 1:
	if (lp.i_am_server == 1) {
		union semun arg;
		arg.val = 1;
		int semnum;
		for (semnum = 0; semnum < MAX_USER_COUNT; semnum++) {
			if (semctl(semid, semnum, SETVAL, arg) == -1) {
				perror("semctl");
				goto error;
			}
		}
	}

	// We must place one special message in the shmdata, that holds amout of members
	// We use it to find our id
	int my_id = -1;

	if (lp.i_am_server == 1) {
		my_id = 0;
		memset(shmdata, 0, SHM_MAX_LEN);
		(*(int *)shmdata) = 1; // Firsh bytes = int
	} else {
		my_id = atomic_get_and_inc_members_count(shmdata);
	}

	printf("My id = %d\n", my_id);

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

	memcpy(nicknames[0], lp.nickname, sizeof(char) * strlen(lp.nickname));

	// Thread for regular messages init
        msg_rcv_in_data_t msg_rcv_in;
	msg_rcv_in.shmdata = shmdata;
	msg_rcv_in.semid = semid;
	msg_rcv_in.my_id = my_id;
	msg_rcv_in.allmsg = allmsg;
	msg_rcv_in.usermsg = usermsg;
	msg_rcv_in.nicknames = nicknames;
        msg_rcv_in.linescount = allmsg_lines_count;
        msg_rcv_in.allmsg_len = allmsg_lines_length;
	msg_rcv_in.allmsg_win = allmsg_win;
	msg_rcv_in.members_win = members_win;
	msg_rcv_in.usermsg_win = usermsg_win;
	msg_rcv_in.std_win_p = std_win_p;
	msg_rcv_in.allmsg_win_p = allmsg_win_p;
	msg_rcv_in.members_win_p = members_win_p;
	msg_rcv_in.usermsg_win_p = usermsg_win_p;

	pthread_mutex_init(&main_mutex, NULL);

	pthread_t msg_rcv_thread;
	pthread_attr_t msg_rcv_attr;

	if (pthread_attr_init(&msg_rcv_attr) != 0) {
                perror("pthread_attr_init");
                goto error;
        }
        pthread_attr_setdetachstate(&msg_rcv_attr, PTHREAD_CREATE_JOINABLE);

        if (pthread_create(&msg_rcv_thread, &msg_rcv_attr, &messages_reciever_work, &msg_rcv_in) != 0) {
                perror("pthread_create");
                goto error;
        }

	// Windows first refreshing to show default data
	pthread_mutex_lock(&main_mutex);

	refresh_windows(std_win_p, allmsg_win_p, members_win_p, usermsg_win_p, allmsg_win, members_win, usermsg_win, allmsg, allmsg_lines_count, nicknames, usermsg);

	pthread_mutex_unlock(&main_mutex);

	while (1)
	{
		// Getting current working dir and files
                int ch = getch();

		pthread_mutex_lock(&main_mutex);
		
                switch(ch)
                {
                        case (int)'\n': // enter
			{
				char msgbuf[MAX_MSG_LEN + MAX_NICKNAME_LEN + 3];

				memset(msgbuf, (int)' ', sizeof(char) * MAX_NICKNAME_LEN);                    // for nickname
				memset(&msgbuf[MAX_NICKNAME_LEN + 2], (int)'-', sizeof(char) * MAX_MSG_LEN);  // for message
				msgbuf[MAX_MSG_LEN + MAX_NICKNAME_LEN + 2] = '\0';
				memcpy(msgbuf, lp.nickname, strlen(lp.nickname));
				msgbuf[MAX_NICKNAME_LEN] = ':';
				msgbuf[MAX_NICKNAME_LEN + 1] = ' ';
				memcpy(&msgbuf[MAX_NICKNAME_LEN + 2], usermsg, strlen(usermsg));
				
				setMessage(shmdata, my_id, msgbuf, semid);

				memset(usermsg, 0, sizeof(char) * MAX_MSG_LEN);

                                break; 
			}
                        case 127: // backspace
                                if (strlen(usermsg) > 0) {
					usermsg[strlen(usermsg) - 1] = '\0';
				}
                                
                                break;
			case KEY_F(10):
				goto exit;
				
				break;
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

		pthread_mutex_unlock(&main_mutex);
	}

exit:
	// close thread, not join, because it is usually blocked
	pthread_cancel(msg_rcv_thread);
	// End ncurses mode
	endwin();
	// End IPC shm connection
	if (lp.i_am_server == 1) {
		shmctl(shmid, IPC_RMID, NULL);
	} else {
		shmdt(shmdata);
	}
	// End Semaphore
	if (lp.i_am_server == 1) {
		semctl(semid, 0, IPC_RMID, 0);
	}
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

error:
	endwin();

	if (lp.i_am_server == 1) shmctl(shmid, IPC_RMID, NULL);

	exit(EXIT_FAILURE);
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

void *messages_reciever_work(void *args)
{
	msg_rcv_in_data_t *data = (msg_rcv_in_data_t *)args;
	for ( ; ; )
	{
		pthread_mutex_lock(&main_mutex);

		int rcvd = 0;
		int curr_id;
		for (curr_id = 0; curr_id < MAX_USER_COUNT; curr_id++)
		{
			char rbuf[MAX_MSG_LEN + MAX_NICKNAME_LEN + 3];

			getMessage(data->shmdata, curr_id, data->my_id, rbuf, data->semid);

			if (strlen(rbuf) == 0) {
				//pthread_mutex_unlock(&main_mutex);
				continue;
        		}	
			
			rcvd = 1;

			// Add new message	
			int i;
			for (i = 0; i < (data->linescount - 1); i++)
				memcpy(data->allmsg[i], data->allmsg[i + 1],  sizeof(char) * data->allmsg_len);		
			memcpy(data->allmsg[data->linescount - 1], rbuf, sizeof(char) * strlen(rbuf));

			// Seek the user's nickname in the list
			char sbuf[MAX_NICKNAME_LEN + 1];
			memcpy(sbuf, rbuf, MAX_NICKNAME_LEN * sizeof(char));
			sbuf[MAX_NICKNAME_LEN] = '\0';		

			for (i = 0; i < data->linescount; i++) {
				if (strcmp(sbuf, data->nicknames[i]) == 0) break;
			}

			// If it is new member, add his nickname to the list
			if (i == data->linescount) {
				for (i = (data->linescount - 1); i > 0; i--)
					memcpy(data->nicknames[i], data->nicknames[i - 1],  sizeof(char) * MAX_NICKNAME_LEN);
	                	memcpy(data->nicknames[0], rbuf, sizeof(char) * MAX_NICKNAME_LEN);
			}
		}

		// Refresh windows
		if (rcvd == 1) {
			refresh_windows(data->std_win_p, data->allmsg_win_p, data->members_win_p, data->usermsg_win_p,
                        	        data->allmsg_win, data->members_win, data->usermsg_win,
                                	data->allmsg, data->linescount, data->nicknames, data->usermsg);
		}

		pthread_mutex_unlock(&main_mutex);
	}	

	return NULL;	
}

int atomic_get_and_inc_members_count(char *shmdata)
{
	int memcnt = *(int *)shmdata;

	(*(int *)shmdata)++;

	return memcnt;
}

int getFlagFromId(int id) {
	int flag = 1;

	if (id == 0) flag = 1;
	else if (id == 1) flag = 2;
	else if (id == 2) flag = 4;
	//flag = flag << id;

	return flag;
}

void getMessage(char *shmdata, int curr_id, int my_id, char *buf, int semid)
{
	memset(buf, 0, (MAX_MSG_LEN + MAX_NICKNAME_LEN + 3) * sizeof(char));

	// lock
        struct sembuf sb = {curr_id, (short)-1, IPC_NOWAIT};
        if (semop(semid, &sb, 1) == -1) {
                if (errno == EAGAIN) {
			return;
		} else {		
			perror("semop");
	                exit(EXIT_FAILURE);
		}		
        }

	size_t offset = sizeof(int) + (size_t)curr_id * ((MAX_MSG_LEN + MAX_NICKNAME_LEN + 3) * sizeof(char) + sizeof(int));

	int flags = *(int *)((size_t)shmdata + offset);
	
	if ((flags & getFlagFromId(my_id)) == 0) {
		// If I have already read this message
		(*(int *)((size_t)shmdata + offset)) = flags | getFlagFromId(my_id);

		memcpy(buf, (char *)((size_t)shmdata + offset + sizeof(int)), (MAX_MSG_LEN + MAX_NICKNAME_LEN + 3) * sizeof(char));
	}

	// unlock
        sb.sem_op = (short)1;
        if (semop(semid, &sb, 1) == -1) {
                perror("semop");
                exit(EXIT_FAILURE);
        }
}

void setMessage(char *shmdata, int my_id, char *buf, int semid)
{
	// lock
        struct sembuf sb = {(unsigned short)my_id, (short)-1, (short)0};
        if (semop(semid, &sb, 1) == -1) {
                perror("semop");
                exit(EXIT_FAILURE);
        }

        size_t offset = sizeof(int) + (size_t)my_id * ((MAX_MSG_LEN + MAX_NICKNAME_LEN + 3) * sizeof(char) + sizeof(int));

	(*(int *)((size_t)shmdata + offset)) = 0;

        memcpy((char *)((size_t)shmdata + offset + sizeof(int)), buf, (MAX_MSG_LEN + MAX_NICKNAME_LEN + 3) * sizeof(char));

	// unlock
	sb.sem_op = (short)1;
	if (semop(semid, &sb, 1) == -1) {
		perror("semop");
		exit(EXIT_FAILURE);
	}
}

