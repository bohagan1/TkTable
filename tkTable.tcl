# table.tcl --
#
# This file defines the default bindings for Tk table widgets
# and provides procedures that help in implementing those bindings.

# SCCS: @(#) table.tcl 1.17 96/12/13 11:42:22
#
# Copyright (c) 1994 The Regents of the University of California.
# Copyright (c) 1994-1995 Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

#--------------------------------------------------------------------------
# tkPriv elements used in this file:
#
# afterId -		Token returned by "after" for autoscanning.
# tablePrev -		The last element to be selected or deselected
#			during a selection operation.
# tableSelection -	All of the items that were selected before the
#			current selection operation (such as a mouse
#			drag) started;  used to cancel an operation.
#--------------------------------------------------------------------------

# Note: the check for existence of %W below is because this binding
# is sometimes invoked after a window has been deleted (e.g. because
# there is a double-click binding on the widget that deletes it). Users
# can put "break"s in their bindings to avoid the error, but this check
# makes that unnecessary.

## Button events

bind Table <1> {
  if [winfo exists %W] {
    tkTable.gBeginSelect %W [%W index @%x,%y]
    focus %W
  }
}
bind Table <B1-Motion> {
  array set tkPriv {x %x y %y}
  tkTable.gMotion %W [%W index @%x,%y]
}
bind Table <Double-1> {
  # empty
}
bind Table <ButtonRelease-1> {
  if [winfo exists %W] {
    tkCancelRepeat
    %W activate @%x,%y
  }
}
bind Table <Shift-1>		{tkTable.gBeginExtend %W [%W index @%x,%y]}
bind Table <Control-1>		{tkTable.gBeginToggle %W [%W index @%x,%y]}
bind Table <B1-Leave>		{
  array set tkPriv {x %x y %y}
  tkTable.gAutoScan %W
}
bind Table <B1-Enter>		{tkCancelRepeat}
bind Table <2>			{%W scan mark %x %y}
bind Table <B2-Motion>		{%W scan dragto %x %y}

## Key events

bind Table <Shift-Up>		{tkTable.gExtendUpDown %W -1}
bind Table <Shift-Down>		{tkTable.gExtendUpDown %W  1}
bind Table <Shift-Left>		{tkTable.gExtendLeftRight %W -1}
bind Table <Shift-Right>	{tkTable.gExtendLeftRight %W  1}
bind Table <Prior>		{%W yview scroll -1 pages; %W activate @0,0}
bind Table <Next>		{%W yview scroll  1 pages; %W activate @0,0}
bind Table <Control-Prior>	{%W xview scroll -1 pages}
bind Table <Control-Next>	{%W xview scroll  1 pages}
bind Table <Home>		{%W see origin}
bind Table <End>		{%W see end}
bind Table <Control-Home> {
  %W selection clear all
  %W activate origin
  %W selection set active
  %W see active
}
bind Table <Control-End> {
  %W selection clear all
  %W activate end
  %W selection set active
  %W see active
}
bind Table <Shift-Control-Home>	{
  tkTable.gDataExtend %W origin
}
bind Table <Shift-Control-End>	{tkTable.gDataExtend %W end}
bind Table <space>		{tkTable.gBeginSelect %W [%W index active]}
bind Table <Select>		{tkTable.gBeginSelect %W [%W index active]}
bind Table <Control-Shift-space> {tkTable.gBeginExtend %W [%W index active]}
bind Table <Shift-Select>	{tkTable.gBeginExtend %W [%W index active]}
bind Table <Escape>		{tkTable.gCancel %W}
bind Table <Control-slash>	{tkTable.gSelectAll %W}
bind Table <Control-backslash> {
 if [string match browse [%W cget -selectmode]] {%W selection clear all}
}
bind Table <Right>	{TableMoveCell %W  0  1}
bind Table <Left>	{TableMoveCell %W  0 -1}
bind Table <Down>	{TableMoveCell %W  1  0}
bind Table <Up>		{TableMoveCell %W -1  0}
bind Table <Any-KeyPress> {if [string comp {} %A] {%W insert active insert %A}}
bind Table <BackSpace> {
  set temp [%W icursor]
  if $temp {%W delete active [expr $temp-1]}
}
bind Table <Delete>		{%W delete active insert}
bind Table <Escape>		{%W reread}
bind Table <Return>		{
  %W set active [%W curvalue]
  TableMoveCell %W 1 0
}
bind Table <Control-Left>	{%W icursor [expr [%W icursor]-1]}
bind Table <Control-Right>	{%W icursor [expr [%W icursor]+1]}
bind Table <Control-e>		{%W icursor end}
bind Table <Control-a>		{%W icursor 0}
bind Table <Control-equal>	{TableSetWidth %W  1}
bind Table <Control-minus>	{TableSetWidth %W -1}

# tkTable.gBeginSelect --
#
# This procedure is typically invoked on button-1 presses. It begins
# the process of making a selection in the table. Its exact behavior
# depends on the selection mode currently in effect for the table;
# see the Motif documentation for details.
#
# Arguments:
# w - The table widget.
# el - The element for the selection operation (typically the
# one under the pointer). Must be in numerical form.

proc tkTable.gBeginSelect {w el} {
  global tkPriv
  if [string match multiple [$w cget -selectmode]] {
    if [$w selection includes $el] {
      $w selection clear $el
    } else {
      $w selection set $el
    }
  } else {
    $w selection clear all
    $w selection set $el
    $w selection anchor $el
    set tkPriv(tableSelection) {}
    set tkPriv(tablePrev) $el
  }
}

# tkTable.gMotion --
#
# This procedure is called to process mouse motion events while
# button 1 is down. It may move or extend the selection, depending
# on the table's selection mode.
#
# Arguments:
# w - The table widget.
# el - The element under the pointer (must be a number).

proc tkTable.gMotion {w el} {
  global tkPriv
  if {![info exists tkPriv(tablePrev)]} {
    set tkPriv(tablePrev) $el
    return
  }
  if {$el == $tkPriv(tablePrev)} return
  switch [$w cget -selectmode] {
    browse {
      $w selection clear all
      $w selection set $el
      set tkPriv(tablePrev) $el
    }
    extended {
      set i $tkPriv(tablePrev)
      $w selection clear anchor $i
      $w select set anchor $el
      set tkPriv(tablePrev) $el
    }
  }
}

# tkTable.gBeginExtend --
#
# This procedure is typically invoked on shift-button-1 presses. It
# begins the process of extending a selection in the table. Its
# exact behavior depends on the selection mode currently in effect
# for the table; see the Motif documentation for details.
#
# Arguments:
# w - The table widget.
# el - The element for the selection operation (typically the
# one under the pointer). Must be in numerical form.

proc tkTable.gBeginExtend {w el} {
  if {[string match extended [$w cget -selectmode]] &&
      [$w selection includes anchor]} {
    tkTable.gMotion $w $el
  }
}

# tkTable.gBeginToggle --
#
# This procedure is typically invoked on control-button-1 presses. It
# begins the process of toggling a selection in the table. Its
# exact behavior depends on the selection mode currently in effect
# for the table; see the Motif documentation for details.
#
# Arguments:
# w - The table widget.
# el - The element for the selection operation (typically the
# one under the pointer). Must be in numerical form.

proc tkTable.gBeginToggle {w el} {
  global tkPriv
  if [string match extended [$w cget -selectmode]] {
    set tkPriv(tableSelection) [$w curselection]
    set tkPriv(tablePrev) $el
    $w selection anchor $el
    if [$w selection includes $el] {
      $w selection clear $el
    } else {
      $w selection set $el
    }
  }
}

# tkTable.gAutoScan --
# This procedure is invoked when the mouse leaves an entry window
# with button 1 down. It scrolls the window up, down, left, or
# right, depending on where the mouse left the window, and reschedules
# itself as an "after" command so that the window continues to scroll until
# the mouse moves back into the window or the mouse button is released.
#
# Arguments:
# w - The entry window.

proc tkTable.gAutoScan {w} {
  global tkPriv
  if {![winfo exists $w]} return
  set x $tkPriv(x)
  set y $tkPriv(y)
  if {$y >= [winfo height $w]} {
    $w yview scroll 1 units
  } elseif {$y < 0} {
    $w yview scroll -1 units
  } elseif {$x >= [winfo width $w]} {
    $w xview scroll 2 units
  } elseif {$x < 0} {
    $w xview scroll -2 units
  } else {
    return
  }
  tkTable.gMotion $w [$w index @$x,$y]
  set tkPriv(afterId) [after 50 tkTable.gAutoScan $w]
}

# tkTable.gUpDown --
#
# Moves the location cursor (active element) up or down by one element,
# and changes the selection if we're in browse or extended selection
# mode.
#
# Arguments:
# w - The table widget.
# amount - +1 to move down one item, -1 to move back one item.

proc tkTable.gUpDown {w amount} {
  global tkPriv
  $w activate [expr [$w index active row]+$amount],[$w index active col]
  $w see active
  switch [$w cget -selectmode] {
    browse {
      $w selection clear all
      $w selection set active
    }
    extended {
      $w selection clear all
      $w selection set active
      $w selection anchor active
      set tkPriv(tablePrev) [$w index active]
      set tkPriv(tableSelection) {}
    }
  }
}

# tkTable.gExtendUpDown --
#
# Does nothing unless we're in extended selection mode; in this
# case it moves the location cursor (active element) up or down by
# one element, and extends the selection to that point.
#
# Arguments:
# w - The table widget.
# amount - +1 to move down one item, -1 to move up one item.

proc tkTable.gExtendUpDown {w amount} {
  if [string comp extended [$w cget -selectmode]] return
  $w activate [expr [$w index active row]+$amount],[$w index active col]
  $w see active
  tkTable.gMotion $w [$w index active]
}

# tkTable.gExtendLeftRight --
#
# Does nothing unless we're in extended selection mode; in this
# case it moves the location cursor (active element) left or right by
# one element, and extends the selection to that point.
#
# Arguments:
# w - The table widget.
# amount - +1 to move right one item, -1 to move left one item.

proc tkTable.gExtendLeftRight {w amount} {
  if [string comp extended [$w cget -selectmode]] return
  $w activate [$w index active row],[expr [$w index active col]+$amount]
  $w see active
  tkTable.gMotion $w [$w index active]
}

# tkTable.gDataExtend
#
# This procedure is called for key-presses such as Shift-KEndData.
# If the selection mode isnt multiple or extend then it does nothing.
# Otherwise it moves the active element to el and, if we're in
# extended mode, extends the selection to that point.
#
# Arguments:
# w - The table widget.
# el - An integer element number.

proc tkTable.gDataExtend {w el} {
  set mode [$w cget -selectmode]
  if [string match extended $mode] {
    $w activate $el
    $w see $el
    if [$w selection includes anchor] {tkTable.gMotion $w $el}
  } elseif {[string match multiple $mode]} {
    $w activate $el
    $w see $el
  }
}

# tkTable.gCancel
#
# This procedure is invoked to cancel an extended selection in
# progress. If there is an extended selection in progress, it
# restores all of the items between the active one and the anchor
# to their previous selection state.
#
# Arguments:
# w - The table widget.

proc tkTable.gCancel {w} {
  global tkPriv
  if [string comp extended [$w cget -selectmode]] return
  set first [$w index anchor]
  set last $tkPriv(tablePrev)
  $w selection clear $first $last
  $w selection set $first $last $tkPriv(tableSelection)
}

# tkTable.gSelectAll
#
# This procedure is invoked to handle the "select all" operation.
# For single and browse mode, it just selects the active element.
# Otherwise it selects everything in the widget.
#
# Arguments:
# w - The table widget.

proc tkTable.gSelectAll {w} {
  if [regexp {^(single|browse)$} [$w cget -selectmode]] {
    $w selection clear all
    $w selection set active
  } else {
    $w selection set origin end
  }
}

proc TableMoveCell {W x y} {
  set r [$W index active row]
  set c [$W index active col]
  incr r $x
  incr c $y
  $W selection clear all
  $W activate $r,$c
  $W selection set active
  $W selection anchor active
}

proc TableSetWidth {W a} {
  set tmp [$W index active col]
  $W width $tmp [expr [$W width $tmp]+$a]
}
