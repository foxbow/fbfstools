#ifndef FBFS_UTILS_H
#define FBFS_UTILS_H

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
#include <sys/statvfs.h>

#include <time.h>

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


int getVerbosity();
int setVerbosity(int);
int incVerbosity();
void muteVerbosity();

/**
 * curses helper functions
 */
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
// void wipeTitles( struct entry_t *files );

struct entry_t *recurse( char *curdir, struct entry_t *files );
struct entry_t *shuffleTitles( struct entry_t *base );
struct entry_t *rewindTitles( struct entry_t *base );
struct entry_t *removeTitle( struct entry_t *entry );
struct entry_t *loadPlaylist( const char *path );
struct entry_t *insertTitle( struct entry_t *base, const char *path );
struct entry_t *skipTitles( struct entry_t *current, int num, int repeat, int mix );
int countTitles( struct entry_t *base );

int loadBlacklist( const char *path );
int loadWhitelist( const char *path );
void addToList( const char *path, const char *line );

void setTitle(const char* title);
void activity();
void fail( const char* msg, const char* info, int error );
int fncmp( const char* str1, const char* str2 );
char *toLower( char *text );
int getFiles( const char *cd, struct dirent ***filelist );
int getDirs( const char *cd, struct dirent ***dirlist );
int isMusic( const char *name );
char *strip( char *buff, const char *text, const size_t maxlen );
char *genPathName( char *name, const char *cd, const size_t len );
int endsWith( const char *text, const char *suffix );
int isURL( const char *uri );

#endif
