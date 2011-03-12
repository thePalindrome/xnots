/* -------------------------------------------------------------------------- *\

  Created	: Fri 14 Apr 2006 07:20:44 PM CDT
  Modified	: Sun 11 Jan 2009 12:36:10 PM PST
  Author	: Gautam Iyer <gi1242@users.sourceforge.net>
  Licence	: GPL2

\* -------------------------------------------------------------------------- */

#include "xnots.h"
#include <X11/extensions/Xrandr.h>

#include <sys/inotify.h>

#include <dirent.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <signal.h>

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

				  LOCAL MACROS

\* -------------------------------------------------------------------------- */

/*
  Length of buffer passed to read when reading from the inotify fd.

  TODO Replace this constant with a buffer that can enlarge. A file that changed
  has a really long file name, then this will cause xnots to hang (because
  repeated calls to read from the inotify fd will return nothing as the buffer
  is too small).
*/
#define BUF_LEN (1024)

/* Interval (micro seconds) to collect inotify events in */
#define IN_INTERVAL	250000

#define EVENT_SIZE  (sizeof (struct inotify_event))

/* Interval during which to collect background updates */
#define BG_INTERVAL	250000

/* Time to wait before saving our note geometries to a file */
#define SAVE_INTERVAL	5000000


/* -------------------------------------------------------------------------- *\

				LOCAL PROTOTYPES

\* -------------------------------------------------------------------------- */

int	isNoteFile		( const char *filename );
int	isNoteDirent		( const struct dirent *entry );
void	termSignalHandler	( int signum );
int	main			( int argc, char **argv );
int	refreshListIndex	( const char *s );
long	elapsedTime		( struct timeval *t );
void	mainLoop		();
void	refreshNotes		();
int	findNoteOfFile		( const char *filename );
void	processXEvent		();
void	processPropertyNotify	( XPropertyEvent *ev );
void	processClientMessage	( int noteNum, XClientMessageEvent *ev );
void	processExpose		( Note *note, XExposeEvent *ev );
void	processConfigureNotify	( Note *note, XConfigureEvent *ev );
void	processMapNotify	( Note *note, XMapEvent *ev );


/* -------------------------------------------------------------------------- *\

				   BEGIN CODE

\* -------------------------------------------------------------------------- */

/*
  Global variable containing info common to all notes.
*/
xnots_t xnots;

void
fatalError(const char *fmt,...)
{
    va_list         ap;

    va_start( ap, fmt);

    fprintf( stderr, APL_NAME " fatal error: " );
    vfprintf( stderr, fmt, ap );
    fprintf( stderr, "\n" );

    va_end( ap );

    exit(1);
}


void
errorMessage( const char *fmt, ... )
{
    va_list         ap;

    va_start( ap, fmt);

    fprintf( stderr, APL_NAME " error: " );
    vfprintf( stderr, fmt, ap );
    fprintf( stderr, "\n" );

    va_end( ap );
}


void
infoMessage( const char *fmt, ... )
{
    va_list         ap;

    if( xnots.verbose )
    {
	va_start( ap, fmt);

	fprintf( stderr, APL_NAME ": " );
	vfprintf( stderr, fmt, ap );
	fprintf( stderr, "\n" );

	va_end( ap );
    }
}



void*
xnots_realloc	( void *ptr, size_t size )
{
    ptr = realloc( ptr, size );

    if( ptr == NULL )
	fatalError( "Memory allocation failure" );

    return ptr;
}


void*
xnots_malloc ( size_t size )
{
    void *ptr = malloc( size );

    if( ptr == NULL )
	fatalError( "Memory allocation failure" );

    return ptr;
}


/*
  Return non-zero if filename is a file that corresponds to a note we should
  open
*/
int
isNoteFile( const char *filename )
{
    /* Check if the file is a directory, or not readable */
    struct stat		buf;

    if (
	  stat( filename, &buf ) != 0		/* Can not stat */
	  || !S_ISREG( buf.st_mode )		/* Not regular file */
	  || access( filename, R_OK ) != 0 	/* No permission to read */
       )
	return 0;

    /*
      Exclude file beginning with ., ending with ~ or .swp. These might be
      temporary files created by the editor.
    */
    return ( fnmatch( ".*", filename, 0 ) == FNM_NOMATCH )
	&& ( fnmatch( "*~", filename, 0 ) == FNM_NOMATCH )
	&& ( fnmatch( "*.swp", filename, 0 ) == FNM_NOMATCH );
}


int
isNoteDirent( const struct dirent *entry )
{
    return isNoteFile( entry->d_name );
}


void
termSignalHandler( int signum )
{
    xnots.terminate = 1;
}


/*
  Process command line options and call initXnots() to initialize global xnots
  structure.
*/
void
processOptions( int argc, char **argv )
{
    char		*homedir = getenv( "HOME" );
    char		config_file[PATH_MAX],
			notes_dir[PATH_MAX];
    char		opt;

    char		text[1024];
    unsigned long	flags, flagsMask;
    char		*options[NOPTIONS];
    int			nbytes;

    int			i;

    /* Get home directory from environment. */
    if( homedir == NULL )
	homedir = ".";

    /*
      Initialize option defaults
    */
    xnots.verbose = 1;
    sprintf( config_file, "%s/.xnotsrc", homedir );
    sprintf( notes_dir, "%s/.xnots", homedir );

    /*
      Process command line options.
    */
    while( (opt = getopt( argc, argv, "c:d:qhv" )) != -1 )
    {
	switch( opt )
	{
	    case 'q':
		/* Be quite about displaying status messages */
		xnots.verbose = 0;
		break;

	    case 'c':
		/* Config file */
		strcpy( config_file, optarg );
		break;

	    case 'd':
		strcpy( notes_dir, optarg );
		break;

	    case '?':
	    case 'h':
	    case 'v':
	    default:
		/* Print version */
		fprintf( stderr, "xNots-" VERSION ". (c) 2006 Gautam Iyer\n" );

		if( opt == 'v' )
		    exit(0);

		/* Print help */
		fprintf( stderr, "\n\
SYNOPSIS\n\
    xnots [-hqv] [-c file] [-d dir]\n\
\n\
OPTIONS\n\
    -c file	Use file as the config file (instead of ~/.xnotsrc).\n\
    -d dir	Look for notes in dir (instead of ~/.xnots).\n\
    -h		Print this help.\n\
    -q		Suppress all status messages.\n\
    -v		Print version info and exit.\n" );

		/* Exit */
		exit( opt == 'h' ? 0 : 1 );
	}
    }

    /* Read options from config file */
    for( i=0; i < NOPTIONS; i++)
	options[i] = NULL;

    nbytes = readOptionsFromFile( options, &flags, &flagsMask, text, 1024,
				    config_file);

    /* Initialize  the global xnots_t structure. */
    initXnots( options, flags, text );

#if 0 /* Debug info */
    for( i=0; i < NOPTIONS; i++)
	fprintf( stderr, "%d. %s\n", i,
		options[i] != NULL ? options[i] : "(nil)" );

    fprintf( stderr, "Binary flags: %lx\n\n", flags );

    fprintf( stderr, "Read %d bytes:\n%.*sEOF\n", nbytes, nbytes, text );
#endif

    freeOptions( options );		/* Not needed any longer */

    if( chdir( notes_dir ) == -1 )
	fatalError( "Could not change to directory %s", notes_dir );
}

int
main( int argc, char **argv )
{
    int	 		i;

    struct dirent	**namelist;
    int			nfiles;

    /* Process command line options and initialize xnots */
    processOptions( argc, argv );

    /* Read saved note geometries */
    readGeometryList();

    /* Open all notes in current directory */
    nfiles = scandir( ".", &namelist, isNoteDirent, alphasort );
    for( i=0; i < nfiles; i++ )
    {
	Note *note;

	if( !isNoteFile( namelist[i]->d_name ) )
	    continue;

	TRACE( 3, ( stderr, "Reading file %s\n", namelist[i]->d_name) );

	note = createNote( &xnots.defaultNote, NULL, namelist[i]->d_name );

	/* TODO Sanity check note here */
	assert( note );

	/* Add note to our list of notes */
	addToList( (void ***) &NOTES, note, &xnots.nNotes, &xnots.nNotesMax );

	free( namelist[i] );
    }
    if( nfiles ) free( namelist );

    /* Display all post-it notes on the desktop. */
    showNotes();

    if( xnots.nNotes == 0 )
	infoMessage( "No notes found. Polling for changes." );

    /*
      When bash is killed, then it sends HUP to all child processes. If we
      ignore HUP, then we will not be killed by bash on HUP signals
    */
    signal( SIGHUP, SIG_IGN );

    /*
      We'll run till we receive a SIGTERM signal. Gracefully exit on SIGTERM
    */
    signal( SIGINT , termSignalHandler );
    signal( SIGTERM, termSignalHandler );
    signal( SIGQUIT, termSignalHandler );

    /* Loop until it's time to exit. */
    mainLoop();

    /* Gracefully free resources. */
    cleanExit();

    return 0;
}


/*
  Sees if there is a file named s in the refresh list. If yes, return the index.
  If no return -1.
*/
int
refreshListIndex( const char *s )
{
    int i;

    for( i = 0; i < RLIST.nFiles; i++ )
    {
	if( !strcmp( s, RLIST.files[i]->filename ) )
	    return i;
    }

    return -1;	/* Not found */
}


/*
  time (in micro seconds) since time t
*/
long
elapsedTime( struct timeval *t )
{
    struct timeval	currentTime;

    gettimeofday( &currentTime, NULL );

    TRACE( 9, ( stderr, "Elapsed time %ld\n",
		1000000l * ( (long) currentTime.tv_sec - (long) t->tv_sec)
		    + ( (long) currentTime.tv_usec - (long) t->tv_usec ) ) );

    return 1000000l * ( (long) currentTime.tv_sec - (long) t->tv_sec)
		    + ( (long) currentTime.tv_usec - (long) t->tv_usec );
}


void
mainLoop()
{
    TRACE( 2, ( stderr, "Entering main loop\n"));

    int		fd	= inotify_init(),
		xfd	= XConnectionNumber( DPY ),
		maxfd	= max( fd, xfd ) + 1;

    int		retval;
    fd_set	rfds;

    char	buffer[BUF_LEN];

    int		i;

    /* Watch the current directory (we've already chdir'ed in here) */
    inotify_add_watch( fd, ".", IN_MOVE | IN_CREATE | IN_DELETE | IN_MODIFY
				 | IN_MOVE_SELF | IN_DELETE_SELF );

    while( !xnots.terminate )
    {
	if( RLIST.nFiles > 0 && elapsedTime( &RLIST.lastChange ) > IN_INTERVAL )
	    /*
	      Files in this list have notes that need to be refreshed.
	    */
	    refreshNotes();

	if (
	      GEOMLIST.lastChange.tv_sec
	      && elapsedTime( &GEOMLIST.lastChange ) > SAVE_INTERVAL
	   )
	    /*
	      Save the list of geometries to disk.
	    */
	    saveGeometryList();

	/*
	  Check to see if we have timeouts from any of the notes
	*/
	for( i=0; i < xnots.nNotes; i++ )
	{
	    if (
		  NOTES[i]->mapped && NOTES[i]->bgTimeout.tv_sec
		  && elapsedTime( &NOTES[i]->bgTimeout ) > BG_INTERVAL
	       )
	    {
		NOTES[i]->bgTimeout.tv_sec = 0;	/* reset timeout */

		if( setTransparentBG( NOTES[i] ) )
		    XClearArea( DPY, NOTES[i]->win, 0, 0, 0, 0, True );
	    }
	}


	if( XPending( DPY ) )
	    /*
	      Process X events only when we have them. Our time is better spent
	      calling select() as opposed to XNextEvent().
	    */
	    processXEvent();

	else
	{
	    TRACE( 5, ( stderr, "No pending events, polling ~/.xnots\n" ) );

	    struct timeval	timeout;
	    long		timeLeft	= 0,
				wantTimeout	= 0;

	    if( RLIST.nFiles > 0 )
	    {
		/*
		  If we've already got a bunch of files in our refresh list,
		  then only wait until IN_INTERVAL elapses before timing out on
		  select.
		*/
		wantTimeout	= 1;
		timeLeft	= IN_INTERVAL -
					elapsedTime( &RLIST.lastChange );
		TRACE( 5, ( stderr, "Wait %ldu for iNotify events\n",
			    timeLeft ) );
	    }

	    if( GEOMLIST.lastChange.tv_sec )
	    {
		/*
		  Time out to save the geometry list.
		*/
		timeLeft = min( wantTimeout ? timeLeft : SAVE_INTERVAL,
			SAVE_INTERVAL - elapsedTime( &GEOMLIST.lastChange ) );
		wantTimeout = 1;
	    }
	    

	    if( !wantTimeout ) timeLeft = BG_INTERVAL;
	    for( i=0; timeLeft > 0 && i < xnots.nNotes; i++ )
	    {
		if( NOTES[i]->bgTimeout.tv_sec )
		{
		    wantTimeout = 1;
		    timeLeft = min( timeLeft,
			    BG_INTERVAL - elapsedTime( &NOTES[i]->bgTimeout ) );

		    TRACE( 5, ( stderr, "Wait atmost %ldu for before"
				" background refresh of note %d\n",
				timeLeft, i ) );
		}
	    }

	    if( wantTimeout )
	    {
		timeLeft = max( timeLeft, 0 );

		timeout.tv_sec	= timeLeft / 1000000;
		timeout.tv_usec	= timeLeft % 1000000;

		TRACE( 5, ( stderr, "Waiting for %lu.%06lus\n",
			    timeout.tv_sec, timeout.tv_usec ) );
	    }

	    FD_ZERO( &rfds );

	    FD_SET( fd,  &rfds );
	    FD_SET( xfd, &rfds );	/* So select will return when we receive
					   X events */

	    /* Wait for ever if we don't already have a refresh list */
	    retval = select( maxfd, &rfds, NULL, NULL,
				wantTimeout ? &timeout : NULL );
	    TRACE( 9, ( stderr, "Select returned %d\n", retval ) );

	    if (retval == -1)
	    {
#if LDEBUG_LEVEL
		/* Happens when we recieve signals */
		perror("select()");
#endif
	    }

	    else if( retval && FD_ISSET( fd, &rfds ) )
	    {
		/* Process an inotify event on watched directory. */

		int len, i = 0;
		len = read (fd, buffer, BUF_LEN);

		if (len < 0)
		{
		    if (errno == EINTR)
			/* need to reissue system call */
			;
		    else
			perror ("read");
		}
		else if (len == 0)
		{
		    /*
		      BUF_LEN too small. Read won't return anything if buflen is
		      not large enough to hold inotify events.
		    */
		    fatalError( "Buffer length %d too small for inotify"
			    "events\n", BUF_LEN );
		}

		else
		{
		    while (i < len)
		    {
			struct inotify_event *event;

			event = (struct inotify_event *) &buffer[i];

			TRACE( 9, ( stderr, "wd=%d mask=%u cookie=%u len=%u\n",
			    event->wd, event->mask,
			    event->cookie, event->len) );

			if (event->len)
			{
			    TRACE( 3, (stderr,
					"inotify event on file %s, mask %u\n",
					event->name, event->mask) );

			    int	rlIndex = refreshListIndex( event->name );

			    /*
			      Save the time this event happened. We'll collect
			      inotify events over a small time interval since
			      most editors when writing a file will create
			      backup's or temp files.
			    */
			    gettimeofday( &RLIST.lastChange, NULL );

			    if( rlIndex >= 0 )
				/*
				  This file is already in our list. Just update
				  the mask.
				*/
				RLIST.files[rlIndex]->mask |= event->mask;
			    else
			    {
				rlistitem_t	*r;

				r = (rlistitem_t *) xnots_malloc(
					sizeof( rlistitem_t ) );

				r->filename = strdup( event->name );
				r->mask	    = event->mask;

				addToList( (void ***) &(RLIST.files), r,
					&(RLIST.nFiles), &(RLIST.nFilesMax) );
			    }
			}
			else if( event->mask & (IN_DELETE_SELF | IN_MOVE_SELF) )
			{
			    TRACE( 3, ( stderr,
				    "Deleted / moved watched directory\n"));
			    xnots.terminate = 1;
			}
			else
			{
			    TRACE( 3, ( stderr,
				    "Ignored inotify event with mask %u\n",
				    event->mask ) );
			}

			i += EVENT_SIZE + event->len;
		    } /* while( i < len ) */
		} /* else [len > 0] */
	    }

	    else
		TRACE( 3, ( stderr, "Received XEvent, or select timeout.\n" ));
	}
    }

    close( fd );

    TRACE( 2, ( stderr, "Done main loop\n") );
}


/*
  Refresh all notes in xnots.refreshList (RLIST) that need to be refreshed.
*/
void
refreshNotes()
{
    int		i;

#if LDEBUG_LEVEL > 2
    fprintf( stderr, "Need refresh on %s", RLIST.files[0]->filename );
    for( i=1; i < RLIST.nFiles; i++ )
	fprintf( stderr, ", %s", RLIST.files[i]->filename );
    fprintf( stderr, ".\n" );
#endif

    for( i = 0; i < RLIST.nFiles; i++ )
    {
	int noteNum = findNoteOfFile( RLIST.files[i]->filename );

	if( isNoteFile( RLIST.files[i]->filename ) )
	{
	    /*
	      It's a note file. Either reload the note associated to it, or
	      create a new note.
	    */
	    if( noteNum == -1 )
	    {
		/* Create a new note */
		Note *note = createNote( &xnots.defaultNote, NULL,
						RLIST.files[i]->filename );

		/* TODO Sanity check note here */

		/* Add note to our list of notes */
		addToList( (void ***) &NOTES, note,
				&xnots.nNotes, &xnots.nNotesMax );

		XMapWindow( DPY, note->win );
	    }
	    else
		reloadNote( noteNum );
	}

	else if( noteNum >= 0 )
	{
	    /* Delete this note from our list */
	    freeNote( NOTES[noteNum] );
	    delItemNumFromList( (void ***) &NOTES, noteNum,
				    &xnots.nNotes, &xnots.nNotesMax);
	}
	else
	    /* Probably we got a temp file created by the editor ... */
	    TRACE( 3, ( stderr, "Skipping non-Note file %s\n",
			    RLIST.files[i]->filename ) );

	/* Free memory associated with RLIST.files[i] */
	free( RLIST.files[i]->filename );
	free( RLIST.files[i] );
    }
    free( RLIST.files );

    RLIST.files		= NULL;
    RLIST.nFiles	= 0;
    RLIST.nFilesMax	= 0;
}


int
findNoteOfFile( const char *filename )
{
    int i;

    const char *s = strrchr( filename, '/' );

    if( s == NULL ) s = filename;
    else s++;

    for( i=0; i < xnots.nNotes; i++ )
    {
	if( !strcmp( s, NOTES[i]->filename ) )
	    return i;
    }

    return -1;		/* Not found */
}


void
processXEvent()
{
    int		i;
    XEvent	ev;

    XNextEvent( DPY, &ev );

#if LDEBUG_LEVEL >= 1
    char	*xEventName[] =
    {
	"Internal",
	"Internal",
	"KeyPress",
	"KeyRelease",
	"ButtonPress",
	"ButtonRelease",
	"MotionNotify",
	"EnterNotify",
	"LeaveNotify",
	"FocusIn",
	"FocusOut",
	"KeymapNotify",
	"Expose",
	"GraphicsExpose",
	"NoExpose",
	"VisibilityNotify",
	"CreateNotify",
	"DestroyNotify",
	"UnmapNotify",
	"MapNotify",
	"MapRequest",
	"ReparentNotify",
	"ConfigureNotify",
	"ConfigureRequest",
	"GravityNotify",
	"ResizeRequest",
	"CirculateNotify",
	"CirculateRequest",
	"PropertyNotify",
	"SelectionClear",
	"SelectionRequest",
	"SelectionNotify",
	"ColormapNotify",
	"ClientMessage",
	"MappingNotify",
    };
#endif

    TRACE( 3, (stderr, "Got XEvent %s\n", xEventName[ev.type] ) );

    /* Process events on the root window first */
    if( ev.xany.window == XROOT )
    {
	if( ev.type == PropertyNotify )
	    processPropertyNotify( (XPropertyEvent*) &ev );

	else if( ev.type == ConfigureNotify )
	{
	    /* Dimensions changed via randr, so notify Xlib. */
	    int oldrWidth = DisplayWidth( DPY, SCREEN ),
		oldrHeight = DisplayHeight( DPY, SCREEN );

	    if( XRRUpdateConfiguration( &ev ) )
	    {
		/*
		  After calling XRRUpdateConfiguration(), calls to DisplayWidth
		  or DisplayHeight() should return the new screen dimensions.
		 */
		int newrWidth = DisplayWidth( DPY, SCREEN ),
		    newrHeight = DisplayHeight( DPY, SCREEN );

		if( newrWidth != oldrWidth || newrHeight != oldrHeight )
		{
		    /*
		      Move notes keeping the center in the same relative
		      position.
		     */
		    int i;

		    TRACE( 3, (stderr,
				"Old geom %dx%d. New %dx%d. Moving notes\n",
				oldrWidth, oldrHeight, newrWidth, newrHeight));

		    for( i=0; i < xnots.nNotes; i++ )
		    {
			Note *n = NOTES[i];

			if( ISSET_FLAG( n->flags, flMoveOnScreenResize ) )
			{
			    int newx, newy;
			    int xc = n->x + n->width/2,
				yc = n->y + n->height/2;

			    if( xc == oldrWidth/2 )
				newx = (newrWidth - n->width)/2;
			    else
				newx = (xc > oldrWidth/2) ?
				    (n->x + n->width) * newrWidth/oldrWidth
					- n->width :
				    n->x * newrWidth/oldrWidth;

			    if( yc == oldrHeight/2 )
				newy = (newrHeight - n->height)/2;
			    else
				newy = (yc > oldrHeight/2) ?
				    (n->y + n->height) * newrHeight/oldrHeight
					- n->height :
				    n->y * newrHeight/oldrHeight;

			    TRACE( 3, ( stderr, "Note %d: Old %ux%u+%d+%d, "
					    "New %ux%u+%d+%d.\n", i,
					    n->width, n->height, n->x, n->y,
					    n->width, n->height, newx, newy
				       ));
			    XMoveWindow( DPY, n->win, newx, newy );
			}
		    }
		} /* if( newrWidth != oldrWidth ... ) */
	    } /* if( XRRUpdateConfiguration() ) */
	    else
	    {
		TRACE( 2, (stderr, "XRRUpdateConfiguration() failed"));
	    }
	}
	return;
    }

    /*
      Find the number of the note the event belonged to.
    */
    for( i=0; i < xnots.nNotes; i++ )
	if( NOTES[i]->win == ev.xany.window )
	    break;

    if ( i == xnots.nNotes )
    {
	/*
	  Event does not belong to one of our displayed notes. Probably because
	  it has been reloaded / destroyed.
	*/

	TRACE( 1, ( stderr, "Received event %s for unknown window (0x%08lx)\n",
		    xEventName[ev.type], ev.xany.window ) );

	return;
    }


    switch( ev.type )
    {
	case Expose:
	    processExpose( NOTES[i], (XExposeEvent*) &ev);
	    break;

	case ConfigureNotify:
	    processConfigureNotify( NOTES[i], (XConfigureEvent*) &ev);
	    break;

	case ClientMessage:
	    processClientMessage( i, (XClientMessageEvent*) &ev);
	    break;

	case MapNotify:
	    processMapNotify( NOTES[i], (XMapEvent*) &ev);
	    break;

	case UnmapNotify:
	    TRACE( 2, ( stderr, "UnmapNotify on note %d\n", i ) );
	    NOTES[i]->mapped = 0;
	    break;

	default:
	    TRACE( 1, ( stderr, "Unhandled event %s on note %d\n",
			xEventName[ev.type], i ) );
    }
}


void
processPropertyNotify( XPropertyEvent *ev )
{
    int	wantRefresh = 0;

    /*
      Wrap all property notify events into one (so that we don't run
      transparency updates too often, or worse still with a bad pixmap).
    */
    do
      {
	if( ev->atom == xnots.xaRootPmapID || ev->atom == xnots.xaSetRootID )
	{
	    struct timespec rqt;

	    wantRefresh = 1;

	    /* Sleep for 10ms every time one of these properties have changed */
	    rqt.tv_sec = 0;
	    rqt.tv_nsec = 10000000; /* 10 ms */
	    nanosleep(&rqt, NULL);
	}
      }
    while( XCheckTypedEvent( DPY, PropertyNotify, (XEvent*) ev) );

    if( wantRefresh )
    {
	int		i;
	struct timeval	t;

	/* Refresh our info about the root background */
	refreshRootBGVars();

	/* Set timeouts so that all notes update their background accordingly */
	gettimeofday( &t, NULL);
	for( i=0; i < xnots.nNotes; i++ )
	{
	    NOTES[i]->bgGrabbed = 0;
	    NOTES[i]->bgTimeout = t;
	}
    }
}


void
processClientMessage( int noteNum, XClientMessageEvent *ev )
{
    if( ev->format == 32 && (Atom) ev->data.l[0] == xnots.xaDelete )
    {
	freeNote( NOTES[noteNum] );
	delItemNumFromList( (void ***) &NOTES, noteNum,
				&xnots.nNotes, &xnots.nNotesMax);

	/* If the last note was deleted, exit */
	if( xnots.nNotes == 0 )
	    xnots.terminate = 1;
    }

}


void
processExpose( Note *note, XExposeEvent *ev )
{
    TRACE( 2, ( stderr, "Expose event on note %08lx\n", note->win ));

    Region r = XCreateRegion();

    /*
      If the root background is not set, then try and set it here. If
      successful, then clear the window and ask for a full refresh.
    */
#if 0
    if( !note->bgGrabbed && setTransparentBG( note ) )
	XClearArea( DPY, note->win, 0, 0, 0, 0, True );
#endif

    /*
      Wrap all expose events into one, and clip to the union of those regions.
    */
    TRACE( 9, ( stderr, "Expose regions: " ) );
    do
      {
	XRectangle rect;

	rect.x		= ev->x;
	rect.y		= ev->y;
	rect.width	= ev->width;
	rect.height	= ev->height;

	XUnionRectWithRegion( &rect, r, r);

	/*
	  XXX 2006-05-07 gi1242: There should be no need to call XClearArea
	  here. When we receive expose events, the X server should necessarily
	  have already cleared the appropriate areas of the window.

	  However in some cases this does not happen. For instance, when bMargin
	  > 0, we receive an expose event on the entire window with the contents
	  of the window intact (not cleared)! This causes jagged font edges when
	  drawn on screen.

	  It would be better to figure out why the window contents are not
	  cleared, as opposed to brute force re-clearing the screen here.

	  NOTE -- XClearArea must be called when the expose event is got by a
	  send-event.
	*/
	XClearArea( DPY, note->win,
		rect.x, rect.y, rect.width, rect.height, False );

	TRACE( 9, ( stderr, "[%ux%u+%d+%d%c]",
			rect.width, rect.height, rect.x, rect.y,
			ev->send_event ? 't' : 'f' ));
      }
    while( XCheckTypedWindowEvent( DPY, note->win, Expose,
		(XEvent*) ev) );
    TRACE( 9, ( stderr, ".\n" ) );

    XftDrawSetClip( note->draw, r );
    pango_xft_render_layout( note->draw, &note->fg, note->playout,
	    note->lMargin * PANGO_SCALE, note->rMargin * PANGO_SCALE );
    XftDrawSetClip( note->draw, None );

    XDestroyRegion( r );
}


void
processConfigureNotify( Note *note, XConfigureEvent *ev )
{
    TRACE( 2, ( stderr, "Configure event on note %08lx\n", note->win ));

    unsigned	rwidth, rheight;

    /* Don't do anything for a unmapped note */
    if( !note->mapped ) return;

    /* Gobble all configure notify events at once */
    do
      {
	TRACE( 3, ( stderr, "Geometry %ux%u+%d+%d\n",
		    ev->width, ev->height, ev->x, ev->y) );
      }
    while( XCheckTypedWindowEvent( DPY, note->win, ConfigureNotify,
		(XEvent*) ev ) );

    /*
      Remember the position of the window. Make sure that we remember the
      position relative to the root window (for root transparency). Notice this
      position will be offset by the Window manager.
    */
    XTranslateCoordinates( DPY, note->win, XROOT,
	    0, 0, &ev->x, &ev->y, &ev->window);

    /*
      If we're off screen, do nothing.
    */
    rwidth	= DisplayWidth( DPY, SCREEN );
    rheight	= DisplayHeight( DPY, SCREEN );

    if (
	  ev->x + ev->width  <= 0 || ev->x >= (int) rwidth
	  || ev->y + ev->height <= 0 || ev->y >= (int) rheight
       )
    {
	TRACE( 9, ( stderr, "CNotify while off screen\n" ) );
	return;
    }

    if (
	  note->x != ev->x || note->y != ev->y
	  || note->width != ev->width || note->height != ev->height
       )
    {
	int resized = 0;

	updateGeometryList( note->filename, ev->x, ev->y,
		ev->width, ev->height );

	note->x		= ev->x;
	note->y		= ev->y;

	if( note->width != ev->width )
	{
	    resized	= 1;
	    note->width = ev->width;

	    resetNoteWidth( note );

	    if( note->height == ev->height )
		correctNoteHeight( note );
	}

	if( note->height != ev->height )
	{
	    resized	 = 1;
	    note->height = ev->height;
	}

	if( resized )
	{
	    /* Window needs to be re-shaped */
	    shapeNoteWindow( note );

	    /*
	      Refresh the note background immediately on resize events.
	      Otherwise we are forced to generate an expose event now. Then
	      after our timeout when we reset the root background, we will have
	      to re-expose our note causing another timeout.
	    */
	    setTransparentBG( note );

	    TRACE( 9, ( stderr, "Exposing note %lx\n", note->win ) );
	    XClearArea( DPY, note->win, 0, 0, 0, 0, True);
	}
	else
	    /*
	      The window has only moved. We can be lazy about background
	      updates.
	    */
	    gettimeofday( &note->bgTimeout, NULL );
    }
}


void
processMapNotify( Note *note, XMapEvent *ev)
{
    /*
      In case the window manager decided to dump us somewhere we did not ask
      for, then move the window back over.
    */
    TRACE( 2, ( stderr, "processMapNotify( %p, ev )\n", note ) );

    int		x, y;
    unsigned	width, height, border_width, depth;
    Window	root;

    note->mapped = 1;
    setMappedWinAttrs( note );

    XGetGeometry( DPY, note->win, &root, &x, &y, &width, &height,
	    &border_width, &depth );

    correctNoteGeometry( note, x, y, width, height );
}


/*
  Window attributes that need to be set for mapped windows.
*/
void
setMappedWinAttrs( Note *note )
{
    if( !ISSET_FLAG( note->flags, flOverrideRedir ) )
    {
	ewmhMessage( DPY, XROOT, note->win,
		XInternAtom( DPY, "_NET_WM_STATE", True),
		_NET_WM_STATE_ADD,
		XInternAtom( DPY, ISSET_FLAG( note->flags, flOntop ) ?
			"_NET_WM_STATE_ABOVE" : "_NET_WM_STATE_BELOW", True),
		0, 0, 0 );

	ewmhMessage( DPY, XROOT, note->win,
		XInternAtom( DPY, "_NET_WM_STATE", True),
		_NET_WM_STATE_ADD,
		XInternAtom( DPY, "_NET_WM_STATE_STICKY", True),
		0, 0, 0 );

	ewmhMessage( DPY, XROOT, note->win,
		XInternAtom( DPY, "_NET_WM_STATE", True),
		_NET_WM_STATE_ADD,
		XInternAtom( DPY, "_NET_WM_STATE_SKIP_PAGER", True),
		XInternAtom( DPY, "_NET_WM_STATE_SKIP_TASKBAR", True),
		0,0);
    }
    else if( ISSET_FLAG( note->flags, flOntop ) )
	XRaiseWindow( DPY, note->win );
    else
	XLowerWindow( DPY, note->win );
}


/*
  Send a message to an EWMH compatible window manager.
*/
Status
ewmhMessage( Display *dpy, Window rootWin, Window clientWin, Atom msgAtom,
	long d0, long d1, long d2, long d3, long d4)
{

    XEvent event;

    if( msgAtom  == None ) return 1;

    event.xclient.type		= ClientMessage;
    event.xclient.serial	= 0;
    event.xclient.send_event	= True;
    event.xclient.message_type	= msgAtom;
    event.xclient.window	= clientWin;
    event.xclient.format	= 32;

    event.xclient.data.l[0]	= d0;
    event.xclient.data.l[1]	= d1;
    event.xclient.data.l[2]	= d2;
    event.xclient.data.l[3]	= d3;
    event.xclient.data.l[4]	= d4;

    return XSendEvent( dpy, rootWin, False,
			    SubstructureRedirectMask | SubstructureNotifyMask,
			    &event);
}
