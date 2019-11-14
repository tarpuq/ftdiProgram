IDIR=/usr/include/libftdi1
CC=gcc
LDIR=/usr/lib/x86_64-linux-gnu
CFLAGS=-I$(IDIR) -L$(LDIR)
LIBS=-lftdi1

fdtiProgram: eeprom.o
	$(CC) -o ftdiProgram eeprom.o $(CFLAGS) $(LIBS)
