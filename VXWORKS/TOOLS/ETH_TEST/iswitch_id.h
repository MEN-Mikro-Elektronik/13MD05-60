
/* 
 * Board identification
 */
#define F302P_NAME	"F302P"
#define AE31_NAME	"AE31-"

#define EEPROD_NAME_LEN 	5

/* which EEPROD structure? */
typedef enum 
{
	art_cpu_board,
	art_system,
	art_system_else,
	art_else
} id_art_type;

/* 
board type:
every type has it´s own testmenue
*/
typedef enum 
{
	id_cpu_pse = 1,
	id_cpu = 2,
	id_cpu_pse_ul = 3,
	id_cpu_pci = 4,
	id_fp_f = 5,
	id_fp_r = 6,
	id_fp_r_poe = 7,
	id_fp_r_ul_poe = 8,
	id_um_poe = 9,
	id_um_ul_poe = 10,
	id_um_fp_r = 11,
	id_um_fp_r_ul = 12,
	id_um = 13,
} id_board_type;

typedef struct {
	char		pci_uplink;
	char		poe;
	char		gb_uplink;
	char *		name;
	u_int8			model;
	id_art_type art_type;
	id_board_type board_type;
} id_board_desc;

enum
{
	err_null,
	err_name,
	err_day,
	err_month,
	err_year		
};

extern const id_board_desc article_matrix[];


u_int32  article_find(EEPROD2 * pEEPROD);
u_int32  eeprod_print(EEPROD2 * pEEPROD);




