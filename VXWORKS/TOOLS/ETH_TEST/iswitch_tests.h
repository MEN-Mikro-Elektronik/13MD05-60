

#ifndef __ISWITCH_TESTS_H
#define __ISWITCH_TESTS_H

/* functions */
int iSwitch_LoopSetup_pbvlan( u_int32 art );
int iSwitch_LoopTest_ul( u_int32 art );
int iSwitch_LoopTest_eth( u_int32 art );
int iSwitch_LoopTest_PSE( u_int32 art );

/*
int iSwitch_LoopTest_ST120( u_int32 art );
int iSwitch_LoopTest_ST125( u_int32 art );
int iSwitch_LoopTest_ST130( u_int32 art );
*/


#define MAX_TESTS 32

/* test cases */
enum 
{
	tc_pbvlan	= 0,
	tc_ethernet = 1,
	tc_uplink	= 2,
	tc_poe_pse	= 3,
};

typedef struct
{
	const char * name;
	int (* testfunc)(u_int32);
} matrix_testcase_desc;

extern const matrix_testcase_desc matrix_testcases[];
extern const u_int32 matrix_test_matrix[];

#endif

