CC=gcc
CFLAGS=-O3 -Wall -Werror -g

LIGHT_O=main.o \
        var.o


all: light

light: $(LIGHT_O)
	$(CC) $(CFLAGS) $(LIGHT_O) -o $@

%.o: %.c bf
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	-rm *.o 
	-rm light
