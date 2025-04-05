package require Tktable

set cols 100
set rows 100

set w [table .tb -cols $cols -rows $rows -height 10 -width 10 \
    -titlecols 1 -titlerows 1 -wrap 1 \
    -xscrollcommand [list .sx set] -yscrollcommand [list .sy set]]
ttk::scrollbar .sx -orient horizontal -command [list .tb xview]
ttk::scrollbar .sy -orient vertical -command [list .tb yview]

grid .tb -row 0 -column 0 -sticky nsew
grid .sy -row 0 -column 1 -sticky nsew
grid .sx -row 1 -column 0 -sticky nsew

grid columnconfigure . 0 -weight 1
grid rowconfigure . 0 -weight 1

for {set row 0} {$row < $rows} {incr row} {
    for {set col 0} {$col < $cols} {incr col} {
	set cell [format "%d,%d" $row $col]
	$w set $cell $cell
    }
}

