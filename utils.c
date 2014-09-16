#include "utils.h"

int rpos=0;

/**
 * show activity roller on console
 */
void activity(){
	char roller[5]="|/-\\";

	rpos=(rpos+1)%4;			
	printf( "%c\r", roller[rpos] ); fflush( stdout );
}	

void fail( const char* msg, const char* info, int error ){
/*
 * Print errormessage, errno and quit
 * msg - Message to print
 * info - second part of the massage, for instance a variable
 * error - errno that was set.
 */
	if(error < 1 )
		fprintf(stderr, "\n%s%s\n", msg, info );
	else
		fprintf(stderr, "\n%s%s\nERROR: %i - %s\n", msg, info, error, strerror( error ) );
	fprintf(stderr, "Press [ENTER]\n" );
	fflush( stdout );
	fflush( stderr );
	while(getc(stdin)!=10);
	if (error < 0 ) exit(error);
	return;
}

/*
 * Inplace conversion of a string to lowercase
 */
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

/**
 * Check if a file is a music file
 */
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

void setBit( int pos, strval_t val ){
	int bytepos;
	unsigned char set=0;
	bytepos=pos/8;
	set = 1<<(pos%8);
	val[bytepos]|=set;
}

int getMax( strval_t val ){
	int pos, retval=0;
	unsigned char c;
	for(pos=0; pos<ARRAYLEN;pos++){
		c=val[pos];
		while( c != 0 ){
			if( c &  1 ) retval++;
			c=c>>1;
		}
	}
	return retval;
}

int computestrval( const char* str, strval_t strval ){
	char c1, c2;
	int cnt;

	for( cnt=0; cnt < strlen( str )-1; cnt++ ){
		c1=str[cnt];
		c2=str[cnt+1];
		if( isalpha( c1 )  && isalpha( c2 ) ){
			c1=tolower( c1 );
			c2=tolower( c2 );
			c1=c1-'a';
			c2=c2-'a';
			setBit( c1*26+c2, strval );
		}
	}
	return getMax( strval );
}

int vecmult( strval_t val1, strval_t val2 ){
	int result=0;
	int cnt;
	unsigned char c;

	for( cnt=0; cnt<ARRAYLEN; cnt++ ){
		c=val1[cnt] & val2[cnt];
		while( c != 0 ){
			if( c &  1 ) result++;
			c=c>>1;
		}
	}

	return result;
}

int fnvcmp( const strval_t val1, const strval_t val2 ){
	int max1, max2;
	max1=getMax( val1 );
	max2=getMax( val2 );
	max1=(max1<max2)?max1:max2;
	// Comparisons with less than 4 pairs are pretty meaningless
	if( max1 < 4 ) return -1;
	return( (100*vecmult( val1, val2 ))/max1);
}

/**
 * Compares two strings and returns the similarity index
 * 100 == most equal
 **/
int fncmp( const char* str1, const char* str2 ){
	strval_t str1val, str2val;
	int maxval, max1, max2;
	long result;
	float step;

	str1val=calloc( ARRAYLEN, sizeof( char ) );
	str2val=calloc( ARRAYLEN, sizeof( char ) );
	memset( str1val, 0, ARRAYLEN );
	memset( str2val, 0, ARRAYLEN );

	max1=computestrval( str1, str1val );
	max2=computestrval( str2, str2val );

	maxval=(max1 < max2) ? max1 : max2;
	if( maxval < 4 ) return -1;
	step=100.0/maxval;

	result=vecmult(  str1val, str2val );

	free( str1val );
	free( str2val );

	return step*result;
}
