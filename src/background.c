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

#include <X11/extensions/Xrender.h>


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

		       FUNCTION PROTOTYPES AND CONSTANTS

\* -------------------------------------------------------------------------- */

void	shadePmap	( Pixmap		pmap,
			  const XRenderColor	*bgColor,
			  unsigned short	alpha,
			  int			x,
			  int			y,
			  unsigned		width,
			  unsigned		height
			);

/* -------------------------------------------------------------------------- *\

				   BEGIN CODE

\* -------------------------------------------------------------------------- */
void
refreshRootBGVars()
{
    TRACE( 2, ( stderr, "refreshRootBGVars()\n" ) );

    unsigned long	nitems, bytes_after;
    Atom		atype;
    int			aformat;
    unsigned char	*prop = NULL;


    xnots.allowXErrors = 1;
    xnots.xErrorReturn = Success;

    /* Get new root pixmap ID in xnots.rootPixmap */
    if (
	  xnots.xaRootPmapID != None
	  && XGetWindowProperty (
				  DPY, XROOT, xnots.xaRootPmapID,
				  0L, 1L, False, XA_PIXMAP,
				  &atype, &aformat, &nitems,
				  &bytes_after, &prop
				)
		    == Success
	  && prop != NULL
       )
    {
	int		u_rootx, u_rooty; /* u_ variables are unused */
	unsigned	u_bw, u_depth;
	Window		u_cr;

	xnots.rootPixmap = *( (Pixmap*) prop );
	XFree( prop );

	/* Save the geometry of the root pixmap in xnots.rpWidth, rpHeight */
	XGetGeometry( DPY, xnots.rootPixmap, &u_cr, &u_rootx, &u_rooty,
		&xnots.rpWidth, &xnots.rpHeight, &u_bw, &u_depth);

	TRACE( 3, ( stderr, "Sucessfully grabbed pixmap %lx (%ux%u).\n",
		    xnots.rootPixmap, xnots.rpWidth, xnots.rpHeight ) );
    }
    else
	xnots.rootPixmap = None;

    xnots.allowXErrors = 0;
    if( xnots.xErrorReturn != Success )	/* Set by the error handler */
	xnots.rootPixmap = None;
}


/*
  Set's the notes background by grabbing the root background. If the background
  is changed, then return 1. Otherwise return 0.
*/
int
setTransparentBG( Note *note )
{
    TRACE( 2, ( stderr, "setTransparentBG( %p )\n", note ) );

    /*
      note->x, y, width, height must be correctly set before calling this
      function.
    */
    int		rWidth	= (int) DisplayWidth ( DPY, SCREEN ),
		rHeight	= (int) DisplayHeight( DPY, SCREEN );

    int		sx = note->x,		/* coordinates to grab root img */
		sy = note->y,
		nx = 0, ny = 0;		/* coordinates to put root image */

    unsigned	nw = note->width,	/* dims of root image to grab */
		nh = note->height;

    Pixmap	pmap;
    GC		gc;
    XGCValues	values;

    if ( sx + nw  <= 0 || sx >= rWidth || sy + nh <= 0 || sy >= rHeight )
	/* Note window is off screen */
	return 0;

    if (
	  note->bgGrabbed
	  && sx == note->prevPos.x && sy == note->prevPos.y
	  && nw == note->prevPos.width && nh == note->prevPos.height
       )
	/* Note background is up to date. */
	return 0;

    if ( xnots.rootPixmap == None || note->alpha == 0xff )
    {
	/* Opaque, or no root background. */
	if( !note->bgGrabbed )
	{
	    /* Only reset the background if bgGrabbed is false */
	    XSetWindowBackground( DPY, note->win, note->bg.pixel );
	    note->bgGrabbed = 1;

	    return 1;
	}
	else
	    return 0;
    }


    pmap = XCreatePixmap( DPY, note->win, note->width, note->height, DEPTH );
    if ( pmap == None )
    {
	/* Unable to create a pixmap (out of memory?) */
	note->bgGrabbed = 0;
	return 0;
    }

    /*
      Enable XErrors. No other returns from this function till the very end, as
      resources now need to be freed.
    */
    xnots.allowXErrors = 1;
    xnots.xErrorReturn = Success;


    values.graphics_exposures = False;
    gc = XCreateGC( DPY, note->win, GCGraphicsExposures, &values);

    if (sx < 0)
    {
	nw += sx;
	nx = -sx;
	sx = 0;
    }
    if (sy < 0)
    {
	nh += sy;
	ny = -sy;
	sy = 0;
    }

    nw = min( nw, (unsigned) (rWidth - sx) );
    nh = min( nh, (unsigned) (rHeight - sy) );


    if( xnots.rpWidth < rWidth || xnots.rpHeight < rHeight )
    {
	/* Tile the root background on the window. */

	values.ts_x_origin	= -sx + nx;
	values.ts_y_origin	= -sy + ny;
	values.fill_style	= FillTiled;
	values.tile		= xnots.rootPixmap;

	XChangeGC( DPY, gc,
		GCFillStyle | GCTileStipXOrigin | GCTileStipYOrigin | GCTile,
		&values);

	XFillRectangle( DPY, pmap, gc, nx, ny, nw, nh);
    }
    else
	XCopyArea( DPY, xnots.rootPixmap, pmap, gc, sx, sy, nw, nh, nx, ny);

    /* TODO Shade pmap */
    if( note->alpha > 0 )
	shadePmap( pmap, &note->bg.color, note->alpha, nx, ny, nw, nh );

    if ( xnots.xErrorReturn == Success )
    {
	TRACE( 3, ( stderr, "Sucessfully grabbed root pixmap at %ux%u+%d+%d\n",
		    note->width, note->height, note->x, note->y ) );

	note->bgGrabbed		= 1;
	note->prevPos.x		= note->x;
	note->prevPos.y		= note->y;
	note->prevPos.width	= note->width;
	note->prevPos.height	= note->height;

	XSetWindowBackgroundPixmap( DPY, note->win, pmap );
    }
    else
	note->bgGrabbed = 0;

    /* Free Pixmap and GC */
    XFreePixmap( DPY, pmap );
    XFreeGC( DPY, gc );

    /* Don't allow any more xerrors */
    xnots.allowXErrors = 0;

    return note->bgGrabbed;
}


void
shadePmap( Pixmap pmap, const XRenderColor *bgColor, unsigned short alpha,
	int x, int y, unsigned width, unsigned height )
{
    Picture			pic;
    XRenderPictFormat		*format;
    XRenderPictureAttributes	attrs;

    XRenderColor		color = *bgColor;

    format	= XRenderFindVisualFormat( DPY, VISUAL);
    pic		= XRenderCreatePicture( DPY, pmap, format, 0, &attrs);

    color.alpha	= (alpha << 8);
    TRACE( 5, ( stderr, "Shading root background to %4hx,%4hx,%4hx,%4hx\n",
		color.red, color.green, color.blue, color.alpha ) );

    XRenderFillRectangle( DPY, PictOpOver, pic, &color, x, y, width, height );

    XRenderFreePicture( DPY, pic );
}
