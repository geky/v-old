CC=gcc
#CFLAGS=-O3 -Wall -lm 
#CFLAGS=-O3 -pg -Wall -lm
CFLAGS=-O0 -g3 -gdwarf-2 -ggdb -Wall -lm
#CFLAGS=-Os -Wall -lm


V_O=main.o    \
    var.o     \
    num.o     \
    str.o     \
    tbl.o     \
    fn.o      \
    parse.o   \
    builtin.o \
    vdbg.o
        

all: v

v: $(V_O)
	$(CC) $(V_O) $(CFLAGS) -o $@

%.s: %.c
	$(CC) $(CFLAGS) $< -S 

%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

clean:
	-rm *.o 
	-rm v
