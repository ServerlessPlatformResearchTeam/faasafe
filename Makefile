CC    := gcc
DIR   := $(PWD)
BDIR  := $(DIR)/build
SRC     := $(wildcard src/*.c src/libs/*.c)
NAME    := faasafe-launcher
CFLAGS  := -pedantic -Wall -Werror -W -g -pthread -fstack-protector -Wunused-function #-pg
LFLAGS  := -lrt -lm -g -lpthread #-pg

SHARED_LIB_NAME := faasafe-shared-lib.so
SHARED_LIB_SRC  := src/shared_library/shared_library.c  src/shared_library/shared_library.h

gh: $(SRC) $(BDIR)
	$(CC) `pkg-config --cflags glib-2.0` $(SRC) -o $(BDIR)/$(NAME) $(LFLAGS) `pkg-config --libs glib-2.0`

sharedlib: $(SRC) $(BDIR)
	$(CC) -fPIC -shared -o $(BDIR)/$(SHARED_LIB_NAME) $(SHARED_LIB_SRC)

$(BDIR):
	@mkdir -p $@
