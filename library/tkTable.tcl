# table.tcl --
#
# Version align with tkTable 2.12, jeff at hobbs org
# This file defines the default bindings for Tk table widgets
# and provides procedures that help in implementing those bindings.
#

#--------------------------------------------------------------------------
# ::tktable::Priv elements used in this file:
#
# x && y -		Coords in widget
# afterId -		Token returned by "after" for autoscanning.
# tablePrev -		The last element to be selected or deselected
#			during a selection operation.
# mouseMoved -		Boolean to indicate whether mouse moved while
#			the button was pressed.
# borderInfo -		Boolean to know if the user clicked on a border
# borderB1 -		Boolean that set whether B1 can be used for the
#			interactiving resizing
# bindingMode -		Should B1 focus or activate a cell
#--------------------------------------------------------------------------

# Ensure that a namespace is created for us
namespace eval ::tktable {
    variable Priv
    array set Priv [list x 0 y 0 afterId {} mouseMoved 0 \
	    borderInfo {} borderB1 1 bindingMode focus]
    namespace export tk_tableCopy tk_tableCut tk_tablePaste
}


#--------------------------------------------------------------------------
# TkTable bindings
#--------------------------------------------------------------------------

#
# Bindings for the "copy", "cut", and "paste" clipboard functions
#
bind Table <<Copy>>		{ ::tktable::tk_tableCopy %W }
bind Table <<Cut>>		{ ::tktable::tk_tableCut %W }
bind Table <<Paste>>		{ ::tktable::tk_tablePaste %W }

##
## Interactive cell resizing, affected by -resizeborders option
## You might want to check for cell returned if you want to
## restrict the resizing of certain cells
##
bind Table <3>			{ %W border mark %x %y }
bind Table <B3-Motion>		{ %W border dragto %x %y }

#
# Button select events
#
bind Table <1>			{ ::tktable::Button1 %W %x %y }
bind Table <B1-Motion>		{ ::tktable::B1Motion %W %x %y }
bind Table <ButtonRelease-1>	{ ::tktable::B1Release %W %x %y }
bind Table <Double-Button-1>	{ ::tktable::DoubleButton1 %W %x %y }

bind Table <Shift-1>		{ ::tktable::BeginExtend %W [%W index @%x,%y] }
bind Table <<ToggleSelection>>	{ ::tktable::BeginToggle %W [%W index @%x,%y] }
bind Table <B1-Enter>		{ ::tktable::CancelRepeat}
bind Table <B1-Leave>		{
    if {$::tktable::Priv(borderInfo) eq ""} {
	array set ::tktable::Priv {x %x y %y}
	::tktable::AutoScan %W
    }
}

#
# Button scan events
#
bind Table <2> {
    %W scan mark %x %y
    array set ::tktable::Priv {x %x y %y}
    set ::tktable::Priv(mouseMoved) 0
}
bind Table <B2-Motion> {
    if {(%x != $::tktable::Priv(x)) || (%y != $::tktable::Priv(y))} {
	set ::tktable::Priv(mouseMoved) 1
    }
    if {$::tktable::Priv(mouseMoved)} { %W scan dragto %x %y }
}
bind Table <ButtonRelease-2> {
    if {!$::tktable::Priv(mouseMoved)} { ::tktable::tk_tablePaste %W [%W index @%x,%y] }
}
# On Windows, paste is done using Shift-Insert.  Shift-Insert already
# generates the <<Paste>> event, so we don't need to do anything here.
if {[tk windowingsystem] ne "win32"} {
    bind Table <Insert> {
	catch {::tktable::tk_tablePaste %W active}
    }
}


#
# Mouse wheel events
#
if {[package vcompare [package present Tk] 8.7] >= 0} {
    bind Table <MouseWheel>		{ tk::MouseWheel %W y %D -40.0 }
    bind Table <Option-MouseWheel>	{ tk::MouseWheel %W y %D -12.0 }
    bind Table <Shift-MouseWheel>	{ tk::MouseWheel %W x %D -40.0 }
    bind Table <Shift-Option-MouseWheel> { tk::MouseWheel %W x %D -12.0 }

} else {
    if {[tk windowingsystem] eq "aqua"} {
	bind Table <MouseWheel>		{ %W yview scroll [expr {-%D}] units }
	bind Table <Shift-MouseWheel>	{ %W xview scroll [expr {-%D}] units }
	bind Table <Option-MouseWheel>	{ %W yview scroll [expr {-10 * %D}] units }
	bind Table <Shift-Option-MouseWheel> { %W xview scroll [expr {-10 * %D}] units }
    } else {
	bind Table <MouseWheel>		{ %W yview scroll [expr {-%D / 120}] units }
	bind Table <Shift-MouseWheel>	{ %W xview scroll [expr {-%D / 120}] units }
    }

    if {[tk windowingsystem] eq "x11"} {
	bind Table <Button-4>		{ %W yview scroll -5 units }
	bind Table <Shift-Button-4>	{ %W xview scroll -5 units }
	bind Table <Button-5>		{ %W yview scroll  5 units }
	bind Table <Shift-Button-5>	{ %W xview scroll  5 units }
    }
}

#
# Touchpad scrolling
#
if {[package vcompare [package present Tk] 8.7] >= 0} {
bind Table <TouchpadScroll> {
    if {%# %% 5 != 0} {
        return
    }
    lassign [tk::PreciseScrollDeltas %D] tk::Priv(deltaX) tk::Priv(deltaY)
    if {$tk::Priv(deltaX) != 0} {
 	%W xview scroll [expr {-$tk::Priv(deltaX)}] units
    }
    if {$tk::Priv(deltaY) != 0} {
	%W yview scroll [expr {-$tk::Priv(deltaY)}] units
    }
}
}

#
# Key events
#

# This forces a cell commit if an active cell exists
bind Table <<Table_Commit>> {
    catch {%W activate active}
}
# Remove this if you don't want cell commit to occur on every Leave for
# the table (via mouse) or FocusOut (loss of focus by table).
event add <<Table_Commit>> <Leave> <FocusOut>

# Left/Right arrow key bindings (none, shift, control, control+shift)
bind Table <<PrevChar>>		{ ::tktable::KeyNav %W left }
bind Table <<NextChar>>		{ ::tktable::KeyNav %W right }
bind Table <<SelectPrevChar>>	{ ::tktable::SelectionExtend %W left }
bind Table <<SelectNextChar>>	{ ::tktable::SelectionExtend %W right }
bind Table <<PrevWord>>		{ ::tktable::KeyNav %W pageLeft }
bind Table <<NextWord>>		{ ::tktable::KeyNav %W pageRight }
bind Table <<SelectPrevWord>>	{ ::tktable::SelectionExtend %W pageLeft }
bind Table <<SelectNextWord>>	{ ::tktable::SelectionExtend %W pageRight }

# Up/down arrow key bindings (none, shift, control, control+shift)
bind Table <<PrevLine>>		{ ::tktable::KeyNav %W up }
bind Table <<NextLine>>		{ ::tktable::KeyNav %W down }
bind Table <<SelectPrevLine>>	{ ::tktable::SelectionExtend %W up }
bind Table <<SelectNextLine>>	{ ::tktable::SelectionExtend %W down }
bind Table <<PrevPara>>		{ ::tktable::KeyNav %W pageTop }
bind Table <<NextPara>>		{ ::tktable::KeyNav %W pageBottom }
bind Table <<SelectPrevPara>>	{ ::tktable::SelectionExtend %W pageTop }
bind Table <<SelectNextPara>>	{ ::tktable::SelectionExtend %W pageBottom }

# Home/End key bindings (none, shift, control, control+shift)
bind Table <<LineStart>>	{ ::tktable::KeyNav %W first }
bind Table <<LineEnd>>		{ ::tktable::KeyNav %W last }
bind Table <<SelectLineStart>>	{ ::tktable::SelectionExtend %W first }
bind Table <<SelectLineEnd>>	{ ::tktable::SelectionExtend %W last }
bind Table <Control-Home>	{ ::tktable::KeyNav %W top }
bind Table <Control-End>	{ ::tktable::KeyNav %W bottom }
bind Table <Control-Shift-Home>	{ ::tktable::SelectionExtend %W top }
bind Table <Control-Shift-End>	{ ::tktable::SelectionExtend %W bottom }
#bind Table <Control-Shift-Home> { ::tktable::DataExtend %W origin }
#bind Table <Control-Shift-End>	{ ::tktable::DataExtend %W end }

# Page Up/Down key bindings (none, shift, control, control+shift, alternate)
bind Table <Prior>		{ ::tktable::PageNav %W up }
bind Table <Next>		{ ::tktable::PageNav %W down }
bind Table <Shift-Prior>	{ ::tktable::PageNav %W left }
bind Table <Shift-Next>		{ ::tktable::PageNav %W right }
bind Table <Control-Prior>	{ ::tktable::ScreenNav %W up }
bind Table <Control-Next>	{ ::tktable::ScreenNav %W down }
bind Table <Control-Shift-Prior> { ::tktable::ScreenNav %W left }
bind Table <Control-Shift-Next> { ::tktable::ScreenNav %W right }
bind Table <Alt-Prior>		{ ::tktable::PageNav %W left }
bind Table <Alt-Next>		{ ::tktable::PageNav %W right }

# Scroll Lock bindings
bind Table <Mod3-Up>		{ %W yview scroll -1 units }
bind Table <Mod3-Down>		{ %W yview scroll 1 units }
bind Table <Mod3-Left>		{ %W xview scroll -1 units }
bind Table <Mod3-Right>		{ %W xview scroll 1 units }
bind Table <Mod3-Prior>		{ %W yview scroll -1 pages}
bind Table <Mod3-Next>		{ %W yview scroll 1 pages}
bind Table <Mod3-Control-Prior>	{ %W yview moveto 0.0 }
bind Table <Mod3-Control-Next>	{ %W yview moveto 1.0 }
bind Table <Mod3-Home>		{ %W xview scroll -1 pages }
bind Table <Mod3-End>		{ %W xview scroll 1 pages }
bind Table <Mod3-Control-Home>	{ %W xview moveto 0.0 }
bind Table <Mod3-Control-End>	{ %W xview moveto 1.0 }

# Other key navigation
bind Table <Return>		{ ::tktable::MoveCell %W 0 1 0 }
bind Table <Shift-Return>	{ ::tktable::MoveCell %W 0 -1 0 }
bind Table <Alt-Return>		{ ::tktable::Insert %W "\n" }
bind Table <Control-Return>	{ ::tktable::ToggleFocus %W }
bind Table <Tab>		{ ::tktable::MoveCell %W 1 0 0; break }
bind Table <Shift-Tab>		{ ::tktable::MoveCell %W -1 0 end; break }
bind Table <Alt-Tab>		{ ::tktable::Insert %W "\t" }
bind Table <Control-Tab>	[bind all <<NextWindow>>]
bind Table <Control-Shift-Tab>	[bind all <<PrevWindow>>]

# Other selection functions
bind Table <<Clear>>		{ ::tktable::SelectClear %W }
bind Table <<SelectAll>>	{ ::tktable::SelectAll %W }
bind Table <<SelectNone>>	{ ::tktable::SelectNone %W }
bind Table <Shift-space>	{ ::tktable::SelectRow %W }
bind Table <Control-space>	{ ::tktable::SelectColumn %W }
bind Table <Select>		{ ::tktable::BeginSelect %W [%W index active] }
bind Table <Shift-Select>	{ ::tktable::BeginExtend %W [%W index active] }

# Cell edit functions
bind Table <KeyPress>		{ ::tktable::Insert %W %A }
bind Table <BackSpace>		{ ::tktable::BackSpace %W }
bind Table <Delete>		{ ::tktable::Delete %W }
bind Table <Escape>		{ %W reread }
bind Table <Control-e>		{ %W icursor end }
bind Table <Control-a>		{ %W icursor 0 }
bind Table <Control-k>		{ %W delete active insert end }

# Column resize functions
bind Table <Control-equal>	{ ::tktable::ChangeWidth %W active  1 }
bind Table <Control-minus>	{ ::tktable::ChangeWidth %W active -1 }

# Ignore all Alt, Meta, and Control key presses unless explicitly bound.
# Otherwise, if a widget binding for one of these is defined, the
# <KeyPress> class binding will also fire and insert the character,
# which is wrong.  Ditto for Tab.

bind Table <Alt-KeyPress>	{# nothing}
bind Table <Meta-KeyPress>	{# nothing}
bind Table <Control-KeyPress>	{# nothing}
if {$::tcl_platform(platform) eq "macintosh"} {
    bind Table <Command-KeyPress> {# nothing}
}

#
# ::tktable::MoveHandler
#
# This performs cell edit or navigation function
#
# Arguments:
# w -	The widget for which the operation will be applied.
# op -	Operation to perform
# edit - Check cell edit first or not
# extend - If part of an extend operation
#
# Results:
# Returns current cell, new cell, current icursor position, and new position
#
proc ::tktable::MoveHandler {w op edit extend} {
    set orig -1
    set pos -1

    if {![catch {$w index active} cell] && $edit &&
	$op ni [list bottom end origin pageBottom pageLeft pageRight pageTop top]} {
	# Cell edit mode
	lassign [split $cell ","] row col
	set pos [$w icursor]
	set orig $pos

	switch $op {
	    "first" - "origin" - "top" {
		# Move to first char
		set pos 0
	    }
	    "last" - "end" - "bottom" {
		# Move to last char
		set pos "end"
	    }
	    "left" {
		# Move left 1 char or cell
		if {$pos > 0} {
		    incr pos -1
		} else {
		    incr col -1
		    set pos end
		}
	    }
	    "right" {
		# Move right 1 char or cell
		incr pos
		set len [string length [$w curvalue]]
		if {$pos > $len || $len == 0} {
		    incr col
		    set pos 0
		}
	    }
	    "up" {
		# Move up 1 cell, keep same position
		incr row -1
		#set pos end
		#set orig -1
	    }
	    "down" {
		# Move down 1 cell, keep same position
		incr row
		#set pos 0
		#set orig -1
	    }
	    default {
		return
	    }
	}

    } elseif {![catch {$w index focus} cell]} {
	# Cell nav mode. Abort for sel extend in wrong mode
	if {$extend && [$w cget -selectmode] ni [list "multiple" "extended"]} {
	    return
	}

	lassign [split $cell ","] row col
	switch $op {
	    "left" {
		# Move left 1 cell
		incr col -1
		set pos end
	    }
	   "right" {
		# Move right 1 cell
		incr col
		set pos 0
	    }
	    "up" {
		# Move up 1 cell
		incr row -1
	    }
	    "down" {
		# Move down 1 cell
		incr row
	    }
	    "first" {
		# Move to first cell in row
		set col [$w index origin col]
	    }
	    "last" {
		# Move to last cell in row
		set col [$w index end col]
	    }
	    "origin" - "top" {
		# Move to origin cell
		lassign [split [$w index origin] ","] row col
	    }
	    "end" - "bottom" {
		# Move to end cell
		lassign [split [$w index end] ","] row col
	    }
	    "pageLeft" {
		# Move to first visible cell in row
		set col [$w index topleft col]
	    }
	    "pageRight" {
		# Move to last visible cell in row
		set col [$w index bottomright col]
	    }
	    "pageTop" {
		# Move to top visible cell in column
		set row [$w index topleft row]
	    }
	    "pageBottom" {
		# Move to bottom visible cell in column
		set row [$w index bottomright row]
	    }
	    default {
		return
	    }
	}

    } else {
	return
    }
    return [list $cell $row $col $orig $pos]
}

#
# ::tktable::KeyNav
#
#   This performs cell edit or navigation function
#
# Arguments:
# w -	The widget for which the operation will be applied.
# op -	Operation to perform
#
# Results:
# Returns nothing
#
proc ::tktable::KeyNav {w op} {
    set res [::tktable::MoveHandler $w $op 1 0]
    if {[llength $res] == 0} return
    lassign $res cell row col orig pos

    # Move to new cell and/or icursor position
    lassign [split $cell ","] r c
    if {$r != $row || $c != $col} {
	::tktable::MoveCell $w [expr {$col - $c}] [expr {$row - $r}] $pos
    } elseif {$pos != $orig} {
	$w icursor $pos
    }
}

#
# ::tktable::SelectionExtend
#
# Extends selection to include new cell
#
# Arguments:
# w -	The widget for which the operation will be applied.
# op -	Operation to perform
#
# Results:
# Returns nothing
#
proc ::tktable::SelectionExtend {w op} {
    set res [::tktable::MoveHandler $w $op 0 0]
    if {[llength $res] == 0} return
    lassign $res cell row col orig pos

    # Extend selection to new cell
    lassign [split $cell ","] r c
    if {$r != $row || $c != $col} {
	::tktable::ExtendSelect $w [expr {$col - $c}] [expr {$row - $r}]
	#::tktable::DataExtend $w origin|end
    }

    # Extend selection to new index position
    if {$pos != $orig} {
	# Not supported
    }
}

#
# ::tktable::PageNav
#
# Move by 1 page (screen full)
#
# Arguments:
# w -	The widget for which the operation will be applied.
# op -	Operation to perform
#
# Results:
# Returns nothing
#
proc ::tktable::PageNav {w op} {
    if {![catch {$w index active} cell]} {
	set fn activate
    } elseif {![catch {$w index focus} cell]} {
	set fn focus
    } else {
	return
    }

    lassign [$w bbox $cell] x y width height
    switch $op {
	"left" {
	    $w xview scroll -1 pages
	}
	"right" {
	    $w xview scroll 1 pages
	}
	"up" {
	    $w yview scroll -1 pages
	}
	"down" {
	    $w yview scroll 1 pages
	}
	default {
	    return
	}
    }
    $w $fn [format "@%d,%d" [expr {$x+($width/2)}] [expr {$y+($height/2)}]]
}

#
# ::tktable::ScreenNav
#
# Move to edge of visible part of table
#
# Arguments:
# w -	The widget for which the operation will be applied.
# op -	Operation to perform
#
# Results:
# Returns nothing
#
proc ::tktable::ScreenNav {w op} {
    if {![catch {$w index active} cell]} {
	set fn activate
    } elseif {![catch {$w index focus} cell]} {
	set fn focus
    } else {
	return
    }

    lassign [split $cell ","] r c
    set row $r
    set col $c

    switch $op {
	"left" {
	    set c [$w index topleft col]
	    if {$c == $col} {
		$w xview scroll -1 pages
		set c [$w index topleft col]
	    }
	}
	"right" {
	    set c [$w index bottomright col]
	    if {$c == $col} {
		$w xview scroll 1 pages
		set c [$w index bottomright col]
	    }
	}
	"up" {
	    set r [$w index topleft row]
	    if {$r == $row} {
		$w yview scroll -1 pages
		set r [$w index topleft row]
	    }
	}
	"down" {
	    set r [$w index bottomright row]
	    if {$r == $row} {
		$w yview scroll 1 pages
		set r [$w index bottomright row]
	    }
	}
	default {
	    return
	}
    }

    # Do move if cell changed
    if {$c != $col || $r != $row} {
	$w $fn [format "%d,%d" $r $c]
    }
}

#
# ::tktable::ToggleFocus
#
# Toggle between cell edit and navigation modes
#
# Arguments:
# w -	The widget for which the selection will be retrieved.
#
# Results:
# Returns nothing
#
proc ::tktable::ToggleFocus {w} {
    if {![catch {$w index active} cell]} {
	$w activate {}
	$w focus $cell
    } elseif {![catch {$w index focus} cell]} {
	$w activate $cell
    }
}

#
# ::tktable::GetSelection --
#
# This tries to obtain the default selection.  On Unix, we first try
# and get a UTF8_STRING, a type supported by modern Unix apps for
# passing Unicode data safely.  We fall back on the default STRING
# type otherwise.  On Windows, only the STRING type is necessary.
#
# Arguments:
# w -	The widget for which the selection will be retrieved.
# sel -	The source of the selection (PRIMARY or CLIPBOARD)
#
# Results:
# Returns the selection, or an error if none could be found
#
if {$::tcl_platform(platform) ne "unix"} {
    proc ::tktable::GetSelection {w {sel PRIMARY}} {
	if {[catch {selection get -displayof $w -selection $sel} txt]} {
	    return -code error "could not find default selection"
	} else {
	    return $txt
	}
    }
} else {
    proc ::tktable::GetSelection {w {sel PRIMARY}} {
	if {[catch {selection get -displayof $w -selection $sel \
		-type UTF8_STRING} txt] \
		&& [catch {selection get -displayof $w -selection $sel} txt]} {
	    return -code error "could not find default selection"
	} else {
	    return $txt
	}
    }
}

#
# ::tktable::CancelRepeat --
#
# A copy of tkCancelRepeat, just in case it's not available or changes.
# This procedure is invoked to cancel an auto-repeat action described
# by ::tktable::Priv(afterId).  It's used by several widgets to auto-scroll
# the widget when the mouse is dragged out of the widget with a
# button pressed.
#
# Arguments:
# None.
#
proc ::tktable::CancelRepeat {} {
    variable Priv
    after cancel $Priv(afterId)
    set Priv(afterId) {}
}

#
# ::tktable::Insert --
#
# Insert into the active cell
#
# Arguments:
# w -	the table widget
# s -	the string to insert
#
# Results:
# Returns nothing
#
proc ::tktable::Insert {w s} {
    if {$s ne {}} {
	$w insert active insert $s
    }
}

#
# ::tktable::BackSpace --
#
# BackSpace (delete char before insert) in the current cell
#
# Arguments:
# w -	the table widget
#
# Results:
# Returns nothing
#
proc ::tktable::BackSpace {w} {
    set cur [$w icursor]
    if {$cur > 0} {
	$w delete active [incr cur -1]
    }
}

#
# ::tktable::Delete --
#
# Delete char after insert in the current cell
#
# Arguments:
# w -	the table widget
#
# Results:
# Returns nothing
#
proc ::tktable::Delete {w} {
    $w delete active insert
}

#
# ::tktable::Button1 --
#
# This procedure is called to handle selecting with mouse button 1.
# It will distinguish whether to start selection or mark a border.
#
# Arguments:
# w -	the table widget
# x -	x coord
# y -	y coord
#
# Results:
# Returns nothing
#
proc ::tktable::Button1 {w x y} {
    variable Priv
    #
    # $Priv(borderInfo) is null if the user did not click on a border
    #
    if {$Priv(borderB1) == 1} {
	set Priv(borderInfo) [$w border mark $x $y]
	# account for what resizeborders are set [Bug 876320] (ferenc)
	set rbd [$w cget -resizeborders]
	if {$rbd eq "none" || ![llength $Priv(borderInfo)]
	    || ($rbd eq "col" && [lindex $Priv(borderInfo) 1] eq "")
	    || ($rbd eq "row" && [lindex $Priv(borderInfo) 0] eq "")} {
	    set Priv(borderInfo) ""
	}
    } else {
	set Priv(borderInfo) ""
    }
    if {$Priv(borderInfo) eq ""} {
	#
	# Only do this when a border wasn't selected
	#
	if {[winfo exists $w]} {
	    ::tktable::BeginSelect $w [$w index @$x,$y]
	    focus $w
	}
	set fn [expr {[$w cget -mode] eq "table" ? "activate" : "focus"}]
	array set Priv [list x $x y $y mouseMoved 0 bindingMode $fn]
    }
}

#
# ::tktable::B1Motion --
#
# This procedure is called to start processing mouse motion events while
# button 1 moves while pressed.  It will distinguish whether to change
# the selection or move a border.
#
# Arguments:
# w -	the table widget
# x -	x coord
# y -	y coord
#
# Results:
# Returns nothing
#
proc ::tktable::B1Motion {w x y} {
    variable Priv

    # If we already had motion, or we moved more than 1 pixel,
    # then we start the Motion routine
    if {$Priv(borderInfo) ne ""} {
	#
	# If the motion is on a border, drag it and skip the rest
	# of this binding.
	#
	$w border dragto $x $y
    } else {
	#
	# If we already had motion, or we moved more than 1 pixel,
	# then we start the Motion routine
	#
	if {
	    $::tktable::Priv(mouseMoved)
	    || abs($x-$::tktable::Priv(x)) > 1
	    || abs($y-$::tktable::Priv(y)) > 1
	} {
	    set ::tktable::Priv(mouseMoved) 1
	}
	if {$::tktable::Priv(mouseMoved)} {
	    ::tktable::Motion $w [$w index @$x,$y]
	}
    }
}

#
# ::tktable::B1Release --
#
# This procedure when B1 is released to activate cell.
#
# Arguments:
# w -	the table widget
# x -	x coord
# y -	y coord
#
# Results:
# Returns nothing
#
proc ::tktable::B1Release {w x y} {
    if {$::tktable::Priv(borderInfo) eq "" && [winfo exists $w]} {
	::tktable::CancelRepeat
	set fn $::tktable::Priv(bindingMode)
	if {$fn eq "focus"} {
	    $w activate {}
	}
	$w $fn [format "@%d,%d" $x $y]
    }
}

#
# ::tktable::DoubleButton1 --
#
# This procedure is used to activate a cell
#
# Arguments:
#   w	- the table widget
#   x	- x coord
#   y	- y coord
#
# Results:
# Returns nothing
#
proc ::tktable::DoubleButton1 {w x y} {
    if {$::tktable::Priv(borderInfo) eq "" && [winfo exists $w]} {
	set ::tktable::Priv(bindingMode) activate
    }
}

#
# ::tktable::BeginSelect --
#
# This procedure is typically invoked on button-1 presses. It begins
# the process of making a selection in the table. Its exact behavior
# depends on the selection mode currently in effect for the table;
# see the Motif documentation for details.
#
# Arguments:
# w	- The table widget.
# el	- The element for the selection operation (typically the
#	one under the pointer).  Must be in row,col form.
#
proc ::tktable::BeginSelect {w el} {
    variable Priv
    if {[scan $el %d,%d r c] != 2} return
    switch [$w cget -selectmode] {
	multiple {
	    if {[$w tag includes title $el]} {
		## in the title area
		if {$r < [$w cget -titlerows]+[$w cget -roworigin]} {
		    ## We're in a column header
		    if {$c < [$w cget -titlecols]+[$w cget -colorigin]} {
			## We're in the topleft title area
			set inc topleft
			set el2 end
		    } else {
			set inc [$w index topleft row],$c
			set el2 [$w index end row],$c
		    }
		} else {
		    ## We're in a row header
		    set inc $r,[$w index topleft col]
		    set el2 $r,[$w index end col]
		}
	    } else {
		set inc $el
		set el2 $el
	    }
	    if {[$w selection includes $inc]} {
		$w selection clear $el $el2
	    } else {
		$w selection set $el $el2
	    }
	}
	extended {
	    $w selection clear all
	    if {[$w tag includes title $el]} {
		if {$r < [$w cget -titlerows]+[$w cget -roworigin]} {
		    ## We're in a column header
		    if {$c < [$w cget -titlecols]+[$w cget -colorigin]} {
			## We're in the topleft title area
			$w selection set $el end
		    } else {
			$w selection set $el [$w index end row],$c
		    }
		} else {
		    ## We're in a row header
		    $w selection set $el $r,[$w index end col]
		}
	    } else {
		$w selection set $el
	    }
	    $w selection anchor $el
	    set Priv(tablePrev) $el
	}
	default {
	    if {![$w tag includes title $el]} {
		$w selection clear all
		$w selection set $el
		set Priv(tablePrev) $el
	    }
	    $w selection anchor $el
	}
    }
}

#
# ::tktable::Motion --
#
# This procedure is called to process mouse motion events while
# button 1 is down. It may move or extend the selection, depending
# on the table's selection mode.
#
# Arguments:
# w	- The table widget.
# el	- The element under the pointer (must be in row,col form).
#
proc ::tktable::Motion {w el} {
    variable Priv
    if {![info exists Priv(tablePrev)]} {
	set Priv(tablePrev) $el
	return
    }
    if {$Priv(tablePrev) eq $el} return
    switch [$w cget -selectmode] {
	browse {
	    $w selection clear all
	    $w selection set $el
	    set Priv(tablePrev) $el
	}
	extended {
	    # avoid tables that have no anchor index yet.
	    if {[catch {$w index anchor}]} { return }
	    scan $Priv(tablePrev) %d,%d r c
	    scan $el %d,%d elr elc
	    if {[$w tag includes title $el]} {
		if {$r < [$w cget -titlerows]+[$w cget -roworigin]} {
		    ## We're in a column header
		    if {$c < [$w cget -titlecols]+[$w cget -colorigin]} {
			## We're in the topleft title area
			$w selection clear anchor end
		    } else {
			$w selection clear anchor [$w index end row],$c
		    }
		    $w selection set anchor [$w index end row],$elc
		} else {
		    ## We're in a row header
		    $w selection clear anchor $r,[$w index end col]
		    $w selection set anchor $elr,[$w index end col]
		}
	    } else {
		$w selection clear anchor $Priv(tablePrev)
		$w selection set anchor $el
	    }
	    set Priv(tablePrev) $el
	}
    }
}

#
# ::tktable::BeginExtend --
#
# This procedure is typically invoked on shift-button-1 presses. It
# begins the process of extending a selection in the table. Its
# exact behavior depends on the selection mode currently in effect
# for the table; see the Motif documentation for details.
#
# Arguments:
# w	- The table widget.
# el	- The element for the selection operation (typically the
#	one under the pointer). Must be in numerical form.
#
proc ::tktable::BeginExtend {w el} {
    # avoid tables that have no anchor index yet.
    if {[catch {$w index anchor}]} { return }
    if {[$w cget -selectmode] eq "extended" && [$w selection includes anchor]} {
	::tktable::Motion $w $el
    }
}

#
# ::tktable::BeginToggle --
#
# This procedure is typically invoked on control-button-1 presses. It
# begins the process of toggling a selection in the table. Its
# exact behavior depends on the selection mode currently in effect
# for the table; see the Motif documentation for details.
#
# Arguments:
# w -	The table widget.
# el -	The element for the selection operation (typically the
#	one under the pointer). Must be in numerical form.
#
proc ::tktable::BeginToggle {w el} {
    if {[$w cget -selectmode] eq "extended"} {
	variable Priv
	set Priv(tablePrev) $el
	$w selection anchor $el
	if {[$w tag includes title $el]} {
	    scan $el %d,%d r c
	    if {$r < [$w cget -titlerows]+[$w cget -roworigin]} {
		## We're in a column header
		if {$c < [$w cget -titlecols]+[$w cget -colorigin]} {
		    ## We're in the topleft title area
		    set end end
		} else {
		    set end [$w index end row],$c
		}
	    } else {
		## We're in a row header
		set end $r,[$w index end col]
	    }
	} else {
	    ## We're in a non-title cell
	    set end $el
	}
	if {[$w selection includes  $end]} {
	    $w selection clear $el $end
	} else {
	    $w selection set   $el $end
        }
    }
}

#
# ::tktable::AutoScan --
# This procedure is invoked when the mouse leaves an table window
# with button 1 down. It scrolls the window up, down, left, or
# right, depending on where the mouse left the window, and reschedules
# itself as an "after" command so that the window continues to scroll until
# the mouse moves back into the window or the mouse button is released.
#
# Arguments:
# w -	The table window.
#
proc ::tktable::AutoScan {w} {
    if {![winfo exists $w]} return
    variable Priv
    set x $Priv(x)
    set y $Priv(y)
    if {$y >= [winfo height $w]} {
	$w yview scroll 1 units
    } elseif {$y < 0} {
	$w yview scroll -1 units
    } elseif {$x >= [winfo width $w]} {
	$w xview scroll 1 units
    } elseif {$x < 0} {
	$w xview scroll -1 units
    } else {
	return
    }
    ::tktable::Motion $w [$w index @$x,$y]
    set Priv(afterId) [after 50 [list ::tktable::AutoScan $w]]
}

#
# ::tktable::MoveCell --
#
# Moves the location cursor (active element) by the specified number
# of cells and changes the selection if we're in browse or extended
# selection mode.  If the new cell is "hidden", we skip to the next
# visible cell if possible, otherwise just abort.
#
# Arguments:
# w -	The table widget.
# x -	+1 to move right one cell, -1 to move left one cell.
# y -	+1 to move down one cell, -1 to move up one cell.
#	index = New icursor index
#
proc ::tktable::MoveCell {w x y {index {}}} {
    if {![catch {$w index active} cell]} {
	set fn activate
    } elseif {![catch {$w index focus} cell]} {
	set fn focus
    } else {
	return
    }

    set orig $cell
    lassign [split $cell ","] r c
    lassign [split [$w index origin] ","] r0 c0
    lassign [split [$w index end] ","] r1 c1
    incr c $x
    if {$c < $c0} {set c $c0} elseif {$c > $c1} {set c $c1}
    incr r $y
    if {$r < $r0} {set r $r0} elseif {$r > $r1} {set r $r1}

    set cell [format "%d,%d" $r $c]
    set new [$w hidden $cell]
    if {$new ne "" && $new ne $cell} {
	set cell $new
    }
    # We couldn't find a non-hidden cell, just don't move
    if {$cell eq $orig} {
	return
    }

    # Move to new cell
    $w $fn $cell
    $w see $cell
    switch [$w cget -selectmode] {
	browse {
	    $w selection clear all
	    $w selection set $cell
	}
	extended {
	    variable Priv
	    $w selection clear all
	    $w selection set $cell
	    $w selection anchor $cell
	    set Priv(tablePrev) $cell
	}
    }

    # Move icursor index
    if {$index ne ""} {
	$w icursor $index
    }
}

#
# ::tktable::ExtendSelect --
#
# Does nothing unless we're in extended selection mode; in this
# case it moves the location cursor (active element) by the specified
# number of cells, and extends the selection to that point.
#
# Arguments:
# w -	The table widget.
# x -	+1 to move right one cell, -1 to move left one cell.
# y -	+1 to move down one cell, -1 to move up one cell.
#
proc ::tktable::ExtendSelect {w x y} {
    if {[$w cget -selectmode] ni [list "extended" "multiple"]} {
	return
    }
    if {![catch {$w index active} cell]} {
	set fn activate
	set id active
    } elseif {![catch {$w index focus} cell]} {
	set fn focus
	set id focus
    } else {
	return
    }

    lassign [split $cell ","] r c
    $w $fn [format "%d,%d" [incr r $y] [incr c $x]]
    if {[$w bbox $id] eq ""} {
	$w xview scroll $x units
	$w yview scroll $y units
    }
    $w see $id
    $w selection set $id
    ::tktable::Motion $w [$w index $id]
}

#
# ::tktable::DataExtend
#
# This procedure is called for key-presses such as Shift-KEndData.
# If the selection mode isn't multiple or extend then it does nothing.
# Otherwise it moves the active element to el and, if we're in
# extended mode, extends the selection to that point.
#
# Arguments:
# w -	The table widget.
# el -	An integer cell number.
#
proc ::tktable::DataExtend {w el} {
    if {![catch {$w index active} cell]} {
	set fn activate
    } elseif {![catch {$w index focus} cell]} {
	set fn focus
    } else {
	return
    }

    set mode [$w cget -selectmode]
    if {$mode eq "extended"} {
	$w $fn $el
	$w see $el
	if {[$w selection includes anchor]} {
	    ::tktable::Motion $w $el
	}
    } elseif {$mode eq "multiple"} {
	$w $fn $el
	$w see $el
    }
}

#
# ::tktable::SelectAll
#
# This procedure is invoked to handle the "select all" operation.
# For single and browse mode, it just selects the active element.
# Otherwise it selects everything in the widget.
#
# Arguments:
# w -	The table widget.
#
proc ::tktable::SelectAll {w} {
    if {[$w cget -selectmode] in [list single browse]} {
	$w selection clear all
	if {[catch {$w selection set active}]} {
	    catch {$w selection set focus}
	}
    } elseif {[$w cget -selecttitle]} {
	$w selection set [$w cget -roworigin],[$w cget -colorigin] end
    } else {
	$w selection set origin end
    }
}

#
# ::tktable::SelectNone
#
# This procedure is invoked to handle the "unselect all" operation.
# For single and browse mode, it just selects the active element.
# Otherwise it unselects everything in the widget.
#
# Arguments:
# w -	The table widget.
#
proc ::tktable::SelectNone {w} {
    if {[$w cget -selectmode] ne "browse"} {
	$w selection clear all
    }
}

#
# ::tktable::SelectClear
#
# Clear (delete without shift) the selected cells.
#
# Arguments:
# w -	The table widget.
#
proc ::tktable::SelectClear {w} {
    $w curselection {}
}

#
# ::tktable::SelectRow
#
# Used to select row with current focus cell
#
# Arguments:
# w -	The table widget.
#
proc ::tktable::SelectRow {w} {
    if {[catch {$w index active} cell]} {
	if {[catch {$w index focus} cell]} {
	    return
	}
    }

    if {[$w cget -selectmode] in [list single browse]} {
	$w selection clear all
	$w selection set $cell
    } else {
	lassign [split $cell ","] row col
	set col1 [$w index origin col]
	set col2 [$w index end col]
	$w selection set [format "%d,%d" $row $col1] [format "%d,%d" $row $col2]
    }
}

#
# ::tktable::SelectColumn
#
# Used to select column with current focus cell
#
# Arguments:
# w -	The table widget.
#
proc ::tktable::SelectColumn {w} {
    if {[catch {$w index active} cell]} {
	if {[catch {$w index focus} cell]} {
	    return
	}
    }

    if {[$w cget -selectmode] in [list single browse]} {
	$w selection clear all
	$w selection set $cell
    } else {
	lassign [split $cell ","] row col
	set row1 [$w index origin row]
	set row2 [$w index end row]
	$w selection set [format "%d,%d" $row1 $col] [format "%d,%d" $row2 $col]
    }
}

#
# ::tktable::ChangeWidth --
#
# Adjust the widget of the specified cell by $a.
#
# Arguments:
# w -	The table widget.
# i -	cell index
# a -	amount to adjust by
#
proc ::tktable::ChangeWidth {w i a} {
    set tmp [$w index $i col]
    if {[set width [$w width $tmp]] >= 0} {
	$w width $tmp [incr width $a]
    } else {
	$w width $tmp [incr width [expr {-$a}]]
    }
}

#
# tk_tableCopy --
#
# This procedure copies the selection from a table widget into the
# clipboard.
#
# Arguments:
# w -		Name of a table widget.
#
proc ::tktable::tk_tableCopy w {
    if {[selection own -displayof $w] eq $w} {
	clipboard clear -displayof $w
	catch {clipboard append -displayof $w [::tktable::GetSelection $w]}
    }
}

#
# tk_tableCut --
#
# This procedure copies the selection from a table widget into the
# clipboard, then deletes the selection (if it exists in the given
# widget).
#
# Arguments:
# w -		Name of a table widget.
#
proc ::tktable::tk_tableCut w {
    if {[selection own -displayof $w] eq $w} {
	clipboard clear -displayof $w
	catch {
	    clipboard append -displayof $w [::tktable::GetSelection $w]
	    $w curselection {}
	    $w selection clear all
	}
    }
}

#
# tk_tablePaste --
#
# This procedure pastes the contents of the clipboard to the specified
# cell (active by default) in a table widget.
#
# Arguments:
# w -		Name of a table widget.
# cell -	Cell to start pasting in.
#
proc ::tktable::tk_tablePaste {w {cell {}}} {
    if {$cell ne {}} {
	if {[catch {::tktable::GetSelection $w} data]} return
    } else {
	if {[catch {::tktable::GetSelection $w CLIPBOARD} data]} {
	    return
	}
	set cell active
    }
    ::tktable::tk_tablePasteHandler $w [$w index $cell] $data
    if {[$w cget -state] eq "normal"} {focus $w}
}

#
# tk_tablePasteHandler --
#
# This procedure handles how data is pasted into the table widget.
# This handles data in the default table selection form.
#
# NOTE: this allows pasting into all cells except title cells,
#       even those with -state disabled
#
# Arguments:
# w -		Name of a table widget.
# cell -	Cell to start pasting in.
#
proc ::tktable::tk_tablePasteHandler {w cell data} {
    #
    # Don't allow pasting into the title cells
    #
    if {[$w tag includes title $cell]} {
        return
    }

    set rows	[expr {[$w cget -rows]-[$w cget -roworigin]}]
    set cols	[expr {[$w cget -cols]-[$w cget -colorigin]}]
    set r	[$w index $cell row]
    set c	[$w index $cell col]
    set rsep	[$w cget -rowseparator]
    set csep	[$w cget -colseparator]
    set vcmd	[$w cget -validatecommand]

    ## Assume separate rows are split by row separator if specified
    ## If you were to want multi-character row separators, you would need:
    # regsub -all $rsep $data <newline> data
    # set data [join $data <newline>]
    if {$rsep ne {}} {
	set data [split $data $rsep]
    }
    set row	$r
    foreach line $data {
	if {$row > $rows} break
	set col	$c
	## Assume separate cols are split by col separator if specified
	## Unless a -separator was specified
	if {$csep ne {}} {
	    set line [split $line $csep]
	}
	## If you were to want multi-character col separators, you would need:
	# regsub -all $csep $line <newline> line
	# set line [join $line <newline>]
	foreach item $line {
	    if {$col > $cols} break
	    set oldTxt [$w get $row,$col]
	    if {$item ne $oldTxt && $vcmd ne ""} {
		set vldCmd [string map [list %W $w %r $row %c $col %s $oldTxt %S $item] $vcmd]
		if {![eval $vldCmd]} {
		    incr col
		    continue ;# skip update
		}
	    }
	    $w set $row,$col $item
	    incr col
	}
	incr row
    }
}

#
# Backwards compatibility
#
namespace import tktable::tk_*

#
# tktable::Sort --
#
# This procedure handles how data is sorted in the table widget.
# This isn't currently used by tktable, but can be called by the user.
# It's behavior may change in the future.
#
# Arguments:
# w -		Name of a table widget.
# start -	start cell of rectangle to sort
# end -		end cell of rectangle to sort
# col -		column within rectangle to sort on
# args -	passed to lsort proc (ie: -integer -decreasing)
#
proc ::tktable::Sort {w start end col args} {
    set start [$w index $start]
    set end   [$w index $end]
    scan $start %d,%d sr sc
    scan $end   %d,%d er ec
    if {($col < $sc) || ($col > $ec)} {
	return -code error "$col is not within sort range $sc to $ec"
    }
    set col [expr {$col - $sc}]

    set data [list]
    for {set i $sr} {$i <= $er} {incr i} {
	lappend data [$w get $i,$sc $i,$ec]
    }

    set i $sr
    foreach row [lsort -index $col {*}$args $data] {
	$w set row $i,$sc $row
	incr i
    }
}
