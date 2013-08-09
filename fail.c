#include "fail.h"

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
