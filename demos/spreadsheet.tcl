#!/bin/sh
# the next line restarts using wish \
exec wish "$0" ${1+"$@"}

## spreadsheet.tcl
##
## This demos shows how you can simulate a 3D table
## and has other basic features to begin a basic spreadsheet
##
## jhobbs@cs.uoregon.edu

array set table {
  library	Tktable
  rows		10
  cols		10
  page		AA
  table		.b.ss
  default	pink
  AA		orange
  BB		blue
  CC		green
}
append table(library) [info shared]

if {[string match {} [info commands table]] && \
    [catch {package require Tktable} err]} {
  if {[catch {load [file join [pwd] .. $table(library)]} err] && \
      [catch {load [file join [pwd] $table(library)]} err]} {
    error $err
  }
}

proc colorize num {
  if {$num>0 && $num%2} { return colored }
}

proc fill {array {r 10} {c 10}} {
  upvar \#0 $array ary
  for {set i 0} {$i < $r} {incr i} {
    for {set j 0} {$j < $c} {incr j} {
      if {$j && $i} {
	set ary($i,$j) "$i x $j"
      } elseif {$i} {
	set ary($i,$j) "$i"
      } elseif {$j} {
	set ary($i,$j) [format %c [expr 65+$j]]
      }
    }
  }
}

proc changepage {w e name el op} {
  global $name table
  if [string comp {} $el] { set name [list $name\($el\)] }
  set i [set $name]
  if [string comp $i [$w cget -var]] {
    $w sel clear all
    $w config -variable $i
    $e config -textvar $i\(active\)
    $w activate origin
    if [info exists table($i)] {
      $w tag config colored -bg $table($i)
    } else {
      $w tag config colored -bg $table(default)
    }
    $w see active
  }
}

pack [label .l -text "TkTable v1 Spreadsheet Example"] -fill x
pack [frame .t] -fill x
pack [frame .b] -fill both -expand 1

pack [label .t.l -textvar table(current) -width 5] -side left
pack [entry .t.e -textvar $table(page)(active)] -side left -fill x -expand 1
tk_optionMenu .t.opt table(page) AA BB CC DD
pack [label .t.p -text PAGE: -width 8 -anchor e] -side left
pack .t.opt -side left

fill $table(page)
fill BB [expr {$table(rows)/2}] [expr {$table(cols)/2}]

trace var table(page) w [list changepage $table(table) .t.e]

set t $table(table)
set sy .b.sy
set sx .b.sx

table $t \
    -rows $table(rows) \
    -cols $table(cols) \
    -variable $table(page) \
    -titlerows 1 \
    -titlecols 1 \
    -yscrollcommand "$sy set" \
    -xscrollcommand "$sx set" \
    -maxwidth 320 \
    -maxheight 240 \
    -coltagcommand colorize \
    -flashmode on \
    -selectmode extended \
    -colstretch all \
    -rowstretch all \
    -batchmode 1 \
    -browsecommand {set table(current) %S}

$t tag config colored -bg $table($table(page))
$t tag config title -fg red
$t width 0 3

scrollbar $sy -command [list $t yview]
scrollbar $sx -command [list $t xview] -orient horizontal
pack $sx -side bottom -fill x
pack $sy -side right -fill y
pack $t -side left -fill both -expand on

puts [list Table is $table(table) with array [$table(table) cget -var]]

##
## Allow interactive prompt for testing
##

if !$tcl_interactive {
  if ![info exists tcl_prompt1] {
    set tcl_prompt1 {puts -nonewline "[history nextid] % "}
  }
  proc read_stdin {} {
    global eventLoop tcl_prompt1
    set l [gets stdin]
    if {[eof stdin]} {
      set eventLoop "done"     ;# terminate the vwait (eventloop)
    } else {
      if [string comp {} $l] {
	if [catch {uplevel \#0 history add [list $l] exec} err] {
	  puts stderr $err
	} elseif {[string comp {} $err]} {
	  puts $err
	}
      }
      catch $tcl_prompt1
      flush stdout
    }
  }
 
  # set up our keyboard read event handler:
  #   Vector stdin data to the socket
  fileevent stdin readable read_stdin
 
  catch $tcl_prompt1
  flush stdout
  # wait for and handle or stdin events...
  vwait eventLoop
}
