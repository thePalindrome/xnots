/* -------------------------------------------------------------------------- *\

  Created	: Fri 14 Apr 2006 07:20:44 PM CDT
  Modified	: Sun 11 Jan 2009 11:42:47 AM PST
  Author	: Gautam Iyer <gi1242@users.sourceforge.net>
  Licence	: GPL2

\* -------------------------------------------------------------------------- */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/time.h>

#include <X11/Xmd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>


#include <pango/pangoxft.h>

/* -------------------------------------------------------------------------- *\

			       MACROS AND OPTIONS

\* -------------------------------------------------------------------------- */

#define APL_NAME	"xnots"
#define APL_VERSION	"0.1"

#define	MAX_LINE_SIZE	(1024)
#define MAX_NOTE_SIZE	(1024)

/* Return values for getGeometryFromList() */
#define GL_CHANGED	(0)
#define GL_UNCHANGED	(1)
#define GL_NOT_FOUND	(2)

/*
  Option index.
*/
enum {
    opDisplay = 0,
    opGeometry,
    opForeground,
    opBackground,
    opAlpha,
    opTitle,
    opFont,
    opFontSize,
    opLMargin,
    opRMargin,
    opTMargin,
    opBMargin,
    opIndent,
    opRoundRadius,
    NOPTIONS
};

/*
  Boolean option index. (Is the bit number of the option).
*/
enum {
    flMarkup = 0,
    flOverrideRedir,
    flOntop,
    flMoveOnScreenResize,
    NFLAGS
};

#define DEFAULT_FLAGS	( (1ul << flMarkup)|(1ul<<flMoveOnScreenResize) )


/* -------------------------------------------------------------------------- *\

			     STRUCTURE DEFINITIONS

\* -------------------------------------------------------------------------- */

/*
  We really should include openmotif-X.X/Xm/MwmUtil.h, but that will add
  openmotif as a dependency. We dont' really need it, except to set WM hints,
  thus this ugly hack.
*/
typedef struct _MWMHints {
	int	flags;
	int	functions;
	int	decorations;
	int	input_mode;
	int	status;
} MWMHints;

typedef struct _Note {
    /*
      When changes are made to this structure, the functions copyNote and
      initNoteOptions must also be changed accordingly to initialize / copy
      these options. If new resources are allocated they must be free'ed in
      freeNote.
    */
    unsigned long	flags;		/* Boolean options for this note */

    int			x,		/* geometry of the note window */
			y;
    unsigned		width,
			height;

    XftColor		bg,		/* Background color of the note */
			fg;		/* Foreground color of the note */

    unsigned		mapped:1;	/* Wether the window is mapped or not */

    unsigned short	alpha;		/* Alpha degree of the note */
    XRectangle		prevPos;	/* Previous position where root
					   background was grabbed */
    unsigned		bgGrabbed:1;	/* Wether root background has been
					   successfully grabbed or not */

    unsigned short	radius;		/* Radius of rounded corners */

    unsigned		lMargin,	/* margins of note text */
			rMargin,
			tMargin,
			bMargin;	/* If bMargin = 0, then note geometry is
					   honored. Otherwise, the note is
					   resized to maintain bMargin */

    /*
      Transparency timeouts, used so that select will not wait for ever incase
      we need a background update.
    */
    struct timeval	bgTimeout;	/* When the timeout was created */

    /*
      Following resources should be duplicated with care (or not duplicated at
      all), and must be free'ed when the note is destroyed.
    */
    char		*filename;	/* Filename containing note data */
    char		*winTitle;	/* Title of the note window */

    PangoLayout		*playout;	/* Text of the note in pango's format */

    Window		win;		/* Window ID of the note window */
    XftDraw		*draw;		/* XftDrawable of the note */
} Note;

/*
  Items in the refresh file list (of notes who's files have been modified, and
  need to be refreshed on screen).
*/
typedef struct _rlistitem_t {
    char	*filename;
    uint32_t	mask;
} rlistitem_t;

/*
  List of notes that need to be refreshed.
*/
typedef struct _refresh_t {
    rlistitem_t		**files;
    int			nFiles,
			nFilesMax;
    struct timeval	lastChange;
} refresh_t;

/*
  Items in the note geometry list. Current note window geometries are stored in
  this list and saved on exit, so that the note positions can be restored on
  future startups.
*/
typedef struct _geom_item_t {
    char		*filename;
    int			x, y;
    unsigned		width, height;
} geom_item_t;

typedef struct _geom_list_t {
    geom_item_t		**items;
    int			nItems,
			nItemsMax;
    struct timeval	lastChange;
} geom_list_t;

typedef struct _xnots_t {
    Note		**notes,	/* Array of all available note ptrs */
			defaultNote;	/* Default options for notes */

    int			nNotes,		/* Number of notes we're showing */
			nNotesMax;	/* Size of the list notes */

    refresh_t		refreshList;	/* List of files who's corresponding
					   notes need to be refreshed. */

    unsigned		verbose:1;	/* Wether info messages should be
					   displayed or not */

    /* Standard X resources common to all notes */
    Display		*dpy;
    PangoContext	*pcontext;
    Visual		*visual;

    Colormap		cmap;
    int			screen;
    unsigned		depth;

    /* X atoms used for various purposes */
    Atom		xaDelete,	/* WM_DELETE_WINDOW */
			xaRootPmapID,	/* _XROOTPMAP_ID */
			xaSetRootID;	/* _XSETROOT_ID */

    /* Error handling variables */
    unsigned		allowXErrors:1;	/* Wether we should complain on xErrors
					   or not */
    unsigned char	xErrorReturn;	/* return code of last XError */


    /* Transparency stuff */
    Pixmap		rootPixmap;	/* Pixmap ID of the root pixmap */
    unsigned		rpWidth,	/* Dimensions of root pixmap */
			rpHeight;

    unsigned		terminate:1;	/* When set, kill all notes and exit */

    geom_list_t		geomList;	/* List of all note geometries stored in
					   memory. */
} xnots_t;

/*
  All essential information about the X connection and notes that we're
  currently displaying can be accessible through the global variable 'xnots'.
*/
extern	xnots_t xnots;

/* -------------------------------------------------------------------------- *\

			      FUNCTION PROTOTYPES

\* -------------------------------------------------------------------------- */

/* ------------------------------ background.c ------------------------------ */
void	refreshRootBGVars	();
int	setTransparentBG	( Note *note );

/* --------------------------------- list.c --------------------------------- */
void		addToList		( void		***listp,
					  void		*itemp,
					  int		*nItems,
					  int		*listSize
					);
int		delFromList		( void ***listp,
					  void *itemp,
			     		  int *nItems,
			     		  int *listSize
					);
void		delItemNumFromList	( void ***listp,
					  int itemNum,
				    	  int *nItems,
				    	  int *listSize
					);

/* --------------------------------- main.c --------------------------------- */
void    	infoMessage		( const char* fmt, ... )
					__attribute__((
					    format( printf, 1, 2 )
					));
void    	errorMessage		( const char* fmt, ... )
					__attribute__((
					    format( printf, 1, 2 )
					));
void		fatalError		( const char *fmt, ... )
					__attribute__((
					    noreturn, format( printf, 1, 2 )
					));
void*		xnots_realloc		( void *ptr, size_t size );
void*		xnots_malloc		( size_t size );
void		setMappedWinAttrs	( Note *note );
Status		ewmhMessage		( Display	*dpy,
					  Window	rootWin,
					  Window	clientWin,
					  Atom		msgAtom,
					  long		d0,
					  long		d1,
					  long		d2,
					  long		d3,
					  long		d4
					);

/* --------------------------------- notes.c -------------------------------- */
void		initXnots		( char		**options,
					  unsigned long flags,
					  char		*text
					);
void		cleanExit		();
void		initNoteOptions		( Note		*note,
					  char		**options,
				 	  unsigned long	flags,
				 	  unsigned long	flagsMask,
				 	  char		*text,
				 	  Bool		reset
					);
void		copyNote		( Note *dst, Note *src );
void		addNote			( Note *note );
void		delNote			( Note *note );
void		delNoteNum		( int i );
Note*		createNote		( Note *template,
					  char *text,
					  const char *filename
					);
void		correctNoteGeometry	( Note		*note,
					  int		x,
					  int		y,
					  unsigned	width,
					  unsigned	height
					);
void		correctNoteHeight	( Note *note );
inline void	resetNoteWidth		( Note *note );
void		freeNote		( Note *note );
void		showNotes		();
void		setNoteText		( Note *note, char *text);
void		reloadNote		( int num );
void		shapeNoteWindow		( Note *note );

/* ------------------------------- options.c -------------------------------- */
int		readOptionsFromFile	( char		**options,
					  unsigned long	*flags,
					  unsigned long	*flagsMask,
					  char		*text,
					  size_t	size,
					  const char	*filename
					);
int		processOption		( char		**options,
					  unsigned long *flags,
			       		  unsigned long *flagsMask,
			       		  char		*optString,
			       		  char		*argString
					);
void		freeOptions		( char **options );
void		rtrimSpaces		( char *s );

/* ------------------------------- savegeom.c ------------------------------- */
void		saveGeometryList	();
void		readGeometryList	();
void		updateGeometryList	( const char	*filename,
					  int		x,
					  int		y,
					  unsigned	width,
					  unsigned	height
					);
void		addGeometryToList	( const char	*filename,
					  int		x,
					  int		y,
					  unsigned	width,
					  unsigned	height
					);
int		getGeometryFromList	( Note *note );

/* -------------------------------------------------------------------------- *\

			GLOBAL MACROS / INLINE FUNCTIONS

\* -------------------------------------------------------------------------- */

#define TTRACE( d, x ) fprintf x

#include <assert.h>


/* Motif window hints, MwmHints.flags */
#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)
/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)
/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)
/* bit definitions for MwmHints.inputMode */
#define MWM_INPUT_MODELESS                  0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL              2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3
#define PROP_MWM_HINTS_ELEMENTS             5

/* Ewmh states */
#define _NET_WM_STATE_REMOVE	(0)
#define _NET_WM_STATE_ADD	(1)
#define _NET_WM_STATE_TOGGLE	(2)


/* Macros to manipulate boolean flags */ 
#define ISSET_FLAG( flags, opt )					\
    ( flags & ( 1LU << opt ) )

#define SET_FLAG( flags, opt )						\
    flags |= (1LU << opt )

#define UNSET_FLAG( flags, opt )					\
    flags &= ~(1LU << opt )

#define TOGGLE_FLAG( flags, opt )					\
    flags ^= (1LU << opt )

/*
  Convenience macros
*/

#define DPY		(xnots.dpy)
#define VISUAL		(xnots.visual)
#define CMAP		(xnots.cmap)
#define SCREEN		(xnots.screen)
#define DEPTH		(xnots.depth)

#define XROOT		(DefaultRootWindow(DPY))

#define NOTES		(xnots.notes)

#define RLIST		(xnots.refreshList)
#define GEOMLIST	(xnots.geomList)

/*
  Max and min using the gcc typeof extensions.
  
  Put these last because the vim syntax files flags them as errors, and
  highlights everything else following as preproc (ugh).
*/
#define max( a, b )							\
({									\
    typeof(a)	_a = (a);                                               \
    typeof(b)	_b = (b);                                               \
                                                                        \
    _a > _b ? _a : _b;                                                  \
})

#define min( a, b )							\
({									\
    typeof(a)	_a = (a);                                               \
    typeof(b)	_b = (b);                                               \
                                                                        \
    _a < _b ? _a : _b;                                                  \
})
