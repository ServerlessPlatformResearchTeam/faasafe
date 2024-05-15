CC    := gcc
DIR   := $(PWD)
BDIR  := $(DIR)/build
SRC     := $(wildcard src/*.c src/libs/*.c)
NAME    := groundhog-launcher
NAME_TRACEE_LIB    := groundhog-tracee-lib.so
TRACEE_LIB_SRC := src/c_lib_tracee.c  src/c_lib_tracee.h 
CFLAGS  := -pedantic -Wall -Werror -W -g -pthread -fstack-protector -Wunused-function #-pg
LFLAGS  := -lrt -lm -g -lpthread #-pg

gh: $(SRC) $(BDIR)
	$(CC) `pkg-config --cflags glib-2.0` $(SRC) -o $(BDIR)/$(NAME) $(LFLAGS) `pkg-config --libs glib-2.0`
	$(CC) -fPIC -shared -o $(BDIR)/$(NAME_TRACEE_LIB) $(TRACEE_LIB_SRC)
	$(CC) -static src/dump_sd_bits/dump_sd_pages.c -O0 -o build/dump_sd_pages -Wall

all: $(SRC) $(BDIR)
	$(CC) `pkg-config --cflags glib-2.0` $(SRC) -o $(BDIR)/$(NAME) $(LFLAGS) `pkg-config --libs glib-2.0`
	$(MAKE) -C tests/
	$(CC) -fPIC -shared -o $(BDIR)/$(NAME_TRACEE_LIB) $(TRACEE_LIB_SRC)
	$(CC) -static src/dump_sd_bits/dump_sd_pages.c -O0 -o build/dump_sd_pages -Wall
	@sudo setcap cap_sys_ptrace,cap_sys_admin+ep build/groundhog-launcher

runtests:
	./test.sh

test: 	clean all runtests

clean:
	$(RM) -r $(BDIR) *.o

$(BDIR):
	@mkdir -p $@
