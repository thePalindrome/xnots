--------------------------------------------------------------------------------

				  DESCRIPTION

xnots is a desktop post-it / sticky note system for the unix geek. It does not
depend on any toolkit, and takes up very little memory / CPU on your computer.
And yes, your desktop notes can be made transparent ... :)

--------------------------------------------------------------------------------

			      INSTALL INSTRUCTIONS

REQUIREMENTS

    1. gcc, GNU Make and the usual tools on a unix like system.
    2. pkg-config (to determine cflags and libs for compilation).

DEPENDENCIES

    0. Xorg / X11 with Xft, Xrandr and XRender extensions.
    1. pango: http://www.pango.org. This must be compiled with Xft support.
    2. inotify kernel support. This is in by default in kernels 2.6.13 and up.

INSTALLATION

    Just type make. There is no configure step :). Edit config.mk for a few
    features.

	env CFLAGS=-O2 make install

    Or

	make prefix=/usr datadir=/usr/share/doc install

    See Makefile for additional configurable variables and targets. Set your
    CFLAGS if you like ;)

--------------------------------------------------------------------------------

				      TODO

Here are a few features I would like, but might not have time to implement. If
you implement it, send me a patch.

    1. PNG background support (using imlib2).
    2. Session management support.
    3. Show a close button on mouse-over. When clicked the note-text file is
       moved to a temp folder (or deleted).


				      BUGS

1. On fvwm, if a note geometry is +0+0, and another window has exactly the same
   geometry, then we seem to drop a ConfigureNotify event
2. If roundRadius is too large, then the notes look ugly.

--------------------------------------------------------------------------------

Author		: gi1242+xNots@NoSpam.com (replace NoSpam with gmail)
Created		: Sat 15 Apr 2006 03:01:43 PM CDT
Modified	: Sat 12 Mar 2011 09:09:26 PM EST

