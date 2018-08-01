/*
 * Generated by descgen V1.8 (MDIS descriptor generator)
 * Wed Jun  8 19:36:27 2011
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
    { DESC_STRING, 12, "HW_TYPE", "PC" },
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
    struct {  /* DEVICE_ID_1 */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DEVICE_ID_1;
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
    { DESC_U_INT32, 20, "DEVICE_IDV2_0", 0x00002201 },
    { DESC_U_INT32, 16, "DEVICE_ID_1", 0x00000801 },
    { DESC_U_INT32, 16, "DEBUG_LEVEL", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_BK", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_OSS", 0xc0008000 },
    { DESC_U_INT32, 24, "DEBUG_LEVEL_DESC", 0xc0008000 },
    { DESC_END, 0 }
};

/*==== Descriptor for "gpio_1" ====*/
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
    } __end_gpio_1;
} gpio_1 = {
    { DESC_U_INT32, 16, "DESC_TYPE", 0x00000001 },
    { DESC_STRING, 16, "HW_TYPE", "Z17_SW" },
    { DESC_STRING, 16, "BOARD_NAME", "fpga" },
    { DESC_U_INT32, 16, "DEVICE_SLOT", 0x00000000 },
    { DESC_U_INT32, 16, "DEBUG_LEVEL", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_MK", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_OSS", 0xc0008000 },
    { DESC_U_INT32, 24, "DEBUG_LEVEL_DESC", 0xc0008000 },
    { DESC_END, 0 }
};

/*==== Descriptor for "f202_2" ====*/
struct {
    struct {  /* DESC_TYPE */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DESC_TYPE;
    struct {  /* HW_TYPE */
        u_int16 __typ, __len; char __name[8];
        char __val[8];
    } HW_TYPE;
    struct {  /* PCI_BUS_PATH */
        u_int16 __typ, __len; char __name[13];
        u_int8 __val[3];
    } PCI_BUS_PATH;
    struct {  /* PCI_BUS_SLOT */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } PCI_BUS_SLOT;
    struct {  /* PCI_DEVICE_ID */
        u_int16 __typ, __len; char __name[16];
        u_int32 val;
    } PCI_DEVICE_ID;
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
    } __end_f202_2;
} f202_2 = {
    { DESC_U_INT32, 16, "DESC_TYPE", 0x00000002 },
    { DESC_STRING, 16, "HW_TYPE", "F202" },
    { DESC_BINARY, 16, "PCI_BUS_PATH",
        { 1 /*PF*/,0x1e }
    },
    { DESC_U_INT32, 20, "PCI_BUS_SLOT", 0x00000004 },
    { DESC_U_INT32, 20, "PCI_DEVICE_ID", 0x0000000d },
    { DESC_U_INT32, 16, "DEBUG_LEVEL", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_BK", 0xc0008000 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_OSS", 0xc0008000 },
    { DESC_U_INT32, 24, "DEBUG_LEVEL_DESC", 0xc0008000 },
    { DESC_END, 0 }
};

/*==== Descriptor for "m22_1" ====*/
struct {
    struct {  /* DESC_TYPE */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DESC_TYPE;
    struct {  /* HW_TYPE */
        u_int16 __typ, __len; char __name[8];
        char __val[4];
    } HW_TYPE;
    struct {  /* BOARD_NAME */
        u_int16 __typ, __len; char __name[11];
        char __val[9];
    } BOARD_NAME;
    struct {  /* DEVICE_SLOT */
        u_int16 __typ, __len; char __name[12];
        u_int32 val;
    } DEVICE_SLOT;

    /*--- directory CHANNEL_0 ---*/
    struct {
        struct {
            u_int16 __typ, __len; char __name[12];
        } __CHANNEL_0;
        struct {  /* INACTIVE */
            u_int16 __typ, __len; char __name[12];
            u_int32 val;
        } INACTIVE;
        struct {  /* INPUT_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } INPUT_EDGE_MASK;
        struct {  /* ALARM_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } ALARM_EDGE_MASK;
        struct {
            u_int16 __typ, __len;
        } __end_CHANNEL_0;
    } CHANNEL_0;

    /*--- directory CHANNEL_1 ---*/
    struct {
        struct {
            u_int16 __typ, __len; char __name[12];
        } __CHANNEL_1;
        struct {  /* INACTIVE */
            u_int16 __typ, __len; char __name[12];
            u_int32 val;
        } INACTIVE;
        struct {  /* INPUT_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } INPUT_EDGE_MASK;
        struct {  /* ALARM_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } ALARM_EDGE_MASK;
        struct {
            u_int16 __typ, __len;
        } __end_CHANNEL_1;
    } CHANNEL_1;

    /*--- directory CHANNEL_2 ---*/
    struct {
        struct {
            u_int16 __typ, __len; char __name[12];
        } __CHANNEL_2;
        struct {  /* INACTIVE */
            u_int16 __typ, __len; char __name[12];
            u_int32 val;
        } INACTIVE;
        struct {  /* INPUT_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } INPUT_EDGE_MASK;
        struct {  /* ALARM_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } ALARM_EDGE_MASK;
        struct {
            u_int16 __typ, __len;
        } __end_CHANNEL_2;
    } CHANNEL_2;

    /*--- directory CHANNEL_3 ---*/
    struct {
        struct {
            u_int16 __typ, __len; char __name[12];
        } __CHANNEL_3;
        struct {  /* INACTIVE */
            u_int16 __typ, __len; char __name[12];
            u_int32 val;
        } INACTIVE;
        struct {  /* INPUT_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } INPUT_EDGE_MASK;
        struct {  /* ALARM_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } ALARM_EDGE_MASK;
        struct {
            u_int16 __typ, __len;
        } __end_CHANNEL_3;
    } CHANNEL_3;

    /*--- directory CHANNEL_4 ---*/
    struct {
        struct {
            u_int16 __typ, __len; char __name[12];
        } __CHANNEL_4;
        struct {  /* INACTIVE */
            u_int16 __typ, __len; char __name[12];
            u_int32 val;
        } INACTIVE;
        struct {  /* INPUT_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } INPUT_EDGE_MASK;
        struct {  /* ALARM_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } ALARM_EDGE_MASK;
        struct {
            u_int16 __typ, __len;
        } __end_CHANNEL_4;
    } CHANNEL_4;

    /*--- directory CHANNEL_5 ---*/
    struct {
        struct {
            u_int16 __typ, __len; char __name[12];
        } __CHANNEL_5;
        struct {  /* INACTIVE */
            u_int16 __typ, __len; char __name[12];
            u_int32 val;
        } INACTIVE;
        struct {  /* INPUT_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } INPUT_EDGE_MASK;
        struct {  /* ALARM_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } ALARM_EDGE_MASK;
        struct {
            u_int16 __typ, __len;
        } __end_CHANNEL_5;
    } CHANNEL_5;

    /*--- directory CHANNEL_6 ---*/
    struct {
        struct {
            u_int16 __typ, __len; char __name[12];
        } __CHANNEL_6;
        struct {  /* INACTIVE */
            u_int16 __typ, __len; char __name[12];
            u_int32 val;
        } INACTIVE;
        struct {  /* INPUT_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } INPUT_EDGE_MASK;
        struct {  /* ALARM_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } ALARM_EDGE_MASK;
        struct {
            u_int16 __typ, __len;
        } __end_CHANNEL_6;
    } CHANNEL_6;

    /*--- directory CHANNEL_7 ---*/
    struct {
        struct {
            u_int16 __typ, __len; char __name[12];
        } __CHANNEL_7;
        struct {  /* INACTIVE */
            u_int16 __typ, __len; char __name[12];
            u_int32 val;
        } INACTIVE;
        struct {  /* INPUT_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } INPUT_EDGE_MASK;
        struct {  /* ALARM_EDGE_MASK */
            u_int16 __typ, __len; char __name[16];
            u_int32 val;
        } ALARM_EDGE_MASK;
        struct {
            u_int16 __typ, __len;
        } __end_CHANNEL_7;
    } CHANNEL_7;
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
    } __end_m22_1;
} m22_1 = {
    { DESC_U_INT32, 16, "DESC_TYPE", 0x00000001 },
    { DESC_STRING, 12, "HW_TYPE", "M22" },
    { DESC_STRING, 20, "BOARD_NAME", "f202_2" },
    { DESC_U_INT32, 16, "DEVICE_SLOT", 0x00000000 },
    {
        { DESC_DIR, 12, "CHANNEL_0" },
        { DESC_U_INT32, 16, "INACTIVE", 0x00000001 },
        { DESC_U_INT32, 20, "INPUT_EDGE_MASK", 0x00000003 },
        { DESC_U_INT32, 20, "ALARM_EDGE_MASK", 0x00000003 },
        { DESC_END, 0 }
    },
    {
        { DESC_DIR, 12, "CHANNEL_1" },
        { DESC_U_INT32, 16, "INACTIVE", 0x00000001 },
        { DESC_U_INT32, 20, "INPUT_EDGE_MASK", 0x00000003 },
        { DESC_U_INT32, 20, "ALARM_EDGE_MASK", 0x00000003 },
        { DESC_END, 0 }
    },
    {
        { DESC_DIR, 12, "CHANNEL_2" },
        { DESC_U_INT32, 16, "INACTIVE", 0x00000001 },
        { DESC_U_INT32, 20, "INPUT_EDGE_MASK", 0x00000003 },
        { DESC_U_INT32, 20, "ALARM_EDGE_MASK", 0x00000003 },
        { DESC_END, 0 }
    },
    {
        { DESC_DIR, 12, "CHANNEL_3" },
        { DESC_U_INT32, 16, "INACTIVE", 0x00000001 },
        { DESC_U_INT32, 20, "INPUT_EDGE_MASK", 0x00000003 },
        { DESC_U_INT32, 20, "ALARM_EDGE_MASK", 0x00000003 },
        { DESC_END, 0 }
    },
    {
        { DESC_DIR, 12, "CHANNEL_4" },
        { DESC_U_INT32, 16, "INACTIVE", 0x00000001 },
        { DESC_U_INT32, 20, "INPUT_EDGE_MASK", 0x00000003 },
        { DESC_U_INT32, 20, "ALARM_EDGE_MASK", 0x00000003 },
        { DESC_END, 0 }
    },
    {
        { DESC_DIR, 12, "CHANNEL_5" },
        { DESC_U_INT32, 16, "INACTIVE", 0x00000001 },
        { DESC_U_INT32, 20, "INPUT_EDGE_MASK", 0x00000003 },
        { DESC_U_INT32, 20, "ALARM_EDGE_MASK", 0x00000003 },
        { DESC_END, 0 }
    },
    {
        { DESC_DIR, 12, "CHANNEL_6" },
        { DESC_U_INT32, 16, "INACTIVE", 0x00000001 },
        { DESC_U_INT32, 20, "INPUT_EDGE_MASK", 0x00000003 },
        { DESC_U_INT32, 20, "ALARM_EDGE_MASK", 0x00000003 },
        { DESC_END, 0 }
    },
    {
        { DESC_DIR, 12, "CHANNEL_7" },
        { DESC_U_INT32, 16, "INACTIVE", 0x00000001 },
        { DESC_U_INT32, 20, "INPUT_EDGE_MASK", 0x00000003 },
        { DESC_U_INT32, 20, "ALARM_EDGE_MASK", 0x00000003 },
        { DESC_END, 0 }
    },
    { DESC_U_INT32, 16, "DEBUG_LEVEL", 0xc0008003 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_MK", 0xc0008001 },
    { DESC_U_INT32, 20, "DEBUG_LEVEL_OSS", 0xc0008000 },
    { DESC_U_INT32, 24, "DEBUG_LEVEL_DESC", 0xc0008000 },
    { DESC_END, 0 }
};
