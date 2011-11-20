#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

FILE *inputFile;
uint16_t totalMemSize;

int8_t readLine ();

uint16_t getTotalMemSize () {
	return totalMemSize;
}

void memdump (void) {
	int16_t i;
	
	for (i = 0; i < 4096; i++) {
		if (i % 32 == 0) {
			printf ("\n%04X: ", i);		
		}
		printf ("%02X", memory[i]);
	}
}

int8_t openFile (uint8_t *filename) {	
	if (filename != NULL) {
		inputFile = fopen ((char * __restrict__) filename, "r");
	}

	if (inputFile == NULL) {
		printf ("Error: Could not open file for reading.\n");
		return -1;
	}
	
	return 0;
}

void loadCode (void) {
	memory = malloc (4096);
	memset (memory, 0, 4096);

	while (readLine () != -1);
}

void closeFile () {
	if (inputFile != NULL) 
		fclose (inputFile);
	free (memory);
}

uint8_t getByte (uint16_t memLoc) {
	return memory [memLoc];
}

void setByte (uint16_t memLoc, int8_t byte) {
	memory [memLoc] = byte;
}

int8_t readLine (void) {
	int8_t i;
	int16_t memLoc;	
	getc (inputFile);						//ignore first ':'

	int8_t byte1 = getc (inputFile);
	int8_t byte2 = getc (inputFile);
	byte1 -= (byte1 > '9') ? '7' : '0';
	byte2 -= (byte2 > '9') ? '7' : '0';

	int8_t numBytes = (byte1 << 4) | byte2;
	if (numBytes == 0) {						//reached the end of the file
		return -1;
	}	
	
	byte1 = getc (inputFile);
	byte2 = getc (inputFile);
	byte1 -= (byte1 > '9') ? '7' : '0';
	byte2 -= (byte2 > '9') ? '7' : '0';
	memLoc = ((byte1 << 4) | byte2) << 8;
	byte1 = getc (inputFile);
	byte2 = getc (inputFile);
	byte1 -= (byte1 > '9') ? '7' : '0';
	byte2 -= (byte2 > '9') ? '7' : '0';
	memLoc |= (byte1 << 4) | byte2;

	getc (inputFile);
	getc (inputFile);						//two zeros before memory


	for (i = 0; i < numBytes; i++) {
		byte1 = getc (inputFile);
		byte2 = getc (inputFile);
		byte1 -= (byte1 > '9') ? '7' : '0';
		byte2 -= (byte2 > '9') ? '7' : '0';
		memory [memLoc+i] = (byte1 << 4) | byte2;
	}
	
	while (getc (inputFile) != 10);					//loop reading chars until we get to next line	

	totalMemSize += numBytes;

	return numBytes*2;
}
