
if { [llength $argv] < 2 } {
	puts stderr "Usage: ll_entrybeg.tcl <ll_entry.c-path> LL|BB"
	exit 1
}

#puts [lindex $argv 0]
set fd [open [lindex $argv 0] w]

if { [lindex $argv 1] == "LL" } {
	set ll_ "ll_"
	set LL_ "LL_"
	set LLE "LL_"
} else {
	set ll_ "bb_"
	set LL_ "BB_"
	set LLE "BBIS_"
}

puts $fd \
"/* This file is generated automatically. Do not edit! */
#include <MEN/men_typs.h>
#include <MEN/mdis_err.h>
#include <MEN/oss.h>
#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/${ll_}defs.h>
#include <MEN/${ll_}entry.h>
#include <${ll_}entryi.h>

typedef struct
{
    char *name;
    void (* getEntry) ( ${LLE}ENTRY* drvP );
}DRV_GETENTRY;

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
static DRV_GETENTRY ${LL_}DrvTable\[\] =
{
"

close $fd

regsub .c$ [lindex $argv 0] i.h ll_entryi_h
set fd2 [open $ll_entryi_h w]

close $fd2
