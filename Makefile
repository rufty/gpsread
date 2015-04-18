##Revision:   $Revision: 1.0 $
##RCSdate:    $Date: 2014/06/11 21:10:10 $

APPNAME=gpsread
VERSION=0.9.1

# Where to install.
PREFIX=/usr/local

# What OS?
UNAME:=$(shell uname)

# Special compiler?
CC=gcc

# Basic options.
CFLAGS=-std=c99 -W -Wall -DVERSION=$(VERSION)
LFLAGS=

# Basic libraries.
ifeq ($(UNAME), Linux)
LIBS=-lm -lconfuse
endif
ifeq ($(UNAME), Darwin)
LIBS=-lm -lintl -lconfuse
endif

# All the code.
CODE = Makefile $(wildcard *.h) $(wildcard *.c)

# All the C source files.
SOURCES = $(wildcard *.c)
# All compiled C source files.
OBJECTS = $(SOURCES:.c=.o)

# Make search path.
VPATH=./rcsrepo

# Compile one file
.c.o:
	$(CC) $(CFLAGS) -MMD -c $<

# Default target.
all: $(APPNAME)

# Pull in header info.
-include *.d

# Link the app.
$(APPNAME): $(OBJECTS)
	$(CC) $(LFLAGS) -o $(APPNAME) $(OBJECTS) $(LIBS)

# Manpage.
$(APPNAME).1.gz: $(APPNAME).1
	cat $(APPNAME).1 | gzip -9 > $(APPNAME).1.gz

# Install the app.
install: $(APPNAME) $(APPNAME).1.gz
	install -D -m755 $(APPNAME) $(PREFIX)/bin/$(APPNAME)
	install -D -m644 $(APPNAME).1.gz $(PREFIX)/man/man1/$(APPNAME).1.gz

# Zap the cruft.
clean:
	rm -f *~
	rm -f *.d
	rm -f *.o
	rm -f tags
	rm -f $(APPNAME)
	rm -f $(APPNAME).1.gz
	rm -rf $(APPNAME).dSYM


# vim:ts=2:sw=2:tw=150:fo=tcnq2b:foldmethod=indent
