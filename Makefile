.POSIX:
.PHONY: all install uninstall clean

VERSION = 1.0

PREFIX = /usr/local
CC = cc
CFLAGS = -Wall -Wextra -DVERSION=\"$(VERSION)\"

all: temu
install: all
	cp -f temu $(DESTDIR)/$(PREFIX)/bin
uninstall:
	rm -f $(DESTDIR)/$(PREFIX)/bin/temu
clean:
	rm -f temu temu.core temu.o a.out
