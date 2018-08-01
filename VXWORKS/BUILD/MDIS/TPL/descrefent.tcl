#***************************************************************************
#***********                                                   *************
#***********         DESCREFENT.TCL                            *************
#***********                                                   *************
#***************************************************************************
#  
#       Author: kp
#        $Date: 2000/03/17 15:08:37 $
#    $Revision: 1.1 $
#
#  Description: Build an entry in MdisDescRefs() dummy function for 
#               each descriptor in the configuration
#
# RESTRICTIONS: 
#
#     Required: cvs, rcsdiff, rlog
#
# Environment:
#				
#
#-------------------------------[ History ]---------------------------------
#
# $Log: descrefent.tcl,v $
# Revision 1.1  2000/03/17 15:08:37  kp
# Initial Revision
#
#---------------------------------------------------------------------------
# (c) Copyright 2016 by MEN mikro elektronik GmbH, Nuremberg, Germany 
#***************************************************************************/

if { [llength $argv] < 4 } {
	puts stderr "Usage: descrefent.tcl <nmbin> <thisdir> <desc_ref.c-path> <descriptor>"
	exit 1
}

set nm [lindex $argv 0]
set thisdir [lindex $argv 1]
set fd [open "[lindex $argv 2]" a]
set desc [lindex $argv 3]

set nm [join [split $nm \\] /]
set thisdir [join [split $thisdir \\] /]

regsub .c$ [lindex $argv 2] i.h desci_h
set fd2 [open $desci_h a]

#
# Call nm to extract the global names in descriptor
#

set syscmd "$nm --extern-only ${desc}.o"
catch { eval exec $syscmd } nmOut	

foreach line [split $nmOut \n] {
	#puts "<$line>"

	if { [llength $line] == 3 } {
		if { ([lindex $line 1] == "D") || ([lindex $line 1] == "R") } {
			set getEntry [lindex $line 2]
			# remove any leading underscore from symbol
			if { [string index $getEntry 0] == "_" } {
				set getEntry [string range $getEntry 1 end] 
			}
			
		    puts $fd "\t  $getEntry, "
		    puts -nonewline "  -> $getEntry"
			puts $fd2 "extern char $getEntry\[\];"
		}
	}
}

puts $fd "0 };
   return( (void*) descs\[0\] );
}

/* this helper allows to seek for a DESC set by its name as given in system.dsc.
   required for passing descriptor addresses over longer distances (not in SDA21 border) */

#define MAX_DESC_NAME_LEN 40

struct mdis_dsc_entries {
	char *devname;
	char *descaddr;
};

char* MdisDescRefGetByName(char *devName) 
{ 
	int idx=0;

	struct mdis_dsc_entries desc_entries\[\]= 
	{ 
"

foreach line [split $nmOut \n] {
	#puts "<$line>"
	if { [llength $line] == 3 } {
		if { ([lindex $line 1] == "D") || ([lindex $line 1] == "R") } {
			set getEntry [lindex $line 2]
			# remove any leading underscore from symbol
			if { [string index $getEntry 0] == "_" } {
				set getEntry [string range $getEntry 1 end] 
			}
			
		    puts $fd "\t\t { \"$getEntry\", $getEntry },"
		}
	}
}

puts ""

close $fd
close $fd2

exit 0
