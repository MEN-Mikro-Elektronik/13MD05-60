
if { [llength $argv] < 1 } {
	puts stderr "Usage: progrefend.tcl <prog_ref.c-path>"
	exit 1
}


set fd [open [lindex $argv 0] a]
puts $fd "}"
close $fd