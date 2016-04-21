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

#include <getopt.h>

#define HOR '-'
#define VER '|'
#define EDG "+"

/* Default values */

#define CMP_ARRAYLEN 85
#define CMP_BITS 680

#ifndef MAXPATHLEN
  #define MAXPATHLEN 256
#endif
#define RANDOM(x) (rand()%x)

struct entry_t {
	long int key;
	struct entry_t *prev;
	char path[MAXPATHLEN];
	char name[MAXPATHLEN];
	char display[MAXPATHLEN];
	long int size;
	char artist[MAXPATHLEN];
	int  rating;
	/** preparation for DB support **
	char title[MAXPATHLEN];
	char album[MAXPATHLEN];
	int  length;
	********************************/
	struct entry_t *next;
};

/*
 * Verbosity handling of the utils functions
 */
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
 * Music helper functions
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
int genPathName( char *name, const char *cd, const size_t len );
int isMusic( const char *name );
int addToWhitelist( const char *line );
int checkWhitelist( struct entry_t *root );

/**
 * General utility functions
 */
void addToList( const char *path, const char *line );
void setTitle(const char* title);
void fail( const char* msg, const char* info, int error );
int fncmp( const char* str1, const char* str2 );
char *toLower( char *text );
int getFiles( const char *cd, struct dirent ***filelist );
int getDirs( const char *cd, struct dirent ***dirlist );
char *strip( char *buff, const char *text, const size_t maxlen );
int endsWith( const char *text, const char *suffix );
int startsWith( const char *text, const char *prefix );
int isURL( const char *uri );
int readline( char *line, size_t len, int fd );
void activity( const char *msg );

#endif
