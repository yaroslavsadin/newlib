/*
   Copyright (c) 2022, Synopsys, Inc. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   1) Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   2) Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

   3) Neither the name of the Synopsys, Inc., nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

// A simple, comprehensive memchr validation test

#define POINTER_RANGE sizeof(void*)
#define BUFFER_SIZE 128
#define MAX_ERRORS 5
#define MAX_BUFFER_DIFFERENCE 10

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

char buffer[BUFFER_SIZE * 2];
unsigned int run= 0;
unsigned int errors = 0;

void reset(){
	memset(buffer, 0, sizeof(buffer));
}

void* mymemchr(const void *s, int c, size_t n){
	uint8_t* p = (uint8_t*)s;
	// Truncate input
	c = c & 0xff;
	while(0 != n--){
		if(c == *p++){
			return --p;
		}
	}
	return NULL;
}

void* memchr_wrapper(const void *s, int c, size_t n){
	void* ret;
	ret = memchr(s, c, n);
	return ret;
}

void test_memchr(size_t pd, size_t zd, size_t size){
	void* ret1, *ret2;

	run++;
	reset();
	buffer[zd] = 0xaf;

	// memchr should only look into 1 byte, so 0xfaf is the same as 0xaf
	ret1 = mymemchr(buffer + pd, 0xfaf, size);
	ret2 = memchr_wrapper(buffer + pd, 0xfaf, size);
	if(ret1 != ret2){
		 printf("Buffer at %p\nwrong position, received  %d (%p), was expecting %d (%p)\npointer disalignment: %d\nTarget byte position: %d\nsize: %d\non run %d\n\n", buffer, ret2 - (void*)buffer, ret2, ret1 - (void*)buffer, ret1, pd, zd, size, run);
		 errors++;
	}

	if(errors > MAX_ERRORS){
		printf("Too many errors, exiting\n");
		abort();
	}
}


int main(int argc, char* argv[]){
	for(size_t size = 0; size <= BUFFER_SIZE; size++){
		// Test disalignment of initial pointer
		for(size_t pd = 0; pd <= POINTER_RANGE; pd++){
			// Test disalignment of character terminator	

			// Right at the beggining (small search)
			for(size_t zd = 0; zd <= POINTER_RANGE; zd++){
				test_memchr(pd, zd, size);
			}

			// Right at the end (big search)
			for(size_t zd = 0; zd <= POINTER_RANGE; zd++){
				test_memchr(pd, size - zd, size);
			}
		}
	}

	if(0 != errors){
		abort();
	}
	return 0;
}






