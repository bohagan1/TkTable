# all.tcl --
#
# This file contains a top-level script to run all of the Tk table
# tests.  Execute it by invoking "source all.tcl" when running tktest
# in this directory.
#
# Copyright (c) 1998-2000 Ajuba Solutions
# Copyright (c) 2000 Jeffrey Hobbs
#
# See the file "license.txt" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# 
# RCS: @(#) $Id: all.tcl,v 1.4 1999/04/21 21:53:29 rjohnson Exp $

package require tcltest
namespace import -force ::tcltest::*

set ::tcltest::testSingleFile false
set ::tcltest::testsDirectory [file dir [info script]]

# We need to ensure that the testsDirectory is absolute
catch {::tcltest::normalizePath ::tcltest::testsDirectory}

puts $::tcltest::outputChannel \
	"Tk $tk_patchLevel tests running in interp:  [info nameofexecutable]"
puts $::tcltest::outputChannel \
	"Tests running in working dir:  $::tcltest::testsDirectory"
if {[llength $::tcltest::skip] > 0} {
    puts $::tcltest::outputChannel \
	    "Skipping tests that match:  $::tcltest::skip"
}
if {[llength $::tcltest::match] > 0} {
    puts $::tcltest::outputChannel \
	    "Only running tests that match:  $::tcltest::match"
}

if {[llength $::tcltest::skipFiles] > 0} {
    puts $::tcltest::outputChannel \
	    "Skipping test files that match:  $::tcltest::skipFiles"
}
if {[llength $::tcltest::matchFiles] > 0} {
    puts $::tcltest::outputChannel \
	    "Only sourcing test files that match:  $::tcltest::matchFiles"
}

set timeCmd {clock format [clock seconds]}
puts $::tcltest::outputChannel "Tests began at [eval $timeCmd]"

# source each of the specified tests
foreach file [lsort [::tcltest::getMatchingFiles]] {
    set tail [file tail $file]
    puts $::tcltest::outputChannel $tail
    if {[catch {source $file} msg]} {
	puts $::tcltest::outputChannel $msg
    }
}

# cleanup
puts $::tcltest::outputChannel "\nTests ended at [eval $timeCmd]"
::tcltest::cleanupTests 1
exit
