#include <types.h>
#include <stdio.h>
#include <MEN/men_typs.h>
#include <MEN/testutil.h>

tu_test1()
{
	int i;

	tu_init_screen("User defined string", 40, 6, 1);
#if 0
	tu_print_errhead("Pos     Detail");
	tu_key_line("Help");
	for(i=0;i<20; i++){
		if(tu_print_error( 2, 1, "%-8s %d", "AB", i )) break;
		tu_print_pass(i,100);
		tu_print_elapsed_time();
	}
#endif
	tu_exit();
}

tu_test2()
{
	int i;

	tu_init_screen("User defined string", 40, 6, 1);
	tu_print_errhead("Pos     Detail");

	tu_key_line("Help");
	for(i=0;i<20; i++){
		if(tu_print_error( 2, 1, "%-8s %d", "AB", i )) break;
		tu_print_pass(i,100);
		tu_print_elapsed_time();
	}
	tu_exit();
}

tu_test3()
{
	int i;

	tu_init_screen("User defined string", 40, 6, 1);
	tu_print_errhead("Pos     Detail");

	tu_key_line("Help");
	for(i=0;i<20; i++){
		if(tu_print_error( 2, 1, "%-8s %d", "AB", i )) break;
		tu_print_pass(i,100);
		tu_print_elapsed_time();
	}
	tu_clear_wdw( WDW_MSG );
	tu_exit();
}

