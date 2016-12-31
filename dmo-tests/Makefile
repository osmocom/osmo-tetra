CFLAGS=-O2 -Wall -march=native `pkg-config --cflags libosmodsp`
LDLIBS=`pkg-config --libs libosmodsp`

main: main.o pi4cxpsk.o

clean:
	rm -f *.o main
