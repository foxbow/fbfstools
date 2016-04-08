#include "utils.h"
#include <getopt.h>

long freekb(const char * part) {
	struct statvfs buf;
	if (0 == statvfs(part, &buf))
		return (buf.f_bavail*buf.f_bsize)/1024;
	else
		fail( "Couldn't get free space for ", part, errno );
	return 0;
}

/* 
 * Deletes all files in the given directory.
 * Ignores subdirs and files starting with a dot.
 */
void clean( char* target ){
	char buff[MAXPATHLEN];
	DIR *directory;
	DIR *dirbuff;
	struct dirent *entry;

	directory=opendir( target );
	if ( NULL == directory ){
		fail( "Target is no directory ", target, errno );
	}

	entry = readdir( directory );
	while( entry != NULL ){
		if( entry->d_name[0]!='.' ){
			sprintf( buff, "%s%s", target, entry->d_name );
			dirbuff=opendir( buff );
			if( NULL == dirbuff ){
				if( -1 == unlink( buff ) ) fail( "Couldn't delete file ", buff, errno );
				if( getVerbosity() > 0 ){
					activity();
				}
			}else{
				closedir( dirbuff );
			}
		}
		entry = readdir( directory );
	}
	closedir( directory );
}

/*
 * copy the given title onto the target. The target file
 * will be named track<index>.mp3
 */
void copy( struct entry_t *title, const char* target, int index ){
	FILE *in, *out;
	char filename[ MAXPATHLEN ];
	unsigned char *buffer;
	int size;

	buffer=malloc( BUFFSIZE );
	if( NULL == buffer ) fail( "Out of memory!", "", errno );
	sprintf( filename, "%strack%03i.mp3", target, index);

	if( getVerbosity() > 2 ) printf( "Copy %s to %s\r", title->path, filename );
	else if( getVerbosity() == 1 )  printf( "Copy Track %03i\r", index );

	in=fopen( title->path, "rb" );
	if( NULL == in ) fail( "Couldn't open infile ", title->path, errno );

	out=fopen( filename, "wb" );
	if( NULL == out ) fail( "Couldn't open outfile ", filename, errno );

	size = fread( buffer, sizeof( unsigned char ), BUFFSIZE, in );
	while( 0 != size ){
		if( 0 == fwrite( buffer, sizeof( unsigned char ), size, out ) )
			fail( "Target is full!", "", errno );
		size = fread( buffer, sizeof( unsigned char ), BUFFSIZE, in );
		if( getVerbosity() > 0 ){
			if( getVerbosity() > 2 ) printf( "Copy %s to %s ", title->path, filename );
			else if( getVerbosity() == 1 )  printf( "Copy Track %03i ", index );
			activity();
		}
	}

	if( getVerbosity() > 0 ){
		if( getVerbosity() > 2 ) printf( "Copy %s to %s ", title->path, filename );
		else if( getVerbosity() == 1 )  printf( "Copy Track %03i ", index );
	}
	fclose( in );
	fclose( out );
	free(buffer);
}

/**
 * copies random tracks into the target dir.
 * list - pointer to the root of the list of tracks.
 * maxlen - maximum size in kB
 * target - target path ending with '/'
 */
void writePlaylist( struct entry_t *list, long int maxlen, const char* target ){
	struct entry_t *base;
	int i, cnt=1, no=0;
	long int size=512; /* 512k Overhead - probably needs to be converted to x% */
	long int maxsize=freekb( target ); /* Space left on device ... */
	if( 0 == maxlen ) size=maxsize;
	else if ( maxsize < maxlen ) maxlen=maxsize;

	while( list->prev != NULL ){
		list=list->prev;
		cnt++;
	}
	base=list;

	if ( getVerbosity() > 2 ) printf("Found %i titles\n", cnt);

	/* Stepping through every item and tossing it away afterwards */
	for(i=cnt; i>0; i--){
		int j, pos;
		list=base;
		pos=RANDOM(i);
		for(j=1; j<=pos; j++){
			list=list->next;
		}

		/* Check if the current file is a valid one */
		if ( ( list->length < MAXSIZE ) && ( list->length > MINSIZE ) ) {
			/* Are we filling the whole device? */
			if( 0 == maxlen ){
				if( list->length < size ){
					copy( list, target, no );
					no++;
					size=freekb( target );
					if( getVerbosity() > 0 ) printf("[%3li%%]\n", 100-((100*size)/maxsize));
				}				
			}else{			
				size=size+list->length;
				if ( size < maxlen ) {
					copy( list, target, no );
					no++;
					if( getVerbosity() ) printf("[%3li%%]\n", (100*size)/maxlen);
				}
			}
		}

		if(list->prev != NULL){
			list->prev->next=list->next;
		}
		if(list->next != NULL){
			list->next->prev=list->prev;
		}
		if(list==base) base=list->next;
		free(list);
	}
}

/**
 * print out CLI usage
 */
void usage( char *progname ){
	printf( "%s - copy music onto a player\n", progname );
	printf( "Usage: %s [-s <sourcedir>] [-t <tagetdir>] [-m <megabytes>] [-n] [-l <path to blacklist>] [-v <verbosity>]\n", progname );
	printf( "-s <path> : set path to directory with music [current dir]\n" );
	printf( "-t <path> : set path to target directory/device [%s]\n", TARGETDIR );
	printf( "-m <num>  : Size of target dir (0=all) [0]\n" );
	printf( "-n        : Do not clean target dir [off]\n" );
	printf( "-l <file> : Blacklist of names to exclude [unset]\n" );
	printf( "-v <num>  : Verbosity (0=quiet, 1=process, 2=1+wait for enter, 3=2+verbose output) [1]\n" );
	exit(0);
}

/**
 * CLI interface
 */
int main( int argc, char **argv ){
	unsigned long free;
	int delete=-1;
	struct entry_t *list;

	char curdir[MAXPATHLEN];
	char target[MAXPATHLEN];
	char blname[MAXPATHLEN];
	char c;
	int mbsize=0;

	srand((unsigned int)time(NULL));
	if( NULL == getcwd( curdir, MAXPATHLEN ) )
		fail( "Could not get current dir!", "", errno );
	strcpy( target, TARGETDIR );

	while( ( c = getopt( argc, argv, "s:t:m:nb:v:" ) ) != -1 ) {
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
			if ( target[ strlen( target ) -1 ] != '/' )
				sprintf( target, "%s/", target );
			break;
		case 'm':
			mbsize = atoi( optarg )*1024;
			break;
		case 'b':
			strcpy( blname, optarg );
			loadBlacklist( blname );
			break;
		case 'v':
			setVerbosity(atoi( optarg ));
			break;
		case 'n':
			delete = 0;
			break;
		default:
			usage( argv[0] );
			break;
		}
	}

	/* Print info and give chance to bail out */
	if( getVerbosity() > 0 ){
		printf( "Source: %s\n", curdir );
		printf( "Target: %s\n", target );
		if( 0 == mbsize )
			printf( "Filling all space on target\n" );
		else
			printf( "Size  : %i MB\n", mbsize/1024 );
		printf( "Using MP3 files between %i and %i MB\n", MINSIZE/1024, MAXSIZE/1024 );
		if( 0 != blname[0] )
			printf( "Using %s as blacklist\n", blname );
		if(!delete)
			printf( "Keeping existing files\n" );
		printf( "Press enter to continue or CTRL-C to stop.\n" );
		fflush( stdout );
		while(getc(stdin)!=10);
	}

	/* Delete all files on the target */
	if(delete){
		if( getVerbosity() ) printf( "Cleaning %s\n", target );
		clean( target );
		if( getVerbosity() ) {
			printf( "OK\n" );
			fflush( stdout );
		}
	}

	free=freekb( target );
	if ( 0 == mbsize ){
		if( getVerbosity() ) printf( "Size: %li MB\n", free/1024 );
	} else 	if ( getVerbosity() > 1)
		printf( "Space on device: %liM\n", free/1024 );

	/* Create title database */
	if( getVerbosity() ) printf("Scanning %s for mp3 files ..\n", curdir );
	fflush( stdout );
	list = recurse( curdir, NULL );
	if( getVerbosity() ){
		printf( "OK\n");
		fflush( stdout );
	}

	/* Copy titles */
	if(list == NULL){
		if( getVerbosity() ) printf ("No music found in %s\n", curdir );
	}else{
		writePlaylist( list, mbsize, target );
	}

	if( getVerbosity() ){
		printf("Done.\n");
		fflush( stdout );
	}
	if( getVerbosity() > 1 )	while(getc(stdin)!=10);

	return 0;
}
