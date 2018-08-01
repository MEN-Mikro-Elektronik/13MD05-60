
if { [llength $argv] < 2 } {
	puts stderr "Usage: ll_entryend.tcl <ll_entry.c-path> LL|BB"
	exit 1
}


#puts [lindex $argv 0]
set fd [open [lindex $argv 0] a]

if { [lindex $argv 1] == "LL" } {
	set LL_ "LL_"
	set MK_ "MK_"
	set LLE "LL_"
} else {
	set LL_ "BB_"
	set MK_ "BK_"
	set LLE "BBIS_"
}

puts $fd \
"
 { NULL, NULL } /* do not remove this line */
};

/**************************** ${LL_}FindEntry **********************************
 *
 *  Description:  gets the ll driver routine entrys.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  devName    device name
 *
 *  Output.....:  drvP       ll drv entrys
 *                return 0 | error
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
int32 ${LLE}FindEntry( char* devName, ${LLE}ENTRY* drvP )
{
u_int32 index;
int32 retVal;

    retVal = 0;

    for( index=0;; index++)
    {
       if( ${LL_}DrvTable\[index\].name == 0 )
       {
           retVal = ERR_${MK_}NO_LLDRV; /* error - not found */
           break;
       }/*if*/

       if( !OSS_StrCmp( NULL, ${LL_}DrvTable\[index\].name, devName) )
       {
           ${LL_}DrvTable\[index\].getEntry( drvP );
           break;
       }/*if*/
    }/*for*/

    return( retVal );
}/*${LL_}FindEntry*/
"

close $fd

