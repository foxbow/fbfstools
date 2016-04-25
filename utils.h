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

/* Default values */

#define CMP_ARRAYLEN 85
#define CMP_BITS 680

#ifndef MAXPATHLEN
  #define MAXPATHLEN 256
#endif
#define RANDOM(x) (rand()%x)

struct entry_t {
	struct entry_t *prev;
	unsigned long key;			// DB key/index
	char path[MAXPATHLEN];		// path on the filesystem to the file
	char name[MAXPATHLEN];		// filename
	char display[MAXPATHLEN];	// displayname (artist - title)
	unsigned long size;			// size in kb
	char artist[MAXPATHLEN];	// Artist info
	int  rating;				// 1=favourite
	char title[MAXPATHLEN];		// Title info (from mp3)
	char album[MAXPATHLEN];		// Album info (from mp3)
	int  length;				// length in seconds (from mp3)
	unsigned long played;		// play counter
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
