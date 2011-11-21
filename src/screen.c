#include <stdio.h>
#include <stdint.h>
#include "common.h"

void printstr (uint16_t strloc) {
	uint16_t i = 0;
	uint8_t buf [82];
	uint8_t j = 1;
	while (memory [strloc+i] != '$') {
#ifdef G_OS_UNIX
		if (memory [strloc+i] == 0xA && 
			(programStatus & PROGRAM_STATUS_CLI) != PROGRAM_STATUS_CLI) {			//don't print redundant line feed 
			i++;										//if we're in UNIX
			continue;
		}
#else
	if (memory [strloc+i] == 0xD && (programStatus & PROGRAM_STATUS_CLI) != PROGRAM_STATUS_CLI) {				//don't print redundant cr's
			i++;										//if we're in Windows
			continue;
	}
#endif		
		buf [j] = memory [strloc+i];
		i++;	
		j++;
		if (j == 81) {
			buf [j] = 0;
			buf [0] = j-1;
			if ((programStatus & PROGRAM_STATUS_CLI) != PROGRAM_STATUS_CLI) {
				g_idle_add ((GSourceFunc) writeBuf, buf);
				g_mutex_lock (bufLock);
				g_cond_wait (bufCond, bufLock);
				g_mutex_unlock (bufLock);	
			}
			else {
				printf ("%s", buf);
			}
			j = 1;
		}
	}
	buf [j] = 0;
	buf [0] = j-1;
	if ((programStatus & PROGRAM_STATUS_CLI) != PROGRAM_STATUS_CLI) {
		g_idle_add ((GSourceFunc) writeBuf, buf);
		g_mutex_lock (bufLock);
		g_cond_wait (bufCond, bufLock);
		g_mutex_unlock (bufLock);		
	}
	else {
		printf ("%s", buf);	
	}
}

void printchar (uint8_t ch) {
#ifdef G_OS_UNIX
	if (ch == 0xA) return;					//Get outta here with yer lf's! (if you're in UNIX...')
#else
	if (ch == 0xD) return;
#endif
	if ((programStatus & PROGRAM_STATUS_CLI) != PROGRAM_STATUS_CLI) {
		uint8_t buf [3];
		buf [1] = ch;
		buf [2] = 0;
		buf [0] = 1;
		g_idle_add ((GSourceFunc) writeBuf, buf);
		g_mutex_lock (bufLock);
		g_cond_wait (bufCond, bufLock);
		g_mutex_unlock (bufLock);				
	}
	else {
		printf ("%c", ch);	
	}
}

uint8_t input (void) {
	if ((programStatus & PROGRAM_STATUS_CLI) != PROGRAM_STATUS_CLI) {
		return readChar ();
	}
	return 0;
}
