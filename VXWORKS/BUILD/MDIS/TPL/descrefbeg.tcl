
if { [llength $argv] < 1 } {
	puts stderr "Usage: descrefbeg.tcl <desc_ref.c-path>"
	exit 1
}


set fd [open [lindex $argv 0] w]

puts $fd \
"/* This file is generated automatically. Do not edit! */
#include <desc_refi.h>

char* MdisDescRefs() { 
	const volatile static char *descs\[\] = {"

close $fd

regsub .c$ [lindex $argv 0] i.h desci_h
set fd2 [open $desci_h w]

close $fd2
