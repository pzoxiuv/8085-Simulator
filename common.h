#ifndef COMMON_H
#define	COMMON_H

#include <glib.h>

#define PROGRAM_STATUS_RUNNING 1
#define PROGRAM_STATUS_LOADED 2
#define PROGRAM_STATUS_INPUT 4
#define PROGRAM_STATUS_STEPPING 8
#define PROGRAM_STATUS_DONE 16
#define PROGRAM_STATUS_BREAKPOINT 32
#define PROGRAM_STATUS_CLI 64
#define PROGRAM_STATUS_DEBUG 128

typedef struct {
	int8_t size;		//number of bytes the instruction has (eg LXI is 3, MVI is 2, MOV is 1);
	void (*exeInstr) (uint8_t, uint16_t);
	uint8_t name [12];
} opcode_t;

opcode_t instrs [0xFF];
uint8_t *memory;
uint16_t breakpoints [20];
uint8_t currentBreakpoint;
uint8_t programStatus;		/* See above for bit meanings */

GMutex *bufLock;
GCond *bufCond;
GMutex *stepLock;
GCond *stepCond;
GMutex *runLock;
GCond *runCond;

int8_t loadProgram (void *);
void stopProgram (void);

void resetMachine (uint8_t);
int8_t nextInstruction (void);
void printRegs (void);
void *runProgram (void *);
void getRegs (uint8_t *);
uint8_t input (void);
void printstr (uint16_t);
void printchar (uint8_t);

uint8_t getByte (uint16_t);
void setByte (uint16_t, int8_t);
int8_t openFile (uint8_t *);
void loadCode (void);
void closeFile (void);
void memdump (void);
uint16_t getTotalMemSize (void);

int8_t initUI (int, char **);
uint8_t readChar (void);
gboolean populateMemList (void);
gboolean populateCodeList (void);
gboolean writeBuf (uint8_t *);
gboolean updateStatus (gpointer);
gboolean updateRegLbls (void);
gboolean updateCodeView (void);

void win32FileSelect (int8_t *);

#endif
