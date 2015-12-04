#include "utils.h"
#include <getopt.h>

/* Default values */
/* Max size of an MP3 file - to avoid full albums */
#define MAXSIZE 15*1024
/* Min size - to avoid fillers */
#define MINSIZE 1024

#ifndef MAXPATHLEN
#define MAXPATHLEN 256
#endif

int verbosity = 1;

void usage( char *progname ){
	/**
	 * print out CLI usage
	 */
	printf( "Usage: %s [-s <sourcedir>] [-b <path to blacklist>] [-v <verbosity>] [playlist]\n", progname );
	printf( "-s <path>  : set path to directory with music [current dir]\n" );
	printf( "-b <file>  : Blacklist of names to exclude [unset]\n" );
	printf( "-v <num>   : Verbosity (0=quiet, 1=process, 2=1+wait for enter, 3=2+verbose output) [1]\n" );
	printf( "[playlist] : Name of playlist to use [playlist.m3u]\n" );
	exit(0);
}

void shufflem3u( struct entry_t *list, FILE *fp ) {
	struct entry_t *base;
	int i, cnt=1, no=0;

	// rewind to the beginning of the list
	while( list->prev != NULL ){
		list=list->prev;
		cnt++;
	}

	base=list;

	if (verbosity > 0 ) printf("Found %i titles\n", cnt);

	// Stepping through every item and tossing it away afterwards
	for(i=cnt; i>0; i--){
		int j, pos;
		list=base;
		pos=RANDOM(i);
		for(j=1; j<=pos; j++){
			list=list->next;
		}

		// Check if the current file is a valid one */
		if ( ( list->length < MAXSIZE ) && ( list->length > MINSIZE ) ) {
			fprintf( fp, "%s/%s\n", list->path, list->name );
		}

		// Remove entry
		if(list==base) base=list->next;

		if(list->prev != NULL){
			list->prev->next=list->next;
		}
		if(list->next != NULL){
			list->next->prev=list->prev;
		}

		free(list);
		list=NULL;
	}

}

int main( int argc, char **argv ){
	/**
	 * CLI interface
	 */
	struct blacklist_t *blacklist=NULL;
	struct entry_t *root=NULL;

	char curdir[MAXPATHLEN];
	char target[MAXPATHLEN];
	char blname[MAXPATHLEN];
	char c;

	FILE *pl;

	if( NULL == getcwd( curdir, MAXPATHLEN ) ) fail( "Could not get current dir!", "", errno );

	strcpy( target, "playlist.m3u" );

	while( ( c = getopt( argc, argv, "s:b:v:" ) ) != -1 ) {
		switch( c ) {
		case 's': 
			strcpy( curdir, optarg );
			if( ( strlen(curdir) == 2 ) && ( curdir[1] == ':' ) ) 
				sprintf( curdir, "%s/", curdir );
			else if ( curdir[ strlen( curdir ) -1 ] == '/' )
				curdir[ strlen( curdir ) -1 ] = 0;
			break;
		case 'b':
			strcpy( blname, optarg );
			blacklist=loadBlacklist( blname );
			break;
		case 'v':
			verbosity = atoi( optarg );
			break;
		default:
			usage( argv[0] );
			break;
		}
	}
	if( optind < argc ) strcpy( target, argv[optind] );

	/* Print info and give chance to bail out */
	printf( "Source: %s\n", curdir );
	printf( "Target: %s\n", target );
	printf( "Using MP3/OGG files between %i and %i MB\n", MINSIZE/1024, MAXSIZE/1024 );
	if( blacklist )
		printf( "Using %s as blacklist\n", blname );
	printf( "Press enter to continue or CTRL-C to stop.\n" );
	fflush( stdout );
	if (verbosity) while(getchar()!=10);

	pl=fopen( target, "w" );
	if( NULL == pl ) fail( "Could not open ", target, errno );

	root=recurse( curdir, root, blacklist );
	shufflem3u( root, pl );
	/*
	while( NULL != root ){
		fprintf( pl, "%s/%s\n", root->path, root->name );
		root=root->prev;
	}
	 */

	fclose( pl );
	printf("Done.\n");

	return 0;
}
