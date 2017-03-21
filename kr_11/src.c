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

#define CHAR_BUF_SIZE 256

typedef struct WIN_PARAMS_struct {
	int bx, by;
	int h, w;
	int attrs_usual, attrs_highline;
} WIN_PARAMS;

//WINDOW *create_newwin(WIN_PARAMS wp);
void destroy_win(WINDOW *local_win);
void init_windows_params(WIN_PARAMS *std_win_p, WIN_PARAMS *left_win_p, WIN_PARAMS *right_win_p, WIN_PARAMS *bot_win_p);
void init_windows(WIN_PARAMS std_win_p, WIN_PARAMS left_win_p, WIN_PARAMS right_win_p, WIN_PARAMS bot_win_p,
                  WINDOW **left_win, WINDOW **right_win, WINDOW **bot_win);
void get_CWD_SUBDIRS(char* cwd, struct dirent ***dirs, int *dirs_cnt);
void clearWin(WIN_PARAMS win_p, WINDOW *win);
int OpenFile(char *fname, char **file_map, long *file_map_size);
int CloseFile(char **file_map, long *file_map_size);
void ExecuteFile(char *fname);
void initNC();
char* InputWindow(char* header, WIN_PARAMS main_win_p);
int PipedExecuteFile4(char *pname1, char **ppar1, char *pname2, char **ppar2);
int PipedExecuteFile2(char *p1, char *p2);

int main(int argc, char **argv)
{
	WINDOW *left_win, *right_win, *bot_win;
	WIN_PARAMS std_win_p, left_win_p, right_win_p, bot_win_p;

	initNC();

	// Creating windows
	init_windows_params(&std_win_p, &left_win_p, &right_win_p, &bot_win_p);
	init_windows(std_win_p, left_win_p, right_win_p, bot_win_p, &left_win, &right_win, &bot_win);

	// Some details on main window
	clearWin(std_win_p, stdscr);
	attron(std_win_p.attrs_highline);
	mvprintw(std_win_p.h - 1, 1, "SAVE - F6, RUN - F7, PIPED RUN - F8, EXIT - F10");
	attron(std_win_p.attrs_usual);
	refresh();

	// The external parameter - current position in the list
	int selected_item = 0;

	// If file opened in the editor. 1 - opened, 0 - not opened
	int file_opened = 0;
	// For the file 
	char *file_map = NULL;
	long file_map_size = 0;

	while (1)
	{
		// Getting current working dir and files
		struct dirent **dirs;
		int dirs_cnt;
		char cwd[CHAR_BUF_SIZE + 1];

		memset(cwd, 0, sizeof(char) * (CHAR_BUF_SIZE + 1));
		get_CWD_SUBDIRS(cwd, &dirs, &dirs_cnt);

		clearWin(left_win_p, left_win);
		mvwprintw(left_win, 0, 0, cwd);

		int i;
		for (i = 0; i < dirs_cnt; i++)
			mvwprintw(left_win, i + 1, 1, dirs[i]->d_name);

		mvwchgat(left_win, selected_item + 1, 0, -1, A_BLINK, 1, NULL);

		wrefresh(left_win);

		// Getting selecetd file's properties via stat function
		char buf[CHAR_BUF_SIZE + 1];
		struct stat cur_stat;

		memset(buf, 0, sizeof(char) * (CHAR_BUF_SIZE + 1));		
		sprintf(buf, "%s/%s", cwd, dirs[selected_item]->d_name);

		stat(buf, &cur_stat);

		clearWin(bot_win_p, bot_win);
		if ((cur_stat.st_mode & S_IFMT) == S_IFDIR) { // Directory
			mvwprintw(bot_win, 0, 0, "Directory");
		}
		else if ((cur_stat.st_mode & S_IFMT) == S_IFREG) { // Regular file
			mvwprintw(bot_win, 0, 0, "Regular file");
			mvwprintw(bot_win, 1, 0, "Size = %ld bytes", cur_stat.st_size);
		}
		else {
			mvwprintw(bot_win, 0, 0, "Not a file or directory");
		}

		wrefresh(bot_win);

		if (file_opened == 0) {
			clearWin(right_win_p, right_win);
			wrefresh(right_win);
		}
		else {
			int curx, cury;
			getyx(right_win, cury, curx);
			clearWin(right_win_p, right_win);
			mvwprintw(right_win, 0, 0, "%s", file_map);
			wmove(right_win, cury, curx++);
			wrefresh(right_win);
		}

		int ch = getch();
		switch(ch)
                {
                        case KEY_DOWN:
				if (file_opened == 0) {
					mvwchgat(left_win, selected_item + 1, 0, -1, A_NORMAL, 1, NULL);
					selected_item = (selected_item + 1) % dirs_cnt;
				}
				else {
                                        int curx, cury;
                                        getyx(right_win, cury, curx);
                                        wmove(right_win, cury + 1, curx);
                                }
				break;
                        case KEY_UP:
				if (file_opened == 0) {
					mvwchgat(left_win, selected_item + 1, 0, -1, A_NORMAL, 1, NULL);
                                	selected_item = (dirs_cnt + selected_item - 1) % dirs_cnt;
				}
				else {
                                        int curx, cury;
                                        getyx(right_win, cury, curx);
                                        wmove(right_win, cury - 1, curx);
                                }
				break;
			case KEY_RIGHT:
				if (file_opened == 1) {
					int curx, cury;
					getyx(right_win, cury, curx);
					wmove(right_win, cury, curx + 1);
				}
				break;
			case KEY_LEFT:
                                if (file_opened == 1) {
                                        int curx, cury;
                                        getyx(right_win, cury, curx);
                                        wmove(right_win, cury, curx - 1);
                                }
                                break;
			case (int)'\n':
				if (file_opened == 0) {
					if ((cur_stat.st_mode & S_IFMT) == S_IFDIR) {
						memset(buf, 0, sizeof(char) * (CHAR_BUF_SIZE + 1));
						sprintf(buf, "%s/%s", cwd, dirs[selected_item]->d_name);

						chdir(buf);
						selected_item = 0;
					}
					else if ((cur_stat.st_mode & S_IFMT) == S_IFREG) {
						memset(buf, 0, sizeof(char) * (CHAR_BUF_SIZE + 1));
                        	                sprintf(buf, "%s/%s", cwd, dirs[selected_item]->d_name);

						if (OpenFile(buf, &file_map, &file_map_size) == 0) {
							file_opened = 1;
							curs_set(2);
							wmove(right_win, 0, 0);
							wrefresh(right_win);
						}
					}
				}
				break;
			case KEY_F(6):
				if (file_opened == 1) {
					CloseFile(&file_map, &file_map_size);
					file_opened = 0;
					curs_set(0);
				}
				break;
			case KEY_F(7):
				if ((cur_stat.st_mode & S_IFMT) == S_IFREG) {
					//TODO: check, if we can execute
					memset(buf, 0, sizeof(char) * (CHAR_BUF_SIZE + 1));
					sprintf(buf, "%s/%s", cwd, dirs[selected_item]->d_name);

					endwin();

					ExecuteFile(buf);

					initNC();
				}
				break;
			case KEY_F(8):
			{
				char *p1 = InputWindow("Type the first program", std_win_p);
				initNC();

				char *p2 = InputWindow("Type the second program", std_win_p);
                                initNC();

				endwin();

				//printf("p1 = %s\np2 = %s\n", p1, p2);
				int retval = PipedExecuteFile2(p1, p2);

				printf("\nProgram \"%s\" and \"%s\" are finished with code = %d\nPress any key and \"Enter\" to continue...\n", p1, p2, retval);

				char buf[256];
				scanf("%s", buf);

				initNC();

				free(p1);
				free(p2);

				break;
			}
			case KEY_F(10):
				if (file_opened == 1) CloseFile(&file_map, &file_map_size);
				endwin();
				free(dirs);

				return 0;
			default:
				if (file_opened == 1) {
					int curx, cury, i;
					getyx(right_win, cury, curx);
					i = cury * right_win_p.w + curx;
					if (i < file_map_size) {
						file_map[i] = (char)ch;
						curx++;
						if (curx >= right_win_p.w) {
							curx = 0;
							cury++;
						}
						wmove(right_win, cury, curx);
					}
				}
                }

		// TODO if I should free it
		//for (i = 0; i < dirs_cnt; i++)
		//      free(dirs[i]);
		free(dirs);
	}

	// End ncurses mode
	endwin();

	return 0;
}

WINDOW *create_newwin(int height, int width, int starty, int startx, int attrs)
{
	WINDOW *local_win;

	local_win = newwin(height, width, starty, startx);

	wattron(local_win, attrs);
	
	mvaddch(starty, startx, '+');
	mvaddch(starty, startx + width, '+');
	mvaddch(starty + height, startx, '+');
	mvaddch(starty + height, startx + width, '+');
	mvhline(starty, startx + 1, '-', width - 1);
	mvhline(starty + height, startx + 1, '-', width - 1);
	mvvline(starty + 1, startx, '|', height - 1);
	mvvline(starty + 1, startx + width, '|', height - 1);

	int i, j;
	for (i = 0; i < height; i++)
	for (j = 0; j < width; j++)
		mvwaddch(local_win, i, j, ' ');	

	wmove(local_win, 0, 0);
	wprintw(local_win, "Hi There !!!");
	wrefresh(local_win);
	

	wattroff(local_win, attrs);

	return local_win;
}

void destroy_win(WINDOW *local_win)
{	
	wrefresh(local_win);
	delwin(local_win);
}

void init_windows_params(WIN_PARAMS *std_win_p, WIN_PARAMS *left_win_p, WIN_PARAMS *right_win_p, WIN_PARAMS *bot_win_p)
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

	(*bot_win_p).h = 3;
        (*bot_win_p).w = (*std_win_p).w - 2;
	(*bot_win_p).bx = 1;
        (*bot_win_p).by = (*std_win_p).h - (*bot_win_p).h - 1;
        (*bot_win_p).attrs_usual = COLOR_PAIR(1);
        (*bot_win_p).attrs_highline = COLOR_PAIR(2);

	(*left_win_p).bx = 1;
        (*left_win_p).by = 1;
	(*left_win_p).h = (*std_win_p).h - (*bot_win_p).h - 3;
	(*left_win_p).w = ((*std_win_p).w - 3) / 2 + ((*std_win_p).w - 3) % 2;
	(*left_win_p).attrs_usual = COLOR_PAIR(1);
        (*left_win_p).attrs_highline = COLOR_PAIR(2);

	(*right_win_p).bx = (*left_win_p).w + 2;
        (*right_win_p).by = 1;
        (*right_win_p).h = (*left_win_p).h;
        (*right_win_p).w = ((*std_win_p).w - 3) / 2;
        (*right_win_p).attrs_usual = COLOR_PAIR(1);
        (*right_win_p).attrs_highline = COLOR_PAIR(2);
}

void init_windows(WIN_PARAMS std_win_p, WIN_PARAMS left_win_p, WIN_PARAMS right_win_p, WIN_PARAMS bot_win_p,
                  WINDOW **left_win, WINDOW **right_win, WINDOW **bot_win)
{
	int i, j;

	// Main window (background)
	attron(std_win_p.attrs_usual);
        for (i = 0; i < std_win_p.h; i++)
        for (j = 0; j < std_win_p.w; j++)
                mvaddch(i, j, ' ');
        move(0, 0);

	// Left window
	(*left_win) = newwin(left_win_p.h, left_win_p.w, left_win_p.by, left_win_p.bx);

	wattron(*left_win, left_win_p.attrs_usual);
	for (i = 0; i < left_win_p.h; i++)
        for (j = 0; j < left_win_p.w; j++)
                mvwaddch(*left_win, i, j, ' ');
        wmove(*left_win, 0, 0);

	// Right window
        (*right_win) = newwin(right_win_p.h, right_win_p.w, right_win_p.by, right_win_p.bx);

        wattron(*right_win, right_win_p.attrs_usual);
        for (i = 0; i < right_win_p.h; i++)
        for (j = 0; j < right_win_p.w; j++)
                mvwaddch(*right_win, i, j, ' ');
        wmove(*right_win, 0, 0);

	// Bottom window
        (*bot_win) = newwin(bot_win_p.h, bot_win_p.w, bot_win_p.by, bot_win_p.bx);

        wattron(*bot_win, bot_win_p.attrs_usual);
        for (i = 0; i < bot_win_p.h; i++)
        for (j = 0; j < bot_win_p.w; j++)
                mvwaddch(*bot_win, i, j, ' ');
        wmove(*bot_win, 0, 0);

	refresh();
	wrefresh(*left_win);
	wrefresh(*right_win);
	wrefresh(*bot_win);
}

void get_CWD_SUBDIRS(char* cwd, struct dirent ***dirs, int *dirs_cnt)
{
        getcwd(cwd, CHAR_BUF_SIZE);

	*dirs_cnt = 0;
	DIR *d = opendir(cwd);
	if (d != NULL) {
		struct dirent *dir;

		while ((dir = readdir(d)) != NULL)
 			(*dirs_cnt)++;
		closedir(d);
	}

	(*dirs) = (struct dirent **)malloc(sizeof(struct dirent *) * (*dirs_cnt));

	*dirs_cnt = 0;
	d = opendir(cwd);
        if (d != NULL) {
		struct dirent *dir;

                while ((dir = readdir(d)) != NULL)
		{
			(*dirs)[*dirs_cnt] = dir;
			(*dirs_cnt)++;
		}
                closedir(d);
        }
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

int OpenFile(char *fname, char **file_map, long *file_map_size)
{
//	clearWin(win_p, win);

	// Getting the file size;
	struct stat cur_stat;
	stat(fname, &cur_stat);
	(*file_map_size) = cur_stat.st_size;

	int fdout;
	if ((fdout = open(fname, O_RDWR)) < 0) {
//		mvwprintw(win, 0, 0, "ERROR when opening file for writing: %s", strerror(errno));
		return 1;
	}
	
	if (((*file_map) = mmap(0, *file_map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fdout, 0)) == MAP_FAILED) {
//		mvwprintw(win, 0, 0, "ERROR when mapping file: %s", strerror(errno));
		return 1;
	}

//	mvwprintw(win, 0, 0, "%s", file_map);
//	wrefresh(win);

	return 0;
}

int CloseFile(char **file_map, long *file_map_size) {

	if (msync(*file_map, *file_map_size, MS_SYNC) < 0) {
//		mvwprintw(win, 0, 0, "ERROR when mapping file: %s", strerror(errno));
                return 1;
	}

	(*file_map) = NULL;
	(*file_map_size) = 0;
	
	return 0;
}

//TODO error code, if we cannot start the program
void ExecuteFile(char *fname)
{
	char **params;
	params = (char **)malloc(sizeof(char *) * 2);
	params[0] = (char *)malloc(sizeof(char) * (strlen(fname) + 1));
	strcpy(params[0], fname);
	params[1] = NULL;

	int pid = fork();

	// Error
        if (pid == -1) {
                perror("MASTER proc, 1st fork");
                exit(1);
        }

        // Child process
        if (pid == 0) {
		execv(fname, params);
        }

	// Otherwise, it is master thread
	int waitstatus;
        wait(&waitstatus);
        int erc = WEXITSTATUS(waitstatus);

	printf("\nProgram \"%s\" finished with code = %d\nPress any key and \"Enter\" to continue...\n", fname, erc);
	//etchar();//getchar();
	char buf[256];
	scanf("%s", buf);

	free(params[0]);
	free(params);
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
	}

	return res;
}

int PipedExecuteFile4(char *pname1, char **ppar1, char *pname2, char **ppar2)
{
        int pid = fork();

        // Error
        if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
        }

        // Child process
        if (pid == 0) {
                int fd[2];

                pipe(fd);

                int pid2 = fork();

                // Error
                if (pid2 == -1) {
                        perror("fork");
                        exit(EXIT_FAILURE);
                }

                // Child process
                if (pid2 == 0) {
                        // stdout
                        dup2(fd[1], 1);
                        close(fd[0]);
                        
                        if (execvp(pname1, ppar1) < 0) {
                                perror("execvp");
                                exit(EXIT_FAILURE);
                        }
		}

                // Otherwise, it is master thread

                // stdin
                dup2(fd[0], 0);
                close(fd[1]);

                if (execvp(pname2, ppar2) < 0) {
                        perror("execvp");
                        exit(EXIT_FAILURE);
                }
	}
        // Otherwise, it is master thread
        int waitstatus;
        wait(&waitstatus);
        return WEXITSTATUS(waitstatus);
}

int PipedExecuteFile2(char *p1, char *p2)
{
	int p1c = 20;

	char **vals1 = (char **)malloc(sizeof(char *) * (p1c + 1));
	char *prog1;	

	p1c = 0;
	char *pch1 = strtok(p1, " ");
        while (pch1 != NULL) {                
		vals1[p1c] = (char *)malloc(sizeof(char) * (strlen(pch1) + 1));
		strcpy(vals1[p1c], pch1);
		p1c++;

		pch1 = strtok(NULL, " ");		
        }
	vals1[p1c] = (char *)NULL;
	prog1 = (char *)malloc(sizeof(char) * (strlen(vals1[0]) + 1));
	strcpy(prog1, vals1[0]);

	//---

	int p2c = 20;

        char **vals2 = (char **)malloc(sizeof(char *) * (p2c + 1));
        char *prog2;

	p2c = 0;
        char *pch2 = strtok(p2, " ");
        while (pch2 != NULL) {
                vals2[p2c] = (char *)malloc(sizeof(char) * (strlen(pch2) + 1));
                strcpy(vals2[p2c], pch2);
		p2c++;

		pch2 = strtok(NULL, " ");
        }
        vals2[p2c] = (char *)NULL;
        prog2 = (char *)malloc(sizeof(char) * (strlen(vals2[0]) + 1));
        strcpy(prog2, vals2[0]);

	//---

	int retval = PipedExecuteFile4(prog1, vals1, prog2, vals2);

	int i;

	for (i = 0; i < p1c; i++)
		free(vals1[i]);
	free(vals1);

	for (i = 0; i < p2c; i++)
		free(vals2[i]);
	free(vals2);

	free(prog1);
	free(prog2);

	return retval;
}
