#include "utils.h"

// Represents a string as a bit array
typedef unsigned char* strval_t;

int rpos=0;
int verbosity=1;
struct blacklist_t *blacklist=NULL;

/**
 * Strip spaces and special chars from the beginning and the end
 * of 'text', truncate it to 'maxlen' and store the result in
 * 'buff'.
**/
char *strip( char *buff, const char *text, const int maxlen ) {
	int len=strlen( text );
	int pos=0;

	// Cut off leading spaces and special chars
	while( ( pos < len ) && ( isspace( text[pos] ) ) ) pos++;
	strncpy( buff, text+pos, maxlen );

	len=(len<maxlen)?len:maxlen;

	// Truncate the string to maximum length
	pos = strlen(buff)-1;
	while( pos >= len ) buff[pos--]='\0';

	// Cut off trailing spaces and special chars - for some reason
	// isspace() does not think that \r is a space?!
	while( ( pos > 0 ) && ( isspace( buff[pos] ) || buff[pos]==10 ) ) buff[pos--]='\0';

	return buff;
}

/**
 * Check if a file is a music file
 */
int isMusic( const char *name ){
	char loname[MAXPATHLEN];
	strncpy( loname, name, MAXPATHLEN );
	toLower( loname );
	if( strstr( loname, ".mp3" ) || strstr( loname, ".ogg" ) ) return -1;
	return 0;
}

/**
 * filters out invalid files/dirs like hidden (starting with '.')
 * or names that are in the blacklist.
 */
static int isValid( const char *name ){
	char loname[MAXPATHLEN];
	struct blacklist_t *ptr = blacklist;

	if( name[0] == '.' ) return 0;

	strncpy( loname, name, MAXPATHLEN );
	toLower( loname );

	while( ptr ){
		if( strstr( loname, ptr->dir ) ) return 0;
		ptr=ptr->next;
	}
	return -1;
}

/**
 * helperfunction for scandir() - just return unhidden directories
 */
static int dsel( const struct dirent *entry ){
	return( ( entry->d_name[0] != '.' ) && ( entry->d_type == DT_DIR ) );
}

/**
 * helperfunction for scandir() - just return unhidden regular files
 */
static int fsel( const struct dirent *entry ){
	return( ( entry->d_name[0] != '.' ) && ( entry->d_type == DT_REG ) );
}

/**
 * helperfunction for scandir() - just return unhidden regular music files
 */
static int msel( const struct dirent *entry ){
	return( ( entry->d_name[0] != '.' ) &&
			( entry->d_type == DT_REG ) &&
			isMusic( entry->d_name ) &&
			isValid( entry->d_name ) );
}

static int getMusic( const char *cd, struct dirent ***musiclist ){
	return scandir( cd, musiclist, msel, alphasort);
}

int getFiles( const char *cd, struct dirent ***filelist ){
	return scandir( cd, filelist, fsel, alphasort);
}

int getDirs( const char *cd, struct dirent ***dirlist ){
	return scandir( cd, dirlist, dsel, alphasort);
}

/**
 * helperfunction for sorting entries
 */
static int compare( const void* op1, const void* op2 )
{
	const char **p1 = (const char **) op1;
	const char **p2 = (const char **) op2;

	return( strcmp( *p1, *p2 ) );
}

/**
 * Sort a list of entries
 */
struct entry_t *sortTitles( struct entry_t *files ){
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

/**
 * clean up a list of entries
 */
struct entry_t *wipeTitles( struct entry_t *files ){
	struct entry_t *buff=files;
	while( buff != NULL ){
		files=buff;
		buff=files->next;
		free(files);
	}
	return NULL;
}


/**
 * show activity roller on console
 */
void activity(){
	char roller[5]="|/-\\";

	rpos=(rpos+1)%4;			
	printf( "%c\r", roller[rpos] ); fflush( stdout );
}	

/*
 * Print errormessage, errno and quit
 * msg - Message to print
 * info - second part of the massage, for instance a variable
 * error - errno that was set.
 */
void fail( const char* msg, const char* info, int error ){
	if(error == 0 )
		fprintf(stderr, "\n%s%s\n", msg, info );
	else
		fprintf(stderr, "\n%s%s\nERROR: %i - %s\n", msg, info, error, strerror( error ) );
	fprintf(stderr, "Press [ENTER]\n" );
	fflush( stdout );
	fflush( stderr );
	while(getc(stdin)!=10);
	if (error != 0 ) exit(error);
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

int loadBlacklist( char *path ){
	FILE *file = NULL;
	struct blacklist_t *ptr = NULL;
	char *buff;
	int cnt=0;

	if( NULL != blacklist ) {
		return -1;
	}

	buff=malloc( MAXPATHLEN );
	if( !buff ) fail( "Out of memory", "", errno );

	file=fopen( path, "r" );
	if( !file ) fail("Couldn't open blacklist ", path,  errno);

	while( !feof( file ) ){
		buff=fgets( buff, MAXPATHLEN, file );
		if( buff && strlen( buff ) > 1 ){
			if( !blacklist ){
				blacklist=malloc( sizeof( struct blacklist_t ) );
				ptr=blacklist;
			}else{
				ptr->next=malloc( sizeof( struct blacklist_t ) );
				ptr=ptr->next;
			}
			if( !ptr ) fail( "Out of memory!", "", errno );
			strncpy( ptr->dir, toLower(buff), strlen( buff )-1 );
			ptr->dir[ strlen(buff)-1 ]=0;
			cnt++;
		}
	}

	if( verbosity > 2 ) {
		printf("Loaded %s with %i entries.\n", path, cnt );
	}

	free( buff );
	fclose( file );
	return 0;
}

struct entry_t *rewindTitles( struct entry_t *base, int *cnt ) {
	*cnt=0;
	// scroll to the end of the list
	while ( NULL != base->next ) base=base->next;
	// scroll back and count entries
	while ( NULL != base->prev ) {
		base=base->prev;
		(*cnt)++;
	}

	if (verbosity > 0 ) printf("Found %i titles\n", *cnt);

	return base;
}

 struct entry_t *shuffleTitles( struct entry_t *base, int *cnt ) {
	struct entry_t *list=NULL;
	struct entry_t *end=NULL;
	struct entry_t *runner=NULL;

	int i, no=0;
	struct timeval tv;

	// improve 'randomization'
	gettimeofday(&tv,NULL);
	srand(getpid()*tv.tv_sec);

	base = rewindTitles( base, cnt );

	// Stepping through every item
	for(i=*cnt; i>0; i--){
		int j, pos;
		runner=base;
		pos=RANDOM(i);
		for(j=1; j<=pos; j++){
			runner=runner->next;
		}

		if ( ( runner->length < MAXSIZE ) && ( runner->length > MINSIZE ) ) {
			// Remove entry from base
			if(runner==base) base=runner->next;

			if(runner->prev != NULL){
				runner->prev->next=runner->next;
			}
			if(runner->next != NULL){
				runner->next->prev=runner->prev;
			}

			// add entry to list
			runner->next=NULL;
			runner->prev=end;
			if( NULL != end ) {
				end->next=runner;
			}
			end=runner;

			if( NULL == base ) {
				base=runner;
			}
		}
	}

//	base=list;
	return rewindTitles(end, cnt);
}

/*
 * Steps recursively through a directory and collects all music files in a list
 * curdir: current directory path
 * files:  the list to store filenames in
 */
struct entry_t *recurse( char *curdir, struct entry_t *files ) {
	struct entry_t *buff=NULL;
	char dirbuff[MAXPATHLEN];
	DIR *directory;
	FILE *file;
	struct dirent **entry;
	int num, i;
	char *pos;

	if( verbosity > 2 ) {
		printf("Checking %s\n", curdir );
	}
	// get all music files according to the blacklist
	num = getMusic( curdir, &entry );
	if( num < 0 ) {
		fail( "getMusic", curdir, errno );
	}
	for( i=0; i<num; i++ ) {
		activity();
		strncpy( dirbuff, curdir, MAXPATHLEN );
		if( '/' != dirbuff[strlen(dirbuff)] ) {
			strncat( dirbuff, "/", MAXPATHLEN );
		}
		strncat( dirbuff, entry[i]->d_name, MAXPATHLEN );
		file=fopen( dirbuff, "r");
		if( NULL == file ) fail("Couldn't open file ", dirbuff,  errno);
		if( -1 == fseek( file, 0L, SEEK_END ) ) fail( "fseek() failed on ", dirbuff, errno );

		buff=(struct entry_t *)malloc(sizeof(struct entry_t));
		if(buff == NULL) fail("Out of memory!", "", errno);
		buff->prev=files;
		buff->next=NULL;
		if(files != NULL)files->next=buff;

		strncpy( buff->name, entry[i]->d_name, MAXPATHLEN );
		strncpy( buff->path, curdir, MAXPATHLEN );
		buff->length=ftell( file )/1024;
		files=buff;
		fclose(file);
		free(entry[i]);
	}
	free(entry);

	num=getDirs( curdir, &entry );
	if( num < 0 ) {
		fail( "getDirs", curdir, errno );
	}
	for( i=0; i<num; i++ ) {
		activity();
		if( isValid( entry[i]->d_name ) ) {
			sprintf( dirbuff, "%s/%s", curdir, entry[i]->d_name );
			files=recurse( dirbuff, files );
		}
		free(entry[i]);
	}
	free(entry);

	return files;
}
/*
struct entry_t *recurse( char *curdir, struct entry_t *files, struct blacklist_t *bl ){
	struct entry_t *buff=NULL;
	char dirbuff[MAXPATHLEN];
	DIR *directory;
	FILE *file;
	struct dirent *entry;
	char *pos;

	directory=opendir( curdir );
	if (directory != NULL){
		entry = readdir( directory );
		if(verbosity) activity();
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
*/

/*
 * sets a bit in a long bitlist
 */
static void setBit( int pos, strval_t val ){
	int bytepos;
	unsigned char set=0;
	bytepos=pos/8;
	set = 1<<(pos%8);
	val[bytepos]|=set;
}

static int getMax( strval_t val ){
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

static int computestrval( const char* str, strval_t strval ){
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

static int vecmult( strval_t val1, strval_t val2 ){
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

	// the max possible matches are defined by the min number of bits set!
	maxval=(max1 < max2) ? max1 : max2;

	if( maxval < 4 ) return -1;
	step=100.0/maxval;

	result=vecmult(  str1val, str2val );

	free( str1val );
	free( str2val );

	return step*result;
}
