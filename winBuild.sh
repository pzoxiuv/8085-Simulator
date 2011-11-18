#! /bin/sh

i486-mingw32-gcc -Wall -o ./windows/screen.o `pkg-config-script --cflags gtk+-2.0` -c -mwindows screen.c
i486-mingw32-gcc -Wall -o ./windows/main.o `pkg-config-script --cflags gtk+-2.0` -c -mwindows main.c
i486-mingw32-gcc -Wall -o ./windows/instructions.o `pkg-config-script --cflags gtk+-2.0` -c -mwindows instructions.c
i486-mingw32-gcc -Wall -o ./windows/parsefile.o `pkg-config-script --cflags gtk+-2.0` -c -mwindows parsefile.c
i486-mingw32-gcc -Wall -o ./windows/ui.o `pkg-config-script --cflags gtk+-2.0` -c -mwindows ui.c
i486-mingw32-gcc -Wall -o ./windows/machine.o `pkg-config-script --cflags gtk+-2.0` -c -mwindows machine.c

i486-mingw32-gcc -mwindows ./windows/*.o -o ./windows/sim.exe `pkg-config-script --libs gtk+-2.0`
#i486-mingw32-gcc -o ./windows/sim -Wall `pkg-config-script --cflags --libs gtk+-2.0` main.c ui.c screen.c parsefile.c machine.c
