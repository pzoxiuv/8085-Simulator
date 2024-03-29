DIR=
SRCDIR=./src/
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

$(DIR)main.o: $(SRCDIR)main.c $(SRCDIR)common.h
	$(CC) $(SRCDIR)main.c -o $(DIR)$@ $(CFLAGS)

$(DIR)ui.o: $(SRCDIR)ui.c $(SRCDIR)common.h
	$(CC) $(SRCDIR)ui.c -o $(DIR)$@ $(CFLAGS)

$(DIR)parsefile.o: $(SRCDIR)parsefile.c $(SRCDIR)common.h
	$(CC) $(SRCDIR)parsefile.c -o $(DIR)$@ $(CFLAGS)

$(DIR)machine.o: $(SRCDIR)machine.c $(SRCDIR)common.h $(SRCDIR)instructions.h
	$(CC) $(SRCDIR)machine.c -o $(DIR)$@ $(CFLAGS) 

$(DIR)instructions.o: $(SRCDIR)instructions.c $(SRCDIR)instructions.h
	$(CC) $(SRCDIR)instructions.c -o $(DIR)$@ $(CFLAGS) 

$(DIR)screen.o: $(SRCDIR)screen.c $(SRCDIR)common.h
	$(CC) $(SRCDIR)screen.c -o $(DIR)$@ $(CFLAGS) 

$(DIR)win32.o: $(SRCDIR)win32.c $(SRCDIR)common.h
	$(CC) $(SRCDIR)win32.c -o $(DIR)$@ $(CFLAGS) -lcomdlg32

clean:
	$(shell rm *.o)
	$(shell rm ./windows/*.o)
