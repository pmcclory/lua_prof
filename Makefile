CFLAGS += -ggdb -O0
all: tracer

tracer: tracer.c
	$(CC) $(CFLAGS) -llua -o $@ $^

clean:
	rm -rf tracer
