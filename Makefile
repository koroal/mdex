NAME=mdex

CC=cc
WARN=-Werror=pedantic -Wall -Wextra -Wconversion -Wno-unused-function -Wno-unused-parameter
CFLAGS=$(WARN) -std=c89 -O3 -D_GNU_SOURCE
LDFLAGS=-lm -lcurl -lminizip

HEADERS=src/*.h
SOURCES=src/*.c
PROGRAM=$(NAME)

all: strip

$(PROGRAM): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROGRAM) $(SOURCES)

strip: $(PROGRAM)
	strip --strip-unneeded $(PROGRAM)

clean:
	rm -f $(PROGRAM)

build: $(PROGRAM)

rebuild: clean build
