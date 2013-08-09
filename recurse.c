#include "recurse.h"
#include "fail.h"
#include "activity.h"

char *toLower( char *text ){
	int i;
	for(i=0;i<strlen(text);i++) text[i]=tolower(text[i]);
	return text;
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
		}
	}
	
	free( buff );
	fclose( file );
	return root;
}

int isMusic( const char *name ){
	char loname[MAXPATHLEN];
	strncpy( loname, name, MAXPATHLEN );
	toLower( loname );	
	if( strstr( loname, ".mp3" ) || strstr( loname, "ogg" ) ) return -1;
	return 0;
}

int isValid( const char *name, struct blacklist_t *bl ){
/**
 * filters out invalid files/dirs like hidden (starting with '.')
 * or names that are in the blacklist.
 */
	char loname[MAXPATHLEN];
	struct blacklist_t *ptr = bl;
		
	if( name[0] == '.' ) return 0;
	
	strncpy( loname, name, MAXPATHLEN );
	toLower( loname );
	
	while( ptr ){
		if( strstr( loname, ptr->dir ) ) return 0;
		ptr=ptr->next;
	}
	return -1;
}

struct entry_t *recurse( char *curdir, struct entry_t *files, struct blacklist_t *bl ){
/* */
	struct entry_t *buff=NULL;
	char dirbuff[MAXPATHLEN];
	DIR *directory;
	FILE *file;
	struct dirent *entry;
	char *pos;
	
	directory=opendir( curdir );
	if (directory != NULL){
		entry = readdir( directory );
		activity();
		while( entry != NULL ){
			if( isValid( entry->d_name, bl ) ){
				sprintf( dirbuff, "%s/%s", curdir, entry->d_name );
				files=recurse( dirbuff, files, bl );
			}
			entry = readdir( directory );
		}
		closedir( directory );
	}else{
		if( isMusic( curdir ) ){
			file=fopen(curdir, "r");
			if( NULL == file ) fail("Couldn't open file ", curdir,  errno);
			if( -1 == fseek( file, 0L, SEEK_END ) ) fail( "fseek() failed on ", curdir, errno );
			buff=(struct entry_t *)malloc(sizeof(struct entry_t));
			if(buff == NULL) fail("Out of memory!", "", errno);
			buff->prev=files;
			buff->next=NULL;
			if(files != NULL)files->next=buff;
		
			pos=strrchr( curdir , '/' );
			if( NULL == pos ){
				strcpy( buff->name, curdir );
				buff->path[0]='\0';
				fprintf( stderr, "No '/' in %s! This should not be possible!", curdir );
			}else{
				strcpy( buff->name, pos+1 );
				*pos='\0';
				strcpy( buff->path, curdir );
			}
			buff->length=ftell( file )/1024;
			files=buff;
			fclose(file);
		}
	}

	return files;	
}
