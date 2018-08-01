
if { [llength $argv] < 1 } {
	puts stderr "Usage: descrefend.tcl <desc_ref.c-path>"
	exit 1
}


set fd [open [lindex $argv 0] a]

puts $fd "
		{ \"\", ((void*)0) }  
	};
	
	while(desc_entries\[idx\].descaddr != ((void*)0)) {
		if (!strncmp(desc_entries\[idx\].devname, devName, MAX_DESC_NAME_LEN))
		{		
			break;
		}
		idx++;
	}
		
   return( desc_entries\[idx\].descaddr );
}
"

close $fd
