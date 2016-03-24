/*
 * mkplaylists.c
 *
 *  Created on: 15.09.2014
 *      Author: bweber
 */

#include "utils.h"
#include <sys/stat.h>

/**
 * create the name.for a playlist based on the directory
 * In best case it's the last two directories (Artist - Album.m3u)
 * Or it's just one directory (Album.m3u)
 * Or it's just playlist.m3u
 */
char *genPLName( char *plname, const char *cd ){
	genPathName( plname, MAXPATHLEN, cd );
	if( strlen(plname) < 2 ) {
		strcpy( plname, "playlist" );
	}
	strncat( plname, ".m3u", MAXPATHLEN-strlen( plname ) );
	return plname;
}

/**
 * recurse through a directory, check each (sub)directory
 * for music files and write a playlist into each directory
 * unless one is already present.
 */
int traverse( char *cd ){
	char curdir[MAXPATHLEN];
	char dirbuff[MAXPATHLEN];
	char plname[MAXPATHLEN];
	FILE *file;

	struct dirent **namelist;
	int i,n;
	int haspl=0;
	int hasmus=0;

	strncpy( curdir, cd, MAXPATHLEN );
	if(curdir[strlen(curdir)-1]!='/'){
		strncat( curdir, "/", MAXPATHLEN-strlen( curdir ) );
	}

	/**
	 * collect files and put them into a playlist unless there is already one
	 */
	n = getFiles( curdir, &namelist );
	if (n < 0) {
		fail("getFiles", curdir, errno );
	} else {
		// check if one of the files is a playlist
		for (i = 0; i < n; i++) {
			// Once a playlist is found we can stop
			if( strstr( namelist[i]->d_name, ".m3u" ) ){
				haspl=1;
				break;
			}
			// Try not to check each and every file after the first is found
			if( (!hasmus) && isMusic( namelist[i]->d_name ) ){
				hasmus=1;
			}
		}

		// If there is music but no playlist, create one
		if( ( 1 == hasmus ) && ( 0 == haspl ) ) {
			genPLName( plname, curdir );
			file=fopen( plname, "w" );
			if( ! file ) fail( "Write open failed for ", plname, errno );

			// Write all music filenames into the playlist
			for( i=0; i<n; i++ ) {
				if( isMusic( namelist[i]->d_name ) ) fprintf( file, "%s\n", namelist[i]->d_name );
			}
			fclose( file );
		}
		// Clean up the namelist
		for (i = 0; i < n; i++) {
			free( namelist[i] );
		}
		free( namelist );
	}

	/**
	 * collect directories and run trough them as well
	 * clean up the namelist in the same go
	 */
	n = getDirs( curdir, &namelist );
	if (n < 0) {
		fail("getdirs", curdir, errno );
	} else {
		for (i = 0; i < n; i++) {
			snprintf( dirbuff, MAXPATHLEN, "%s%s", curdir, namelist[i]->d_name );
			traverse( dirbuff );
			free( namelist[i] );
		}
		free( namelist );
	}

	return 0;
}

int main( int argc, char **argv ) {
	if( argc == 1 ) {
		traverse( "." );
	} else {
		traverse( argv[1] );
	}
	return 0;
}
