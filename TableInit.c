"proc TableSetCell {W x y} {\n\
  set args [$W whatcell $x $y]\n\
  $W setcell [lindex $args 0] [lindex $args 1]\n\
}",
"proc TableMoveCell {W x y} {\n\
  set args [$W setcell]\n\
  $W setcell [expr [lindex $args 0]+$x] [expr [lindex $args 1]+$y]\n\
}",
"proc TableSetWidth {W a} {\n\
  set temp [expr [lindex [$W setcell] 1]-[lindex [$W configure -colorig] 4]]\n\
  $W width [list $temp [expr [$W getwidth $temp]+$a]]\n\
}",
"bind Table <Button-1> {TableSetCell %W %x %y;focus %W}",
"bind Table <Right> {TableMoveCell %W 0 1}",
"bind Table <Left>  {TableMoveCell %W 0 -1}",
"bind Table <Down> {TableMoveCell %W 1 0}",
"bind Table <Up> {TableMoveCell %W -1 0}",
"bind Table <Any-KeyPress> {if {\"%A\" != \"\"} {%W insert insert %A}}",
"bind Table <BackSpace> {\n\
  set temp [%W icursor]\n\
  if {$temp} {%W delete [expr $temp-1]}\n\
}",
"bind Table <Delete> {%W delete insert}",
"bind Table <Escape> {%W reread}",
"bind Table <Return> {TableMoveCell %W 1 0}",
"bind Table <Control-Left> {%W icursor [expr [%W icursor]-1]}",
"bind Table <Control-Right> {%W icursor [expr [%W icursor]+1]}",
"bind Table <End> {%W icursor end}",
"bind Table <Home> {%W icursor 0}",
"bind Table <Control-equal> {TableSetWidth %W 1}",
"bind Table <Control-minus> {TableSetWidth %W -1}",
