# ---------------------------------------------------------------------------- #
#
# Created	: Fri 14 Apr 2006 07:20:44 PM CDT
# Modified	: Mon 22 Aug 2016 11:33:50 AM EDT
# Author	: Gautam Iyer <gi1242@users.sourceforge.net>
# Licence	: GPL2
#
# ---------------------------------------------------------------------------- #

#
# Source files
#
headers 	= xnots.h
sources		= background.c list.c main.c notes.c options.c savegeom.c

objects		= $(sources:.c=.o)

CC		= gcc

#
# Compile flags
#
include ../config.mk

ifdef NO_DEBUG
    CFLAGS += -DNDEBUG -DDEBUG_LEVEL=0
else
    CFLAGS += -DDEBUG_LEVEL=1 -g
endif

CFLAGS		+= -fno-strict-aliasing -fgnu89-inline
CFLAGS		+= -DVERSION=\"$(VERSION)\" -Wall
CFLAGS		+= $(shell pkg-config $(PKGS) --cflags)
LDLIBS		+= $(shell pkg-config $(PKGS) --libs)
CXXFLAGS	 = $(CFLAGS)

#
# Make rules
#
xnots:	$(objects)
	$(CC) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o xnots

.PHONY: clean
clean:
	rm -f $(objects) xnots

.PHONY: buildall
buildall: clean xnots

#
# Source file dependencies
#
background.o:	$(headers)			background.c
list.o:		$(headers)			list.c
main.o:		$(headers)			main.c
notes.o:	$(headers)			notes.c
options.o:	$(headers)			options.c
savegeom.o:	$(headers)			savegeom.c
