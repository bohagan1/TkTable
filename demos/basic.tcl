#!/bin/sh
# the next line restarts using wish \
exec wish "$0" ${1+"$@"}

## basic.tcl
##
## This demo shows the basic use of the table widget
##
## jhobbs@cs.uoregon.edu

array set table {
  library	Tktable
  rows		20
  cols		20
  table		.t
  array		t
}
append table(library) [info shared]

## Ensure that the table library extension is loaded
if {[string match {} [info commands table]] && \
    [catch {package require Tktable} err]} {
  if {[catch {load [file join [pwd] .. $table(library)]} err] && \
      [catch {load [file join [pwd] $table(library)]} err]} {
    error $err
  }
}

proc fill { array x y } {
  upvar $array f
  for {set i -$x} {$i<$x} {incr i} {
    for {set j -$y} {$j<$y} {incr j} { set f($i,$j) "r:$i,c:$j" }
  }
}

## Test out the use of a procedure to define tags on rows and columns
proc rowProc row { if {$row>0 && $row%2} { return OddRow } }
proc colProc col { if {$col>0 && $col%2} { return OddCol } }

pack [label .l -text "TkTable v1 Test"] -fill x

fill $table(array) $table(rows) $table(cols)
table $table(table) -rows $table(rows) -cols $table(cols) \
    -variable $table(array) \
    -titlerows 2 -titlecols 2 \
    -yscrollcommand {.sy set} -xscrollcommand {.sx set} \
    -roworigin -2 -colorigin -2 \
    -maxwidth 350 -maxheight 300 \
    -rowtagcommand rowProc -coltagcommand colProc \
    -selectmode extended \
    -rowstretch all -colstretch last \
    -flashmode on
scrollbar .sy -command [list $table(table) yview]
scrollbar .sx -command [list $table(table) xview] -orient horizontal
pack .sx -side bottom -fill x
pack .sy -side right -fill y
pack $table(table) -side left -fill both -expand on

$table(table) tag config OddRow -bg orange -fg purple
$table(table) tag config OddCol -bg brown -fg pink

update

## This will show the use of the flash mode
after 1000 [list array set $table(array) { 1,0 "Flash Me" 3,2 "And Me" }]

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
