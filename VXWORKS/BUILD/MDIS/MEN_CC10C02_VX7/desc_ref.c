/* This file is generated automatically. Do not edit! */
#include <desc_refi.h>

char* MdisDescRefs() { 
	const volatile static char *descs[] = {
	  cpu, 
	  FPGA, 
	  gpio_1, 
	  gpio_10, 
	  gpio_11, 
	  gpio_12, 
	  gpio_2, 
	  gpio_3, 
	  gpio_4, 
	  gpio_5, 
	  gpio_6, 
	  gpio_7, 
	  gpio_8, 
	  gpio_9, 
	  owb_1, 
	  pwm_1, 
	  pwm_2, 
	  smb_z001_10, 
	  smb_z001_11, 
	  smb_z001_12, 
	  smb_z001_13, 
	  smb_z001_14, 
	  smb_z001_15, 
	  smb_z001_16, 
	  smb_z001_17, 
	  smb_z001_18, 
	  smb_z001_19, 
	  smb_z001_20, 
	  smb_z001_21, 
	  smb_z001_22, 
	  smb_z001_23, 
	  smb_z001_24, 
	  smb_z001_25, 
	  smb_z001_3, 
	  smb_z001_4, 
	  smb_z001_5, 
	  smb_z001_6, 
	  smb_z001_7, 
	  smb_z001_8, 
	  smb_z001_9, 
	  smb2_0, 
	  smb2_1, 
	  smb2_10, 
	  smb2_11, 
	  smb2_12, 
	  smb2_13, 
	  smb2_14, 
	  smb2_15, 
	  smb2_16, 
	  smb2_17, 
	  smb2_18, 
	  smb2_19, 
	  smb2_2, 
	  smb2_20, 
	  smb2_21, 
	  smb2_22, 
	  smb2_23, 
	  smb2_24, 
	  smb2_25, 
	  smb2_3, 
	  smb2_4, 
	  smb2_5, 
	  smb2_6, 
	  smb2_7, 
	  smb2_8, 
	  smb2_9, 
	  smbpci_nat_1, 
	  speed_1, 
0 };
   return( (void*) descs[0] );
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

	struct mdis_dsc_entries desc_entries[]= 
	{ 

		 { "cpu", cpu },
		 { "FPGA", FPGA },
		 { "gpio_1", gpio_1 },
		 { "gpio_10", gpio_10 },
		 { "gpio_11", gpio_11 },
		 { "gpio_12", gpio_12 },
		 { "gpio_2", gpio_2 },
		 { "gpio_3", gpio_3 },
		 { "gpio_4", gpio_4 },
		 { "gpio_5", gpio_5 },
		 { "gpio_6", gpio_6 },
		 { "gpio_7", gpio_7 },
		 { "gpio_8", gpio_8 },
		 { "gpio_9", gpio_9 },
		 { "owb_1", owb_1 },
		 { "pwm_1", pwm_1 },
		 { "pwm_2", pwm_2 },
		 { "smb_z001_10", smb_z001_10 },
		 { "smb_z001_11", smb_z001_11 },
		 { "smb_z001_12", smb_z001_12 },
		 { "smb_z001_13", smb_z001_13 },
		 { "smb_z001_14", smb_z001_14 },
		 { "smb_z001_15", smb_z001_15 },
		 { "smb_z001_16", smb_z001_16 },
		 { "smb_z001_17", smb_z001_17 },
		 { "smb_z001_18", smb_z001_18 },
		 { "smb_z001_19", smb_z001_19 },
		 { "smb_z001_20", smb_z001_20 },
		 { "smb_z001_21", smb_z001_21 },
		 { "smb_z001_22", smb_z001_22 },
		 { "smb_z001_23", smb_z001_23 },
		 { "smb_z001_24", smb_z001_24 },
		 { "smb_z001_25", smb_z001_25 },
		 { "smb_z001_3", smb_z001_3 },
		 { "smb_z001_4", smb_z001_4 },
		 { "smb_z001_5", smb_z001_5 },
		 { "smb_z001_6", smb_z001_6 },
		 { "smb_z001_7", smb_z001_7 },
		 { "smb_z001_8", smb_z001_8 },
		 { "smb_z001_9", smb_z001_9 },
		 { "smb2_0", smb2_0 },
		 { "smb2_1", smb2_1 },
		 { "smb2_10", smb2_10 },
		 { "smb2_11", smb2_11 },
		 { "smb2_12", smb2_12 },
		 { "smb2_13", smb2_13 },
		 { "smb2_14", smb2_14 },
		 { "smb2_15", smb2_15 },
		 { "smb2_16", smb2_16 },
		 { "smb2_17", smb2_17 },
		 { "smb2_18", smb2_18 },
		 { "smb2_19", smb2_19 },
		 { "smb2_2", smb2_2 },
		 { "smb2_20", smb2_20 },
		 { "smb2_21", smb2_21 },
		 { "smb2_22", smb2_22 },
		 { "smb2_23", smb2_23 },
		 { "smb2_24", smb2_24 },
		 { "smb2_25", smb2_25 },
		 { "smb2_3", smb2_3 },
		 { "smb2_4", smb2_4 },
		 { "smb2_5", smb2_5 },
		 { "smb2_6", smb2_6 },
		 { "smb2_7", smb2_7 },
		 { "smb2_8", smb2_8 },
		 { "smb2_9", smb2_9 },
		 { "smbpci_nat_1", smbpci_nat_1 },
		 { "speed_1", speed_1 },

		{ "", ((void*)0) }  
	};
	
	while(desc_entries[idx].descaddr != ((void*)0)) {
		if (!strncmp(desc_entries[idx].devname, devName, MAX_DESC_NAME_LEN))
		{		
			break;
		}
		idx++;
	}
		
   return( desc_entries[idx].descaddr );
}

