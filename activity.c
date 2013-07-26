#include <stdio.h>
int rpos=0;

void activity(){
	char roller[5]="|/-\\";

	rpos=(rpos+1)%4;			
	printf( "%c\r", roller[rpos] ); fflush( stdout );
}	
