#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <string.h>
#include <stdio.h>
#include "common.h"

#define SF	0x80
#define ZF	0x40
#define CF	0x01
#define A 7
#define B 0
#define C 1
#define D 2
#define E 3
#define H 4
#define L 5
#define M 6

int8_t done;

uint8_t regs [8];
uint8_t flags;
uint16_t pc;
uint16_t sp;

void updateFlags (uint8_t);				//updates ZF and SF flags
void fillOpcodeTable (void);

void mov (uint8_t, uint16_t);
void add (uint8_t, uint16_t);
void sub (uint8_t, uint16_t);
void jmp (uint8_t, uint16_t);
void logic (uint8_t, uint16_t);
void cmp (uint8_t, uint16_t);
void rotate (uint8_t, uint16_t);
void lxi (uint8_t, uint16_t);
void stck (uint8_t, uint16_t);
void math16 (uint8_t, uint16_t);
void call (uint8_t, uint16_t);
void ret (uint8_t, uint16_t);
void xchg (uint8_t, uint16_t);
void mem (uint8_t, uint16_t);
void unimpl (uint8_t, uint16_t);

#endif
