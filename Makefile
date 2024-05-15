CC    := gcc
DIR   := $(PWD)
BDIR  := $(DIR)/build
SRC     := $(wildcard src/*.c src/libs/*.c)
NAME    := faasafe-launcher
NAME_TRACEE_LIB    := faasafe-tracee-lib.so
CFLAGS  := -pedantic -Wall -Werror -W -g -pthread -fstack-protector -Wunused-function #-pg
LFLAGS  := -lrt -lm -g -lpthread #-pg

gh: $(SRC) $(BDIR)
	$(CC) `pkg-config --cflags glib-2.0` $(SRC) -o $(BDIR)/$(NAME) $(LFLAGS) `pkg-config --libs glib-2.0`

$(BDIR):
	@mkdir -p $@
