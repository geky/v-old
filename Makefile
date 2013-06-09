CC=gcc
#CFLAGS=-O3 -Wall -lm 
CFLAGS=-O3 -pg -Wall -lm
#CFLAGS=-O0 -g3 -gdwarf-2 -ggdb -Wall -lm
#CFLAGS=-Os -Wall -lm


LIGHT_O=main.o  \
        light.o \
        var.o   \
        num.o   \
        str.o   \
        tbl.o   \
        fn.o
        

all: light

light: $(LIGHT_O)
	$(CC) $(CFLAGS) $(LIGHT_O) -o $@

%.s: %.c
	$(CC) $(CFLAGS) $< -S 

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	-rm *.o 
	-rm light
