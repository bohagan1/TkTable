#
# TkTable Demo
#

package require Tktable

#
# Create menubar
#
proc create_menubar {w table} {
    set m [menu $w]

    # Add items
    foreach name [list file edit column row cell help] {
	set title [string totitle $name]
	set $name [menu [format "%s.%s" $m $name] -title $title -tearoff 0]
	$m add cascade -label $title -menu [set $name]
    }

    # File menu
# Add save, save as, print on Tk 9, load
    foreach {name command} [list Exit [list exit]] {
	$file add command -label $name -command $command
    }

    # Edit menu
    foreach {name event} [list \
	Undo "<<Undo>>" Redo "<<Redo>>" - - \
	Copy "<<Copy>>" Cut "<<Cut>>" Paste "<<Paste>>" - - \
	"Clear Contents" "<<Clear>>" - - \
	"Select All" "<<SelectAll>>" "Unselect All" "<<SelectNone>>"] {
	if {$name ne "-"} {
	    $edit add command -label $name -command [list event generate $table $event]
	} else {
	    $edit add separator
	}
    }

    # Column menu
    # Column: -cols, -colorigin, -colwidth, -colstretchmode, -titlecols
    foreach {name fn} [list \
	"Delete" delete_col \
	"Insert Before" insert_before \
	"Insert After" insert_after \
	- - \
	"Hide" none \
	"Unhide" none \
	- - \
	"Sort" none] {
	if {$name ne "-"} {
	    $column add command -label $name -command [list perform_fn $table $fn] \
		-state [expr {$fn ne "none" ? "normal" : "disabled"}]
	} else {
	    $column add separator
	}
    }

    # Row menu
    # Row: -rows, -roworigin, -titlerows
    foreach {name fn} [list \
	"Delete" delete_row \
	"Insert Above" insert_above \
	"Insert Below" insert_below \
	- - \
	"Hide" none \
	"Unhide" none \
	- - \
	"Sort" none] {
	if {$name ne "-"} {
	    $row add command -label $name -command [list perform_fn $table $fn] \
		-state [expr {$fn ne "none" ? "normal" : "disabled"}]
	} else {
	    $row add separator
	}
    }

    # Cell menu
    # clear contents, reset, sort
    foreach {name fn} [list \
	"Clear" curvalue \
	"Abort Changes" reread \
	- - \
	"Make Span" spans \
	"Remove Span" nospan] {
	if {$name ne "-"} {
	    $cell add command -label $name -command [list perform_fn $table $fn] \
		-state [expr {$fn ne "none" ? "normal" : "disabled"}]
	} else {
	    $cell add separator
	}
    }

    return $m
}

#
# Create display
#
proc create_table {w} {
    set corigin -10
    set rorigin -10
    set cols 100
    set rows 100
    set row 0

    # Table frame
    set f [ttk::frame $w -borderwidth 2 -relief groove]
    grid $f -sticky nsew
    grid columnconfigure $w 0 -weight 1
    grid rowconfigure $w  0 -weight 1

    # Create table
    set table [table $f.table -height 10 -width 10 \
	-titlecols 1 -titlerows 1 -exportselection 0 -selectmode extended \
	-cols $cols -rows $rows  -colorigin $corigin -roworigin $rorigin \
	-xscrollcommand [list $f.sx set] -yscrollcommand [list $f.sy set] \
	-browsecommand {select_handler %W %s %S}]
    set sx [ttk::scrollbar $f.sx -orient horizontal -command [list $table xview]]
    set sy [ttk::scrollbar $f.sy -orient vertical -command [list $table yview]]

    grid $table -row $row -column 0 -sticky nsew
    grid $sy -row $row -column 1 -sticky nsew
    grid $sx -row [incr row] -column 0 -sticky nsew
    grid columnconfigure $f 0 -weight 1
    grid rowconfigure $f 0 -weight 1

    # Populate cells with cell id
    for {set r $rorigin} {$r < ($rows + $rorigin)} {incr r} {
	for {set c $corigin} {$c < ($cols + $corigin)} {incr c} {
	    set cell [format "%d,%d" $r $c]
	    $table set $cell $cell
	}
    }

#    bind $table <<TableSelect>> [list after 500 select_handler $table]
    return $table
}

#
# Widget Update Handler
#
proc callback_handler {w opt var} {
    $w configure $opt [set $var]
}

#
# Cell select handler
#
proc select_handler {w prev cell} {
    if {$cell ne ""} {
	set ::data(cell) $cell
	set ::data(value) [$w get $cell]
    } elseif {![catch {$w index focus} cell]} {
	set ::data(cell) $cell
	set ::data(value) [$w get $cell]
    } else {
	set ::data(cell) ""
	set ::data(value) ""
    }
}

#
# Cell toolbar
#
proc cell_toolbar {w table} {
    # Container frame
    set f [ttk::frame $w -borderwidth 2 -relief groove -padding 5]
    grid $f -sticky nsew

    # Toolbar widgets
    array set ::data [list cell "" value ""]
    set l [ttk::label $f.c -text "Cell:"]
    set c [ttk::label $f.l -textvariable ::data(cell) -width 6]
    set e [ttk::entry $f.e -textvariable ::data(value)]
    set b [ttk::button $f.b -text "Clear" -command [format "%s delete 0 end;focus %s" $e $e]]
    grid $l $c $e $b -sticky nsew
    grid columnconfigure $f 2 -weight 1

    # Create entry bindings
    foreach {event fn} [list <Return> down <Shift-Return> up <Tab> right \
	<Shift-Tab> left <Escape> abort <FocusIn> none <FocusOut> none] {
	bind $e $event [format {edit_handler %s %s %s} $e $table $fn]
    }
    return $f
}

#
# Cell edit handler
#
proc edit_handler {e table fn} {
    if {[catch {$table index active} cell]} {
	if {[catch {$table index focus} cell]} {
	    return
	}
    }

    if {$fn eq "abort"} {
	$table reread
	select_handler $table $cell $cell
	return
    }

    # Update cell to edited text
    $table set $cell [$e get]

    # Go to next cell
    switch -- $fn {
	"left" {
	    event generate $table <<PrevChar>>
	}
	"right" {
	    event generate $table <<NextChar>>
	}
	"up" {
	    event generate $table <<PrevLine>>
	}
	"down" {
	    event generate $table <<NextLine>>
	}
    }
}

#
# Selection control options toolbar
#
proc selection_toolbar {w table} {
    # Container frame
    set f [ttk::frame $w -borderwidth 2 -relief groove -padding 5]
    set col -1

    set modes [list browse single multiple extended]
    set types [list row col cell both]

    # Toolbar widgets
    grid [ttk::label $f.l -text "Selection:"] -row 0 -column [incr col]
    foreach {label opt type enums} [list \
	"Export" -exportselection check {} \
	"Invert" -invertselected check {} \
	"Select Mode" -selectmode enum $modes \
	"Select Titles" -selecttitle check {} \
	"Select Type" -selecttype enum $types] {
	set var ::options($opt)
	set $var [$table cget $opt]

	if {$type eq "enum"} {
	    set l [ttk::label $f.l$col -text "${label}:"]
	    grid $l -row 0 -column [incr col] -padx {10 0}
	    set cb [ttk::combobox $f.cb$col -textvariable $var -values $enums \
		-width [max_size $enums] -exportselection 0]
	    bind $cb <<ComboboxSelected>> [list callback_handler $table $opt $var]
	    grid $cb -row 0 -column [incr col] -padx {0 10}
	} else {
	    set cb [ttk::checkbutton $f.cb$col -text $label -variable $var \
		-command [list callback_handler $table $opt $var]]
	    grid $cb -row 0 -column [incr col] -padx 10
	}
    }

    # Get selection
    set col -1
    set var ::data(selection)
    set $var ""
    set g [ttk::button $f.get -text "Get" -command [list get_selection $table $var] -width 10]
    set e [ttk::entry $f.sel -textvariable $var]
    set c [ttk::button $f.clear -text "Clear" -command [list clear_selection $table $var] -width 10]
    grid $g -row 1 -column [incr col]
    grid $e -row 1 -column [incr col] -sticky nsew -columnspan 6
    grid columnconfigure $f $col -weight 1
    grid $c -row 1 -column [incr col 6]
    return $f
}

#
# Widget control options toolbar
#
proc text_toolbar {w table} {
    set col 0
    set row 0

    # Container frame
    set f [ttk::frame $w -borderwidth 2 -relief groove -padding 5]

    # Toolbar widgets
    grid [ttk::label $f.l -text "Text:"] -row $row -column $col
    incr col

    foreach {label opt type enums} [list \
	"Anchor" -anchor enum [list n ne e se s sw w nw center] \
	"Auto Clear" -autoclear check {} \
	"Ellipsis" -ellipsis enum [list "" "..." ] \
	"Justify" -justify enum [list left right center] \
	"Multi-line" -multiline check {} \
	"Wrap Text" -wrap check {}] {
	set var ::options($opt)
	set $var [$table cget $opt]

	if {$type eq "enum"} {
	    set l [ttk::label $f.l$col -text "${label}:"]
	    grid $l -row 0 -column [incr col] -padx {10 0}
	    set cb [ttk::combobox $f.cb$col -textvariable $var -values $enums \
		-width [max_size $enums] -exportselection 0]
	    bind $cb <<ComboboxSelected>> [list callback_handler $table $opt $var]
	    grid $cb -row 0 -column [incr col] -padx {0 10}
	} else {
	    set cb [ttk::checkbutton $f.cb$col -text $label -variable $var \
		-command [list callback_handler $table $opt $var]]
	    grid $cb -row 0 -column [incr col] -padx 10
	}
    }
    return $f
}

#
# Modes control options toolbar
#
proc modes_toolbar {w table} {
    # Container frame
    set f [ttk::frame $w -borderwidth 2 -relief groove -padding 5]
    set col -1

    # Toolbar widgets
    grid [ttk::label $f.l -text "Modes:"] -row 0 -column [incr col]
    foreach {label opt type enums} [list \
	"Binding" -mode enum [list table spreadsheet] \
	"Cell Flash" -flashmode check {} \
	"Cursor Blink" -insertofftime enum [list 0 300 600 900] \
	"Drawing" -drawmode enum [list slow compatible fast single] \
	"Resize" -resizeborders enum [list row col both none] \
	"State" -state enum [list normal readonly disabled]] {
	set var ::options($opt)
	set $var [$table cget $opt]

	if {$type eq "enum"} {
	    set l [ttk::label $f.l$col -text "${label}:"]
	    grid $l -row 0 -column [incr col] -padx {10 0}
	    set cb [ttk::combobox $f.cb$col -textvariable $var -values $enums \
		-width [max_size $enums] -exportselection 0]
	    bind $cb <<ComboboxSelected>> [list callback_handler $table $opt $var]
	    grid $cb -row 0 -column [incr col] -padx {0 10}
	} else {
	    set cb [ttk::checkbutton $f.cb$col -text $label -variable $var \
		-command [list callback_handler $table $opt $var]]
	    grid $cb -row 0 -column [incr col] -padx 10
	}
    }
    return $f
}

#
# Get max size
#
proc max_size {list} {
    set size 0
    foreach item $list {
	set len [string length $item]
	if {$len > $size} {
	    set size $len
	}
    }
    return $size
}

#
# Widget Update Handler
#
proc callback_handler {w opt var} {
    $w configure $opt [set $var]
}

#
# Get selection handler
#
proc get_selection {w var} {
    set $var [$w curselection]
}

#
# Clear selection handler
#
proc clear_selection {w var} {
    $w selection clear all
    set $var ""
}

#
#
#
proc perform_fn {w fn} {
    if {[catch {$w index active} cell] &&  [catch {$w index focus} cell]} {
	return
    }
    lassign [split $cell ","] row col
    switch -- $fn {
	"curvalue" {
	    $w curvalue {}
	}
	"insert_above" {
	    $w insert rows -- $row -1
	}
	"insert_below" {
	    $w insert rows -- $row 1
	}
	"insert_before" {
	    $w insert cols -- $col -1
	}
	"insert_after" {
	    $w insert cols -- $col 1
	}
	"delete_col" {
	    $w delete cols -- $col
	}
	"delete_row" {
	    $w delete rows -- $row
	}
	"nospan" {
	    $w spans $cell 0,0
	}
	"reread" {
	    $w reread
	}
	"spans" {
	    lassign [lrepeat 2 {*}[split $cell ","]] r1 c1 r2 c2
	    foreach cell [$w curselection] {
		lassign [split $cell ","] r c
		if {$r < $r1} {set r1 $r}
		if {$r > $r2} {set r2 $r}
		if {$c < $c1} {set c1 $c}
		if {$c > $c2} {set c2 $c}
	    }
	    if {$r2 > $r1 || $c2 > $c1} {
		set cell [format "%d,%d" $r1 $c1]
		$w spans $cell [format "%d,%d" [expr {$r2 - $r1}] [expr {$c2 - $c1}]]
		$w selection clear all
		$w activate $cell
		$w selection set $cell
	    }
	}
    }
}

#
# Main
#
proc main {} {
    set w .table
    wm withdraw .
    toplevel $w
    wm title $w "TkTable Demo"
    wm iconname $w "tktable"
    wm minsize $w 300 300
    wm protocol $w WM_DELETE_WINDOW [list exit]

    # Treeview
    set table [create_table $w.tf]
    grid $w.tf -sticky nsew -row 2
    grid rowconfigure $w 2 -weight 1
    grid columnconfigure $w 0 -weight 1

    # Add menu
    set m [create_menubar $w.m $table]
    $w configure -menu $m

    # Cell Toolbar
    set f [cell_toolbar $w.cf $table]
    grid $f -sticky nsew -row 1

    # Selection Toolbar
    set f [selection_toolbar $w.sf $table]
    grid $f -sticky nsew -row 3

    # Modes Toolbar
    set f [modes_toolbar $w.mf $table]
    grid $f -sticky nsew -row 4

    # Options toolbar
    set f [text_toolbar $w.of $table]
    grid $f -sticky nsew -row 5

    # Status toolbar
    # Active, Focus, messages, num selected, table size, sizer
}

main
