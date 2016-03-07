/*
 * fixdatenames.c
 *
 *  Created on: 07.03.2016
 *      Author: Björn Weber
 */

#include "utils.h"
#include <sys/stat.h>

struct fparts {
	char prefix[16];
	char date[16];
	char postfix[16];
};

int recursive=0;

void dumpparts( struct fparts parts ) {
	printf("pre: >%s<\n", parts.prefix );
	printf("dat: >%s<\n", parts.date );
	printf("pos: >%s<\n", parts.postfix );
}

void splitname( const char *fname, struct fparts *parts ){
	int cnt=0;
	int pos=0;
	int state=0;

	if( NULL == fname ) {
		fail("splitname arg", fname, ENOTRECOVERABLE );
	}

	parts->prefix[0]=0;
	parts->date[0]=0;
	parts->postfix[0]=0;

	for( cnt=0; cnt < strlen( fname ); cnt ++ ) {
		switch( state ) {
		case 0:
			if( isdigit( fname[cnt] ) ) {
				state=1;
				parts->prefix[pos]=0;
				pos=0;
				parts->date[pos]=fname[cnt];
			} else {
				parts->prefix[pos]=fname[cnt];
			}
		break;
		case 1:
			if( !isdigit( fname[cnt] ) ) {
				state=2;
				parts->date[pos]=0;
				pos=0;
				parts->postfix[pos]=fname[cnt];
			} else {
				parts->date[pos]=fname[cnt];
			}
		break;
		case 2:
			parts->postfix[pos]=fname[cnt];
		break;
		default:
			fail("splitname state", fname, state );
		break;
		}
		pos++;
	}
	switch(state){
	case 0: parts->prefix[pos]=0;
	break;
	case 1: parts->date[pos]=0;
	break;
	case 2: parts->postfix[pos]=0;
	break;
	default:
		fail("splitname final", fname, state );
	break;
	}
}

char *joinname( const struct fparts parts, char *name, int len ) {
	strncpy( name, parts.prefix, len );
	strncat( name, parts.date, len );
	strncat( name, parts.postfix, len );
	return name;
}

/**
 * recurse through a directory, check each (sub)directory
 * for music files and write a playlist into each directory
 * unless one is already present.
 */
int traverse( char *cd ){
	char curdir[MAXPATHLEN];
	char dirbuff[MAXPATHLEN];
	char fullpath[MAXPATHLEN];
	char newname[MAXPATHLEN];

	FILE *file;
	struct stat st;
	struct fparts parts;

	struct dirent **namelist;
	int i,n;

	strncpy( curdir, cd, MAXPATHLEN );
	if(curdir[strlen(curdir)-1]!='/'){
		strncat( curdir, "/", MAXPATHLEN-strlen( curdir ) );
	}

	/**
	 * collect files and put them into a playlist unless there is already one
	 */
	n = scandir( curdir, &namelist, fsel, alphasort);
	if (n < 0) {
		fail("scandir", curdir, errno );
	} else {
		// Clean up the namelist
		for (i = 0; i < n; i++) {
			strcpy( fullpath, curdir );
			strcat( fullpath, namelist[i]->d_name );

			if( 0 != stat( fullpath, &st ) ) {
				fail("stat", fullpath, errno );
			}
			strftime( dirbuff, sizeof(dirbuff), "%Y%m%d", localtime(&st.st_mtime) );

			splitname( fullpath, &parts );
			if( strlen(parts.date) > 5 ) {
// dumpparts( parts );
				strncpy( parts.date, dirbuff, 15 );
				joinname( parts, newname, MAXPATHLEN );
				if( !strstr( fullpath, newname) ) {
					printf("mv %s %s\n", fullpath, newname );
				}
			}
			free( namelist[i] );
		}
		free( namelist );
	}

	if( recursive ) {
		/**
		 * collect directories and run trough them as well
		 * clean up the namelist in the same go
		 */
		n = scandir( curdir, &namelist, dsel, alphasort);
		if (n < 0) {
			fail("scandir", curdir, errno );
		} else {
			for (i = 0; i < n; i++) {
				snprintf( dirbuff, MAXPATHLEN, "%s%s", curdir, namelist[i]->d_name );
				traverse( dirbuff );
				free( namelist[i] );
			}
			free( namelist );
		}
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
