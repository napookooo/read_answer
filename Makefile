CC=gcc
CFLAGS=-std=c18 -Wall -Werror -O7
FLAGS=-lm
SRCDIR=src
BUILDDIR=build

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS := $(subst $(SRCDIR), $(BUILDDIR), $(patsubst %.c, %.o, $(SRCS)))
BIN = main

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(CFLAGS) $(FLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@ $(CC) -c -o $@ $< $(CFLAGS) $(FLAGS)

clean:
	rm -r $(BUILDDIR)/*
	rm main
