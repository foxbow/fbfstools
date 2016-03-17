#include "utils.h"

#include <getopt.h>


// struct entry_t *root=NULL;

/**
 * print out CLI usage
 */
void usage( char *progname ){
	printf( "Usage: %s [-s <sourcedir>] [-t <trigger>] [-r <range>]\n", progname );
	printf( "-s <path>    : set path to directory with music [current dir]\n" );
	printf( "-r <range>   : file size range for similarity [%i]k\n", RANGE );
	printf( "-t <trigger> : triggervalue for filename similarity [%i]%%\n", TRIGGER );
	exit(0);
}

int remtitle( struct entry_t *title ){
	char path[ MAXPATHLEN ];
	sprintf( path, "%s/%s", title->path, title->name );
	printf("Deleting %s!\n", path );
	if ( title->prev != NULL ) {
		if( title->prev->next  != NULL ) title->prev->next=title->next;
		else fail( "title->prev->next == NULL!", "", 0 );
	}
	if ( title->next != NULL ) {
		if( title->next->prev != NULL ) title->next->prev=title->prev;
		else fail( "title->next->prev == NULL", "", 0 );
	}
	// free( title );
	return remove( path ) ;
}

/**
 * CLI interface
 */
int main( int argc, char **argv ){
	int result=0;
	int trigger=TRIGGER;
	int range=RANGE;
	char curdir[MAXPATHLEN];
	char c;
	struct entry_t *runner, *buff;
	struct entry_t *root=NULL;

	if( NULL == getcwd( curdir, MAXPATHLEN ) )
		fail( "Could not get current directory!", "", errno);

	while( ( c = getopt( argc, argv, "s:t:r:" ) ) != -1 ) {
		switch( c ) {
		case 's': 
			strcpy( curdir, optarg );
			if( ( strlen(curdir) == 2 ) && ( curdir[1] == ':' ) ) 
				sprintf( curdir, "%s/", curdir );
			else if ( curdir[ strlen( curdir ) -1 ] == '/' )
				curdir[ strlen( curdir ) -1 ] = 0;
			break;
		case 't':
			trigger = atoi( optarg );
			break;
		case 'r':
			range = atoi( optarg );
			break;
		default:
			usage( argv[0] );
			break;
		}
	}

	// Print info and give chance to bail out
	printf( "Source: %s\n", curdir );
	printf( "Range: %i KB, Triggervalue: %i\n", range, trigger );
	printf( "Press enter to continue or CTRL-C to stop." );
	fflush( stdout );
	while(getc(stdin)!=10);

	root=recurse( curdir, root );

	printf("Done scanning\n");
	fflush( stdout );

	if( NULL == root ) fail( "No music found at ", curdir, 0 );

	// rewind list
	while(NULL != root->prev) root=root->prev;

	// check through while there's at least two remaining entries
	while( NULL != root->next ){
		runner=root->next;
		while( NULL != runner ){
			if( ABS( root->length - runner->length )  < range ){
				result=fncmp( root->name, runner->name );
				if(  result >= trigger ){
					printf( "(1) %s/%s\n", root->path, root->name );
					printf( "(2) %s/%s\n", runner->path, runner->name ); 
					printf( "(%i/%li) Delete (1/2) or keep? ",  result, ABS( root->length - runner->length )  ); fflush(stdout);
					c=getchar();
					while(c!=10){
						switch(c){
						case '1':
							if( root->prev == NULL )  fail( "Sorry, can't delete first title in list...", "", -1 );
							else{
								buff=root->prev;
								if(remtitle( root )) fail( "Could not delete file!", root->name, -1 );
								free(root);
								root=buff;
							}
							break;
						case '2':
							buff=runner->prev;
							if(remtitle( runner )) fail( "Could not delete file!", runner->name, -1 );
							free(runner);
							runner=buff;
							if( runner == root ) runner=runner->next;
							if( runner == NULL ) runner=root;
							break;
						}
						c=getchar();
					}
				}
			}	
			runner=runner->next;
		}
		if( root->next != NULL ){
			root=root->next;
			free( root->prev );
			root->prev=NULL;
		}
		activity();
	}
	free( root );

	printf("Done.\n");
	fflush( stdout );

	return 0;
}

