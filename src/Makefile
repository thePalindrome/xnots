# ---------------------------------------------------------------------------- #
#
# Created	: Fri 14 Apr 2006 07:20:44 PM CDT
# Modified	: Sat 06 May 2006 11:44:14 PM CDT
# Author	: Gautam Iyer <gi1242@users.sourceforge.net>
# Licence	: GPL2
#
# ---------------------------------------------------------------------------- #

#
# Source files
#
headers 	= xnots.h
extheaders	= inotify.h inotify-syscalls.h
sources		= background.c list.c main.c notes.c options.c savegeom.c

objects		= $(sources:.c=.o)

CC		= gcc

#
# Compile flags
#
override CFLAGS		+= $(shell pkg-config pangoxft xrender --cflags) -Wall
override LDLIBS		+= $(shell pkg-config pangoxft xrender --libs)
override CXXFLAGS	 = $(CFLAGS)

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
main.o:		$(headers) $(extheaders)	main.c
notes.o:	$(headers)			notes.c
options.o:	$(headers)			options.c
savegeom.o:	$(headers)			savegeom.c
