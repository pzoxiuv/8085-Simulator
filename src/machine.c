#include <stdint.h>
#include <stdio.h>
#include "common.h"
#include "instructions.h"

/*
 * Sets some variables to their starting values, fills the array of opcodes if it's the first time
 * a program is being run.
 */

void resetMachine (uint8_t firstRun) {
	done = 0;
	pc = 0x100;
	flags = 0;
	
	if (firstRun)
		fillOpcodeTable ();
}

/*
 * Gets the byte in memory at address in pc (program counter).  This is the opcode.  Depending on
 * the opcode, it then reads in one or two more bytes and puts together the immediate data, and
 * finally executes the instruction by calling the function associated with the opcode.
 */

int8_t nextInstruction (void) {
	uint8_t imm8 = 0;
	uint16_t imm16 = 0;
	uint8_t opcode;
	uint8_t i;

	done = 0;
	
	for (i = 0; i < currentBreakpoint; i++) {
		if (breakpoints [i] == pc) 
			programStatus |= PROGRAM_STATUS_BREAKPOINT;
	}

	opcode = getByte (pc);
	pc++;

	if ((programStatus & PROGRAM_STATUS_DEBUG) == PROGRAM_STATUS_DEBUG) {
		printf ("%04X:\t%02x", pc-1, opcode);	
	}
		
	if (instrs [opcode].size == 2 || instrs [opcode].size == 3) {
		imm8 = getByte (pc);
		pc++;		
		imm16 = imm8 & 0x00FF;		

		if (instrs [opcode].size == 3) {
			imm16 = (getByte (pc) << 8) | imm8;		//imm8 is lower byte of imm16 - eg lxi 1234 would be stored as 21 3412
			pc++;
		}
	}

	if ((programStatus & PROGRAM_STATUS_DEBUG) == PROGRAM_STATUS_DEBUG) {
		if (instrs[opcode].size == 2) {
			printf (" %02x", imm8);
		}
		else if (instrs[opcode].size == 3) {
			printf (" %04x", imm16);
		}
		printf ("\t");
	}

	instrs [opcode].exeInstr (opcode, imm16);

	return done;
}

/*
 * Prints the value in the general registers, SP, PC, and FLAGS.  Useful for debugging.
 */

void printRegs (void) {
	printf ("A: %02X  B: %02X  C: %02X  D: %02X  E: %02X  H: %02X  L: %02X SP: %04X FLAGS: %02X", regs [A], regs [B], regs [C],
			regs [D], regs [E], regs [H], regs [L], sp, flags); 
}

/*
 * Puts the registers into a buffer (parameter `ptr`) so that another function can access them
 * easily.
 */

void getRegs (uint8_t *ptr) {
	ptr [0] = regs [A];
	ptr [1] = regs [B];
	ptr [2] = regs [C];
	ptr [3] = regs [D];
	ptr [4] = regs [E];
	ptr [5] = regs [H];
	ptr [6] = regs [L];
	ptr [7] = pc >> 8;
	ptr [8] = pc & 0x00FF;
	ptr [9] = sp >> 8;
	ptr [10] = sp & 0x00FF;
	ptr [11] = flags;
}

/*
 * Updates the zero flag and sign flag based on the value in the A register, which is passed in as 
 * parameter `r1`.
 */

void updateFlags (uint8_t r1) {
	if (regs [r1] == 0) {
		flags |= ZF;
	}
	else {
		flags &= 0xBF;
	}

	if ((regs [r1] & 0x80) == 0x80) {
		flags |= SF;
	}
	else {
		flags &= 0x7F;
	}
}

