CC=gcc
CFLAGS=-std=c18 -O2 -Wall -Werror -Wno-deprecated-declarations `pkg-config --cflags gtk4 gio-2.0`
LDFLAGS=`pkg-config --libs gtk4 gio-2.0`
SRCDIR=src
BUILDDIR=build

SRCS=$(wildcard $(SRCDIR)/*.c)
OBJS=$(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SRCS))
BIN=main

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR) main
