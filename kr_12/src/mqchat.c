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
#include "clinit.h"

#define MAX_MSG_LEN 55

typedef struct WIN_PARAMS_struct {
	int bx, by;
	int h, w;
	int attrs_usual, attrs_highline;
} WIN_PARAMS;

void destroy_win(WINDOW *local_win);
void init_windows_params(WIN_PARAMS *std_win_p, WIN_PARAMS *allmsg_win_p, WIN_PARAMS *members_win_p, WIN_PARAMS *usermsg_win_p);
void init_windows(WIN_PARAMS std_win_p, WIN_PARAMS allmsg_win_p, WIN_PARAMS members_win_p, WIN_PARAMS usermsg_win_p,
                  WINDOW **allmsg_win, WINDOW **members_win, WINDOW **usermsg_win);
void clearWin(WIN_PARAMS win_p, WINDOW *win);
void refresh_windows(WIN_PARAMS std_win_p, WIN_PARAMS allmsg_win_p, WIN_PARAMS members_win_p, WIN_PARAMS usermsg_win_p,
                     WINDOW *allmsg_win, WINDOW *members_win, WINDOW *usermsg_win,
                     char *usermsg);
void initNC();

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

	// NCurses initialization
	initNC();

	// Windows defining
	WINDOW *allmsg_win, *members_win, *usermsg_win;
	WIN_PARAMS std_win_p, allmsg_win_p, members_win_p, usermsg_win_p;

	// Windows initializing
	init_windows_params(&std_win_p, &allmsg_win_p, &members_win_p, &usermsg_win_p);

	init_windows(std_win_p, allmsg_win_p, members_win_p, usermsg_win_p, &allmsg_win, &members_win, &usermsg_win);

	// Global parameters initializing
	char usermsg[MAX_MSG_LEN + 1];
	memset(usermsg, 0, sizeof(char) * MAX_MSG_LEN);
	
	// Windows first refreshing to show default data
	refresh_windows(std_win_p, allmsg_win_p, members_win_p, usermsg_win_p, allmsg_win, members_win, usermsg_win, usermsg);

	while (1)
	{
		// Getting current working dir and files
                int ch = getch();
                switch(ch)
                {
                        case (int)'\n': // enter
				memset(usermsg, 0, sizeof(char) * MAX_MSG_LEN);

                                break; 
                        case 127: // backspace
                                if (strlen(usermsg) > 0) {
					usermsg[strlen(usermsg) - 1] = '\0';
				}
                                
                                break;
			case KEY_F(10):
                                endwin();
                                
                                exit(0);
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

		refresh_windows(std_win_p, allmsg_win_p, members_win_p, usermsg_win_p, allmsg_win, members_win, usermsg_win, usermsg);
	}

	// End ncurses mode
	endwin();

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
	attron(std_win_p.attrs_usual);
	//TODO print current messages list
	wrefresh(allmsg_win);

	// Members list window
	clearWin(members_win_p, members_win);
        attron(members_win_p.attrs_usual);
        //TODO print members list
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
/*
char* InputWindow(char* header, WIN_PARAMS main_win_p)
{
	int i, j, curx, cury;
	WINDOW *win;
	WIN_PARAMS win_p;

	win_p = main_win_p;
	win_p.w = main_win_p.w / 3;
	win_p.h = 5;
	win_p.bx = (main_win_p.w - win_p.w) / 2;
	win_p.by = (main_win_p.h - win_p.h) / 2;
	
	win = newwin(win_p.h, win_p.w, win_p.by, win_p.bx);

        wattron(win, win_p.attrs_usual);
        for (i = 0; i < win_p.h; i++)
        for (j = 0; j < win_p.w; j++)
                mvwaddch(win, i, j, ' ');

	mvwprintw(win, 1, 1, header);

        wmove(win, 3, 1);
	curs_set(2);

	wrefresh(win);

	char* res = (char *)malloc(sizeof(char) * (win_p.w - 1));
	memset(res, 0, sizeof(char) * (win_p.w - 1));
	while (1) {
		int ch = getch();
		switch(ch)
		{
while (1) {
                int ch = getch();
                switch(ch)
                {
                        case (int)'\n':
                                break;
                        case 127:
                                getyx(win, cury, curx);
                                mvwaddch(win, cury, curx, ' ');
                                res[curx - 1] = '\0';
                                wmove(win, cury, curx - 1);
                                wrefresh(win);
                                break;
                        default:
                                getyx(win, cury, curx);
                                mvwaddch(win, cury, curx, ch);
                                res[curx - 1] = (char)ch;
                                if (curx >= (win_p.w - 2)) {
                                        wmove(win, cury, curx);
                                }
                                wrefresh(win);
                }

                if (ch == (int)'\n') break;
        }			case (int)'\n':
				break;
			case 127:
				getyx(win, cury, curx);
				mvwaddch(win, cury, curx, ' ');
				res[curx - 1] = '\0';
				wmove(win, cury, curx - 1);
                                wrefresh(win);
				break;
			default:
				getyx(win, cury, curx);
				mvwaddch(win, cury, curx, ch);
				res[curx - 1] = (char)ch;
				if (curx >= (win_p.w - 2)) {
					wmove(win, cury, curx);
				}
				wrefresh(win);
		}

		if (ch == (int)'\n') break;
	}

	return res;
}
*/
