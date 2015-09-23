/* -------------------------------------------------------------------------- *\

  Author	: gi1242+xNots@NoSpam.com (replace "NoSpam" with "gmail")
  Created	: Fri 14 Apr 2006 07:20:44 PM CDT

  Copyright 2006, 2008 Gautam Iyer

  This file is part of xNots.

  xNots is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  xNots is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with xNots.  If not, see <http://www.gnu.org/licenses/>.

\* -------------------------------------------------------------------------- */

#include "xnots.h"


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
