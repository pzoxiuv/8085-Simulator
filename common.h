#ifndef COMMON_H
#define	COMMON_H

#include <glib.h>

typedef struct {
	int8_t size;		//number of bytes the instruction has (eg LXI is 3, MVI is 2, MOV is 1);
	void (*exeInstr) (uint8_t, uint16_t);
	uint8_t name [12];
} opcode_t;

opcode_t instrs [0xFF];
uint8_t *memory;
uint8_t debug, cli, stepping, stopped;

uint8_t isRunning (void);
void stopProgram (void);

void initMachine (void);
void resetMachine (void);
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
void loadProgram (void);
void closeFile (void);
void memdump (void);
uint16_t getTotalMemSize (void);

int8_t initUI (int, char **);
uint8_t readChar (void);
gboolean populateMemList (void);
gboolean populateCodeList (void);
gboolean updateRegLbls (uint8_t *);
gboolean writeBuf (uint8_t *);
gboolean updateCodeView (uint8_t *);
gboolean updateStatusDone (void);

GMutex *bufLock;
GCond *bufCond;
GMutex *stepLock;
GCond *stepCond;
GMutex *runLock;
GCond *runCond;

#endif
