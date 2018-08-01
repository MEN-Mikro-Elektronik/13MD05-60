/*
 * Generated by descgen V1.8 (MDIS descriptor generator)
 * Tue Aug 18 15:50:23 2015
 * You can edit this file on your own risk. You should
 * better edit the descgen input file
 */

#include <MEN/men_typs.h>
#include <MEN/desctyps.h>

/*==== Descriptor for "cpu" ====*/
struct {
    struct {  /* DESC_TYPE */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DESC_TYPE;
    struct {  /* HW_TYPE */
        u_int16 __typ, __len; char __name[8];
        char __val[4];
    } HW_TYPE;
    struct {
        u_int16 __typ, __len;
    } __end_cpu;
} cpu = {
    { DESC_U_INT32, 16, "DESC_TYPE", 0x00000000 },
    { DESC_STRING, 12, "HW_TYPE", "A17" },
    { DESC_END, 0 }
};

/*==== Descriptor for "fpga" ====*/
struct {
    struct {  /* DESC_TYPE */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DESC_TYPE;
    struct {  /* HW_TYPE */
        u_int16 __typ, __len; char __name[8];
        char __val[20];
    } HW_TYPE;
    struct {  /* PCI_BUS_NUMBER */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } PCI_BUS_NUMBER;
    struct {  /* PCI_DEVICE_NUMBER */
        u_int16 __typ, __len; char __name[20];
        u_int32 val;
    } PCI_DEVICE_NUMBER;
    struct {  /* DEVICE_IDV2_0 */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } DEVICE_IDV2_0;
    struct {  /* DEBUG_LEVEL */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DEBUG_LEVEL;
    struct {  /* DEBUG_LEVEL_BK */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } DEBUG_LEVEL_BK;
    struct {  /* DEBUG_LEVEL_OSS */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } DEBUG_LEVEL_OSS;
    struct {  /* DEBUG_LEVEL_DESC */
        u_int16 __typ, __len; char __name[20];
        u_int32 val;
    } DEBUG_LEVEL_DESC;
    struct {
        u_int16 __typ, __len;
    } __end_fpga;
} fpga = {
    { DESC_U_INT32, 16, "DESC_TYPE", 0x00000002 },
    { DESC_STRING, 28, "HW_TYPE", "CHAMELEON_PCITBL" },
    { DESC_U_INT32, 20, "PCI_BUS_NUMBER", 0x00000000 },
    { DESC_U_INT32, 24, "PCI_DEVICE_NUMBER", 0x0000001c },
    { DESC_U_INT32, 20, "DEVICE_IDV2_0", 0x00007f00 },
    { DESC_U_INT32, 16, "DEBUG_LEVEL", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_BK", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_OSS", 0xc0008000 },
    { DESC_U_INT32, 24, "DEBUG_LEVEL_DESC", 0xc0008000 },
    { DESC_END, 0 }
};

/*==== Descriptor for "gpio" ====*/
struct {
    struct {  /* DESC_TYPE */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DESC_TYPE;
    struct {  /* HW_TYPE */
        u_int16 __typ, __len; char __name[8];
        char __val[12];
    } HW_TYPE;
    struct {  /* BOARD_NAME */
        u_int16 __typ, __len; char __name[11];
        char __val[5];
    } BOARD_NAME;
    struct {  /* DEVICE_SLOT */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DEVICE_SLOT;
    struct {  /* DEBUG_LEVEL */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DEBUG_LEVEL;
    struct {  /* DEBUG_LEVEL_MK */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } DEBUG_LEVEL_MK;
    struct {  /* DEBUG_LEVEL_OSS */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } DEBUG_LEVEL_OSS;
    struct {  /* DEBUG_LEVEL_DESC */
        u_int16 __typ, __len; char __name[20];
        u_int32 val;
    } DEBUG_LEVEL_DESC;
    struct {
        u_int16 __typ, __len;
    } __end_gpio;
} gpio = {
    { DESC_U_INT32, 16, "DESC_TYPE", 0x00000001 },
    { DESC_STRING, 20, "HW_TYPE", "Z17_Z127_SW" },
    { DESC_STRING, 16, "BOARD_NAME", "fpga" },
    { DESC_U_INT32, 16, "DEVICE_SLOT", 0x00000000 },
    { DESC_U_INT32, 16, "DEBUG_LEVEL", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_MK", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_OSS", 0xc0008000 },
    { DESC_U_INT32, 24, "DEBUG_LEVEL_DESC", 0xc0008000 },
    { DESC_END, 0 }
};

/*==== Descriptor for "obpmc" ====*/
struct {
    struct {  /* DESC_TYPE */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DESC_TYPE;
    struct {  /* HW_TYPE */
        u_int16 __typ, __len; char __name[8];
        char __val[4];
    } HW_TYPE;
    struct {  /* PCI_BUS_NUMBER */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } PCI_BUS_NUMBER;
    struct {  /* DEVICE_SLOT_0 */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } DEVICE_SLOT_0;
    struct {  /* DEVICE_SLOT_1 */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } DEVICE_SLOT_1;
    struct {  /* DEBUG_LEVEL */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DEBUG_LEVEL;
    struct {  /* DEBUG_LEVEL_BK */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } DEBUG_LEVEL_BK;
    struct {  /* DEBUG_LEVEL_OSS */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } DEBUG_LEVEL_OSS;
    struct {  /* DEBUG_LEVEL_DESC */
        u_int16 __typ, __len; char __name[20];
        u_int32 val;
    } DEBUG_LEVEL_DESC;
    struct {
        u_int16 __typ, __len;
    } __end_obpmc;
} obpmc = {
    { DESC_U_INT32, 16, "DESC_TYPE", 0x00000002 },
    { DESC_STRING, 12, "HW_TYPE", "PCI" },
    { DESC_U_INT32, 20, "PCI_BUS_NUMBER", 0x00000000 },
    { DESC_U_INT32, 20, "DEVICE_SLOT_0", 0x0000001e },
    { DESC_U_INT32, 20, "DEVICE_SLOT_1", 0x0000001b },
    { DESC_U_INT32, 16, "DEBUG_LEVEL", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_BK", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_OSS", 0xc0008000 },
    { DESC_U_INT32, 24, "DEBUG_LEVEL_DESC", 0xc0008000 },
    { DESC_END, 0 }
};

/*==== Descriptor for "smb2" ====*/
struct {
    struct {  /* DESC_TYPE */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DESC_TYPE;
    struct {  /* HW_TYPE */
        u_int16 __typ, __len; char __name[8];
        char __val[8];
    } HW_TYPE;
    struct {  /* BOARD_NAME */
        u_int16 __typ, __len; char __name[11];
        char __val[5];
    } BOARD_NAME;
    struct {  /* DEVICE_SLOT */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DEVICE_SLOT;
    struct {  /* SMB_BUSNBR */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } SMB_BUSNBR;
    struct {  /* DEBUG_LEVEL */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DEBUG_LEVEL;
    struct {  /* DEBUG_LEVEL_MK */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } DEBUG_LEVEL_MK;
    struct {  /* DEBUG_LEVEL_OSS */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } DEBUG_LEVEL_OSS;
    struct {  /* DEBUG_LEVEL_DESC */
        u_int16 __typ, __len; char __name[20];
        u_int32 val;
    } DEBUG_LEVEL_DESC;
    struct {
        u_int16 __typ, __len;
    } __end_smb2;
} smb2 = {
    { DESC_U_INT32, 16, "DESC_TYPE", 0x00000001 },
    { DESC_STRING, 16, "HW_TYPE", "SMB2" },
    { DESC_STRING, 16, "BOARD_NAME", "smb2" },
    { DESC_U_INT32, 16, "DEVICE_SLOT", 0x00000000 },
    { DESC_U_INT32, 16, "SMB_BUSNBR", 0x00000000 },
    { DESC_U_INT32, 16, "DEBUG_LEVEL", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_MK", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_OSS", 0xc0008000 },
    { DESC_U_INT32, 24, "DEBUG_LEVEL_DESC", 0xc0008000 },
    { DESC_END, 0 }
};

/*==== Descriptor for "bmc_1" ====*/
struct {
    struct {  /* DESC_TYPE */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DESC_TYPE;
    struct {  /* HW_TYPE */
        u_int16 __typ, __len; char __name[8];
        char __val[8];
    } HW_TYPE;
    struct {  /* BOARD_NAME */
        u_int16 __typ, __len; char __name[11];
        char __val[5];
    } BOARD_NAME;
    struct {  /* DEVICE_SLOT */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DEVICE_SLOT;
    struct {  /* SMB_BUSNBR */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } SMB_BUSNBR;
    struct {  /* DEBUG_LEVEL */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DEBUG_LEVEL;
    struct {  /* DEBUG_LEVEL_MK */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } DEBUG_LEVEL_MK;
    struct {  /* DEBUG_LEVEL_OSS */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } DEBUG_LEVEL_OSS;
    struct {  /* DEBUG_LEVEL_DESC */
        u_int16 __typ, __len; char __name[20];
        u_int32 val;
    } DEBUG_LEVEL_DESC;
    struct {
        u_int16 __typ, __len;
    } __end_bmc_1;
} bmc_1 = {
    { DESC_U_INT32, 16, "DESC_TYPE", 0x00000001 },
    { DESC_STRING, 16, "HW_TYPE", "SMB2" },
    { DESC_STRING, 16, "BOARD_NAME", "smb2" },
    { DESC_U_INT32, 16, "DEVICE_SLOT", 0x00000000 },
    { DESC_U_INT32, 16, "SMB_BUSNBR", 0x00000000 },
    { DESC_U_INT32, 16, "DEBUG_LEVEL", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_MK", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_OSS", 0xc0008000 },
    { DESC_U_INT32, 24, "DEBUG_LEVEL_DESC", 0xc0008000 },
    { DESC_END, 0 }
};
