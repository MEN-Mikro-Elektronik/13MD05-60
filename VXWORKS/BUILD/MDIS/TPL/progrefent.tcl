#***************************************************************************
#***********                                                   *************
#***********         PROGREFENT.TCL                            *************
#***********                                                   *************
#***************************************************************************
#  
#       Author: kp
#        $Date: 2000/03/17 15:08:45 $
#    $Revision: 1.1 $
#
#  Description: Build an entry in MdisProgRefs() dummy function for 
#               each program in the configuration
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
# $Log: progrefent.tcl,v $
# Revision 1.1  2000/03/17 15:08:45  kp
# Initial Revision
#
#---------------------------------------------------------------------------
# (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#***************************************************************************/


if { [llength $argv] < 4 } {
	puts stderr "Usage: ll_entrydrv.tcl <makebin> <thisdir> <prog_ref.c-path> <COMMAKE>"
	exit 1
}

set make [lindex $argv 0]
set thisdir [lindex $argv 1]
set fd [open "[lindex $argv 2]" a]
set commake [lindex $argv 3]

set make [join [split $make \\] /]
set thisdir [join [split $thisdir \\] /]


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
# generate line in the entry file
#

puts $fd "  $makeName\(\); "

puts "  -> $makeName"

close $fd

exit 0
