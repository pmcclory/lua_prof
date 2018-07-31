CFLAGS += -ggdb -O0 -Werror -Wno-pointer-to-int-cast -Wall
all: tracer

rproc.o: rproc.c
	$(CC) $(CFLAGS) -c -o $@ $^

tracer: tracer.c rproc.o
	$(CC) $(CFLAGS) -llua -o $@ $^

clean:
	rm -rf tracer rproc.o
