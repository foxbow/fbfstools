#include "utils.h"

// Represents a string as a bit array
typedef unsigned char* strval_t;

int rpos=0;
static int _ftverbosity=1;
struct blacklist_t *blacklist=NULL;

int getVerbosity() {
	return _ftverbosity;
}

int setVerbosity(int v) {
	_ftverbosity=v;
	return _ftverbosity;
}

int incVerbosity() {
	_ftverbosity++;
	return _ftverbosity;
}

void muteVerbosity() {
	_ftverbosity=0;
}
/**
 * Some ANSI code magic to set the terminal title
 **/
void setTitle(const char* title) {
	char buff[128];
	strncpy(buff, "\033]2;", 128 );
	strncat(buff, title, 128 );
	strncat(buff, "\007\000", 128 );
	fputs(buff, stdout);
	fflush(stdout);
}

/**
 * Strip spaces and special chars from the beginning and the end
 * of 'text', truncate it to 'maxlen' and store the result in
 * 'buff'.
**/
char *strip( char *buff, const char *text, const size_t maxlen ) {
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
 * takes a directory and tries to guess info from the structure
 * Either it's Artist/Album for directories or just the Artist from an mp3
 */
char *genPathName( char *name, const char *cd, const size_t len ){
	char *p0, *p1, *p2;
	char curdir[MAXPATHLEN];
	int pl;

	// Create working copy of the path and cut off trailing /
	strncpy( curdir, cd, MAXPATHLEN );
	if( '/' == curdir[strlen(curdir)] ) {
		curdir[strlen(curdir)-1]=0;
		pl=1; // generating playlist name
	}

	// cut off .mp3
	p0 = strstr( curdir, ".mp3" );
	if( NULL != p0 ) {
		*p0=0;
		pl=0; // guessing artist/title combination
	}

	strcpy( name, "" );

	p0=strrchr( curdir, '/' );
	if( NULL != p0  ) {
		// cut off the last part
		*p0=0;
		p0++;

		p1=strrchr( curdir, '/' );
		if( ( NULL == p1 ) && ( strlen(curdir) < 2 ) ) {
			// No second updir found, so it's just the last dir name
			strncat( name, p0, len  );
		} else {
			if( NULL == p1 ) {
				p1 = curdir;
			} else {
				*p1=0;
				p1++;
			}
			if( pl ) {
				strncat( name, p1, len );
				strncat( name, " - ", len );
				strncat( name, p0, len );
			}
			else {
				p2=strrchr( curdir, '/' );
				if( NULL == p2 ) {
					p2=p1;
				}
				else {
					*p2=0;
					p2++;
				}
				pl=0;
				if( isdigit(p0[0]) ) {
					pl=1;
					if( isdigit(p0[1]) ) {
						pl=2;
					}
				}
				if( ( pl > 0 ) && ( ' ' == p0[pl] ) ) {
					p0=p0+pl+3;
				}
				strncat( name, p2, len );
				strncat( name, " - ", len );
				strncat( name, p0, len );
			}
		}
	}

	return name;
}

/*
 * Print errormessage, errno and quit
 * msg - Message to print
 * info - second part of the massage, for instance a variable
 * error - errno that was set.
 */
void fail( const char* msg, const char* info, int error ){
	endwin();
	if(error == 0 )
		fprintf(stderr, "\n%s%s\n", msg, info );
	else
		fprintf(stderr, "\n%s%s\nERROR: %i - %s\n", msg, info, ABS(error), strerror( ABS(error) ) );
	fprintf(stderr, "Press [ENTER]\n" );
	fflush( stdout );
	fflush( stderr );
	while(getc(stdin)!=10);
	if (error > 0 ) exit(error);
	return;
}

/**
 * Draw a horizontal line
 */
void dhline(int r, int c, int len) {
	mvhline(r, c + 1, HOR, len - 1);
	mvprintw(r, c, EDG );
	mvprintw(r, c + len, EDG );
}

/**
 * Draw a vertical line
 */
void dvline(int r, int c, int len) {
	mvhline(r + 1, c, VER, len - 2);
	mvprintw(r, c, EDG );
	mvprintw(r + len, c, EDG );
}

/**
 * draw a box
 */
void drawbox(int r0, int c0, int r1, int c1) {
	dhline(r0, c0, c1 - c0);
	dhline(r1, c0, c1 - c0);
	mvvline(r0 + 1, c0, VER, r1 - r0 - 1);
	mvvline(r0 + 1, c1, VER, r1 - r0 - 1);
}

/**
 * reads from the fd into the line buffer until either a CR
 * comes or the fd stops sending characters.
 * returns number of read bytes or -1 on overflow.
 */
int readline( char *line, size_t len, int fd ){
	int cnt=0;
	char c;

	while ( 0 != read(fd, &c, 1 ) ) {
		if( cnt < len ) {
			line[cnt]=c;
			cnt++;
			if( '\n' == c ) {
				if( cnt < len ) {
					line[cnt]=0;
					cnt++;
					return cnt;
				} else {
					return -1;
				}
			}
			// avoid reading behind string end
			if( 0 == c ) {
				return cnt;
			}
		} else {
			return -1;
		}
	}

	// avoid returning unterminated strings.
	if( cnt < len ) {
		line[cnt]=0;
		cnt++;
		return cnt;
	} else {
		return -1;
	}

	return cnt;
}

/**
 * checks if text ends with suffix
 * this function is case insensitive
 */
int endsWith( const char *text, const char *suffix ){
	int i, tlen, slen;
	tlen=strlen(text);
	slen=strlen(suffix);
	if( tlen < slen ) {
		return 0;
	}
	for( i=slen; i>0; i-- ) {
		if( tolower(text[tlen-i]) != tolower(suffix[slen-i]) ) {
			return 0;
		}
	}
	return -1;
}

/**
 * Check if a file is a music file
 */
int isMusic( const char *name ){
	if( endsWith( name, ".mp3" ) || endsWith( name, ".ogg" ) ) return -1;
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
 * @todo: needs some work!
 */
int isURL( const char *uri ){
	char line[MAXPATHLEN];
	strncpy( line, uri, MAXPATHLEN );
	toLower(line);
	if( strstr( line, "://" ) ) return -1;
	return 0;
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
 * clean up a list of entries
 */
void wipeTitles( struct entry_t *files ){
	struct entry_t *buff=files;
	while( buff != NULL ){
		files=buff;
		buff=files->next;
		free(files);
	}
}

/**
 * show activity roller on console
 * this will only show when the global verbosity is larger than 0
 */
void activity(){
	char roller[5]="|/-\\";
	if( _ftverbosity ) {
		rpos=(rpos+1)%4;
		printf( "%c\r", roller[rpos] ); fflush( stdout );
	}
}	

/*
 * Inplace conversion of a string to lowercase
 */
char *toLower( char *text ){
	int i;
	for(i=0;i<strlen(text);i++) text[i]=tolower(text[i]);
	return text;
}

/**
 * load a blacklist file into the blacklist structure
 */
int loadBlacklist( const char *path ){
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

	if( _ftverbosity > 2 ) {
		printf("Loaded %s with %i entries.\n", path, cnt );
	}

	free( buff );
	fclose( file );
	return 0;
}

/**
 * add a line to a playlist/blacklist
 */
void addToList( const char *path, const char *line ) {
	FILE *fp;
	fp=fopen( path, "a" );
	if( NULL == fp ) {
		fail( "Could not open list for writing ", path, errno );
	}
	fputs( line, fp );
	fputc( '\n', fp );
	fclose( fp );
}

/**
 * load a standard m3u playlist into a list of titles that the tools can handle
 */
struct entry_t *loadPlaylist( const char *path ) {
	FILE *fp;
	int cnt;
	struct entry_t *current=NULL;
	char *buff;

	buff=malloc( MAXPATHLEN );
	if( !buff ) fail( "Out of memory", "", errno );

	fp=fopen( path, "r" );
	if( !fp ) fail("Couldn't open playlist ", path,  errno);

	while( !feof( fp ) ){
		activity();
		buff=fgets( buff, MAXPATHLEN, fp );
		if( buff && ( strlen( buff ) > 1 ) && ( buff[0] != '#' ) ){
			current=addTitle( current, buff );
		}
	}
	fclose( fp );
	current=rewindTitles( current, &cnt );

	if( _ftverbosity > 2 ) {
		printf("Loaded %s with %i entries.\n", path, cnt );
	}

	return current;
}

/**
 * helperfunction to remove an entry from a list of titles
 */
struct entry_t *removeTitle( struct entry_t *entry ) {
	struct entry_t *buff=NULL;
	if( NULL != entry->next ) {
		buff=entry->next;
		buff->prev=entry->prev;
	}
	if( NULL != entry->prev ) {
		buff=entry->prev;
		buff->next=entry->next;
	}
	free(entry);
	return buff;
}

/**
 * helperfunction to add an entry to a list of titles
 * caveat: this does an insert after /base/ rather than an append to the end
 */
struct entry_t *addTitle( struct entry_t *base, const char *path ){
	struct entry_t *root;
	char buff[MAXPATHLEN];
	char *b;

	root = (struct entry_t*) malloc(sizeof(struct entry_t));
	if (NULL == root) {
		fail("Malloc failed", "", errno);
	}
	if( NULL != base ) {
		root->next=base->next;
		if( NULL != base->next ) {
			base->next->prev=root;
		}
		base->next=root;
	} else {
		root->next = NULL;
	}
	root->prev=base;
	root->length = 0;

	strcpy( buff, path );
	b = strrchr( buff, '/');
	if (NULL != b) {
		strncpy(root->name, b + 1, MAXPATHLEN);
		b[0] = 0;
		strncpy(root->path, buff, MAXPATHLEN);
	} else {
		strncpy(root->name, buff, MAXPATHLEN);
		strncpy(root->path, "", MAXPATHLEN);
	}
	strncpy(root->display, root->name, MAXPATHLEN );
	return root;
}

/**
 * move to the start of the list of titles
 */
struct entry_t *rewindTitles( struct entry_t *base, int *cnt ) {
	*cnt=0;
	// scroll to the end of the list
	while ( NULL != base->next ) base=base->next;
	// scroll back and count entries
	while ( NULL != base->prev ) {
		base=base->prev;
		(*cnt)++;
	}

	if (_ftverbosity > 0 ) printf("Found %i titles\n", *cnt);

	return base;
}

/**
 * mix a list of titles into a new order
 */
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

	if( _ftverbosity > 2 ) {
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
		genPathName( buff->display, dirbuff, MAXPATHLEN );
//		strncpy( buff->title, entry[i]->d_name, MAXPATHLEN );
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
