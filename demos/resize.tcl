## resize.tcl
##
## jhobbs@cs.uoregon.edu
##
## This is broken code for an all-Tcl solution to resizing the row size
## It doesn't work for columns, it doesn't work well for rows
## I'll either improve this or write something in C (a better idea).

bind Table <3> {
  if [winfo exists %W] {
    tkTableBeginDragRC %W %x %y [%W index @%x,%y]
    focus %W
  }
}
bind Table <B3-Motion> {
  tkTableDragRC %W %x %y
}
bind Table <ButtonRelease-3> {
  if [winfo exists %W] {
    tkTableEndDragRC %W
  }
}

proc tkTableBeginDragRC {w x y cell} {
  global tkPriv
  scan $cell %d,%d r c
  if {$r < [$w cget -titlerows]+[$w cget -roworigin]} {
    ## We're in a column header
    set owh	height
    set wh	width
    set xy	x
    set hv	h
    set oxy	y
  } elseif {$c < [$w cget -titlecols]+[$w cget -colorigin]} {
    ## We're in a row header
    set xy	y
    set oxy	x
    set hv	v
    set owh	width
    set wh	height
  } else {
    return
  }

  catch {destroy $w.___pane}
  set f [frame $w.___pane -bd 1 -$wh 2 -relief sunken]
  scan [winfo geom $w] %dx%d+%d+%d wx wy width height
  foreach {sx sy cx cy} [$w bbox $cell] {
    set size [$w $wh $r]
    break
  }
  place $f -rel$owh 1 -$xy [set $xy] -$oxy [set c$oxy] -anchor nw
  raise $f
  update
  set tkPriv(tablePrev) [list $xy $wh $size $cell]
}

proc tkTableDragRC {w x y} {
  if [winfo exists $w.___pane] {
    global tkPriv
    foreach {xy wh size cell} $tkPriv(tablePrev) break
    place $w.___pane -$xy [set $xy]
    if {[string match height $wh]} {
      $w height [$w index $cell row] [expr {$size+(-$tkPriv($xy)+[set $xy])}]
    }
  }
}

proc tkTableEndDragRC {w} {
  global tkPriv
  catch {destroy $w.___pane}
}
