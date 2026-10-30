CC      ?= cc
CFLAGS  ?= -O2 -g
CFLAGS  += -std=gnu11 -Wall -Wextra -Werror -Isrc
X11_INC ?= /usr/include
X11_LIB ?= -lX11
BIN     := build/tilt

all: $(BIN)

$(BIN): src/wm.c src/client.c src/layout.c src/tilt.h src/layout.h
	@mkdir -p build
	$(CC) $(CFLAGS) -I$(X11_INC) src/wm.c src/client.c src/layout.c -o $@ $(X11_LIB)

check:
	./tests/run-tests.sh

clean:
	rm -rf build

.PHONY: all check clean
