#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Directory access */
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <strings.h>

#ifdef __MINGW_H
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <dir.h>
#else
  #include <sys/types.h>
  #include <sys/statvfs.h>
#endif

#include <time.h>

/* the ABS macro misses here and there */
#ifndef ABS
#define ABS(x) ((x<0)?-(x):x)
#endif

#define HOR '-'
#define VER '|'
#define EDG "+"

/* Default values */
/* Max size of an MP3 file - to avoid full albums */
#define MAXSIZE 15*1024
/* Min size - to avoid fillers */
#define MINSIZE 1024
/* Buffer for copying files */
#define BUFFSIZE 512*1024
/* Default target for copying */
#define TARGETDIR "player/"
// #define ARRAYLEN 676
#define ARRAYLEN 85
#define BITS 680
// 90% similarity is enough
#define TRIGGER 90
// 10k filesize difference
#define RANGE 10

#ifndef MAXPATHLEN
  #define MAXPATHLEN 256
#endif
#define RANDOM(x) (rand()%x)

struct entry_t {
	struct entry_t *prev;
	char path[MAXPATHLEN];
	char name[MAXPATHLEN];
	char display[MAXPATHLEN];
	long int length;
	struct entry_t *next;
};

/**
 * structure to hold names of directories that should be ignored
 */
struct blacklist_t {
	char dir[MAXPATHLEN];
	struct blacklist_t *next;
};

/**
 * curses helper functions
 */
void cfail(const char *msg, const char *info, int error );
void dhline(int r, int c, int len);
void dvline(int r, int c, int len);
void drawbox(int r0, int c0, int r1, int c1);
/**
 * reads from the fd into the line buffer until either a CR
 * comes or the fd stops sending characters.
 * returns number of read bytes or -1 on overflow.
 */
int readline( char *line, size_t len, int fd );
/**
 * entry helper functions
 */
// unused?!
// struct entry_t *sortTitles( struct entry_t *files );
// struct entry_t *wipeTitles( struct entry_t *files );

struct entry_t *recurse( char *curdir, struct entry_t *files );
struct entry_t *shuffleTitles( struct entry_t *base, int *cnt );
struct entry_t *rewindTitles( struct entry_t *base, int *cnt );

int loadBlacklist( char *path );

void setTitle(const char* title);
void activity();
void fail( const char* msg, const char* info, int error );
int fncmp( const char* str1, const char* str2 );
char *toLower( char *text );
int getFiles( const char *cd, struct dirent ***filelist );
int getDirs( const char *cd, struct dirent ***dirlist );
int isMusic( const char *name );
char *strip( char *buff, const char *text, const int maxlen );
char *genPathName( char *name, char *cd, size_t len );

#endif
