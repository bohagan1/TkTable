#!/bin/sh
# the next line restarts using wish \
exec wish "$0" ${1+"$@"}

## valid.tcl
##
## This demos shows the use of the validation mechanism of the table
##
## jhobbs@cs.uoregon.edu

array set table {
  library	Tktable
  rows		10
  cols		10
  page		AA
  table		.b.ss
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
	continue
      } elseif {$i} {
	set ary($i,$j) "$i"
      } elseif {$j%3==1} {
	set ary($i,$j) AlphaNum
      } elseif {$j%2==1} {
	set ary($i,$j) Alpha
      } elseif {$j} {
	set ary($i,$j) Real
      }
    }
  }
}

proc validate {c val} {
  if {$c%3==1} {
    ## Alphanum
    set expr {^[A-Za-z0-9 ]*$}
  } elseif {$c%2==1} {
    ## Alpha
    set expr {^[A-Za-z ]*$}
  } elseif {$c} {
    ## Real
    set expr {^[-+]?[0-9]*\.?[0-9]*([0-9]\.?e[-+]?[0-9]*)?$}
  }
  if [regexp $expr $val] {
    return 1
  } else {
    bell
    return 0
  }
}

pack [label .l -text "TkTable v1 Validated Table Example"] -fill x
pack [frame .b] -fill both -expand 1

fill $table(page)

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
    -validate yes \
    -vcmd {validate %c %S}

$t tag config colored -bg lightblue
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
