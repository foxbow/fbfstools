/*
 * mkplaylists.c
 *
 *  Created on: 15.09.2014
 *      Author: bweber
 */

#include "utils.h"
#include <sys/stat.h>

int compare( const void* op1, const void* op2 )
{
    const char **p1 = (const char **) op1;
    const char **p2 = (const char **) op2;

    return( strcmp( *p1, *p2 ) );
}

struct entry_t *sort( struct entry_t *files ){
	char **titles;
	int num=0;
	int i;
	struct entry_t *buff=files;

	while( buff != NULL ) {
		num++;
		buff=buff->next;
	}

	titles=(char **)calloc( num, sizeof(char *) );
	for( i=0; i<num; i++) titles[i]=(char*)calloc( MAXPATHLEN, sizeof( char ) );

	buff=files;
	i=0;
	while( buff != NULL ) {
		strcpy( titles[i++], buff->name );
		buff=buff->next;
	}

    qsort( titles, num, sizeof( char * ), compare );

	buff=files;
	i=0;
	while( buff != NULL ) {
		strcpy( buff->name, titles[i++] );
		buff=buff->next;
	}

	for( i=0; i<num; i++) free(titles[i]);
	free(titles);

    return files;
}

struct entry_t *wipe( struct entry_t *files ){
	struct entry_t *buff=files;
	while( buff != NULL ){
		files=buff;
		buff=files->next;
		free(files);
	}
	return NULL;
}

int traverse( char *curdir ){
/* */
	struct entry_t *files=NULL;
	struct entry_t *buff=NULL;
	char dirbuff[MAXPATHLEN];
	char plname[MAXPATHLEN];
	struct stat st;
	DIR *directory;
	FILE *file;
	struct dirent *entry;
	char *pos;

	directory=opendir( curdir );
	if (directory != NULL){
		entry = readdir( directory );
		activity();
		while( entry != NULL ){
			if( isValid( entry->d_name, NULL ) ){
				sprintf( dirbuff, "%s/%s", curdir, entry->d_name );
				if( stat( dirbuff, &st ) ) {
					fail( "stat() failed on: ", curdir, errno );
				}
				if( S_ISDIR( st.st_mode ) ) {
					traverse( dirbuff );
				} else {
					if( strstr( dirbuff, ".mp3" ) ) {
						buff=(struct entry_t *)malloc(sizeof(struct entry_t));
						if(buff == NULL) fail("Out of memory!", "", errno);
						buff->prev=files;
						buff->next=NULL;
						if(files != NULL)files->next=buff;

						pos=strrchr( dirbuff , '/' );
						if( NULL == pos ){
							strcpy( buff->name, dirbuff );
							buff->path[0]='\0';
							fprintf( stderr, "No '/' in %s! This should not be possible!", curdir );
						}else{
							strcpy( buff->name, pos+1 );
							*pos='\0';
							strcpy( buff->path, dirbuff );
						}
						files=buff;
					}
					if( strstr( dirbuff, ".m3u" ) ) {
						files=wipe(files);
						break;
					}
				}
			} // skip hidden files/dirs
			entry = readdir( directory );
		}
		closedir( directory );

		if(files){
			char *p0, *p1;
			strcpy( plname, curdir );
			strcat( plname, "/" );
			p1=strrchr( curdir, '/' );
			if( p1 == NULL ) {
				strcat( plname, "playlist" );
			} else {
				*p1=0;
				p1++;
				p0=strrchr( curdir, '/' );
				if( p0 == NULL ) {
					strcat( plname, p1 );
				} else {
					*p0=0;
					p0++;
					strcat( plname, p0 );
					strcat( plname, " - " );
					strcat( plname, p1 );
				}
			}
			strcat( plname, ".m3u" );

			file=fopen( plname, "w" );
			if( ! file ) fail( "Write open failed for ", plname, errno );

			buff=files;
			while( buff->prev != NULL ) buff=buff->prev;
			buff=sort(buff);
			while( buff != NULL ){
				fprintf( file, "%s\n", buff->name );
				buff=buff->next;
			}

			plname[0]=0;
			files=wipe(files);
		}
	}else{
		fprintf( stderr, "%s is not a directory!\n", curdir );
	}

	return 0;
}


int main( int argc, char **argv ) {
	traverse( "." );
	return 0;
}
