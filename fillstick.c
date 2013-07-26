#include <stdio.h>
#include <stdlib.h>
/* Directory access */
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

#ifdef __MINGW_H
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <dir.h>
#else
  #include <sys/types.h>
  #include <sys/statvfs.h>
#endif

/* Error */
#include <errno.h>
#include <strings.h>

#include <time.h>

/* Default values */
/* Max size of an MP3 file - to avoid full albums */
#define MAXSIZE 15*1024
/* Min size - to avoid fillers */
#define MINSIZE 1024
/* Buffer for copying files */
#define BUFFSIZE 512*1024
/* Default target for copying */
#define TARGETDIR "player/"

#ifndef MAXPATHLEN
  #define MAXPATHLEN 256
#endif
#define RANDOM(x) rand()%x

struct entry_t {
	struct entry_t *prev;
  char path[MAXPATHLEN];
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

/* 
 * derelict struct to rebuild directory structure in memory.
 * 
struct dirnode_t {
	struct dirnode_t *prev;
	char path[MAXPATHLEN];
	struct title_t *entries;
	struct dirnode_t *directories;
	struct dirnode_t *next;
};
 */

int verbosity = 1;
int rpos = 0;
char roller[5]="|/-\\";

void fail( const char* msg, const char* info, int error ){
/*
 * Print errormessage, errno and quit
 * msg - Message to print
 * info - second part of the massage, for instance a variable 
 * error - errno that was set.
 */ 
  if(0 == error)
		fprintf(stderr, "\n%s%s\n", msg, info );
  else		
		fprintf(stderr, "\n%s%s\nERROR: %i - %s\n", msg, info, error, strerror( error ) );
	fprintf(stderr, "Press [ENTER] to terminate.\n" );		
	fflush( stdout );
	fflush( stderr );
	while(getc(stdin)!=10);	
	exit(error);
}

char *toLower( char *text ){
	int i;
	for(i=0;i<strlen(text);i++) text[i]=tolower(text[i]);
	return text;
}

long freekb(const char * part) {
/**
 */
#ifdef __MINGW_H
DWORD free;
PULARGE_INTEGER buff=NULL;
	if(GetDiskFreeSpaceEx(part, (PULARGE_INTEGER)&free, buff, buff))
		return free/1024;
	else
	  fail( "Couldn't get free space for ", part, errno );	
#else
struct statvfs buf;
	if (0 == statvfs(part, &buf))
		return (buf.f_bavail*buf.f_bsize)/1024;
	else
		fail( "Couldn't get free space for ", part, errno );
#endif
/*
struct statfs s;
	if (0 == statfs(part, &s))
		return s.f_bavail;
	else
		fail( "Couldn't get free space for ", part, errno );
*/
	return 0;
}

int ismusic( char *file ){
/*
 * checks if the given filename ends on mp3
 */	
	if ( ( strstr( file, ".mp3" ) != NULL )
	  || ( strstr( file, ".MP3" ) != NULL )
	   ) return -1;
	else return 0;
}

struct blacklist_t *loadBlacklist( char *path ){
	FILE *file = NULL;
	struct blacklist_t *root = NULL;
	struct blacklist_t *ptr = NULL;
	char *buff;

	buff=malloc( MAXPATHLEN );
	if( !buff ) fail( "Out of memory", "", errno );
	
	file=fopen( path, "r" );
	if( !file ) fail("Couldn't open blacklist ", path,  errno);
	
	while( !feof( file ) ){
		buff=fgets( buff, MAXPATHLEN, file );
		if( buff && strlen( buff ) > 1 ){
			if( !root ){
				root=malloc( sizeof( struct blacklist_t ) );
				ptr=root;
			}else{
				ptr->next=malloc( sizeof( struct blacklist_t ) );
				ptr=ptr->next;
			}
			if( !ptr ) fail( "Out of memory!", "", errno );
			strncpy( ptr->dir, toLower(buff), strlen( buff )-1 );
			ptr->dir[ strlen(buff)-1 ]=0;
			if( verbosity > 1 ) {
				printf( "blacklist: %s\n", ptr->dir );
				fflush( stdout );
			}
		}
	}
	
  fflush( stdout );
  
	free( buff );
	fclose( file );
	return root;
}

int isValidFile( const char *name, struct blacklist_t *bl ){
/**
 * filters out invalid files/dirs like hidden (starting with '.')
 * or names that are in the blacklist.
 */
	char *buff;
	struct blacklist_t *ptr = bl;
	
	buff=calloc( MAXPATHLEN, sizeof( char ) );
	if (NULL == buff) fail( "Out of memory!", "", errno );
	
	if( name[0] == '.' ) return 0;
	strcpy( buff, name );
	toLower( buff );
	while( ptr ){
		if( strstr( buff, ptr->dir ) ){
			if( verbosity > 2 ){
				printf( "\n- %s (%s)", name, ptr->dir ); 
				fflush( stdout );
			}
			free(buff);
			return 0;
		}
		ptr=ptr->next;
	}
	free(buff);
	return -1;
}

void roll(){
	rpos=(rpos+1)%4;			
	printf( "%c\r", roller[rpos] ); fflush( stdout );
}	

struct entry_t *recurse( char *curdir, struct entry_t *files, struct blacklist_t *bl ){
/* */
	struct entry_t *buff=NULL;
	char dirbuff[MAXPATHLEN];
	DIR *directory;
	FILE *file;
	struct dirent *entry;
	
	directory=opendir( curdir );
	if (directory != NULL){
	  entry = readdir( directory );
		if( verbosity ){
			roll();
		}
 		while( entry != NULL ){
 			if( isValidFile( entry->d_name, bl ) ){
 		 		sprintf( dirbuff, "%s/%s", curdir, entry->d_name );
   		 	files=recurse( dirbuff, files, bl );
 			}
		  entry = readdir( directory );
  	}
		closedir( directory );
	}else{
		if(ismusic(curdir)){
			file=fopen(curdir, "r");
			if( NULL == file ) fail("Couldn't open file ", curdir,  errno);
			if( -1 == fseek( file, 0L, SEEK_END ) ) fail( "fseek() failed on ", curdir, errno );
			buff=(struct entry_t *)malloc(sizeof(struct entry_t));
			if(buff == NULL) fail("Out of memory!", "", errno);
			buff->prev=files;
			buff->next=NULL;
			if(files != NULL)files->next=buff;
			strcpy( buff->path, curdir );
			buff->length=ftell( file )/1024;
			files=buff;
			fclose(file);
		}
	}

	return files;	
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
				if( verbosity > 0 ){
					roll();
				}
			}else{
				closedir( dirbuff );
			}
		}
	  entry = readdir( directory );
	}
	closedir( directory );
}

void copy( struct entry_t *title, const char* target, int index ){
/* 
 * copy the given title onto the target. The target file
 * will be named track<index>.mp3
 */
	FILE *in, *out;
	char filename[ MAXPATHLEN ];
	unsigned char *buffer;
	int size;
	
	buffer=malloc( BUFFSIZE );
	if( NULL == buffer ) fail( "Out of memory!", "", errno );
	sprintf( filename, "%strack%03i.mp3", target, index);
	
	if( verbosity > 2 ) printf( "Copy %s to %s\r", title->path, filename );
	else if( verbosity == 1 )  printf( "Copy Track %03i\r", index );
	
	in=fopen( title->path, "rb" );
	if( NULL == in ) fail( "Couldn't open infile ", title->path, errno );
	
	out=fopen( filename, "wb" );
	if( NULL == out ) fail( "Couldn't open outfile ", filename, errno );
	
	size = fread( buffer, sizeof( unsigned char ), BUFFSIZE, in );
	while( 0 != size ){
		if( 0 == fwrite( buffer, sizeof( unsigned char ), size, out ) )
			fail( "Target is full!", "", errno );
		size = fread( buffer, sizeof( unsigned char ), BUFFSIZE, in );
		if( verbosity > 0 ){
			if( verbosity > 2 ) printf( "Copy %s to %s ", title->path, filename );
			else if( verbosity == 1 )  printf( "Copy Track %03i ", index );
			roll();
		}
	}
	
	if( verbosity > 0 ){
		if( verbosity > 2 ) printf( "Copy %s to %s ", title->path, filename );
		else if( verbosity == 1 )  printf( "Copy Track %03i ", index );
	}
	fclose( in );
	fclose( out );
	free(buffer);
}

void writePlaylist( struct entry_t *list, long int maxlen, const char* target ){
/**
 * copies random tracks into the target dir.
 * list - pointer to the root of the list of tracks.
 * maxlen - maximum size in kB
 * target - target path ending with '/'
 */
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

  if (verbosity > 2 ) printf("Found %i titles\n", cnt);

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
					if( verbosity > 0 ) printf("[%3li%%]\n", 100-((100*size)/maxsize));
				}				
			}else{			
		  	size=size+list->length;
				if ( size < maxlen ) {
					copy( list, target, no );
					no++;
					if( verbosity > 0 ) printf("[%3li%%]\n", (100*size)/maxlen);
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

void usage( char *progname ){
/**
 * print out CLI usage
 */
	printf( "Usage: %s [-s <sourcedir>] [-t <tagetdir>] [-m <megabytes>] [-n] [-l <path to blacklist>] [-v <verbosity>]\n", progname );
	printf( "-s <path> : set path to directory with music [current dir]\n" );
	printf( "-t <path> : set path to target directory/device [%s]\n", TARGETDIR );
	printf( "-m <num>  : Size of target dir (0=all) [0]\n" );
	printf( "-n        : Do not clean target dir [off]\n" );
	printf( "-l <file> : Blacklist of names to exclude [unset]\n" );
	printf( "-v <num>  : Verbosity (0=quiet, 1=process, 2=1+wait for enter, 3=2+verbose output) [1]\n" );
	exit(0);
}

int main( int argc, char **argv ){
/**
 * CLI interface
 */
	unsigned long free;
	int delete=-1;
	struct entry_t *list;
	struct blacklist_t *blacklist=NULL;
	
	char curdir[MAXPATHLEN];
	char target[MAXPATHLEN];
	char blname[MAXPATHLEN];
	char c;
	int mbsize=0;
	
	srand((unsigned int)time(NULL));
	getcwd( curdir, MAXPATHLEN );
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
      	blacklist=loadBlacklist( blname );
      	break;
      case 'v':
      	verbosity = atoi( optarg );
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
  if( verbosity > 0 ){
		printf( "Source: %s\n", curdir );
		printf( "Target: %s\n", target );
		if( 0 == mbsize )
			printf( "Filling all space on target\n" );
		else
			printf( "Size  : %i MB\n", mbsize/1024 );
		printf( "Using MP3 files between %i and %i MB\n", MINSIZE/1024, MAXSIZE/1024 );
		if( blacklist )
			printf( "Using %s as blacklist\n", blname );
    if(!delete)
      printf( "Keeping existing files\n" );
		printf( "Press enter to continue or CTRL-C to stop.\n" );
		fflush( stdout );
		while(getc(stdin)!=10);
  }

/* Delete all files on the target */	
  if(delete){
		if( verbosity > 0 ) printf( "Cleaning %s\n", target ); 
		clean( target );
		if( verbosity > 0 ) {
			printf( "OK\n" );
			fflush( stdout );
		}
  }

	free=freekb( target );
	if ( 0 == mbsize ){
		if( verbosity > 0 ) printf( "Size: %li MB\n", free/1024 );
	} else 	if (verbosity > 1) 
		printf( "Space on device: %liM\n", free/1024 );

/* Create title database */
	if( verbosity > 0 ) printf("Scanning %s for mp3 files ..\n", curdir );
	fflush( stdout );
	list = recurse( curdir, NULL, blacklist );
	if( verbosity > 0 ){
	 	printf( "OK\n");
	  fflush( stdout );
	}
  
/* Copy titles */
	if(list == NULL){
			if( verbosity > 0 ) printf ("No music found in %s\n", curdir );
	}else{
		writePlaylist( list, mbsize, target );
	}

	if( verbosity > 0 ){
	 	printf("Done.\n");
		fflush( stdout );
	}
	if( verbosity > 1 )	while(getc(stdin)!=10);
	
	return 0;
}

