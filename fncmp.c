#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "fncmp.h"
/**
 * Similarity check for strings
 * Will check against pairs of letters
**/


// #define ARRAYLEN 676
#define ARRAYLEN 85
#define BITS 680

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
