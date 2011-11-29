#include <stdint.h>	
#include "instructions.h"

/*
 * This file contains the actual implementations for the 8085 instructions.
 * 
 * Each function needs to have the same prototype, so they all pass in the 8 bit opcode and 16 bits
 * of immediate data regardless of whether or not the instruction needs it.
 */

#define GETM(h,l) ((h << 8) | l)

/*
 * Set done to -1 if we try executing an unimplemented/invalid opcode to indicate an error.
 */

void unimpl (uint8_t opcode, uint16_t imm16) {
	done = -1;
}

/*
 * Implements all mov and mvi instructions.  Uses the opcode to figure out which instruction is
 * going to be executed and what registers are going to be used.
 */

void mov (uint8_t opcode, uint16_t imm16) {
	int8_t r1, r2;
	if (opcode > 0x3F && opcode < 0x80) {					//MOV instruction (not MVI), with B,C,D,E,H or L as R1
		r1 = (((opcode & 0x30) >> 4) * 2) + ((opcode & 0x08) >> 3);	//opcode is 4X, X < 8 for B=r1, 4X, X > 7 for C=r1, etc.
		r2 = opcode & 0x07;	
		if (r2 == M) {							//r2 is M, update M with memory at address <HL> in memory
			regs [r2] = getByte (GETM (regs [H], regs [L]));
		}
		if (r1 == M) {							//r1 is M, update memory location in <HL> with r2's value
			setByte (GETM (regs [H], regs [L]), regs [r2]);	
			return;
		}
		regs [r1] = regs [r2];
		return;
	}
	else if (opcode < 0x40) {						//must be a MVI instruction
		r1 = (((opcode & 0x30) >> 4) * 2) + ((opcode & 0x08) >> 3);
		if (r1 == M) {							//r1 = M
			setByte (GETM (regs [H], regs [L]), (int8_t) (imm16 & 0x00FF));	
			return;	
		}
		regs [r1] = (uint8_t) (imm16 & 0x00FF);
	}
}

/*
 * Implements add, adc, aci and adi instructions.
 */

void add (uint8_t opcode, uint16_t imm16) {
	int16_t sum = 0;
	uint8_t r1, r2;
	
	if (opcode > 0x7F) {
		r1 = A;
		if (opcode == 0xC6 || opcode == 0xCE) {
			r2 = (uint8_t) (imm16 & 0x00FF);
		}
		else {
			if ((opcode & 0x07) == M) {
				r2 = getByte (GETM (regs [H], regs [L]));
			}
			else {
				r2 = regs [opcode & 0x07];
			}
		}
		if (((opcode & 0x08) >> 3) == 1) {					//if 4th bit was set, it was an ADC instruction
			sum += (flags & 0x01);	
		}
	}

	else {
		r1 = (((opcode & 0x30) >> 4) * 2) + ((opcode & 0x08) >> 3);
		if (r1 == M) {
			setByte (GETM (regs [H], regs [L]), getByte (GETM (regs [H], regs [L])));	
			return;
		}
		r2 = 1;
	}

	sum += regs [r1] + r2;
	
	regs [r1] = sum & 0x00FF;

	updateFlags (r1);

	if (opcode > 0x7F) {
		if (sum > 0xFF) {
			flags |= CF;
		}
		else {
			flags &= 0xFE;
		}
	}	
}

/*
 * Implements sub, sbb, sui, sbi instructions in a similar way to the add function.
 */

void sub (uint8_t opcode, uint16_t imm16) {
	int16_t diff = 0;
	uint8_t r1, r2;

	if (opcode > 0x8F) {
		r1 = A;
		if (opcode == 0xD6 || opcode == 0xDE || opcode == 0xF6) {		//0xF6 = 0xFE-0x8, 0xFE = CPI
			r2 = (uint8_t) (imm16 & 0x00FF);
		}
		else {
			if ((opcode & 0x07) == M) {
				r2 = getByte (GETM (regs [H], regs [L]));
			}
			else {
				r2 = regs [opcode & 0x07];
			}
		}
		if (((opcode & 0x08) >> 3) == 1) {					//if 4th bit was set, it was an SBB instruction
			diff -= (flags & 0x01);	
		}
	}

	else {
		r1 = (((opcode & 0x30) >> 4) * 2) + ((opcode & 0x08) >> 3);
		if (r1 == M) {
			setByte (GETM (regs [H], regs [L]), getByte (GETM (regs [H], regs [L])) - 1);	
			return;
		}
		r2 = 1;
	}

	diff += regs [r1] - r2;	

	if (opcode > 0x8F) {
		if (regs [r1] < r2) {
			flags |= CF;
		}
		else {
			flags &= 0xFE;
		}
	}

	regs [r1] = diff & 0x00FF;

	updateFlags (r1);
}

/*
 * Implements stc, cmc, cma, ana, xra, and ora instructions.
 */

void logic (uint8_t opcode, uint16_t imm16) {
	uint8_t r1, r2;

	r1 = A;

	if (opcode == 0x37) {
		flags |= 0x01;
		return;
	}

	if (opcode == 0x3F) {
		flags += (flags & 0x01) ? -1 : 1;
		return;
	}

	if (opcode == 0x2F) {
		regs [r1] = ~regs [r1];
		return;
	}

	if (opcode < 0xC0) {
		r2 = opcode & 0x07;
		if (r2 == M) {
			r2 = getByte (GETM (regs [H], regs [L]));
		}
		else {
			r2 = regs [r2];		
		}
	}
	else {
		r2 = (uint8_t) (imm16 & 0x00FF);
	}

	if ((opcode > 0x9F && opcode < 0xA8) || opcode == 0xE6) {
		regs [r1] &= r2;
	}
	else if ((opcode > 0xA7 && opcode < 0xB0) || opcode == 0xEE) {
		regs [r1] ^= r2;
	}
	else if ((opcode > 0xAF && opcode < 0xB8) || opcode == 0xF6) {
		regs [r1] |= r2;
	}

	updateFlags (r1);
	
	flags &= 0xFE;	
}

/*
 * Implements cmp and cpi instructions, via the sub function.
 * 
 * TODO:  Check if cpi actually works...
 */

void cmp (uint8_t opcode, uint16_t imm16) {
	uint8_t a = regs [A];
	opcode -= 0x8;								//subtract eight from opcode so we do sub, not sbb
	sub (opcode, imm16);							
	regs [A] = a;
}

/*
 * Implements all jump instructions except jnk, jk, jpe and jpo.
 */

void jmp (uint8_t opcode, uint16_t imm16) {
	if (opcode == 0xC3) {							//unconditional jmp
		if (imm16 == 0) 
			done = -1;
		else
			pc = imm16;
		return;
	}

	if ((((flags & ZF) >> 6) == ((opcode & 0x08) >> 3)) && ((opcode & 0xF0) == 0xC0)) {
		pc = imm16;
		return;
	}

	else if ((((flags & SF) >> 7) == ((opcode & 0x08) >> 3)) && ((opcode & 0xF0) == 0xF0)) {
		pc = imm16;
		return;
	}
	
	else if (((flags & CF) == ((opcode & 0x08) >> 3)) && ((opcode & 0xF0) == 0xD0)) {
		pc = imm16;
		return;
	}
}

/*
 * Implements rotate instructions (rlc, rrc, ral, and rar).
 */

void rotate (uint8_t opcode, uint16_t imm16) {
	imm16 = 0;
	
	if (opcode == 0x07) {
		if (regs [A] >> 7) {
			flags |= 0x01;
		}
		else {
			flags &= 0xFE;		
		}
		regs [A] = regs [A] << 1;
		regs [A] |= flags & 0x01;
		
		return;
	}
	else if (opcode == 0x0F) {
		if (regs [A] & 0x01) {
			flags |= 0x01;
		}
		else {
			flags &= 0xFE;		
		}
		regs [A] = regs [A] >> 1;
		regs [A] |= (flags & 0x01) << 7;
		
		return;
	}
	else if (opcode == 0x17) {
		imm16 = flags & 0x01;
		if (regs [A] >> 7) {
			flags |= 0x01;
		}
		else {
			flags &= 0xFE;		
		}
		regs [A] = regs [A] << 1;
		regs [A] |= imm16;
	
		return;
	}
	else {
		imm16 = flags & 0x01;
		if (regs [A] & 0x01) {
			flags |= 0x01;
		}
		else {
			flags &= 0xFE;		
		}
		regs [A] = regs [A] >> 1;
		regs [A] |= imm16 << 7;
		
		return;	
	}
}

/*
 * Implements lxi instructions.
 */

void lxi (uint8_t opcode, uint16_t imm16) {
	if (opcode == 0x31) {
		sp = imm16;
		return;
	}

	uint8_t r1 = ((opcode & 0x30) >> 4) * 2;
	uint8_t r2 = r1 + 1;

	regs [r1] = (imm16 & 0xFF00) >> 8;
	regs [r2] = imm16 & 0x00FF;
}

/*
 * Implements push/pop instructions.
 */r

void stck (uint8_t opcode, uint16_t imm16) {
	uint8_t r1 = ((opcode & 0x30) >> 4) * 2;
	uint8_t r2 = r1 + 1;

	if (opcode == 0xF5) {
		sp--;
		setByte (sp, flags);
		sp--;
		setByte (sp, regs [A]);		
		return;
	}
	
	else if (opcode == 0xF1) {
		regs [A] = getByte (sp);
		sp++;
		flags = getByte (sp);
		sp++;
		return;
	}

	if ((opcode & 0x05) == 0x05) {
		sp--;
		setByte (sp, regs [r1]);
		sp--;
		setByte (sp, regs [r2]);
	}
	
	else {
		regs [r2] = getByte (sp);
		sp++;
		regs [r1] = getByte (sp);
		sp++;
	}
}

/* 
 * Implements 16 bit math instructions: dcx, inx, and dad.
 */

void math16 (uint8_t opcode, uint16_t imm16) {
	uint8_t r1 = ((opcode & 0x30) >> 4) * 2;
	uint8_t r2 = r1 + 1;
	uint16_t incr = 1;

	if ((opcode & 0x0F) == 0x09) {				//dad instr
		if (opcode == 0x39) {
			incr = sp;
		}
		else {
			incr = (regs [r1] << 8) | regs [r2];
		}
		r1 = H;
		r2 = L;
	}

	else if ((opcode & 0x08) >> 3) {				//dcx instr
		incr = -1;
	}

	if (opcode == 0x33 || opcode == 0x3B) {
		sp += incr;
		return;
	}

	imm16 = (regs [r1] << 8) | regs [r2];
	imm16 += incr;
	regs [r1] = (imm16 & 0xFF00) >> 8;
	regs [r2] = imm16 & 0x00FF;
}

/*
 * Implements all the call instructions, except cpe and cpo.  If the address being called (which
 * is in parameter imm16) is 5, it means the program is calling bdos, so function calls the 
 * appropriate input/output function depending on what is in the C register.
 */

void call (uint8_t opcode, uint16_t imm16) {
	if (opcode != 0xCD) {
		if ((((flags & ZF) >> 6) != ((opcode & 0x08) >> 3)) && ((opcode & 0xF0) == 0xC0)) {
			return;
		}
	
		else if ((((flags & SF) >> 7) != ((opcode & 0x08) >> 3)) && ((opcode & 0xF0) == 0xF0)) {
			return;
		}
		
		else if (((flags & CF) != ((opcode & 0x08) >> 3)) && ((opcode & 0xF0) == 0xD0)) {
			return;
		}
	}

	if (imm16 == 5) {
		if (regs [C] == 9) {
			printstr ((regs [D] << 8) | regs [E]);
		}
		else if (regs [C] == 2) {
			printchar (regs [E]);
		}
		else if (regs [C] == 1) {
			regs [A] = input ();		
		}
		return;
	}

	sp--;
	setByte (sp, pc >> 8);
	sp--;
	setByte (sp, pc & 0x00FF);

	pc = imm16;	
}

/*
 * Implements all the return instructions except rpe and rpo.
 */

void ret (uint8_t opcode, uint16_t imm16) {
	if (opcode != 0xC9) {	
		if ((((flags & ZF) >> 6) != ((opcode & 0x08) >> 3)) && ((opcode & 0xF0) == 0xC0)) {
			return;	
		}

		else if ((((flags & SF) >> 7) != ((opcode & 0x08) >> 3)) && ((opcode & 0xF0) == 0xF0)) {
			return;
		}
	
		else if (((flags & CF) != ((opcode & 0x08) >> 3)) && ((opcode & 0xF0) == 0xD0)) {
			return;
		}
	}

	pc = getByte (sp);
	sp++;
	pc |= getByte (sp) << 8;
	sp++;
}

/*
 * Implements the xchg instruction.
 */

void xchg (uint8_t opcode, uint16_t imm16) {
	imm16 = regs [E];
	imm16 |= regs [D] << 8;
	regs [E] = regs [L];
	regs [D] = regs [H];
	regs [L] = imm16 & 0x00FF;
	regs [H] = imm16 >> 8;
}

/*
 * Implements instructions dealing with memory and some other data movement: ldax, lda, sta, 
 * stax, sphl, lhld, xthl, pchl.
 */

void mem (uint8_t opcode, uint16_t imm16) {
	switch (opcode) {
		case 0x0A:
			regs [A] = getByte ((regs [B] << 8) | regs [C]);
			break;
		case 0x1A:
			regs [A] = getByte ((regs [D] << 8) | regs [E]);
			break;
		case 0x32:
			setByte (imm16, regs [A]);
			break;
		case 0x3A:
			regs [A] = getByte (imm16);
			break;
		case 0x02:
			setByte ((regs [B] << 8) | regs [C], regs [A]);
			break;
		case 0x12:
			setByte ((regs [D] << 8) | regs [E], regs [A]);
			break;	
		case 0xF9:
			sp = (regs [H] << 8) | regs [L];
			break;
		case 0x2A:
			regs [H] = getByte (imm16);
			regs [L] = getByte (imm16+1);
			break;
		case 0xE3:
			imm16 = getByte (sp+1) << 8;
			imm16 |= getByte (sp);
			setByte (sp+1, regs [H]);
			setByte (sp, regs [L]);
			regs [H] = imm16 >> 8;
			regs [L] = imm16 & 0x00FF;
			break;	
		case 0xE9:
			pc = (regs [H] << 8) | regs [L];
	}
}

/*
 * This function is called once at the start of the simulator, and fills a two dimensional array
 * of opcode_t structs with 255 structures, one for each opcode.  It sets the size, name, and
 * pointer to the implementing function for each opcode.
 */

void fillOpcodeTable (void) {
	uint16_t i;
	uint8_t *regNames = (uint8_t *) "bcdehlma";

	for (i = 0x00; i < 0xFF; i++) {
		instrs [i].size = 1;
		instrs [i].exeInstr = *unimpl;
		strcpy ((char *) instrs [i].name, (const char *) "nop");
	}

	for (i = 0x06; i < 0x46; i += 0x8) {
		instrs [i].size = 2;
		instrs [i].exeInstr = *mov;
		sprintf ((char *) instrs [i].name, "mvi %c, ", regNames [(((i & 0x30) >> 4) * 2) + ((i & 0x08) >> 3)]);
	}

	for (i = 0x40; i < 0x80; i++) {
		if (i == 0x76)
			continue;
		instrs [i].size = 1;
		instrs [i].exeInstr = *mov;
		sprintf ((char *) instrs [i].name, "mov %c, %c", regNames [(((i & 0x30) >> 4) * 2) + ((i & 0x08) >> 3)], regNames [i & 0x07]);
	}

	for (i = 0x80; i < 0x90; i++) {
		instrs [i].size = 1;
		instrs [i].exeInstr = *add;
		if (i < 0x88)
			sprintf ((char *) instrs [i].name, "add %c", regNames [i & 0x07]);
		else 
			sprintf ((char *) instrs [i].name, "adc %c", regNames [i & 0x07]);
	}

	for (i = 0x90; i < 0xA0; i++) {
		instrs [i].size = 1;
		instrs [i].exeInstr = *sub;
		if (i < 0x98) 
			sprintf ((char *) instrs [i].name, "sub %c", regNames [i & 0x07]);
		else 
			sprintf ((char *) instrs [i].name, "sbb %c", regNames [i & 0x07]);
	}

	instrs [0xC6].size = 2;
	instrs [0xC6].exeInstr = *add;
	strcpy ((char *) instrs [0xC6].name, (const char *) "adi ");
	instrs [0xCE].size = 2;
	instrs [0xCE].exeInstr = *add;
	strcpy ((char *) instrs [0xCE].name, (const char *) "aci ");
	instrs [0xD6].size = 2;
	instrs [0xD6].exeInstr = *sub;
	strcpy ((char *) instrs [0xD6].name, (const char *) "sui ");
	instrs [0xDE].size = 2;
	instrs [0xDE].exeInstr = *sub;
	strcpy ((char *) instrs [0xDE].name, (const char *) "sbi ");
	
	for (i = 0x4; i < 0x44; i += 0x8) {
		instrs [i].size = 1;
		instrs [i].exeInstr = *add;
		sprintf ((char *) instrs [i].name, "inr %c", regNames [(((i & 0x30) >> 4) * 2) + ((i & 0x08) >> 3)]);	
	}

	for (i = 0x5; i < 0x45; i += 0x8) {
		instrs [i].size = 1;
		instrs [i].exeInstr = *sub;	
		sprintf ((char *) instrs [i].name, "dcr %c", regNames [(((i & 0x30) >> 4) * 2) + ((i & 0x08) >> 3)]);
	}

	for (i = 0xC2; i <= 0xFA; i += 0x8) {
		instrs [i].size = 3;
		instrs [i].exeInstr = *jmp;	
	} 
	
	instrs [0xC3].size = 3;
	instrs [0xC3].exeInstr = *jmp;
	strcpy ((char *) instrs [0xC3].name, "jmp ");
	strcpy ((char *) instrs [0xC2].name, "jnz ");
	strcpy ((char *) instrs [0xD2].name, "jnc ");
	strcpy ((char *) instrs [0xE2].name, "jpo ");
	strcpy ((char *) instrs [0xF2].name, "jp ");
	strcpy ((char *) instrs [0xCA].name, "jz ");
	strcpy ((char *) instrs [0xDA].name, "jc ");
	strcpy ((char *) instrs [0xEA].name, "jpe ");
	strcpy ((char *) instrs [0xFA].name, "jm ");

	for (i = 0xA0; i < 0xB8; i++) {
		instrs [i].size = 1;
		instrs [i].exeInstr = *logic;
		if (i < 0xA8)
			sprintf ((char *) instrs [i].name, "ana %c", regNames [i & 0x07]);
		else if (i < 0xB0) 
			sprintf ((char *) instrs [i].name, "xra %c", regNames [i & 0x07]);
		else
			sprintf ((char *) instrs [i].name, "ora %c", regNames [i & 0x07]);
	}

	instrs [0xE6].size = 2;
	instrs [0xE6].exeInstr = *logic;
	strcpy ((char *) instrs [0xE6].name, (const char *) "ani ");
	instrs [0xEE].size = 2;
	instrs [0xEE].exeInstr = *logic;
	strcpy ((char *) instrs [0xEE].name, (const char *) "xri ");
	instrs [0xF6].size = 2;
	instrs [0xF6].exeInstr = *logic;
	strcpy ((char *) instrs [0xF6].name, (const char *) "ori ");
	instrs [0x37].size = 1;
	instrs [0x37].exeInstr = *logic;
	strcpy ((char *) instrs [0x37].name, (const char *) "stc");
	instrs [0x2F].size = 1;
	instrs [0x2F].exeInstr = *logic;
	strcpy ((char *) instrs [0x2F].name, (const char *) "cma");
	instrs [0x3F].size = 1;
	instrs [0x3F].exeInstr = *logic;
	strcpy ((char *) instrs [0x3F].name, (const char *) "cmc");
	
	for (i = 0xB8; i < 0xC0; i++) {
		instrs [i].size = 1;
		instrs [i].exeInstr = *cmp;
		sprintf ((char *) instrs [i].name, "cmp %c", regNames [i & 0x07]);
	}
	
	instrs [0xFE].size = 2;
	instrs [0xFE].exeInstr = *cmp;
	strcpy ((char *) instrs [0xFE].name, (const char *) "cpi ");

	for (i = 0x07; i < 0x20; i += 0x8) {
		instrs [i].size = 1;
		instrs [i].exeInstr = *rotate;
 	}

	strcpy ((char *) instrs [0x07].name, (const char *) "rlc");
	strcpy ((char *) instrs [0x0F].name, (const char *) "rrc");
	strcpy ((char *) instrs [0x17].name, (const char *) "ral");
	strcpy ((char *) instrs [0x1F].name, (const char *) "rar");

	for (i = 0x01; i < 0x41; i += 0x10) {
		instrs [i].size = 3;
		instrs [i].exeInstr = *lxi;
		if (i != 0x31)
			sprintf ((char *) instrs [i].name, "lxi %c, ", regNames [((i & 0x30) >> 4) * 2]);
		else
			strcpy ((char *) instrs [i].name, (const char *) "lxi sp, ");
	}

	for (i = 0xC1; i <= 0xF1; i += 0x10) {
		instrs [i].size = 1;
		instrs [i].exeInstr = *stck;
		if (i != 0xF1)
			sprintf ((char *) instrs [i].name, "pop %c", regNames [((i & 0x30) >> 4) * 2]);
		else
			strcpy ((char *) instrs [i].name, (const char *) "pop psw");
	}

	for (i = 0xC5; i <= 0xF5; i += 0x10) {
		instrs [i].size = 1;
		instrs [i].exeInstr = *stck;
		if (i != 0xF5)
			sprintf ((char *) instrs [i].name, "push %c", regNames [((i & 0x30) >> 4) * 2]);
		else
			strcpy ((char *) instrs [i].name, (const char *) "push psw");
	}

	for (i = 0x03; i < 0x43; i += 0x8) {
		instrs [i].size = 1;
		instrs [i].exeInstr = *math16;
		if (i == 0x33)
			strcpy ((char *) instrs [i].name, (const char *) "inx sp");
		else if (i == 0x3B)
			strcpy ((char *) instrs [i].name, (const char *) "dcx sp");
		else if ((i & 0x08) >> 3) 
			sprintf ((char *) instrs [i].name, "dcx %c", regNames [((i & 0x30) >> 4) * 2]);
		else
			sprintf ((char *) instrs [i].name, "inx %c", regNames [((i & 0x30) >> 4) * 2]);
	}

	for (i = 0x9; i < 0x49; i += 0x10) {
		instrs [i].size = 1;
		instrs [i].exeInstr = *math16;	
		if (i != 0x39)
			sprintf ((char *) instrs [i].name, "dad %c", regNames [((i & 0x30) >> 4) * 2]);
		else
			strcpy ((char *) instrs [i].name, (const char *) "dad sp");			
	}

	for (i = 0xC4; i <= 0xFC; i += 0x8) {
		instrs [i].size = 3;
		instrs [i].exeInstr = *call;
	}

	instrs [0xCD].size = 3;
	instrs [0xCD].exeInstr = *call;
	strcpy ((char *) instrs [0xCD].name, (const char *) "call ");
	strcpy ((char *) instrs [0xC4].name, (const char *) "cnz ");
	strcpy ((char *) instrs [0xCC].name, (const char *) "cz ");
	strcpy ((char *) instrs [0xD4].name, (const char *) "cnc ");
	strcpy ((char *) instrs [0xDC].name, (const char *) "cc ");
	strcpy ((char *) instrs [0xF4].name, (const char *) "cp ");
	strcpy ((char *) instrs [0xFC].name, (const char *) "cm ");

	for (i = 0xC0; i <= 0xF8; i += 0x8) {
		instrs [i].size = 1;
		instrs [i].exeInstr = *ret;
	}	

	instrs [0xC9].size = 1;
	instrs [0xC9].exeInstr = *ret;
	strcpy ((char *) instrs [0xC9].name, (const char *) "ret");
	strcpy ((char *) instrs [0xC0].name, (const char *) "rnz");
	strcpy ((char *) instrs [0xC8].name, (const char *) "rz");
	strcpy ((char *) instrs [0xD0].name, (const char *) "rnc");
	strcpy ((char *) instrs [0xD8].name, (const char *) "rc");
	strcpy ((char *) instrs [0xF0].name, (const char *) "rp");
	strcpy ((char *) instrs [0xF8].name, (const char *) "rm");

	instrs [0xEB].size = 1;
	instrs [0xEB].exeInstr = *xchg;
	strcpy ((char *) instrs [0xEB].name, (const char *) "xchg");

	strcpy ((char *) instrs [0x76].name, (const char *) "hlt");

	instrs [0x0A].size = 1;
	instrs [0x0A].exeInstr = *mem;
	strcpy ((char *) instrs [0x0A].name, (const char *) "ldax b");
	instrs [0x1A].size = 1;
	instrs [0x1A].exeInstr = *mem;
	strcpy ((char *) instrs [0x1A].name, (const char *) "ldax d");
	instrs [0x02].size = 1;
	instrs [0x02].exeInstr = *mem;
	strcpy ((char *) instrs [0x02].name, (const char *) "stax b");
	instrs [0x12].size = 1;
	instrs [0x12].exeInstr = *mem;
	strcpy ((char *) instrs [0x12].name, (const char *) "stax d");
	instrs [0x32].size = 3;
	instrs [0x32].exeInstr = *mem;
	strcpy ((char *) instrs [0x32].name, (const char *) "sta ");
	instrs [0x3A].size = 3;
	instrs [0x3A].exeInstr = *mem;
	strcpy ((char *) instrs [0x3A].name, (const char *) "lda ");
	instrs [0xF9].size = 1;
	instrs [0xF9].exeInstr = *mem;
	strcpy ((char *) instrs [0xF9].name, (const char *) "sphl");
	instrs [0x2A].size = 3;
	instrs [0x2A].exeInstr = *mem;
	strcpy ((char *) instrs [0x2A].name, (const char *) "lhld");
	instrs [0xE3].size = 1;
	instrs [0xE3].exeInstr = *mem;
	strcpy ((char *) instrs [0xE3].name, (const char *) "xthl");
	instrs [0xE9].size = 1;
	instrs [0xE9].exeInstr = *mem;
	strcpy ((char *) instrs [0xE9].name, (const char *) "pchl");
}
