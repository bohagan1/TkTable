#!/bin/sh
# next line is a comment in tcl \
exec wish "$0" ${1+"$@"}

## If your Tk is dynamically loadable then you can use tclsh
## instead of wish as the executable above

if {[string match {} [info commands table]] && \
    [catch {package require Tktable} err]} {
  if {[catch {load [file join [pwd] .. $table(library)]} err] && \
      [catch {load [file join [pwd] $table(library)]} err]} {
    error $err
  }
}

# scrollable table of buttons

set rows 20
set cols 20

frame .a
frame .b

# create the table
table .a.t \
    -rows [expr $rows +1] \
    -cols [expr $cols +1] \
    -titlerows 1 \
    -titlecols 1 \
    -roworigin -1 \
    -colorigin -1 \
    -maxheight 250 \
    -maxwidth 400 \
    -width 5 \
    -variable tab \
    -flashmode off \
    -cursor top_left_arrow \
    -borderwidth 2 \
    -state disabled \
    -xscrollcommand ".b.h set" \
    -yscrollcommand ".a.v set"

# horizontal scrollbar
scrollbar .b.h \
    -orient horiz \
    -relief sunken \
    -command ".a.t boundary left"

# vertical scrollbar
scrollbar .a.v \
    -relief sunken \
    -command ".a.t boundary top"

# create a filler for the lower right corner between the scrollbars
frame .b.pad \
    -width [expr [.a.v cget -width] + \
        [.a.v cget -bd]*2 + [.a.v cget -highlightthickness]*2 ] \
    -height [expr [.b.h cget -width] + \
        [.b.h cget -bd]*2 + [.b.h cget -highlightthickness]*2 ]

# pack the various widgets.  getting the packing order right is tricky
pack .a.v -side right -fill y
pack .a.t -side left -fill both -expand true
pack .b.h -side left -fill x -expand true
pack .b.pad -side right
pack .b -side bottom -fill x
pack .a -side top -fill both -expand true

# set up tags for the various states of the buttons
.a.t tag configure OFF -bg red -relief raised
.a.t tag configure ON -bg green -relief sunken
.a.t tag configure sel -bg gray75 -relief flat

# clean up if mouse leaves the widget
bind .a.t <Leave> {
    %W selection clear all
}

# highlight the cell under the mouse
bind .a.t <Motion> {
    %W selection clear all
    %W selection set @%x,%y
}

# mousebutton 1 toggles the value of the cell
bind .a.t <1> {
    set rc [%W cursel]
    if {$tab($rc) == "ON"} {
	set tab($rc) OFF
        %W tag celltag OFF $rc
    } {
	set tab($rc) ON
        %W tag celltag ON $rc
    }
}

# inititialize the array, titles, and celltags
for {set i 0} {$i < $rows} {incr i} {
    set tab($i,-1) $i
    for {set j 0} {$j < $cols} {incr j} {
        if {! $i} {set tab(-1,$j) $j}
	set tab($i,$j) "OFF"
        .a.t tag celltag OFF $i,$j
    }
}
