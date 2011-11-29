#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "common.h"

/* Loads the program by calling a bunch of helper functions to open the .hex file, load the code 
 * into memory, and set the inital values for stuff like the program counter and flags.
 * Returns -1 if there was an error opening the .hex file, 0 otherwise.
 */

int8_t loadProgram (void *filename) {
	if (openFile (filename) == -1) {
		return -1;
	}
	
	loadCode ();
	resetMachine (1);
	
	return 0;
}

/*
 * Main program loop: calls nextInstruction () to execute next instruction, updates UI, waits for
 * various signals if breakpoints are reached or the user is stepping, handles finishing execution
 * and starting again.
 */
void *runProgram (void *data) {
	int8_t instructionResult = 0;
	
	while (1) {
	
		while (instructionResult != -1 && (programStatus & PROGRAM_STATUS_RUNNING) == PROGRAM_STATUS_RUNNING) {
			instructionResult = nextInstruction ();
			
			if ((programStatus & PROGRAM_STATUS_DEBUG) == PROGRAM_STATUS_DEBUG) {
				printRegs ();
			}
			if ((programStatus & PROGRAM_STATUS_STEPPING) == PROGRAM_STATUS_STEPPING ||
				(programStatus & PROGRAM_STATUS_BREAKPOINT) == PROGRAM_STATUS_BREAKPOINT) {
				g_idle_add ((GSourceFunc) updateRegLbls, NULL);		
				g_idle_add ((GSourceFunc) updateCodeView, NULL);
				if ((programStatus & PROGRAM_STATUS_BREAKPOINT) == PROGRAM_STATUS_BREAKPOINT)
					g_idle_add ((GSourceFunc) updateStatus, GINT_TO_POINTER (3));
				g_mutex_lock (stepLock);
				g_cond_wait (stepCond, stepLock);
				g_mutex_unlock (stepLock);
			}
		}
		
		if ((programStatus & PROGRAM_STATUS_CLI) == PROGRAM_STATUS_CLI) {
			stopProgram ();
			return NULL;
		}
		
		g_idle_add ((GSourceFunc) updateStatus, GINT_TO_POINTER (1));
		
		programStatus &= ~PROGRAM_STATUS_RUNNING;	//unset loaded bit, so step/run return right away
		programStatus |= PROGRAM_STATUS_DONE;		//set done bit.  When reset is clicked, loaded is set
													//so when run or step is set, they clear/set stepping
		g_mutex_lock (stepLock);					//and send the second cond (first cond set by reset)
		g_cond_wait (stepCond, stepLock);			
		g_mutex_unlock (stepLock);
		
		g_mutex_lock (stepLock);					
		g_cond_wait (stepCond, stepLock);			
		g_mutex_unlock (stepLock);
		
		resetMachine (0);
		instructionResult = 0;
	}
	
	return NULL;
}

/*
 * TODO: Is this function really needed?
 */

void stopProgram (void) {
	closeFile ();
}

/*
 * Parses command line arguments, either loads program and enters main program loop (cli) or call
 * initUI to initializes the UI, load program, and enter main program loop.
 */

int main (int argc, char **argv) { 
	
	programStatus = 0;
	uint8_t i;
	void *filename = malloc (100);

	for (i = 1; i < argc; i++) {
		if (argv [i][0] == '-') {
			if (argv [i][1] == 'c') {
				programStatus |= PROGRAM_STATUS_CLI;
			}
			else if (argv [i][1] == 's') {
				programStatus |= PROGRAM_STATUS_STEPPING;
			}
			else if (argv [i][1] == 'd') {
				programStatus |= PROGRAM_STATUS_DEBUG;
			}
			else {
				printf ("Unrecognized option %s.\n", argv [i]);
				exit (0);		
			}
		}
		else {
			strcpy (filename, argv [i]);
		}
	}

	if ((programStatus & PROGRAM_STATUS_CLI) != PROGRAM_STATUS_CLI) {
		initUI (argc, argv);	
	}
	
	else {
		loadProgram (filename);
		runProgram (NULL);
	}
	
	free (filename);

	return 0;
}
