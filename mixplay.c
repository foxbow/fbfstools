#include "utils.h"
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <ncurses.h>

/* Default values */
/* Max size of an MP3 file - to avoid full albums */
#define MAXSIZE 15*1024
/* Min size - to avoid fillers */
#define MINSIZE 1024

#define BUFLEN 256

#ifndef MAXPATHLEN
#define MAXPATHLEN 256
#endif

#define TITLES 75

extern int verbosity;
volatile int running=1;

void usage( char *progname ){
	/**
	 * print out CLI usage
	 */
	printf( "Usage: %s [-b <file>] [-m] [path]\n", progname );
	printf( "-b <file>  : Blacklist of names to exclude [unset]\n" );
	printf( "-m         : Mix, enable shuffle mode on playlist\n" );
	printf( "-r         : Repeat\n");
	printf( "[path]     : path to the music files [.]\n" );
	exit(0);
}

/**
 * Print an error message, print the log and exit
 */
int cfail(const char *msg) {
	endwin();
	printf("ERROR: %s", msg);
	running = 0;
	return EXIT_FAILURE;
}

/**
 * Draw a horizontal line
 */
void dhline(int r, int c, int len) {
	mvhline(r, c + 1, HOR, len - 1);
	mvprintw(r, c, EDG );
	mvprintw(r, c + len, EDG );
}

/**
 * Draw a vertical line
 */
void dvline(int r, int c, int len) {
	mvhline(r + 1, c, VER, len - 2);
	mvprintw(r, c, EDG );
	mvprintw(r + len, c, EDG );
}

/**
 * draw a box
 */
void drawbox(int r0, int c0, int r1, int c1) {
	dhline(r0, c0, c1 - c0);
	dhline(r1, c0, c1 - c0);
	mvvline(r0 + 1, c0, VER, r1 - r0 - 1);
	mvvline(r0 + 1, c1, VER, r1 - r0 - 1);
}

/**
 * Some ANSI code magic to set the terminal title
 **/
void setTitle(const char* title) {
	char buff[BUFLEN];
	strcpy(buff, "\033]2;");
	strcat(buff, title);
	strcat(buff, "\007\000");
	fputs(buff, stdout);
	fflush(stdout);
}

/**
 * Draw the application frame
 */
void drawframe(char *station, char *titel[]) {
	int i, maxlen, pos, rows;
	int row, col;
	char buff[BUFLEN];

	refresh();
	getmaxyx(stdscr, row, col);

	// Keep a minimum size to make sure
	if ((row > 6) && (col > 19)) {
		// main frame
		drawbox(1, 1, row - 2, col - 2);
		// separator
		dhline(3, 1, col - 3);
		// outer frame
		mvhline(row - 1, 1, ' ', col);
		mvvline(1, col - 1, ' ', row);

		maxlen = col - 6;

		// Set the stream title
		strncpy(buff, station, maxlen);
		pos = (col - (strlen(buff) + 2)) / 2;
		mvprintw(1, pos, " %s ", buff);

		// Set the current playing title
		strncpy(buff, titel[0], maxlen);
		buff[maxlen] = 0;
		pos = (col - strlen(buff)) / 2;
		mvhline(2, 2, ' ', maxlen + 2);
		mvprintw(2, pos, "%s", buff);

		setTitle(buff);

		// Set the history
		rows = row - 5;
		if (rows > TITLES)
			rows = TITLES;
		for (i = 1; i < rows; i++) {
			strncpy(buff, titel[i], maxlen);
			buff[maxlen] = 0;
			mvhline(i + 3, 2, ' ', maxlen + 2);
			mvprintw(i + 3, 3, "%s", buff);
		}
	}
	refresh();
}

void sendplay( int fd, struct entry_t *song ) {
	char line[BUFLEN];

	strncpy( line, "load ", BUFLEN );
	if( strlen(song->path) != 0 ){
		strncat( line, song->path, BUFLEN );
		strncat( line, "/", BUFLEN );
	}
	strncat( line, song->name, BUFLEN );
	strncat( line, "\n", BUFLEN );
	write( fd, line, BUFLEN );
}

int main(int argc, char **argv) {
	/**
	 * CLI interface
	 */
	struct entry_t *root = NULL;
	struct entry_t *current = NULL;

	int p_status[2];
	int p_command[2];

	char line[BUFLEN];
	char song[BUFLEN] = "";
	char tbuf[BUFLEN];
	char station[BUFLEN] = "mixplay "VERSION;
	char *titel[TITLES];
	char curdir[MAXPATHLEN];
	char target[MAXPATHLEN];
	char blname[MAXPATHLEN] = "";
	char c;
	char *b;
	int mix = 0;
	int i, cnt = 1;
	fd_set fds;
	pid_t pid;
	int redraw;
	int repeat = 0;
	int cmd;

	FILE *reader;
	verbosity = 0;

	if (NULL == getcwd(curdir, MAXPATHLEN))
		fail("Could not get current dir!", "", errno);

	while ((c = getopt(argc, argv, "mb:s:r")) != -1) {
		switch (c) {
		case 'm':
			mix = 1;
			break;
		case 'b':
			strncpy(blname, optarg, MAXPATHLEN);
			break;
		case 'r':
			repeat = 1;
			break;
		case 's':
			strncpy(song, optarg, BUFLEN);
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	if (song[0] != 0) {
		// play single song...
		root = (struct entry_t*) malloc(sizeof(struct entry_t));
		if (NULL == root) {
			fail("Malloc failed", "", errno);
		}
		root->next = NULL;
		root->prev = NULL;
		root->length = 0;
		b = strchr(song, '/');
		if (NULL != b) {
			strncpy(root->name, b + 1, MAXPATHLEN);
			b[0] = 0;
			strncpy(root->path, song, MAXPATHLEN);
		} else {
			strncpy(root->name, song, MAXPATHLEN);
			strncpy(root->path, "", MAXPATHLEN);
		}
	} else {
		if (optind < argc) {
			strcpy(curdir, argv[optind]);
			if ((strlen(curdir) == 2) && (curdir[1] == ':'))
				sprintf(curdir, "%s/", curdir);
			else if (curdir[strlen(curdir) - 1] == '/')
				curdir[strlen(curdir) - 1] = 0;
		}

		if (0 != blname[0])
			loadBlacklist(blname);

		root = recurse(curdir, root);
	}

	if (NULL != root) {
		if (mix)
			root = shuffleTitles(root, &cnt);
		else
			root = rewindTitles(root, &cnt);

		// create communication pipes
		pipe(p_status);
		pipe(p_command);

		pid = fork();
		if (0 > pid) {
			fail("could not fork", "", errno);
		}
		// child process
		if (0 == pid) {
			/* COMMAND CODES
			 * -------------
			 * Command codes may be abbreviated by its first character.
			 * QUIT - Stop playing and quit player
			 * LOAD <a> - Load and play a file/URL.
			 * STOP - Stop playing (without quitting the player)
			 * PAUSE - Pause/unpause playing
			 * JUMP [+|-]<a> - Skip <a> frames:
			 * +32	forward 32 frames
			 * -32	rewind 32 frames
			 * 0	jump to the beginning
			 * 1024	jump to frame 1024
			 */
			if (dup2(p_command[0], STDIN_FILENO) != STDIN_FILENO) {
				fail("Could not dup stdin for player", "", errno);
			}
			if (dup2(p_status[1], STDOUT_FILENO) != STDOUT_FILENO) {
				fail("Could not dup stdout for player", "", errno);
			}
			// dup2( STDERR_FILENO, p_status[0] );

			close(p_command[1]);
			close(p_status[0]);

			close(p_command[0]);
			close(p_status[1]);

			execlp("mpg123", "mpg123", "-R", "2>/dev/null", NULL);
			fail("Could not exec", "mpg123", errno);
		}

		else {
			// Initialize title backlog
			for (i = 0; i < TITLES; i++) {
				titel[i] = (char *) calloc(BUFLEN, sizeof(char));
				strcpy(titel[i], "---");
			}

			close(p_command[0]);
			close(p_status[1]);
			reader = fdopen(p_status[0], "r");

			// Start curses mode
			initscr();
			curs_set(0);
			cbreak();
			keypad(stdscr, TRUE);
			noecho();
			drawframe(station, titel);

			while (running) {
				while ((NULL != fgets(line, 512, reader))) {
					redraw = 0;
					switch (line[1]) {
					case 'R':
						/* @R MPG123 <a>
						 * Startup version message
						 */
						current = root;
						sendplay(p_command[1], current);
						break;
					case 'I':
						/* @I ID3.2.year:2016
						 * @I ID3.2.comment:http://www.faderhead.com
						 * @I ID3.2.genre:EBM / Electronic / Electro
						 *
						 * @I <a>
						 * Status message after loading a song (no ID3 song info)
						 * a = filename without path and extension
						 */
						if (NULL != (b = strstr(line, "title:"))) {
							strip(tbuf, b + 6, BUFLEN);
							// do not redraw yet, wait for the artist
						}
						// line starts with 'Artist:' this means we had a 'Title:' line before
						else if (NULL != (b = strstr(line, "artist:"))) {
							for (i = TITLES - 1; i > 0; i--)
								strcpy(titel[i], titel[i - 1]);
							strip(titel[0], b + 7, BUFLEN);
							strcat(titel[0], " - ");
							strip(titel[0] + strlen(titel[0]), tbuf,
									BUFLEN - strlen(titel[0]));
						}
						// Album
						else if (NULL != (b = strstr(line, "album:"))) {
							strip(station, b + 6, BUFLEN);
							redraw = 1;
						}

						break;
					case 'S':
						/* @S <a> <b> <c> <d> <e> <f> <g> <h> <i> <j> <k> <l>
						 * Status message after loading a song (stream info)
						 * a = mpeg type (string)
						 * b = layer (int)
						 * c = sampling frequency (int)
						 * d = mode (string)
						 * e = mode extension (int)
						 * f = framesize (int)
						 * g = stereo (int)
						 * h = copyright (int)
						 * i = error protection (int)
						 * j = emphasis (int)
						 * k = bitrate (int)
						 * l = extension (int)
						 */
						// ignore for now
						// printf("%s",line);
						break;
					case 'F':
						/* @F <a> <b> <c> <d>
						 * Status message during playing (frame info)
						 * a = framecount (int)
						 * b = frames left this song (int)
						 * c = seconds (float)
						 * d = seconds left (float)
						 */
						break;
					case 'P':
						/* @P <a> <b>
						 * Playing status
						 * a = 0: playing stopped
						 * b = EOF: stopped, because end of song reached
						 * a = 1: playing paused
						 * a = 2: playing unpaused
						 */
						cmd = atoi(&line[3]);
						switch (cmd) {
						case 0:
							if (current->next == NULL) {
								if (repeat) {
									current = root;
									if (mix)
										current = shuffleTitles(root, &cnt);
								} else {
									// puts("STOP");
									break;
								}
							} else {
								current = current->next;
							}
							sendplay(p_command[1], current);
							break;
						case 1:
							printf("PAUSE\n");
							break;
						case 2:
							printf("PLAYING\n");
							break;
						default:
							printf("Unknown command %i!\n", cmd);
						}
						break;
					case 'E':
						/* @E <a>
						 * An error occured
						 * Errors may be also reported by mpg123 through
						 * stderr (without @E)
						 * a = error message (string)
						 */
						printf("ERROR: %s", &line[3]);
						running = 0;
						break;
					default:
						printf("MPG123 : %s", line);
						break;
					} // case()

					if (redraw) {
						drawframe(station, titel);
					}
				} // fgets() > 0
				reader = fdopen(p_status[0], "r");
				drawframe(station, titel);
			} // while(running)
			kill(pid, SIGTERM);
			endwin();
		} // fork() parent
	} // root!=NULL
	else {
		printf("No music found at %s!\n", curdir);
	}
	return 0;
}
