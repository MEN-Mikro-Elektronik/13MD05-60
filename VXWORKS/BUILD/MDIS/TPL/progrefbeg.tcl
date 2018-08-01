
if { [llength $argv] < 1 } {
	puts stderr "Usage: progrefbeg.tcl <prog_ref.c-path>"
	exit 1
}


set fd [open [lindex $argv 0] w]

puts $fd \
"/* This file is generated automatically. Do not edit! */
void MdisProgRefs() { 
"

close $fd