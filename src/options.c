/* -------------------------------------------------------------------------- *\

  Created	: Fri 14 Apr 2006 07:20:44 PM CDT
  Modified	: Wed 26 Mar 2008 11:54:32 AM PDT
  Author	: Gautam Iyer <gi1242@users.sourceforge.net>
  Licence	: GPL2

\* -------------------------------------------------------------------------- */

#include "xnots.h"
#include <ctype.h>

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

			 LOCAL STRUCTURES AND VARIABLES

\* -------------------------------------------------------------------------- */

/* Option data */
typedef struct {
    int		index;

    unsigned	boolean:1;

    /* char	*shortName; */
    char	*longName;
    /* char	*description; */
} OData;


#if 0
#define OPTION( ind, bl, sname, lname, desc )				\
    assert( n < oDataSize );						\
    userOption[n].boolean		= (bl);					\
    userOption[n].shortName	= (sname);				\
    userOption[n].longName		= (lname);				\
    userOption[n].description	= (desc);				\
    n++;
#endif

#define FLAG( bitNum, sname, lname, desc )				\
    { (bitNum), 1, (lname) }

#define OPTION( optNum, sname, lname, desc )				\
    { (optNum), 0, (lname) }


/*
  List of all options understood by xnots.
*/
OData userOption[] =
{
    OPTION( opDisplay,	   "dpy", "display",	 "X display to connect to" ),
    OPTION( opGeometry,	   "gm",  "geometry",	 "Geometry" ),

    OPTION( opForeground,  "fg",  "foreground",	 "Foreground color"),
    OPTION( opBackground,  "bg",  "background",	 "Background color"),
    OPTION( opAlpha,	   "a",	  "alpha",	 "Alpha level of note" ),

    FLAG( flOverrideRedir, "bwm", "bypassWM",	 "Bypass the window manager"),
    FLAG( flOntop,	   "top", "onTop",	 "Keep note on top" ),
    OPTION( opTitle,	   "t",	  "title",	 "Note window title" ),

    FLAG( flMarkup,	   "um",  "useMarkup",	 "Use pango markup in note" ),

    OPTION( opFont,	   "fn",  "font",	 "Font face to use" ),
    OPTION( opFontSize,	   "sz",  "size",	 "Font size to use" ),

    OPTION( opLMargin,	   "lm",  "leftMargin",	 "Left text margin" ),
    OPTION( opRMargin,	   "rm",  "rightMargin", "Right text margin" ),
    OPTION( opTMargin,	   "tm",  "topMargin",	 "Top text margin" ),
    OPTION( opBMargin,	   "bm",  "botMargin",	 "Bottom text margin" ),

    OPTION( opIndent,	   "i",	  "indent",	 "Paragraph indents" ),

    OPTION( opRoundRadius, "r",	  "roundRadius", "Radius of rect corners" )
};

#undef OPTION
#undef FLAG

const int	nUserOptions = sizeof( userOption ) / sizeof( OData );

/* -------------------------------------------------------------------------- *\

				LOCAL MACROS

\* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- *\

				   BEGIN CODE

\* -------------------------------------------------------------------------- */

/*
  Read options from file into options. Any trailing text is saved into the
  buffer text.

  Returns the number of chars read into the text buffer (excluding the
  terminating 0). The returned buffer is always null terminated.
*/
int
readOptionsFromFile(
	char		**options,
	unsigned long	*flags,
	unsigned long	*flagsMask,
	char		*text,
	size_t		size,
	const char	*filename)
{
    FILE	*f;
    char	buffer[MAX_LINE_SIZE];

    int		nbytes = 0;	/* Bytes read into text */

    *flags	= DEFAULT_FLAGS;
    *flagsMask	= 0UL;

    if( ( f = fopen( filename, "r" ) ) == NULL )
    {
	errorMessage( "Unable to read file %s", filename );
	*text = 0;
	return 0;
    }

    while( !feof(f) && fgets( buffer, MAX_LINE_SIZE, f) )
    {
	switch (*buffer)
	{
	    case '\n':		/* Blank line */
	    case '#':		/* Comment */
		TRACE( 9, ( stderr, "Ignoring blank / comment\n" ));
		continue;

	    case '*':		/* Option */
		processOption( options, flags, flagsMask, buffer, NULL);
		break;

	    default:		/* Part of text */
		nbytes	=  min( strlen( buffer ), size );

		strncpy( text, buffer, nbytes );
		/*
		  Options can only be read at the beginning of the note file. So
		  once we read one non-option, we need to exit the option loop.
		*/
		goto OptionEnd;
	}
    }

OptionEnd:
    /*
      We get here after processing all options and comments in the file. Now
      everything else is part of the note text.
    */
    TRACE( 9, ( stderr,
		"Done processing options. Reading remainder of file\n" ) );

    nbytes += fread( text + nbytes, sizeof(char), size - nbytes, f);

    /*
      Null terminate text, and don't include the terminator in the count of
      bytes returned. Also convert the last newline in the file to a 0
    */
    if( nbytes >= size )
	nbytes = size - 1;
    if( text[ nbytes-1 ] == '\n' )
	nbytes--;
    text[ nbytes ] = 0;

    fclose (f);

    return nbytes;
}


/*
  Process option found in opt. If arg is NULL, then the argument for the option
  (if required) will be after the first ':' in opt.

  Returns 1 if the parameter in arg was used.
*/
int
processOption( char **options,
	unsigned long *flags, unsigned long *flagsMask,
	char *optString, char *argString )
{
    char	*opt = optString,
		*arg = argString;

    int		i;

    if( arg == NULL && *opt == '*' )
    {
	/*
	  Reading option from file.
	*/
	char *p;

	while( *(++opt) && isspace( *opt ) );	/* Skip '*' & leading spaces */

	if( (p = strchr( opt, ':' )) != NULL )
	{
	    *p = '\0';				/* Terminate opt */
	    while( *(++p) && isspace( *p ) );	/* Skip ':' & foll. spaces */

	    /* This is where the argument starts */
	    arg = p;
	}

	rtrimSpaces( opt );
	rtrimSpaces( arg );
    }
    else if ( ( *opt == '-' || *opt == '+' ) && arg == NULL )
    {
	/*
	  If we're using a long option, then skip the leading -- or ++
	*/
	if( *opt == opt[1] )
	    opt += 2;
    }
    else
    {
	errorMessage( "Option string '%s' not recognized", opt );
	return 0;
    }

    /*
      Main loop doing required action for each option. Long options can be read
      from a config file, or command line. In either case the prefix "--" or "*"
      will already have been stripped. Short options are only read from command
      line, and the "-" prefix will not be stripped.
    */
    for( i=0; i < nUserOptions; i++ )
    {
	if(
#if 0 /* Command line short options will probably never be implemented */
	    (
	      /* Check for short option */
	      ( *opt == '-' || (userOption[i].boolean && *opt == '+') )
	      && !strcmp( opt, userOption[i].shortName )
	    ) ||
#endif
	    /* Check for long option */
	    !strcmp( opt, userOption[i].longName )
	  )
	{
	    int optNum = userOption[i].index;

	    if( userOption[i].boolean )
	    {
		if( *optString == '*' )
		{
		    /* arg will never be null when we get here */
		    if (
			    !strcasecmp( arg, "true" )
			    || !strcasecmp( arg, "yes" )
			    || !strcmp( arg, "1" )
		       )
		    {
			SET_FLAG( *flags, optNum );
		    }
		    else if (
			      !strcasecmp( arg, "false" )
			      || !strcasecmp( arg, "no" )
			      || !strcasecmp( arg, "0" )
			    )
		    {
			UNSET_FLAG( *flags, optNum );
		    }
		    else
		    {
			UNSET_FLAG( *flags, optNum );
			errorMessage( "Unrecognized argument (%s) to boolean"
				" option '%s'", arg, opt );
		    }
		}
		else
		{
		    if( *optString == '-' )
			SET_FLAG( *flags, optNum );
		    else
			UNSET_FLAG( *flags, optNum );
		}

		/* Indicate that we've altered the optNum'th bit */
		SET_FLAG( *flagsMask, optNum );
		/*
		  The argument argString will never be used when processing
		  boolean arguments.
		*/
		return 0;
	    }
	    else
	    {
		if( arg == NULL )
		{
		    errorMessage( "Option '%s' needs argument", opt );
		    return 0;
		}

		if( options[ optNum ] != NULL )
		{
		    /* Memory needs to be re-alloc'ed */
		    int len = strlen( arg ) + 1;

		    options[ optNum ] = (char *) xnots_realloc(
			    options[ optNum ], len * sizeof(char) );

		    strcpy( options[ optNum ], arg );
		}
		else
		    options[ optNum ] = strdup( arg );

		return 1;
	    }
	}
    }

    /*
      If we got here, then we encountered an unrecognized option.
    */
    errorMessage( "Unrecognized option '%s'", opt);
    return 0;
}


void
freeOptions( char **options )
{
    int i;

    for( i=0; i < NOPTIONS; i++ )
	if ( options[i] != NULL )
	{
	    free( options[i] );
	    options[i] = NULL;
	}
    ;
}


/*
  Trim off any white-spaces on the right of the string. The string is edited in
  place.
*/
void
rtrimSpaces( char *s )
{
    char *tail = rindex( s, '\0' ) - 1;

    while( tail > s && isspace( *tail ) )
	*(tail--) = '\0';
}
