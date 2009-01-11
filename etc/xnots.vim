" Created	: Wed 26 Apr 2006 01:20:53 AM CDT
" Modified	: Sun 11 Jan 2009 12:03:30 PM PST
" Author	: Gautam Iyer <gi1242@users.sourceforge.net>
" Description	: Vim syntax file for xnots sticky notes

" Quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

syn case match
syn spell toplevel
syn sync fromstart

" Note header
syn region	xnotsHeader	contains=xnotsComment,xnotsOptions keepend
				\ start='\%^' end='^[^*#]'me=s-1

" Comments
syn match	xnotsComment	contained contains=@Spell '^#.*'

" Errors
syn match	xnotsError	contained '\v\S+'

" Note options start in the header
syn match	xnotsOptions	contained transparent skipwhite nextgroup=xnotsBoolKwds,xnotsNumKwds,xnotsColorKwds,xnotsStrKwds,xnotsError '^\*'

" Boolean options
syn keyword	xnotsBoolKwds	contained transparent skipwhite
				\ nextgroup=xnotsBoolColon,xnotsError
				\ bypassWM onTop useMarkup moveOnScreenResize
syn match	xnotsBoolColon	contained transparent skipwhite
				\ nextgroup=xnotsBoolVal,xnotsError
				\ ':'
syn keyword	xnotsBoolVal	contained skipwhite nextgroup=xnotsError
				\ 0 1 yes no true false

" Number options
syn keyword	xnotsNumKwds	contained transparent skipwhite
				\ nextgroup=xnotsNumColon,xnotsError
				\ alpha size leftMargin rightMargin topMargin
				\ botMargin indent roundRadius
syn match	xnotsNumColon	contained transparent skipwhite
				\ nextgroup=xnotsNumVal,xnotsError
				\ ':'
syn match	xnotsNumVal	contained '\v[+-]?(0[0-7]+|\d+|0x[0-9a-fA-F]+)$'

" Color options
syn keyword	xnotsColorKwds	contained transparent skipwhite
				\ nextgroup=xnotsColorColon,xnotsError
				\ background foreground
syn match	xnotsColorColon	contained transparent skipwhite
				\ nextgroup=xnotsColorVal
				\ ':'
syn match	xnotsColorVal	contained '\v#[0-9a-fA-F]{6}\s*$'

" String options
syn keyword	xnotsStrKwds	contained transparent skipwhite
				\ nextgroup=xnotsStrColon,xnotsError
				\ display geometry font title
syn match	xnotsStrColon	contained transparent skipwhite
				\ nextgroup=xnotsStrVal
				\ ':'
syn match	xnotsStrVal	contained '\v.+$'


"
" Note text. Simple pango markup is supported
"

" Unmatched close tags
syn match	xnotsTagErr	'\v\<.{-}\>'

" Flag incorrectly matched tags
syn region	xnotsTagRegion	transparent
				\ contains=xnotsTagRegion,xnotsSpecial,@Spell
				\ matchgroup=xnotsTag
				\ start='\v\<\s*\z(\w+).{-}\>'
				\ matchgroup=xnotsTagErr end='\v\<\s*/.{-}\>'
				\ matchgroup=ErrorMsg	 end='\_.\%$'
				\ matchgroup=xnotsTag	 end='\v\<\s*/\z1\s*\>'

" Special symbols by name.
syn match	xnotsSpecial	'&\w\+;'

"
" Highlighting groups
"
hi def link xnotsHeader		Statement
hi def link xnotsComment	Comment

hi def link xnotsBoolVal	Boolean
hi def link xnotsStrVal		String
hi def link xnotsColorVal	Constant
hi def link xnotsNumVal		Number

hi def link xnotsError		Error
hi def link xnotsTagErr		xnotsError

hi def link xnotsTag		Identifier

hi def link xnotsSpecial	Special


let b:current_syntax = 'xnots'
