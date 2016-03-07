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


// Represents a string
typedef unsigned char* strval_t;

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

#ifndef MAXPATHLEN
  #define MAXPATHLEN 256
#endif
#define RANDOM(x) rand()%x

struct entry_t {
	struct entry_t *prev;
	char path[MAXPATHLEN];
	char name[MAXPATHLEN];
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
 * entry helper functions
 */
struct entry_t *sort( struct entry_t *files );
struct entry_t *wipe( struct entry_t *files );
struct entry_t *recurse( char *curdir, struct entry_t *files, struct blacklist_t *bl );

struct blacklist_t *loadBlacklist( char *path );

void activity();
void fail( const char* msg, const char* info, int error );
int isMusic( const char *name );
int fncmp( const char* str1, const char* str2 );
int computestrval( const char* str, strval_t strval );
int fnvcmp( const strval_t val1, const strval_t val2 );
char *toLower( char *text );
int dsel( const struct dirent *entry );
int fsel( const struct dirent *entry );

#endif
