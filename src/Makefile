# ---------------------------------------------------------------------------- #
#
# Created	: Fri 14 Apr 2006 07:20:44 PM CDT
# Modified	: Sun 11 Jan 2009 10:50:59 AM PST
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
ifeq ($(VERSION),)
    # Get the version from Makefile in the parent directory
    VERSION	= $(shell sed -n '/^VERSION/{s/^.*= //p;q}' ../Makefile)
endif

ifdef NO_DEBUG
    CFLAGS += -DNDEBUG -DDEBUG_LEVEL=0
else
    CFLAGS += -DDEBUG_LEVEL=1 -g
endif

CFLAGS		+= -fno-strict-aliasing
CFLAGS		+= -DVERSION=\"$(VERSION)\" -Wall
CFLAGS		+= $(shell pkg-config pangoxft xrender xrandr --cflags)
LDLIBS		+= $(shell pkg-config pangoxft xrender xrandr --libs)
CFLAGS		+= $(shell pkg-config xext --silence-errors --cflags)
LDLIBS		+= $(shell pkg-config xext --silence-errors --libs)
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
