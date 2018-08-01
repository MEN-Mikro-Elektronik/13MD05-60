
#ifndef _SYSTEMTEST_SYSINFO_H
#define _SYSTEMTEST_SYSINFO_H

#define STRESS_SI_STRLEN_MAX	0x10

typedef int (* STRESS_SI_FUNC)(char *, STRESS_LOG_HDL *);
typedef struct {
	char name[STRESS_SI_STRLEN_MAX];
	STRESS_SI_FUNC 	func;
	char unit[STRESS_SI_STRLEN_MAX];
	char valstr[STRESS_SI_STRLEN_MAX];
}STRESS_SI_TYPE;

extern STRESS_SI_TYPE G_stressSiList[]; 

int stress_si_time(char * sP, STRESS_LOG_HDL * lP);
int stress_si_date(char * sP, STRESS_LOG_HDL * lP);
int stress_si_ttime(char * sP, STRESS_LOG_HDL * lP);
int stress_si_temp1(char * sP, STRESS_LOG_HDL * lP);
int stress_si_temp2(char * sP, STRESS_LOG_HDL * lP);
int stress_si_temp3(char * sP, STRESS_LOG_HDL * lP);
int stress_si_logint(char * sP, STRESS_LOG_HDL * lP);
int stress_si_idle(char * sP, STRESS_LOG_HDL * lP);
int stress_si_int(char * sP, STRESS_LOG_HDL * lP);
int stress_si_ram(char * sP, STRESS_LOG_HDL * lP);
int stress_si_space(char * sP, STRESS_LOG_HDL * lP);
int stress_si_v1(char * sP, STRESS_LOG_HDL * lP);
int stress_si_v1a(char * sP, STRESS_LOG_HDL * lP);
int stress_si_v1_2(char * sP, STRESS_LOG_HDL * lP);
int stress_si_v1_2a(char * sP, STRESS_LOG_HDL * lP);
int stress_si_v1_8(char * sP, STRESS_LOG_HDL * lP);
int stress_si_v2_5(char * sP, STRESS_LOG_HDL * lP);
int stress_si_v3_3(char * sP, STRESS_LOG_HDL * lP);
int stress_si_v5_1(char * sP, STRESS_LOG_HDL * lP);
int stress_si_vm12(char * sP, STRESS_LOG_HDL * lP);
int stress_si_v12(char * sP, STRESS_LOG_HDL * lP);
int stress_si_vm15(char * sP, STRESS_LOG_HDL * lP);
int stress_si_v15(char * sP, STRESS_LOG_HDL * lP);

#endif
