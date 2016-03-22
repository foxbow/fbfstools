#include "utils.h"
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <ncurses.h>

#define BUFLEN 256

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
 * Draw the application frame
 */
void drawframe(char *station, struct entry_t *current, char *status ) {
	int i, maxlen, pos, rows;
	int row, col;
	int middle;
	char buff[BUFLEN];
	struct entry_t *runner;

	refresh();
	getmaxyx(stdscr, row, col);

	// Keep a minimum size to make sure
	if ((row > 6) && (col > 19)) {
		// main frame
		drawbox(1, 1, row - 2, col - 2);
		// outer frame
		mvhline(row - 1, 1, ' ', col);
		mvvline(1, col - 1, ' ', row);

		maxlen = col - 6;

		middle=row/2;

		// title
		dhline( middle-1, 1, col-3 );
		strncpy(buff, station, maxlen);
		pos = (col - (strlen(buff) + 2)) / 2;
		mvprintw(middle-1, pos, " %s ", buff);

		// Set the current playing title
		if( NULL != current ) {
			strip(buff, current->title, maxlen);
		}
		else {
			strcpy( buff, "---" );
		}
		setTitle(buff);

//		buff[maxlen] = 0;
		pos = (col - strlen(buff)) / 2;
		mvhline(middle, 2, ' ', maxlen + 2);
		mvprintw(middle, pos, "%s", buff);

		dhline( middle+1, 1, col-3 );

		// print the status
		strncpy(buff, status, maxlen);
		pos = (col - (strlen(buff) + 2)) / 2;
		mvprintw( row - 2, pos, " %s ", buff);

		// song list
		if( NULL != current ) {
			// previous songs
			runner=current->prev;
			for( i=middle-2; i>1; i-- ){
				if( NULL != runner ) {
					strip( buff, runner->title, maxlen );
					runner=runner->prev;
				}
				else {
					strcpy( buff, "---" );
				}
				mvhline( i, 2, ' ', maxlen + 2);
				mvprintw( i, 3, "%s", buff);
			}
			// past songs
			runner=current->next;
			for( i=middle+2; i<row-2; i++ ){
				if( NULL != runner ) {
					strip( buff, runner->title, maxlen );
					runner=runner->next;
				}
				else {
					strcpy( buff, "---" );
				}
				mvhline( i, 2, ' ', maxlen + 2);
				mvprintw( i, 3, "%s", buff);
			}
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

/**
 * reads from the fd into the line buffer until either a CR
 * comes or the fd stops sending characters.
 * returns number of read bytes or -1 on overflow.
 */
int readline( char *line, size_t len, int fd ){
	int cnt=0;
	char c;

	while ( 0 != read(fd, &c, 1 ) ) {
		if( cnt < len ) {
			line[cnt]=c;
			cnt++;
			if( '\n' == c ) {
				if( cnt < len ) {
					line[cnt]=0;
					cnt++;
					return cnt;
				} else {
					return -1;
				}
			}
			// avoid reading behind string end
			if( 0 == c ) {
				return cnt;
			}
		} else {
			return -1;
		}
	}

	// avoid returning unterminated strings.
	if( cnt < len ) {
		line[cnt]=0;
		cnt++;
		return cnt;
	} else {
		return -1;
	}

	return cnt;
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
	char status[BUFLEN] = "INIT";
	char tbuf[BUFLEN];
	char station[BUFLEN] = "mixplay "VERSION;
	char curdir[MAXPATHLEN];
	char target[MAXPATHLEN];
	char blname[MAXPATHLEN] = "";
	char c;
	char *b;
	int mix = 0;
	int i, cnt = 1;
	fd_set fds;
	struct timeval to;
	pid_t pid;
	int redraw;
	int repeat = 0;

	int order=1;

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
			close(p_command[0]);
			close(p_status[1]);

			// Start curses mode
			initscr();
			curs_set(0);
			cbreak();
			keypad(stdscr, TRUE);
			noecho();
			drawframe(station, NULL, status );

			while (running) {
				FD_ZERO( &fds );
				FD_SET( fileno(stdin), &fds );
				FD_SET( p_status[0], &fds );
				to.tv_sec=1;
				to.tv_usec=0; // 1/4 sec
				i=select( FD_SETSIZE, &fds, NULL, NULL, &to );
				redraw=0;

				// Interpret keypresses
				if( FD_ISSET( fileno(stdin), &fds ) ) {
					switch( getch() ){
					case ' ':
						write( p_command[1], "PAUSE\n", 7 );
					break;
					case 's':
						order=0;
						write( p_command[1], "STOP\n", 6 );
					break;
					case KEY_DOWN:
					case 'n':
						order=1;
						write( p_command[1], "STOP\n", 6 );
					break;
					case KEY_UP:
					case 'p':
						order=-1;
						write( p_command[1], "STOP\n", 6 );
					break;
					case 'q':
						write( p_command[1], "QUIT\n", 6 );
						running=0;
					break;
					case KEY_LEFT:
						write( p_command[1], "JUMP -64\n", 10 );
					break;
					case KEY_RIGHT:
						write( p_command[1], "JUMP +64\n", 10 );
					break;
					case 'r':
						write( p_command[1], "JUMP 0\n", 8 );
					break;
					}
				}

				// Interpret mpg123 output
				if( FD_ISSET( p_status[0], &fds ) ) {
					readline(line, 512, p_status[0]);
					redraw = 1;
					switch (line[1]) {
					int cmd, in, rem, q;
					case 'R': // startup
						current = root;
						sendplay(p_command[1], current);
						break;
					case 'I': // ID3 info
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
							strip(current->title, b + 7, BUFLEN);
							strcat(current->title, " - ");
							strip(current->title + strlen(current->title), tbuf,
									BUFLEN - strlen(current->title));
						}
						// Album
						else if (NULL != (b = strstr(line, "album:"))) {
							strip(station, b + 6, BUFLEN);
						}
						break;
					case 'J': // JUMP reply
						// Ignored
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
						break;
					case 'F':
						/* @F <a> <b> <c> <d>
						 * Status message during playing (frame info)
						 * a = framecount (int)
						 * b = frames left this song (int)
						 * c = seconds (float)
						 * d = seconds left (float)
						 */
						b=strrchr( line, ' ' );
						rem=atoi(b);
						*b=0;
						b=strrchr( line, ' ' );
						in=atoi(b);
						q=in/((rem+in)/30);
						memset( tbuf, 0, BUFLEN );
						for( i=0; i<30; i++ ) {
							if( i < q ) tbuf[i]='=';
							if( i == q ) tbuf[i]='>';
							else tbuf[i]=' ';
						}
						sprintf(status, "%i:%02i [%s] %i:%02i", in/60, in%60, tbuf, rem/60, rem%60 );
						// sprintf(status, "%i:%02i PLAYING %i:%02i", in/60, in%60, rem/60, rem%60 );
						break;
					case 'P': // Player status
						cmd = atoi(&line[3]);
						switch (cmd) {
						case 0:
							if( 0 == order ) {
								strcpy( status, "STOP" );
								break;
							}
							if( 1 == order ) {
								if (current->next == NULL) {
									if (repeat) {
										current = root;
										if (mix)
											current = shuffleTitles(root, &cnt);
									} else {
										strcpy( status, "STOP" );
										break;
									}
								} else {
									current = current->next;
								}
							}
							else {
								if (current->prev != NULL) {
									current=current->prev;
								} else {
									strcpy( status, "STOP" );
									break;
								}
							}
							sendplay(p_command[1], current);
							break;
						case 1:
							strcpy( status, "PAUSE" );
							break;
						case 2:
							strcpy( status, "PLAYING" );
							break;
						default:
							sprintf( status, "Unknown status %i!", cmd);
							drawframe( station, current, status );
							sleep(1);
						}
						break;
					case 'E':
						sprintf( status, "ERROR: %s", line);
						drawframe( station, current, status );
						sleep(1);
						// These are rarely deadly
						// running=0;
						break;
					default:
						sprintf( status, "MPG123 : %s", line);
						drawframe( station, current, status );
						sleep(1);
						//running=0;
						break;
					} // case()
				} // fgets() > 0
				if( redraw ) drawframe(station, current, status );
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
