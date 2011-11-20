DIR=
OBJECTS=$(DIR)main.o $(DIR)ui.o $(DIR)screen.o $(DIR)parsefile.o $(DIR)machine.o $(DIR)instructions.o $(DIR)win32.o
CC=gcc
CFLAGS=`pkg-config --cflags gtk+-2.0` -c -Wall
LDFLAGS=`pkg-config --libs gtk+-2.0`
EXE=sim

windows: DIR=./windows/
windows: CC=i486-mingw32-gcc
windows: CFLAGS=`pkg-config-script --cflags gtk+-2.0` -c -mms-bitfields -O2 -mwindows -Wall
windows: LDFLAGS=`pkg-config-script --libs gtk+-2.0` -lcomdlg32
windows: EXE=sim.exe
windows: OBJECTS = ./windows/main.o ./windows/ui.o ./windows/screen.o ./windows/parsefile.o ./windows/machine.o ./windows/instructions.o ./windows/win32.o
windows: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(DIR)$(EXE) $(LDFLAGS)

all: $(OBJECTS)
	$(CC) -o $(DIR)$(EXE) $(LDFLAGS) $(OBJECTS)

$(DIR)main.o: main.c common.h
	$(CC) main.c -o $(DIR)$@ $(CFLAGS)

$(DIR)ui.o: ui.c common.h
	$(CC) ui.c -o $(DIR)$@ $(CFLAGS)

$(DIR)parsefile.o: parsefile.c common.h
	$(CC) parsefile.c -o $(DIR)$@ $(CFLAGS)

$(DIR)machine.o: machine.c common.h instructions.h
	$(CC) machine.c -o $(DIR)$@ $(CFLAGS) 

$(DIR)instructions.o: instructions.c instructions.h
	$(CC) instructions.c -o $(DIR)$@ $(CFLAGS) 

$(DIR)screen.o: screen.c common.h
	$(CC) screen.c -o $(DIR)$@ $(CFLAGS) 

$(DIR)win32.o: win32.c common.h
	$(CC) win32.c -o $(DIR)$@ $(CFLAGS) -lcomdlg32

clean:
	$(shell rm *.o)
	$(shell rm ./windows/*.o)
