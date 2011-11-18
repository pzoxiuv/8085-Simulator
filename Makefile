DIR=
OBJECTS=$(DIR)main.o $(DIR)ui.o $(DIR)screen.o $(DIR)parsefile.o $(DIR)machine.o $(DIR)instructions.o
CC=gcc
CFLAGS=`pkg-config --cflags gtk+-2.0` -c -Wall
LDFLAGS=`pkg-config --libs gtk+-2.0`
EXE=sim

all: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(DIR)$(EXE)

$(DIR)main.o: main.c common.h
	$(CC) $(CFLAGS) main.c -o $(DIR)$@

$(DIR)ui.o: ui.c common.h
	$(CC) $(CFLAGS) ui.c -o $(DIR)$@

$(DIR)parsefile.o: parsefile.c common.h
	$(CC) $(CFLAGS) parsefile.c -o $(DIR)$@

$(DIR)machine.o: machine.c common.h instructions.h
	$(CC) $(CFLAGS) machine.c -o $(DIR)$@

$(DIR)instructions.o: instructions.c instructions.h
	$(CC) $(CFLAGS) instructions.c -o $(DIR)$@

$(DIR)screen.o: screen.c common.h
	$(CC) $(CFLAGS) screen.c -o $(DIR)$@

clean:
	$(shell rm sim)
	$(shell rm *.o)
