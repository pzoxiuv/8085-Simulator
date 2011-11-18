#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "common.h"

uint8_t running = 0;

uint8_t isRunning (void) {
	return running;
}

void *runProgram (void *filename) {
	running = 1;
	stopped = 0;

	int8_t programState;
	uint8_t regs [13];

	if (openFile (filename) == -1) {
		if (!cli) {
			g_thread_exit (0);
		}
		return NULL;
	}
	
	loadProgram ();
	initMachine ();

	if (!cli) {
		g_idle_add ((GSourceFunc) populateCodeList, NULL);
		//g_idle_add ((GSourceFunc) populateMemList, NULL);

		/*if ((uint8_t *) args [0] != (uint8_t *) 2) {
			g_mutex_lock (runLock);
			g_cond_wait (runCond, runLock);
			g_mutex_free (runLock);
		}*/
	}
	
	while (1) {

		programState = nextInstruction ();
	
		while (programState != -1 && stopped == 0) {
			if ((programState == 2 || stepping) && !cli) {
				getRegs (regs);
				g_idle_add ((GSourceFunc) updateRegLbls, regs);
				g_idle_add ((GSourceFunc) updateCodeView, regs);
			}
			if (debug) {
				printRegs ();	
				if (!(cli && stepping)) {
					printf ("\n");
				}	
			}
			if (stepping && !cli) {
				g_mutex_lock (stepLock);
				g_cond_wait (stepCond, stepLock);
				g_mutex_unlock (stepLock);
			}
			else if (stepping) {
				getchar ();
			}
			programState = nextInstruction ();
		}
		
		printf ("\n\nDone.\n");
	
		if (!cli) {
			getRegs (regs);
			g_idle_add ((GSourceFunc) updateRegLbls, regs);
			//usleep (20000);						//Fix! Possibly use a mutex or something.
		}
		
		else {
			stopProgram ();
			return NULL;
		}
		
		g_idle_add ((GSourceFunc) updateStatusDone, NULL);
		
		running = 2;								//done, but reset not clicked
	
		g_mutex_lock (stepLock);					//reuse step lock to wait after finishing
		g_cond_wait (stepCond, stepLock);			//the program
		g_mutex_unlock (stepLock);
		
		running = 3;								//reset clicked
		
		g_mutex_lock (stepLock);					//now wait for step or run button to continue
		g_cond_wait (stepCond, stepLock);			
		g_mutex_unlock (stepLock);
		
		if (stepping == 2) {
			stopProgram ();
			return NULL;
		}
		
		stopped = 0;
		
		resetMachine ();
	}
	
	return NULL;
}

void stopProgram (void) {
	closeFile ();
	if (!cli) {
		//g_thread_exit (0);
	}
}

int main (int argc, char **argv) { 
	
	debug = 0;
	cli = 0;
	stepping = 0;
	uint8_t i;
	void *filename = malloc (100);

	for (i = 1; i < argc; i++) {
		if (argv [i][0] == '-') {
			if (argv [i][1] == 'c') {
				cli = 1;
			}
			else if (argv [i][1] == 's') {
				stepping = 1;
			}
			else if (argv [i][1] == 'd') {
				debug = 1;
			}
			else if (argv [i][1] == 'z') {
				strcpy (filename, "/media/windows/Users/Alex/8085 Tools/test.hex");
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

	if (!cli) {
		initUI (argc, argv);	
	}
	
	else {
		runProgram (filename);
	}
	
	free (filename);

	return 0;
}
