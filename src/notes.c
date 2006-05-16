/* -------------------------------------------------------------------------- *\

  Created	: Fri 14 Apr 2006 07:20:44 PM CDT
  Modified	: Sat 13 May 2006 05:24:06 PM CDT
  Author	: Gautam Iyer <gi1242@users.sourceforge.net>
  Licence	: GPL2

\* -------------------------------------------------------------------------- */

#include "xnots.h"

#include <X11/extensions/shape.h>

#define LDEBUG_LEVEL DEBUG_LEVEL

#if LDEBUG_LEVEL
# define	TRACE( d, x )						\
{									\
    if( d <= LDEBUG_LEVEL ) fprintf x ;					\
}
#else
# define	TRACE( d, x )
#endif


/* -------------------------------------------------------------------------- *\

				LOCAL PROTOTYPES

\* -------------------------------------------------------------------------- */

int	xnotsXErrorHandler	( Display *display, XErrorEvent *event);
void	readNoteFromFile	( Note		*note,
				  const char	*filename,
				  Bool		reset
				);
void	initWindowAttrs		( Note *note );

/* -------------------------------------------------------------------------- *\

				   BEGIN CODE

\* -------------------------------------------------------------------------- */

int
xnotsXErrorHandler( Display *display, XErrorEvent *event)
{
    xnots.xErrorReturn = event->error_code;

#if LDEBUG_LEVEL == 0
    /* Only print debug info if x errors are allowed */
    if ( !xnots.allowXErrors )
#endif
    {
	char	error_msg[1024];

	XGetErrorText ( display, event->error_code, error_msg, 1023);
	errorMessage( "%s", error_msg );
    }

    return 0;		/* ignored anyway */
}


/*
  Function that opens the connection to the X server. This should only be called
  once on initialization (after all options are read).
*/
void
initXnots( char **options, unsigned long flags, char *text )
{
    TRACE( 2, ( stderr, "initXnots()\n"));


    DPY = XOpenDisplay( options[ opDisplay ] );

    if( DPY == NULL )
	fatalError( "Could not connect to display %s.",
		options[ opDisplay ] != NULL ?
			options[ opDisplay ] : "$DISPLAY" );

    /* To debug X events set uncomment this */
#if 0
    XSynchronize( DPY, True );
#endif

    SCREEN	= DefaultScreen( DPY);
    VISUAL	= DefaultVisual( DPY, SCREEN);
    CMAP	= DefaultColormap( DPY, SCREEN);
    DEPTH	= DefaultDepth( DPY, SCREEN );

    xnots.allowXErrors	= 0;
    xnots.xErrorReturn	= Success;

    XSetErrorHandler( xnotsXErrorHandler );

    xnots.xaDelete	= XInternAtom( DPY, "WM_DELETE_WINDOW", False);
    xnots.xaRootPmapID	= XInternAtom( DPY, "_XROOTPMAP_ID", False);
    xnots.xaSetRootID	= XInternAtom( DPY, "_XSETROOT_ID", False);

    xnots.pcontext	= pango_xft_get_context( DPY, SCREEN );

    xnots.notes	= NULL;
    xnots.nNotesMax	= 0;
    xnots.nNotes	= 0;

    RLIST.files		= NULL;
    RLIST.nFiles	= 0;
    RLIST.nFilesMax	= 0;

    GEOMLIST.items	= NULL;
    GEOMLIST.nItems	= 0;
    GEOMLIST.nItemsMax	= 0;

    xnots.terminate	= 0;

    /* Watch for root background changes */
    XSelectInput( DPY, XROOT, PropertyChangeMask );

    /* Setup root background info */
    refreshRootBGVars();

    /* Setup xnots.defaultNote */
    initNoteOptions( &xnots.defaultNote, options, flags, 0u, text, True );
}


void
cleanExit()
{
    int i;

    infoMessage( "Exiting." );

    if( GEOMLIST.lastChange.tv_sec )
	saveGeometryList();

    for( i = 0; i < xnots.nNotes; i++ )
	freeNote( NOTES[i] );

    g_object_unref( xnots.pcontext);
    XCloseDisplay( DPY );
}


/*
  If reset is true, then any old information in the note structure is ignored.
  Otherwise, old options are preserved unless over-ridden by options / flags.

  flagsMask specifies the bits in flags that have been changed. (If reset is
  false, only these bits are copied into note->flags).
*/
void
initNoteOptions( Note *note, char **options,
	unsigned long flags, unsigned long flagsMask,
	char *text, Bool reset )
{
    TRACE( 2, ( stderr, "initNoteOptions( ..., '%.20s' )\n",
		text ? text : "(nil)" ));

    int		resetWidth = 0;		/* Wether the pango layout width needs
					   to be re-computed or not */

    if( reset )
    {
	note->flags		= flags;

	note->win		= None;
	note->draw		= NULL;

	note->mapped		= 0;

	/*
	  Initialize note geometry
	*/
	note->x		= 0;
	note->y		= 0;
	note->width	= 100;
	note->height	= 100;

	/*
	  Initialize note margins
	*/
	note->lMargin	= 10;
	note->rMargin	= 10;
	note->tMargin	= 10;
    }
    else
    {
	note->flags		&= ~flagsMask;
	note->flags		|= (flags & flagsMask );
    }

    /* Set title of the note */
    if( reset || options[ opTitle ] )
    {
	if( !reset ) free( note->winTitle );
	note->winTitle = ( options[opTitle] == NULL ?
				NULL : strdup( options[opTitle] ) );
    }

    if( options[ opGeometry ] != NULL )
    {
	int		gMask;

	int		x, y;
	unsigned	width, height;
	
	gMask = XParseGeometry( options[opGeometry], &x, &y, &width, &height );

	if( gMask & XNegative )
	    x += DisplayWidth( DPY, SCREEN ) - width;
	if( gMask & YNegative )
	    y += DisplayHeight( DPY, SCREEN ) - height;

	if (
	     reset || x != note->x || y != note->y
	     || width != note->width || height != note->height
	   )
	{
	    if( width != note->width )
		resetWidth = 1;

	    note->x	 = x;
	    note->y	 = y;
	    note->width  = width;
	    note->height = height;

	    note->bgGrabbed = 0;

	    TRACE( 3, ( stderr, "Got note geometry %ux%u%d%d (0x%x)\n",
		    width, height, x, y, gMask ) );
	}
    }

    /*
      Alloc fg / bg colors.
    */
    if( reset || options[ opForeground ] )
	XftColorAllocName( DPY, VISUAL, CMAP,
		options[ opForeground ] ? options[ opForeground ] : "#000000",
		&note->fg );

    if( reset || options[ opBackground ] )
    {
	XftColor	newbg;

	XftColorAllocName( DPY, VISUAL, CMAP,
		options[ opBackground ] ? options[ opBackground ] : "#a0a000",
		&newbg );

	if( reset || note->bg.pixel != newbg.pixel )
	{
	    note->bg		= newbg;
	    note->bgGrabbed	= 0;
	}
    }


    if( reset || options[ opAlpha ] )
    {
	unsigned alpha = options[opAlpha] ?
			    strtoul( options[opAlpha], NULL, 0 ) : 0x80;

	if( alpha > 0xff )	alpha = 0x80;

	if( reset || note->alpha != (unsigned short) alpha )
	{
	    note->alpha		= (unsigned short) alpha;
	    note->bgGrabbed	= 0;
	}
    }

    if( note->bgGrabbed == 0 )
	gettimeofday( &note->bgTimeout, NULL );


    /* Note radius */
    if( reset || options[ opRoundRadius ] )
    {
	unsigned radius = options[opRoundRadius] ? 
			    strtoul( options[opRoundRadius], NULL, 0 ) : 0;

	note->radius = (unsigned short) radius;
	TRACE( 9, ( stderr, "Setting radius = %hu\n", note->radius ) );
    }

    /*
      Margins
    */
    if( reset || options[ opLMargin ] )
    {
	unsigned margin = options[ opLMargin ] ?
			    strtoul( options[opLMargin], NULL, 0 ) : 10;

	if( reset || note->lMargin != margin )
	{
	    resetWidth = 1;
	    note->lMargin = margin;
	}
    }

    if( reset || options[ opRMargin ] )
    {
	unsigned margin = options[ opRMargin ] ?
			    strtoul( options[opRMargin], NULL, 0 ) : 10;

	if( reset || note->rMargin != margin )
	{
	    resetWidth = 1;
	    note->rMargin = margin;
	}
    }

    if( reset || options[ opTMargin ] )
    {
	unsigned margin = options[ opTMargin ] ?
			    strtoul( options[opTMargin], NULL, 0 ) : 10;

	note->tMargin = margin;
    }

    if( reset || options[ opBMargin ] )
    {
	unsigned margin = options[ opBMargin ] ?
			    strtoul( options[opBMargin], NULL, 0 ) : 0;

	note->bMargin = margin;
    }

    /*
      Pango initialization.
    */
    if( reset )
    {
	note->playout = pango_layout_new( xnots.pcontext );
	pango_layout_set_wrap( note->playout, PANGO_WRAP_WORD_CHAR );
    }

    if( reset || resetWidth )
	resetNoteWidth( note );

    if( reset || options[ opIndent ] )
    {
	PangoTabArray	*tabs;
	int		indent;
	
	indent = options[ opIndent ] ?
			strtol( options[opIndent], NULL, 0 ) : -10;
	pango_layout_set_indent( note->playout, indent * PANGO_SCALE );

	/*
	  Also set tabs so a tab will skip the 'hanging' indent when indent is
	  negative.
	*/
	if( indent < 0 )
	{
	    tabs = pango_tab_array_new_with_positions( 1, TRUE,
			    PANGO_TAB_LEFT, abs( indent ) );
	    pango_layout_set_tabs( note->playout, tabs );
	    pango_tab_array_free( tabs );
	}
    }

    if( reset || options[opFont] || options[opFontSize] )
    {
	/*
	  Need to set some font information.
	*/
	PangoFontDescription	*pfontd;
	
	if( reset )
	    pfontd = pango_font_description_new();
	else
	    pfontd = pango_font_description_copy(
			    pango_layout_get_font_description( note->playout )
			);

	if( reset || options[opFont] )
	    pango_font_description_set_family( pfontd,
		    options[opFont] ? options[ opFont ] : "Sans" );

	if ( reset || options[opFontSize] )
	{
	    int size = 0;
	    
	    if( options[opFontSize] )
		size = atoi( options[opFontSize] );

	    if( reset || size > 0 )
		pango_font_description_set_size( pfontd,
			(size > 0 ? size : 10 ) * PANGO_SCALE );
	}

	pango_layout_set_font_description( note->playout, pfontd );
	pango_font_description_free( pfontd );
    }


    if( reset || ( text && strlen(text) > 0 ) )
	setNoteText( note, text );
}


/*
  Copies note src into dest. Contents of dst had better be empty, as they are
  all ignored.
*/
void
copyNote( Note *dst, Note *src )
{
    TRACE( 2, ( stderr, "copyNote()\n" ));

    dst->flags	= src->flags;

    dst->x	= src->x;
    dst->y	= src->y;
    dst->width	= src->width;
    dst->height	= src->height;

    dst->bg	= src->bg;
    dst->fg	= src->fg;

    dst->mapped	= 0;		/* Reset to unmapped */

    dst->alpha	= src->alpha;

    dst->radius	= src->radius;

    /* Reset bgGrabbed instead of copying it. Also ignore prevPos */
    dst->bgGrabbed	  = 0;
    gettimeofday( &dst->bgTimeout, NULL);	/* timeout now */

    dst->lMargin = src->lMargin;
    dst->rMargin = src->rMargin;
    dst->tMargin = src->tMargin;
    dst->bMargin = src->bMargin;


    /* Don't copy filename over, but reset it instead. */
    dst->filename = NULL;

    /* strdup the winTitle instead of coping the pointer. */
    dst->winTitle = ( src->winTitle == NULL ? NULL : strdup( src->winTitle ) );

    /*
      The pango layout needs to be duplicated (not just blindly copied over).
    */
    dst->playout = pango_layout_copy( src->playout );
}


/*
  Adds a note to our list of notes
*/
void
addNote( Note *note )
{
    TRACE( 2, ( stderr, "addNote()\n" ));

    const int nNotesInc = 10;

    if( xnots.nNotes == xnots.nNotesMax )
    {
	xnots.nNotesMax += nNotesInc;

	/* Need need space for more notes */
	NOTES = (Note**) xnots_realloc( NOTES,
				    xnots.nNotesMax * sizeof(Note*) );
    }

    NOTES[xnots.nNotes++] = note;
}


/*
  Find note on our list of notes and delete it.
*/
void
delNote( Note *note )
{
    TRACE( 2, ( stderr, "delNote()\n"));

    int i;

    for( i=0; i < xnots.nNotes; i++ )
	if( note == NOTES[i] )
	{
	    delNoteNum( i );
	    return;
	}

    /* If we got here, then we tried to delete a note that's not on our list */
    assert( 0 );
}


void
delNoteNum( int i )
{
    TRACE( 2, ( stderr, "delNoteNum()\n"));

    int		j;
    Note	*note = NOTES[i];
    const int 	nNotesInc = 10;

    /* Sanity check */
    assert( i >= 0 && i < xnots.nNotes );

    /*
      Free resources associated with the i'th note.
    */
    freeNote( note );

    /*
      Move remaining notes up in our list of notes, and shorten our list of
      notes.
    */
    for( j=i; j < xnots.nNotes; j++ )
	NOTES[j] = NOTES[j+1];

    xnots.nNotes--;


    /*
      See if we can reduce the size of our note list.
    */
    if (
	    xnots.nNotes <= xnots.nNotesMax - nNotesInc
	    && xnots.nNotes >= 0
       )
    {
	xnots.nNotesMax -= nNotesInc;

	if( xnots.nNotesMax > 0 )
	{
	    NOTES = (Note**) xnots_realloc( NOTES,
				    xnots.nNotesMax * sizeof(Note*) );
	}
	else
	{
	    free( NOTES );
	    NOTES = NULL;
	}
    }

    return;
}


/*
  Create a note. If template is not NULL, then copy options from it.
  If filename is NULL, then use text for the text of the note. Otherwise read
  options from file.
*/
Note *
createNote( Note *template, char *text, const char *filename )
{
    TRACE( 2, ( stderr, "createNote( %p, %s, %s)\n", template,
		text ? text : "(nil)", filename ? filename : "(nil)" ) );

    Note		*note;

    /*
      We should have atleast a template to copy from, or a file to read options
      from.
    */
    assert( template != NULL || filename != NULL );

    /*
      Get memory for the note.
    */
    note = (Note*) xnots_malloc( 1 * sizeof(Note) );

    if( template != NULL )
	copyNote( note, template );

    if( filename != NULL )
    {
	/*
	  If template == NULL then pass True to initNoteOptions, so that all the
	  note options will be reset to defaults.
	*/
	readNoteFromFile( note, filename, template == NULL );

	/* Set note filename */
	note->filename = strdup( filename );
    }
    else
	note->filename = NULL;

    if( filename == NULL && text != NULL && strlen( text ) > 0 )
	setNoteText( note, text );

    /*
      Create the note window and set default window attributes. initWindowAttrs
      will create the window for us if note->win == None.
    */
    note->win = None;
    initWindowAttrs( note );

    /*
      Accept the delete protocol, so that when the window is closed we can exit
      gracefully.
    */
    XSetWMProtocols( DPY, note->win, &xnots.xaDelete, 1);

    /*
      Set WM hints so notes never receive input
    */
    {
	XWMHints		wmHints;

	/*
	  Set the WM hints.
	*/
	wmHints.flags	= InputHint;
	wmHints.input	= False;	/* Notes never need input */
	XSetWMHints( DPY, note->win, &wmHints );
    }

    /*
      Try and make the note borderless. This does not work on Fvwm.
    */
    {
	Atom		prop;
	CARD32		hints;		/* KDE/GNOME hints */
	MWMHints	mwmhints;	/* Motif hints */

	hints			= (CARD32) 0;
	mwmhints.flags		= MWM_HINTS_DECORATIONS;
	mwmhints.decorations	= 0;

	/* Motif compatible WM */
	prop = XInternAtom( DPY, "_MOTIF_WM_HINTS", True);
	if( prop != None )
	    XChangeProperty( DPY, note->win, prop, prop,
		    32, PropModeReplace, (unsigned char*) &mwmhints,
		    PROP_MWM_HINTS_ELEMENTS);

	/* GNOME compatible WM */
	prop = XInternAtom( DPY, "_WIN_HINTS", True);
	if( prop != None )
	    XChangeProperty( DPY, note->win, prop, prop,
		    32, PropModeReplace, (unsigned char*) &hints, 1);

	/* KDE compatible WM */
	prop = XInternAtom ( DPY, "KWM_WIN_DECORATION", True);
	if( prop != None )
	    XChangeProperty( DPY, note->win, prop, prop,
		    32, PropModeReplace, (unsigned char*) &hints, 1);
    }
    

    /*
      Initialize XftDrawable.
    */
    note->draw = XftDrawCreate( DPY, note->win,
			VISUAL, CMAP );

    return note;
}


/*
  x, y, width, height are the current (actual) dimensions of the note window. If
  these differ from the expected values stored in note, then we send resize /
  move requests to the X server to put our note window in the position stored in
  the note structure.
*/
void
correctNoteGeometry( Note *note, int x, int y, unsigned width, unsigned height)
{
    TRACE( 2, ( stderr, "correctNoteGeometry( %lx, %d, %d, %u, %u)\n",
		note->win, note->x, note->y, note->width, note->height ) );

    if( x != note->x || y != note->y )
	XMoveWindow( DPY, note->win, note->x, note->y );

    if( width != note->width || height != note->height )
    {
	resetNoteWidth( note );
	XResizeWindow( DPY, note->win, note->width, note->height );
    }

    correctNoteHeight( note );
}

/*
  Resize the note window to honor the bottom margin. note->width must be
  accurate before calling this function for notes with mapped windows.
*/
void
correctNoteHeight( Note *note )
{
    TRACE( 2, ( stderr, "correctNoteHeight()\n" ) );

    if( note->bMargin > 0 )
    {
	PangoRectangle	extents;
	unsigned	height;

	pango_layout_get_pixel_extents( note->playout, NULL, &extents );
	height = extents.height + note->tMargin + note->bMargin;

	if( note->height != height )
	{
	    if( note->mapped )
	    {
		/*
		  We should not alter note geometry for mapped notes. Send
		  resize requests to the X server instead.
		*/
		TRACE( 9, ( stderr, "Calling XResizeWindow( %lx, %u, %u )\n",
			    note->win, note->width, height ) );
		XResizeWindow( DPY, note->win, note->width, height );
	    }
	    else
	    {
		TRACE( 9, ( stderr, "Setting note height to %u\n", height ) );
		note->height	= height;
		note->bgGrabbed	= 0;

		updateGeometryList( note->filename, note->x, note->y,
			note->width, note->height );
	    }
	}
    }
}

/*
  Reset the width of note internals (e.g. the pango layout) to note->width.
*/
inline void
resetNoteWidth( Note *note )
{
    pango_layout_set_width( note->playout,
	    (note->width - note->lMargin - note->rMargin ) * PANGO_SCALE );
}

/*
  If note->win = None, then create the note window, and set it's properties.
  Otherwise just set the properties of the desired window.
*/
void
initWindowAttrs( Note *note )
{
    TRACE( 2, ( stderr, "initWindowAttrs( %p )\n", note ));

    unsigned long	 attrsMask = 0;
    XSetWindowAttributes attrs;

    /*
      Create the note window.
    */
    if( ISSET_FLAG( note->flags, flOverrideRedir ) )
    {
	attrs.override_redirect	= 1;
	attrsMask |= CWOverrideRedirect;
    }

    attrs.event_mask		= ExposureMask | StructureNotifyMask;
    attrsMask 			|= CWEventMask;

    if( note->win == None || !note->bgGrabbed || note->alpha == 0xff )
    {
	attrs.background_pixel	= note->bg.pixel;
	attrsMask		|= CWBackPixel;
    }


    if( note->win == None )
    {
	if( getGeometryFromList( note ) == GL_NOT_FOUND )
	    addGeometryToList( note->filename, note->x, note->y,
		    note->width, note->height );

	correctNoteHeight( note );
	note->win = XCreateWindow( DPY, XROOT,
		note->x, note->y, note->width, note->height, 0, CopyFromParent,
		CopyFromParent, CopyFromParent, attrsMask, &attrs );

	/* Newly created note windows are not mapped */
	note->mapped = 0;
    }
    else
	XChangeWindowAttributes( DPY, note->win, attrsMask, &attrs );

    /* Make the window the correct shape */
    shapeNoteWindow( note );

    /*
      Set the window and icon names.
    */
    {
	char		*winName = NULL;

	if( note->winTitle )
	    winName = note->winTitle;
	else if ( note->filename != NULL )
	{
	    winName = strrchr( note->filename, '/' );
	    if( winName == NULL )
		winName = note->filename;
	    else
		winName++;
	}

	if( winName != NULL )
	{
	    XChangeProperty( DPY, note->win, XA_WM_NAME, XA_STRING, 8,
		    PropModeReplace, winName, strlen( winName ) );
	    XChangeProperty( DPY, note->win, XA_WM_ICON_NAME, XA_STRING, 8,
		    PropModeReplace, winName, strlen( winName ) );
	}
    }


    /*
      Set the class hints.
    */
    {
	XClassHint	cHint;

	cHint.res_name	= note->filename;
	cHint.res_class	= "XNots";
	XSetClassHint( DPY, note->win, &cHint );
    }


    /*
      Put the window on all desktops
    */
    {
	Atom		wmDeskAtom = XInternAtom( DPY, "_NET_WM_DESKTOP", True);
	unsigned int	desktop = 0xffffffffu;

	if( wmDeskAtom != None )
	    XChangeProperty( DPY, note->win, wmDeskAtom, XA_CARDINAL, 32,
		    PropModeReplace, (unsigned char *) &desktop, 1 );
    }
}


/*
  Close a note, and free all resources associated with it.
*/
void
freeNote( Note *note )
{
    TRACE( 2, ( stderr, "freeNote()\n"));

    free( note->filename );
    free( note->winTitle );

    g_object_unref( note->playout );

    if( note->draw )
	XftDrawDestroy( note->draw );

    if( note->win )
	XDestroyWindow( DPY, note->win );

    /*
      Don't free the default note, or include it in our count of notes.
    */
    free( note );
}


/*
  Map all the note windows
*/
void
showNotes()
{
    TRACE( 2, ( stderr, "showNotes()\n"));

    int i;

    for( i=0; i < xnots.nNotes; i++ )
    {
	TRACE( 3, ( stderr, "Calling XMapWindow for note %d (winid %08lx).\n",
		    i, NOTES[i]->win ));
	XMapWindow( DPY, NOTES[i]->win );
    }
}


void
setNoteText( Note *note, char *text)
{
    TRACE( 2, ( stderr, "setNoteText()\n"));

    /*
      Always set the text using set_text first. That way if there is an error
      with the markup, the post-it will fall back to the raw text.
    */
    pango_layout_set_text( note->playout, text, -1);
    if( ISSET_FLAG( note->flags, flMarkup ) )
	pango_layout_set_markup( note->playout, text, -1);
}


void
reloadNote( int num )
{
    TRACE( 2, ( stderr, "reloadNote( %d )\n", num ) );
    assert( NOTES[num]->filename );

    Note	*note	= NOTES[num];

    int		x	= note->x,
		y	= note->y;
    unsigned	width	= note->width,
		height	= note->height;


    readNoteFromFile( note, note->filename, 0 );

    /* Reset the desired window attributes */
    /*
      2006-04-22 XXX: If the useMarkup flag is unset then the text still has
      some resitual attributes.
    */
    initWindowAttrs( note );

    /*
      Only set the ewmh attributes if note window is mapped. For unmapped notes,
      these attrs will be set on a map-notify event.
    */
    if( note->mapped )
    {
	setMappedWinAttrs( note );
	correctNoteGeometry( note, x, y, width, height);
    }

    /* Resend expose events */
    XClearArea( DPY, note->win, 0, 0, 0, 0, True );
}


/*
  If reset is true, then any previous information in the note structure
  (including WinID etc) will be reset. If not it will be preserved.
*/
void
readNoteFromFile( Note *note, const char *filename, Bool reset )
{
    assert( filename != NULL );
    TRACE( 2, ( stderr, "readNoteFromFile( %p, %s, %d )\n",
		note, filename, reset ) );

    char		buffer[MAX_NOTE_SIZE];
    char		*options[NOPTIONS];

    unsigned long	flags,
			flagsMask;
    int			i;


    for( i=0; i < NOPTIONS; i++ )
	options[i] = NULL;

    readOptionsFromFile( options, &flags, &flagsMask,
	    buffer, MAX_NOTE_SIZE, filename );

    initNoteOptions( note, options, flags, flagsMask, buffer, reset);

    freeOptions( options );
}


/*
  Shape the note window if note->radius > 0
*/
void
shapeNoteWindow( Note *note )
{
    TRACE( 2, ( stderr, "shapeNoteWindow( %lx ). Diameter %hu\n",
		note->win, note->radius * 2 ) );

    Pixmap	mask;
    GC		gc;
    XGCValues	values;
    unsigned	diameter = 2*(unsigned) note->radius;

    mask = XCreatePixmap( DPY, note->win, note->width, note->height, 1 );
    if( mask == None )
    {
	infoMessage( "Unable to shape note window\n" );
	return;
    }

    values.fill_style	= FillSolid;
    values.arc_mode	= ArcPieSlice;
    gc	= XCreateGC( DPY, mask, GCFillStyle | GCArcMode, &values );

    /* Clear the mask */
    XSetForeground( DPY, gc, 0 );
    XFillRectangle( DPY, mask, gc, 0, 0, note->width, note->height );

    /* Fill the mask with a rounded rectangle image */
    XSetForeground( DPY, gc, 1);

    /* Top */
    XFillArc( DPY, mask, gc, 0, 0, diameter, diameter,
	    180 * 64, -90*64);
    XFillRectangle( DPY, mask, gc, note->radius, 0,
	    note->width - diameter, note->radius);
    XFillArc( DPY, mask, gc, note->width - diameter, 0,
	    diameter, diameter, 90*64, -90*64 );

    /* Middle */
    XFillRectangle( DPY, mask, gc, 0, note->radius,
	    note->width, note->height - diameter );

    /* Bottom */
    XFillArc( DPY, mask, gc, 0, note->height - diameter,
	    diameter, diameter, -180*64, 90*64 );
    XFillRectangle( DPY, mask, gc, note->radius, note->height - note->radius,
	    note->width - diameter, note->radius);
    XFillArc( DPY, mask, gc, note->width - diameter, note->height - diameter,
	    diameter, diameter, -90 * 64, 90*64 );

    /*
      Shape the window
    */
    XShapeCombineMask( DPY, note->win, ShapeBounding, 0, 0, mask, ShapeSet );

    XFreePixmap( DPY, mask );
    XFreeGC( DPY, gc );
}
