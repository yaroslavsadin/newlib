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

// A minor strcat and strcpy corruption verification test

#define MAX_ERRORS 5
#define STRING_SIZE 128
#define BUFFER_SIZE STRING_SIZE * 2
#define MAX_BUFFER_DIFFERENCE 10

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char test_string[STRING_SIZE];

char test_buffer_src_1[BUFFER_SIZE];
char test_buffer_dest_1[BUFFER_SIZE];

char test_buffer_src_2[BUFFER_SIZE];
char test_buffer_dest_2[BUFFER_SIZE];

void reset(char filling_char){
	for(int i = 0; i < BUFFER_SIZE; i++){
		test_buffer_src_1[i] = filling_char;
		test_buffer_dest_1[i] = filling_char;

		test_buffer_src_2[i] = filling_char;
		test_buffer_dest_2[i] = filling_char;
	}
}


char* mystrcat(char *dest, const char *src){
	size_t dest_len = strlen(dest);
	size_t i;

	for (i = 0 ; src[i] != '\0' ; i++)
	   dest[dest_len + i] = src[i];
	dest[dest_len + i] = '\0';

	return dest;
}

 char* strcat_wrapper(char* dest, const char* src){
	char* ret;
	ret = strcat(dest, src);
	return ret;
}


void get_buffer_difference(const char* buf1, const char* buf1name, const char* buf2, const char* buf2name, size_t buf_size, const char* dst_string){
	char* sprintf_ptr;
	int different_indexes[buf_size];
	int differences;
	int curent_difference;

	// This is a controlled test, ignore dst_string overflow
	sprintf_ptr = (char*)dst_string;

	for(int i = 0 ; i < buf_size; i++){
		if( buf1[i] != buf2[i]){
			sprintf_ptr += sprintf(sprintf_ptr, "(%s) 0x%.2x != (%s) 0x%.2x in position %d,   ", buf1name, buf1[i], buf2name, buf2[i], i);
			different_indexes[differences++] = i;

			if(differences > MAX_BUFFER_DIFFERENCE){
				break;
			}
		}
	}

	sprintf_ptr += sprintf(sprintf_ptr, "\n\n%s: ", buf1name);
	curent_difference = 0;
	for(int i = 0 ; i < buf_size; i++){
		if(different_indexes[curent_difference] == i && curent_difference < differences){
			sprintf_ptr += sprintf(sprintf_ptr,"[0x%.2x] ", buf1[i]);
			curent_difference++;
		}else{
			sprintf_ptr += sprintf(sprintf_ptr,"0x%.2x ", buf1[i]);
		}

		if(i % 20 == 0){
			*sprintf_ptr++ = '\n';
		}
		if(i % 100 == 0){
			*sprintf_ptr++ = '\n';
		}
	}
	sprintf_ptr += sprintf(sprintf_ptr, "\n\n%s: ", buf2name);
	curent_difference = 0;
	for(int i = 0 ; i < buf_size; i++){
		if(different_indexes[curent_difference] == i && curent_difference < differences){
			sprintf_ptr += sprintf(sprintf_ptr,"[0x%.2x] ", buf2[i]);
			curent_difference++;
		}else{
			sprintf_ptr += sprintf(sprintf_ptr,"0x%.2x ", buf2[i]);
		}
		if(i % 20 == 0){
			*sprintf_ptr++ = '\n';
		}
		if(i % 100 == 0){
			*sprintf_ptr++ = '\n';
		}
	}
	*sprintf_ptr = '\0';
}

int main(int argc, char* argv[]){
	char* ret1, *ret2;
	unsigned int experiment = 0;
	unsigned int errors = 0;

	// Set base string
	for(int i = 0; i < STRING_SIZE; i++){
		test_string[i] = i%('Z' - 'A' + 1) + 'A';
	}

	// Test disalignment of dest pointer
	for(int pd = 0; pd < STRING_SIZE; pd++){

		// Test disalignment of src pointer
		for(int pd2 = 0; pd2 < STRING_SIZE; pd2++){

			// Test for 0 and 0xff padding
			for(int fill = 0; fill < 2; fill += 0xff){
				experiment ++;

				// Set the remainder of src and dest to different values, to detect corruption
				// Test having these bytes at NULL byte and non NULL byte to make sure the operations look only at the first NULL byte found
				// This might trick some poorly optimized code into behaving incorrectly, wich can only be found in these conditions
				memset(test_buffer_src_1, 0xff & (0xff - fill), sizeof(test_buffer_src_1));
				memset(test_buffer_src_2, 0xff & (0xff - fill), sizeof(test_buffer_src_2));

				memset(test_buffer_dest_1, 0xff & (fill), sizeof(test_buffer_dest_1));
				memset(test_buffer_dest_2, 0xff & (fill), sizeof(test_buffer_dest_2));

				// Valid strcat operation
				memcpy(test_buffer_src_1 + pd, test_string, STRING_SIZE - pd);
				test_buffer_src_1[pd + STRING_SIZE - pd] = '\0';

				memcpy(test_buffer_dest_1 + pd2, test_string, STRING_SIZE - pd2);
				test_buffer_dest_1[pd2 + STRING_SIZE - pd2] = '\0';

				ret1 = mystrcat (test_buffer_dest_1 + pd2, test_buffer_src_1+ pd);

				// std strcat operation
				memcpy(test_buffer_src_2 + pd, test_string, STRING_SIZE - pd);
				test_buffer_src_2[pd + STRING_SIZE - pd] = '\0';

				memcpy(test_buffer_dest_2 + pd2, test_string, STRING_SIZE - pd2);
				test_buffer_dest_2[pd2 + STRING_SIZE - pd2] = '\0';

				ret2 = strcat_wrapper(test_buffer_dest_2 + pd2, test_buffer_src_2 + pd);

				// Validate destination string and buffer
				char str_err[BUFFER_SIZE*15 + 1];
				if(0 != memcmp(test_buffer_dest_1, test_buffer_dest_2, sizeof(test_buffer_dest_1))){
					get_buffer_difference(test_buffer_dest_1, "Def Dest", test_buffer_dest_2, "Cust Dest", sizeof(test_buffer_dest_1), str_err);
					printf("Destination buffers dont match (%p %p)\n%s\nSource pointer disalignment: %d\nDest pointer disalignment: %d\nSize: %d\nOn experiment %lu\n\n", ret2, ret1, str_err, pd, pd2, sizeof(test_buffer_dest_1) - pd, experiment);
					errors ++;
				}

				// Validate source string and buffer (unlikely but possible)
				if(0 != memcmp(test_buffer_src_1, test_buffer_src_2, sizeof(test_buffer_src_1))){
					get_buffer_difference(test_buffer_src_1, "Def Src", test_buffer_src_2, "Cust Src", sizeof(test_buffer_src_1), str_err);
					printf("Source buffers dont match (%p %p)\n%s\nSource pointer disalignment: %d\nDest pointer disalignment: %d\nSize: %d\nOn experiment %lu\n\n", ret2, ret1, str_err, pd, pd2, sizeof(test_buffer_src_1) - pd, experiment);
					errors ++;
				}

				if(errors > MAX_ERRORS){
					printf("Too many errors, exiting\n");
					abort();
				}

			}
		}
	}
	if(0 != errors){
		abort();
	}
	return 0;
}






