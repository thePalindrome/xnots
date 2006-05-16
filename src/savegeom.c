/* -------------------------------------------------------------------------- *\

  Created	: Fri 14 Apr 2006 07:20:44 PM CDT
  Modified	: Sun 07 May 2006 01:20:28 PM CDT
  Author	: Gautam Iyer <gi1242@users.sourceforge.net>
  Licence	: GPL2

\* -------------------------------------------------------------------------- */

#include "xnots.h"

#define LDEBUG_LEVEL DEBUG_LEVEL

#if LDEBUG_LEVEL
# define	TRACE( d, x )						\
{									\
    if( d <= LDEBUG_LEVEL ) fprintf x ;					\
}
#else
# define	TRACE( d, x )
#endif

/*
  Save geometries we have in memory of all notes into the file .xnotsinfo.
*/
void
saveGeometryList()
{
    TRACE( 2, ( stderr, "saveGeometryList()\n" ) );

    FILE	*f;
    int		i;

    if( (f = fopen( ".xnotsinfo", "w" ) ) == NULL )
    {
	infoMessage( "Could not open .xnotsinfo for writing" );
	return;
    }

    for( i=0; i < GEOMLIST.nItems; i++ )
    {
	geom_item_t	*geom = GEOMLIST.items[i];
	char		xchar, ychar;
	int		xpos, ypos;

	if( (int) geom->width + geom->x == (int) DisplayWidth( DPY, SCREEN ) )
	{
	    xpos  = 0;
	    xchar = '-';
	}
	else
	{
	    xpos  = geom->x;
	    xchar = '+';
	}

	if( (int) geom->height + geom->y == (int) DisplayHeight( DPY, SCREEN ) )
	{
	    ypos  = 0;
	    ychar = '-';
	}
	else
	{
	    ypos  = geom->y;
	    ychar = '+';
	}


	TRACE( 9, ( stderr, "%s:%ux%u%c%d%c%d\n", geom->filename,
		geom->width, geom->height, xchar, xpos, ychar, ypos ) );
	fprintf( f, "%s:%ux%u%c%d%c%d\n", geom->filename,
		geom->width, geom->height, xchar, xpos, ychar, ypos );
    }

    fclose( f );

    /* Successfully saved file. Reset timeout */
    GEOMLIST.lastChange.tv_sec = 0;
}


/*
  Read geometries of notes from file .xnotsinfo.
*/
void
readGeometryList()
{
    TRACE( 2, ( stderr, "readGeometryList()\n" ) );

    const int	lineSize = NAME_MAX + 30;
    FILE	*f;
    char	line[ lineSize ];

    if( (f = fopen( ".xnotsinfo", "r" ) ) == NULL )
    {
	infoMessage( ".xnotsinfo not found or error opening" );
	return;
    }


    while( !feof(f) )
    {
	char		*geom;
	int		gMask;
	int		x, y;
	unsigned	width, height;

	geom_item_t	*item;

	if( fgets( line, lineSize, f) == NULL )
	    break;

	if( (geom = strrchr( line, ':' )) == NULL )
	    continue;
	else
	{
	    char	*s;

	    *(geom++) = '\0';
	    if( ( s = strchr( geom, '\n' ) ) != NULL )
		*s = '\0';
	}

	gMask = XParseGeometry( geom, &x, &y, &width, &height );
	if (
	      !(gMask & XValue) || !(gMask & YValue)
	      || !(gMask & WidthValue) || !(gMask & HeightValue)
	   )
	{
	    infoMessage( "Invalid geometry '%s' for note %s", geom, line );
	    continue;
	}

	item = (geom_item_t*) xnots_malloc( 1 * sizeof(geom_item_t) );

	item->filename	= strdup( line );
	item->x		= x;
	item->y		= y;
	item->width	= width;
	item->height	= height;

	if( gMask & XNegative )
	    item->x += DisplayWidth( DPY, SCREEN ) - item->width;
	if( gMask & YNegative )
	    item->y += DisplayHeight( DPY, SCREEN ) - item->height;

	addToList( (void***) &GEOMLIST.items, item, &GEOMLIST.nItems,
		&GEOMLIST.nItemsMax );

	TRACE( 9, ( stderr, "Read geometry %ux%u+%d+%d for file %s\n",
	    item->width, item->height, item->x, item->y, item->filename ) );
    }

    /*
      Immediately after reading the geometry list from file, we do not have to
      save it :).
    */
    GEOMLIST.lastChange.tv_sec = 0;

    fclose( f );
}


/*
  Update the information we have in memory about the geometry of the note
  with file "filename". If we have no information about "filename", then add it
  to our list.
*/
void
updateGeometryList( const char *filename, int x, int y,
	unsigned width, unsigned height)
{
    TRACE( 2, ( stderr, "updateGeometryList()\n" ) );

    int i;

    for( i=0; i < GEOMLIST.nItems; i++ )
    {
	geom_item_t	*geom = GEOMLIST.items[i];

	if( !strcmp( geom->filename, filename ) )
	{
	    if (
		  geom->x != x || geom->y != y
		  || geom->width != width || geom->height != height
	       )
	    {
		/* Add note geometry to our list. */
		geom->x		= x;
		geom->y		= y;
		geom->width	= width;
		geom->height	= height;

		/* Set a timeout so that the geometry list will be saved */
		gettimeofday( &GEOMLIST.lastChange, NULL);
	    }

	    return;
	}
    }

    /*
      If we got here, then we have not stored geometry for the current note. Add
      a new item to GEOMLIST for the current note.
    */
    addGeometryToList( filename, x, y, width, height);
}


void
addGeometryToList( const char *filename, int x, int y,
	unsigned width, unsigned height)
{
    TRACE( 2, ( stderr, "addGeometryToList()\n" ) );

    geom_item_t	*newgeom = (geom_item_t*) xnots_malloc( sizeof(geom_item_t) );

    newgeom->filename	= strdup( filename );

    newgeom->x		= x;
    newgeom->y		= y;
    newgeom->width	= width;
    newgeom->height	= height;

    addToList( (void***) &GEOMLIST.items, newgeom, &GEOMLIST.nItems,
	    &GEOMLIST.nItemsMax);

    /* Set a timeout so that the geometry list will be saved */
    gettimeofday( &GEOMLIST.lastChange, NULL);
}


/*
  Reads the list of stored geometries. If a stored geometry is found for the
  current note, then the note geometry is updated. Returns GL_CHANGED,
  GL_UNCHANGED or GL_NOT_FOUND.

  NOTE: When the note geometry is updated, only note->x, y, width, height are
  changed. Move / resize requests are not sent to the X server.
*/
int
getGeometryFromList( Note *note )
{
    TRACE( 2, ( stderr, "getGeometryFromList()\n" ) );

    int i;

    for( i=0; i < GEOMLIST.nItems; i++ )
    {
	geom_item_t	*geom = GEOMLIST.items[i];

	if( !strcmp( note->filename, geom->filename ) )
	{
	    /* Restore note geometry, and return true if changed */
	    if (
		  note->x != geom->x || note->y != geom->y
		  || note->width != geom->width || note->height != geom->height
	       )
	    {
		note->x		= geom->x;
		note->y		= geom->y;
		note->height	= geom->height;

		if( note->width != geom->width )
		{
		    note->width	= geom->width;
		    resetNoteWidth( note );
		}

		return GL_CHANGED;
	    }
	    else
		return GL_UNCHANGED;
	}
    }

    /*
      If we got here then our geometry list does not contain geometry info for
      this note.
    */
    return GL_NOT_FOUND;
}
