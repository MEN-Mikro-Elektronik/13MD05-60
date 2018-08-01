#***************************************************************************
#***********                                                   *************
#***********         LL_ENTRYDRV.TCL                           *************
#***********                                                   *************
#***************************************************************************
#  
#       Author: kp
#        $Date: 2004/05/12 16:30:10 $
#    $Revision: 1.2 $
#
#  Description: Build an entry in Low Level driver table from common makefile
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
# $Log: ll_entrydrv.tcl,v $
# Revision 1.2  2004/05/12 16:30:10  UFranke
# changed
#  - remove leading 0 - M022_GetEntry now M22_GetEntry - MDIS4/2004
#
# Revision 1.1  2000/03/17 15:08:40  kp
# Initial Revision
#
#---------------------------------------------------------------------------
# (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#***************************************************************************/


if { [llength $argv] < 6 } {
	puts stderr "Usage: ll_entrydrv.tcl <makebin> <nmbin> <thisdir> <ll_entry.c-path> <COMMAKE> <objprefix>"
	exit 1
}

set make [lindex $argv 0]
set nm   [lindex $argv 1]
set thisdir [lindex $argv 2]
set fd [open "[lindex $argv 3]" a]
set commake [lindex $argv 4]
set objprefix [lindex $argv 5]

set make [join [split $make \\] /]
set nm [join [split $nm \\] /]
set thisdir [join [split $thisdir \\] /]


regsub .c$ [lindex $argv 3] i.h ll_entryi_h
set fd2 [open $ll_entryi_h a]


#
# Call make to get the MAK_NAME
#

set syscmd "$make -n -f ${thisdir}getmakname.mak COMMAKE=$commake"
catch { eval exec $syscmd } makeOut	

#puts "makeout: <$makeOut>"

set makeNameLine [split $makeOut =]

if { [lindex $makeNameLine 0] != "MAK_NAME" } {
	puts stderr "Couldn't determine MAK_NAME for $commake"
	exit 1
}

set makeName [lindex $makeNameLine 1]

#puts "My make name is $makeName, current dir is [pwd]"

#
# Call nm to extract the name of the GetEntry function 
#

set syscmd "$nm --extern-only ../$objprefix$makeName.o"
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

#puts "GetEntry: $getEntry"


#
# generate line in the entry file
#

# convert makeName to upper case. Make special name for Mxxx devices

set makeName [string toupper $makeName]
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

