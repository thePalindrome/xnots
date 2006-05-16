/* -------------------------------------------------------------------------- *\

  Created	: Fri 14 Apr 2006 07:20:44 PM CDT
  Modified	: Sat 06 May 2006 07:49:04 PM CDT
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


/* -------------------------------------------------------------------------- *\

				LOCAL CONSTANTS

\* -------------------------------------------------------------------------- */

/*
  When increasing / decreasing the size of a list of pointers, request these
  many more items.
*/
const int 	listSizeInc = 8;


/* -------------------------------------------------------------------------- *\

				   BEGIN CODE

\* -------------------------------------------------------------------------- */


/*
  Add pointer itemp to the list *listp. The list pointed to by listp should be a
  list of POINTERS. The size of listp is increased or decreased as needed. If
  listSize is 0, then listp is malloc'ed.
*/
void
addToList( void ***listp, void *itemp, int *nItems, int *listSize )
{
    if( *nItems == *listSize )
    {
	*listSize += listSizeInc;
	*listp = (void **) xnots_realloc( *listp, *listSize * sizeof(void*));
    }

    (*listp)[ (*nItems)++ ] = itemp;
}


int
delFromList( void ***listp, void *itemp, int *nItems, int *listSize)
{
    int i;

    for( i=0; i < *nItems; i++ )
	if( (*listp)[i] == itemp )
	{
	    delItemNumFromList( listp, i, nItems, listSize);
	    return 1;
	}
    ;

    TRACE( 1, ( stderr, "Warning: Item not found on list" ) );
    return 0;
}


/*
  Removes the itemNum'th item from the list of pointers *listp. Memory occupied
  by the item must be freed (if necessary) before calling this function.
*/
void
delItemNumFromList( void ***listp, int itemNum, int *nItems, int *listSize)
{
    TRACE( 2, ( stderr, "delItemNumFromList()\n"));

    int		j;

    /* Sanity check */
    assert( itemNum >= 0 && itemNum < *nItems );

    /*
      Move remaining notes up in our list of notes, and shorten our list of
      notes.
    */
    for( j=itemNum; j < *nItems; j++ )
	(*listp)[j] = (*listp)[j+1];

    (*nItems)--;

    /*
      See if we can reduce the size of our note list.
    */
    if (
	  *nItems <= *listSize - listSizeInc
	  && *nItems >= 0
       )
    {
	*listSize -= listSizeInc;

	if( *listSize > 0 )
	{
	    *listp = (void**) xnots_realloc( *listp,
				    *listSize * sizeof(void*) );
	}
	else
	{
	    free( *listp );
	    *listp = NULL;
	}
    }
}
