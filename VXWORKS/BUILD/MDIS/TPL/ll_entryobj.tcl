#***************************************************************************
#***********                                                   *************
#***********         LL_ENTRYOBJ.TCL                           *************
#***********                                                   *************
#***************************************************************************
#  
#       Author: kp
#        $Date: 2000/03/17 15:08:42 $
#    $Revision: 1.1 $
#
#  Description: Build an entry in Low Level driver table from an ll obj file
#
#
# RESTRICTIONS: 
#
#     Required: cvs, rcsdiff, rlog
#
# Environment:
#				
#				
#
#-------------------------------[ History ]---------------------------------
#
# $Log: ll_entryobj.tcl,v $
# Revision 1.1  2000/03/17 15:08:42  kp
# Initial Revision
#
#---------------------------------------------------------------------------
# (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#***************************************************************************/


if { [llength $argv] < 5 } {
	puts stderr "Usage: ll_entryobj.tcl <nmbin> <thisdir> <ll_entry.c-path> <objfile> <objprefix>"
	exit 1
}

set nm   [lindex $argv 0]
set thisdir [lindex $argv 1]
set fd [open "[lindex $argv 2]" a]
set objfile [lindex $argv 3]
set objprefix [lindex $argv 4]

set nm [join [split $nm \\] /]
set thisdir [join [split $thisdir \\] /]


regsub .c$ [lindex $argv 2] i.h ll_entryi_h
set fd2 [open $ll_entryi_h a]


#
# Call nm to extract the name of the GetEntry function 
#

set syscmd "$nm --extern-only ../$objfile"
catch { eval exec $syscmd } nmOut	

# extract name of GetEntry function
set getEntry {}

foreach line [split $nmOut \n] {
	#puts "<$line>"

	if { [llength $line] == 3 } {
		if { ([lindex $line 1] == "T") && \
				([string match "*GetEntry*" [lindex $line 2]]) } {
			set getEntry [lindex $line 2]
			# remove any leading underscore from symbol
			if { [string index $getEntry 0] == "_" } {
				set getEntry [string range $getEntry 1 end] 
			}
			break
		}
	}
}

# guess mak_name from object file name
regsub $objprefix $objfile {} makeName
regsub {\.o} $makeName {} makeName
#puts "GetEntry: $getEntry makeName:$makeName"


#
# generate line in the entry file
#

# convert makeName to upper case. Make special name for Mxxx devices

set makeName [string toupper $makeName]
if {[regexp {^M[0-9]$} $makeName]} {
	set makeName "M00[string index $makeName 1]"
}

if {[regexp {^M[0-9][0-9]$} $makeName]} {
	set makeName "M0[string index $makeName 1][string index $makeName 2]"
}	
#puts "NEW MAKE $makeName"
puts $fd "  { \"$makeName\", $getEntry },"

puts "  -> $makeName $getEntry"

#
# generate declaration in header file
#
puts $fd2 "extern void $getEntry\(\);"

close $fd
close $fd2

exit 0
