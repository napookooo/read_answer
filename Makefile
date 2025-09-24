CC=gcc
CFLAGSCMD=-std=c18 -O7 -fsanitize=address -Wall -fno-omit-frame-pointer -D_POSIX_C_SOURCE=200809L
FLAGSCMD=-lm
CFLAGSUI=-std=c18 -O2 -Wall -Wno-deprecated-declarations `pkg-config --cflags gtk4 gio-2.0`
LDFLAGSUI=`pkg-config --libs gtk4 gio-2.0`
SRCDIR=src
BUILDDIR=build

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS := $(subst $(SRCDIR), $(BUILDDIR), $(patsubst %.c, %.o, $(SRCS)))
BIN = main

all: $(BIN)

$(BIN): $(OBJS)
	# $(CC) -o read_ans ./build/main.o ./build/cJSON.o $(CFLAGSCMD) $(FLAGSCMD)
	# fix later
	gcc -o read_ans ./src/main.c ./src/cJSON.c -lm
	$(CC) -o ui ./build/ui.o $(CFLAGSUI) $(LDFLAGSUI)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	# @ $(CC) -c -o ./build/main.o ./src/main.c $(CFLAGSCMD) $(FLAGSCMD)
	# @ $(CC) -c -o ./build/cJSON.o ./src/cJSON.c $(CFLAGSCMD) $(FLAGSCMD)
	@ $(CC) -c -o ./build/ui.o ./src/ui.c $(CFLAGSUI) $(LDFLAGSUI)

clean:
	rm -r $(BUILDDIR)/*
	rm main
