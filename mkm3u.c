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

void usage( char *progname ){
/**
 * print out CLI usage
 */
   printf( "Usage: %s [-s <sourcedir>] [-b <path to blacklist>] [-v <verbosity>] <playlist>\n", progname );
   printf( "-s <path> : set path to directory with music [current dir]\n" );
   printf( "-b <file> : Blacklist of names to exclude [unset]\n" );
   printf( "-v <num>  : Verbosity (0=quiet, 1=process, 2=1+wait for enter, 3=2+verbose output) [1]\n" );
   exit(0);
}

int main( int argc, char **argv ){
/**
 * CLI interface
 */
	int verbosity = 1;
	struct blacklist_t *blacklist=NULL;
	struct entry_t *root;
	
	char curdir[MAXPATHLEN];
	char target[MAXPATHLEN];
	char blname[MAXPATHLEN];
	char c;
   
	FILE *pl;
   
	if( NULL == getcwd( curdir, MAXPATHLEN ) ) fail( "Could not get current dir!", "", errno );

	strcpy( target, "playlist.m3u" );

	while( ( c = getopt( argc, argv, "s:b:t:v:" ) ) != -1 ) {
		switch( c ) {
		case 's': 
			strcpy( curdir, optarg );
			if( ( strlen(curdir) == 2 ) && ( curdir[1] == ':' ) ) 
				sprintf( curdir, "%s/", curdir );
			else if ( curdir[ strlen( curdir ) -1 ] == '/' )
				curdir[ strlen( curdir ) -1 ] = 0;
		break;
		case 't':
			strcpy( target, optarg );
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
	
	while( NULL != root ){
		fprintf( pl, "%s/%s\n", root->path, root->name );
		root=root->prev;
	}
	
	fclose( pl );
	printf("Done.\n");
	
	return 0;
}
