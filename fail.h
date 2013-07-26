#ifndef FAIL_H
#define FAIL_H

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void fail( const char* msg, const char* info, int error );
#endif
