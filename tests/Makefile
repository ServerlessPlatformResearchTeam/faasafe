SRC := $(wildcard *.c)
BINS := $(patsubst %.c,%,$(SRC))

CFLAGS=-pedantic -Werror -W -g -pthread -fstack-protector 
LFLAGS=-lpthread -lm -lrt -g

.PHONY: all
all: $(BINS)

bin:
	mkdir $@

%: %.c | bin 
	$(CC) $(CFLAGS) lib/* ../src/c_lib_tracee.c ../src/c_lib_tracee.h $< -o $@  $(LFLAGS)
	$(CC) -static test_sd_stability.c -O0 -o bin/test_sd_stability -Wall
	mv $@ bin/

clean:
	rm -f  bin/*
