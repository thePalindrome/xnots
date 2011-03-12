/*
 * Created  : Sun 09 Jan 2011 11:51:30 AM EST
 * Modified : Tue 11 Jan 2011 10:33:05 PM EST
 * Author   : GI <gi1242+js@nospam.com> (replace nospam with gmail)
 *
 * Creates modal popups on click.
 *
 * Simplest usage: Use rel='modalPopup.open' for an anchor, and put the modal
 * popup dialogue in the next element. E.g.
 *
 *  <a rel='modalPopup.open'>Open Popup</a>
 *  <div>Popup text goes here</div>
 *
 * Alternately, one can choose the ID of the element to put in the popup:
 *
 *  <a rel='modalPopup.open.myId'>Open Popup</a>
 *  ...
 *  <div id='myId'>Popup text goes here</div>
 *
 * To include a close button in your popup dialogue, use an anchor with
 * rel='modalPopup.close'. If you don't provide one, it will be added as the
 * first element. The class attribute of this element will be set to the value
 * of the class attribute of the popup, followed by the string
 * 'modalPopupClose'. E.g.:
 *
 *  <div class='myClass'>Popup text.</div>
 *
 * and
 *
 *  <div class='myClass'><a rel='modalPopup.close' class='myClassModalPopupClose'>(Close)</a>Popup text.</div>
 *
 * will generate equivalent output.
 *	
 */

var oldKeyHandler;

function showModalPopup( id )
{
    var e = document.getElementById( id ).cloneNode(true);

    // Translucent div that covers the screen (for dimming outside)
    var t = document.createElement( 'div' );
    t.style.position = 'fixed';
    t.style.left = '0px';
    t.style.top = '0px';
    t.style.width = '100%';
    t.style.height = '100%';
    t.style.backgroundColor = '#000000';
    t.style.opacity = .5;
    t.style.zIndex = 100;
    t.display = 'block'
    t.id = 'modalPopupTranslucentScreen';

    // Invisible (opaque) div that covers the screen.
    var i = document.createElement( 'div' );
    i.style.position = 'fixed';
    i.style.left = '0px';
    i.style.top = '0px';
    i.style.width = '100%';
    i.style.height = '100%';
    i.style.zIndex = 101;
    i.display = 'block'
    i.id = 'modalPopupInvisibleScreen';

    // Add the popup's div, and center it.
    document.body.appendChild( t );
    document.body.appendChild( i );
    i.appendChild( e );
    e.style.display = 'block';
    e.style.position = 'absolute';
    e.style.marginTop = (-.5 * e.offsetHeight).toString() + "px";
    e.style.marginLeft = (-.5 * e.offsetWidth).toString() + "px";
    e.style.top = '50%';
    e.style.left = '50%';

    e.style.maxWidth = e.offsetWidth.toString() + 'px';
    e.style.maxHeight = e.offsetHeight.toString() + 'px';
}

function closeModalPopup()
{
    var t = document.getElementById( 'modalPopupTranslucentScreen' );
    var i = document.getElementById( 'modalPopupInvisibleScreen' );
    document.body.removeChild(t);
    document.body.removeChild(i);
}

function hasCloseTag(d)
{
    var e;

    for( e=d.firstElementChild; e != null; e = e.nextElementSibling )
    {
	if( e.nodeName == 'A' && e.rel == 'modalPopup.close' )
	    return true; // Close element exists
	else if ( hasCloseTag(e) )
	    return true;
    }
    return false;
}

// Add a close tag if needed. d must be an element.
function addCloseTag( d )
{
    // <a href='#' onclick="closeModalPopup(); return false;">(Close)</a>
    var aClose = document.createElement("a");
    aClose.className = d.className + 'ModalPopupClose';
    aClose.href = '#';
    aClose.setAttribute( 'onclick', 'closeModalPopup(); return false;' );
    aClose.setAttribute( 'rel', 'modalPopup.close' );
    aClose.innerHTML = '(Close)';

    // Insert show link before d, and close link inside d.
    d.insertBefore( aClose, d.firstChild);
}

function makeIntoPopup( id )
{
    var d, e, s;
    
    if( (d = document.getElementById( id )) == null )
	throw ( "No element with id " + id.toString() );

    if( ! hasCloseTag(d) )
	addCloseTag( d );

    // Set a few display styles
    d.style.display = 'none'; // So that element will be initially hidden
}


function initModalPopupRefs()
{
    var i, anchors;
    anchors=document.getElementsByTagName('a');

    // Modify anchors with rel='modalPopup.xx' or rel='modalPopup'
    for (i=0; i < anchors.length; i++)
    {
	var a=anchors[i];

	if( a.rel == 'modalPopup.close' )
	{
	    if( a.href == '' )
		a.href = '#';
	    a.setAttribute( 'onclick', 'closeModalPopup(); return false;' );
	}
	else
	{
	    var m = a.rel.match( /modalPopup\.open(?:\.(.*))?/ );

	    if( m )
	    {
		// Set the onclick attribute to show a modal popup
		var divId;

		if( a.href == '' ) a.href = '#';

		if( m[1] )
		    // rel="modalPopup.open.X". Use ID=X.
		    divId = m[1];
		else
		{
		    // Use the next element as the popup
		    var d = a.nextElementSibling;
		    if( d == null )
			// Try using parent Node
			d = a.parentNode.nextElementSibling;
		    if( d == null )
		    {
			// Give up.
			alert( "Anchor #" + i + ", has no next element." );
			continue;
		    }
		    else
		    {
			if( d.id == '' )
			    d.id = 'modalPopupAutoID' + i.toString();
			divId = d.id;
		    }
		}
		a.setAttribute( 'onclick',
			"showModalPopup( '" + divId + "'); " +
			"return false;" );

		makeIntoPopup( divId );
	    } // if (m)
	} // if( a.rel != 'modalPopup.close' )
    } // for( anchors )
}

// window.onload = initTogglers;
var oldonload = window.onload;
if( typeof oldonload == 'function' )
{
    window.onload = function() { oldonload(); initModalPopupRefs(); };
}
else
{
    window.onload = initModalPopupRefs;
}
