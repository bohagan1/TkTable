
proc fill { array x y } {
upvar $array f
for { set i -$x } { $i < $x } { incr i } {
	for { set j -$y } { $j < $y } { incr j } {
		set f($i,$j) $i,$j } } }

#
# Test out the use of a procedure to define tags on rows and columns
#
proc rowProc row {
    if {$row <= 0 } {
 return {}
    }
    if [expr $row % 2] {
      return OddRow
    }
    return {}
}
proc colProc col {
    if {$col <= 0 } {
      return {}
    }
    if [expr $col % 2] {
      return OddCol
    }
    return {}
}

wm maxsize . 1000 1000

button .b -text junk
pack .b
update

fill f  15 15 
table .a -cursorbg blue  -rows 15 -cols 15 -ysc { .sv set } -xscr {.sh set } -rowt 2 -colt 2 -var f -roworigin -2 -colorigin -2 -maxwidth 300 -maxheight 300
scrollbar .sv -comm { .a toprow } 
scrollbar .sh -com  { .a leftcol } -orient horizontal
pack .sh -side bottom -fill x
pack .sv -side right -fill y
pack .a -side left -fill both -expand on

.a tag con Flash -bg red -fg white
.a flash tag Flash
.a flash mode on
.a draw fast

.a tag con OddRow -bg orange -fg purple
.a tag con OddCol -bg brown -fg pink
.a con -procrowtag rowProc
.a con -proccoltag colProc

update
for {set g 1 } { $g < 1000 } {incr g } { update  }

set f(1,0) "Flash me"

