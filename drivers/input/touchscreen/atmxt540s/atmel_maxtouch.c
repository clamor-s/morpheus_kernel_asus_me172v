/*
 *  Atmel maXTouch Touchscreen Controller
 *
 *
 *  Copyright (C) 2010 Atmel Corporation
 *  Copyright (C) 2009 Raphael Derosso Pereira <raphaelpereira@gmail.com>
 *  Copyright (C) 2011 NVIDIA Corporation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/debugfs.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/earlysuspend.h>
#include <linux/delay.h>

#include <linux/uaccess.h>
#include "./atmel_maxtouch.h"
#include <linux/miscdevice.h>

#include <linux/poll.h>
#include <linux/kfifo.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/switch.h>

#include <mach/hardware.h>
#include "atmel_firmware.h"
/*
 * This is a driver for the Atmel maXTouch Object Protocol
 *
 * When the driver is loaded, mxt_init is called.
 * mxt_driver registers the "mxt_driver" structure in the i2c subsystem
 * The mxt_idtable.name string allows the board support to associate
 * the driver with its own data.
 *
 * The i2c subsystem will call the mxt_driver.probe == mxt_probe
 * to detect the device.
 * mxt_probe will reset the maXTouch device, and then
 * determine the capabilities of the I2C peripheral in the
 * host processor (needs to support BYTE transfers)
 *
 * If OK; mxt_probe will try to identify which maXTouch device it is
 * by calling mxt_identify.
 *
 * If a known device is found, a linux input device is initialized
 * the "mxt" device data structure is allocated,
 * as well as an input device structure "mxt->input"
 * "mxt->client" is provided as a parameter to mxt_probe.
 *
 * mxt_read_object_table is called to determine which objects
 * are present in the device, and to determine their adresses.
 *
 *
 * Addressing an object:
 *
 * The object is located at a 16-bit address in the object address space.
 *
 * The address is provided through an object descriptor table in the beginning
 * of the object address space. This address can change between firmware
 * revisions, so it's important that the driver will make no assumptions
 * about addresses but instead reads the object table and gets the correct
 * addresses there.
 *
 * Each object type can have several instances, and the number of
 * instances is available in the object table as well.
 *
 * The base address of the first instance of an object is stored in
 * "mxt->object_table[object_type].chip_addr",
 * This is indexed by the object type and allows direct access to the
 * first instance of an object.
 *
 * Each instance of an object is assigned a "Report Id" uniquely identifying
 * this instance. Information about this instance is available in the
 * "mxt->report_id" variable, which is a table indexed by the "Report Id".
 *
 * The maXTouch object protocol supports adding a checksum to messages.
 * By setting the most significant bit of the maXTouch address,
 * an 8 bit checksum is added to all writes.
 *
 *
 * How to use driver.
 * -----------------
 * Example:
 * In arch/avr32/boards/atstk1000/atstk1002.c
 * an "i2c_board_info" descriptor is declared.
 * This contains info about which driver ("mXT224"),
 * which i2c address and which pin for CHG interrupts are used.
 *
 * In the "atstk1002_init" routine, "i2c_register_board_info" is invoked
 * with this information. Also, the I/O pins are configured, and the I2C
 * controller registered is on the application processor.
 *
 *
 */
#define WRITE_MEM_OK		0x01
#define WRITE_MEM_FAILED	0x02
#define READ_MEM_OK		0x01
#define READ_MEM_FAILED	0x02

#define FW_WAITING_BOOTLOAD_COMMAND 0xC0
#define FW_WAITING_FRAME_DATA       0x80
#define FW_FRAME_CRC_CHECK          0x02
#define FW_FRAME_CRC_PASS           0x04
#define FW_FRAME_CRC_FAIL           0x03

#define I2C_M_WR 0
#define I2C_MAX_SEND_LENGTH     600
#define MXT_IOC_MAGIC 0xC9
#define MXT_IOC_MAXNR 16
#define MXT_FW_UPDATE _IOW(MXT_IOC_MAGIC,2,int )
#define MXT_FW_VERSION _IOR(MXT_IOC_MAGIC,3,int)
#define MXT_SET_FW_SIZE _IOW(MXT_IOC_MAGIC,4,int)
#define MXT_GOLDEN_CALIBRATION _IOW(MXT_IOC_MAGIC,5,int)
#define MXT_CALIBRATION_STATUS _IOR(MXT_IOC_MAGIC,6,int)
#define MXT_HW_RESET _IOW(MXT_IOC_MAGIC,7,int)
#define MXT_SW_RESET _IOW(MXT_IOC_MAGIC,8,int)
#define MXT_CALIBRATE _IOW(MXT_IOC_MAGIC,9,int)
#define MXT_BACKUP _IOW(MXT_IOC_MAGIC,10,int)
#define MXT_SELFTEST _IOW(MXT_IOC_MAGIC,11,int)
#define MXT_SELFTEST_STATUS _IOW(MXT_IOC_MAGIC,12,int)
#define MXT_FW_DEFAULT_UPDATE _IOW(MXT_IOC_MAGIC,13,int)
#define MXT_GET_CONFIG_CHECKOSUM _IOW(MXT_IOC_MAGIC,14,int)
#define MXT_CLEAR_CONFIG _IOW(MXT_IOC_MAGIC,15,int)
#define MXT_LOAD_NEWER_CONFIG _IOW(MXT_IOC_MAGIC,16,int)



#define MAX_X_SCAN	600
#define MAX_Y_SCAN	1024

#define I2C_RETRY_COUNT 5
#define I2C_PAYLOAD_SIZE 254


/* Maps a report ID to an object type (object type number). */
#define	REPORT_ID_TO_OBJECT(rid, mxt)			\
	(((rid) == 0xff) ? 0 : mxt->rid_map[rid].object)

/* Maps a report ID to an object type (string). */
#define	REPORT_ID_TO_OBJECT_NAME(rid, mxt)			\
	object_type_name[REPORT_ID_TO_OBJECT(rid, mxt)]

/* Returns non-zero if given object is a touch object */
#define IS_TOUCH_OBJECT(object) \
	((object == MXT_TOUCH_MULTITOUCHSCREEN_T9) || \
	 (object == MXT_TOUCH_KEYARRAY_T15) ||	\
	 (object == MXT_TOUCH_PROXIMITY_T23) ? 1 : 0)

#define mxt_debug(level, ...) \
	do { \
		if (debug >= (level)) \
			pr_debug(__VA_ARGS__); \
	} while (0)

static long FW_SIZE=0;
struct i2c_client *mxt_client;
struct mxt_data *global_mxt=NULL;
u32 fw_checksum;
u32 cfg_crc;
static int debug = NO_DEBUG;
// static int debug = DEBUG_RAW;
static int comms;
static bool cfg_flag;
static bool reset_flag;
static u8 touch_vendor_id;
static u8 boot_cali_status;
static bool selftest_flag;
static int mxt_firmware_id;
static int calibration_flag;
static struct timer_list ts_timer_read_dummy;
static int d_flag=0;
static int dummy_count = 0;
static atomic_t touch_char_available = ATOMIC_INIT(1);
static int T7_BYTE_0_IDLEACQINT = 0;
static int T7_BYTE_1_ACTVACQINT = 0;
//module_param(debug, int, 0644);
//module_param(comms, int, 0644);

//MODULE_PARM_DESC(debug, "Activate debugging output");
//MODULE_PARM_DESC(comms, "Select communications mode");

/* Mapping from report id to object type and instance */
struct report_id_map {
	u8 object;
	u8 instance;
/*
 * This is the first report ID belonging to object. It enables us to
 * find out easily the touch number: each touch has different report
 * ID (which are assigned to touches in increasing order). By
 * subtracting the first report ID from current, we get the touch
 * number.
 */
	u8 first_rid;
};

static const u8 *object_type_name[] = {
	[0] = "Reserved",
	[5] = "GEN_MESSAGEPROCESSOR_T5",
	[6] = "GEN_COMMANDPROCESSOR_T6",
	[7] = "GEN_POWERCONFIG_T7",
	[8] = "GEN_ACQUIRECONFIG_T8",
	[9] = "TOUCH_MULTITOUCHSCREEN_T9",
	[15] = "TOUCH_KEYARRAY_T15",
	[18] = "SPT_COMMSCONFIG_T18",
	[19] = "SPT_GPIOPWM_T19",
	[23] = "TOUCH_PROXIMITY_T23",
	[25] = "SPT_SELFTEST_T25",
	[37] = "DEBUG_DIAGNOSTICS_T37",
	[38] = "SPT_USER_DATA_T38",
	[40] = "PROCI_GRIPSUPPRESSION_T40",
	[42] = "PROCI_TOUCHSUPPRESSION_T42",
	[44] = "SPT_MESSAGECOUNT_T44",
	[46] = "SPT_CTECONFIG_T46",
	[47] = "PROCI_STYLUS_T47",
	[55] = "PROCI_ADAPTIVETHRESHOLD_T55",
	[56] = "PROCI_SHIELDLESS_T56", 
	[57] = "PROCI_EXTRATOUCHAREADATA_T57",
	[61] = "SPT_TIMER_T61",
	[62] = "PROCG_NOISESUPPRESSION_T62",
	[65] = "LENSBENDING_T65",
	[66] = "GOLDENREFERENCES_T66",
	[69] = "PALMGESTUREPROCESSOR_T69"
};
typedef struct
{
	u16 size_id;
	u16  pressure;
	u16 x;
	u16 y;
} report_finger_info_struct;
static report_finger_info_struct fingerInfo[10];

//static u16 get_object_address(uint8_t object_type,
//			      uint8_t instance,
//			      struct mxt_object *object_table, int max_objs);

static int mxt_read_block_wo_addr(struct i2c_client *client,
				  u16 length, u8 *value);
/* Routines for memory access within a 16 bit address space */

//static int mxt_write_byte(struct i2c_client *client, u16 addr, u8 value);
static int mxt_write_block(struct i2c_client *client, u16 addr, u16 length,
			   u8 *value);
static int mxt_init_Boot(struct mxt_data *mxt, int mode, void * fw_ptr);
static int __devinit mxt_read_object_table(struct i2c_client *client,
					   struct mxt_data *mxt,
					   u8 *raw_id_data);
/* TODO: */
/* Bootloader specific function prototypes. */
/*
static int mxt_read_byte_bl(struct i2c_client *client, u8 * value)
	{ return 0; }
static int mxt_read_block_bl(struct i2c_client *client, u16 length,
		u8 * value) { return 0; }
static int mxt_write_byte_bl(struct i2c_client *client, u8 value)
	{ return 0; }
static int mxt_write_block_bl(struct i2c_client *client, u16 length,
		u8 *value) { return 0; }
*/

static u8 asus_read_chg(void);
static u8 asus_valid_interrupt(void);

static u8 mxt_valid_interrupt_dummy(void)
{
	return 1;
}
#define DBG_MODULE	0x00000001
#define DBG_CDEV	0x00000002
#define DBG_PROC	0x00000004
#define DBG_PARSER	0x00000020
#define DBG_SUSP	0x00000040
#define DBG_CONST	0x00000100
#define DBG_IDLE	0x00000200
#define DBG_WAKEUP	0x00000400


static unsigned int DbgLevel = DBG_MODULE|DBG_SUSP| DBG_CDEV|DBG_PROC;
#define TOUCH_DBG(level, fmt, args...)  { if( (level&DbgLevel)>0 ) \
					printk( KERN_DEBUG "[touch] " fmt, ## args); }
#define DEBUG_MESSAGE 1
#define TOUCH_PRINTK(fmt, args...)  { if(DEBUG_MESSAGE) \
					printk( KERN_ERR "[touch] " fmt, ## args); }


extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
extern int wmt_setsyspara(char *varname, unsigned char *varval);


static struct asus_config ac={
	.selftest_calibration_status =0,
	.calibration_status =0,
	.touch_panel_config_checksum[OFILM_GFF]=OFILM_GFF_DEFAULT,
	.touch_panel_config_checksum[UNUSE1]=UNUSE1_DEFAULT,
	.touch_panel_config_checksum[JTOUCH]=JTOUCH_DEFAULT,
	.touch_panel_config_checksum[OFILM] =OFILM_DEFAULT,
	.tp.TP_ID0 =1,
	.tp.TP_ID0 =1,
};

struct wmt_gpt_s {
        char name[20];
        unsigned int bitmap;
        unsigned int ctraddr;
        unsigned int ocaddr;
        unsigned int idaddr;
        unsigned int peaddr;
        unsigned int pcaddr;
        unsigned int itbmp;
        unsigned int itaddr;
        unsigned int isbmp;
        unsigned int isaddr;
        unsigned int irq;
};

		//0xFE1100C0 	//GP0 Output Data Register for Dedicated [7:0]
static struct wmt_gpt_s wmt_ts_gpt ={
	.name = "mxt768",
	.bitmap = 0x3200,/*irq GPIO 9, tp_id0 gpio12 tp_id1 gpio13*/
	.ctraddr = 0xFE110040,	//GP0 Enable Register for Dedicated [7:0]
	.ocaddr = 0xFE110080,	//GP0 Output Enable Register for Dedicated [7:0]	//<ASUS-Darren1_Chang20121004+> from 0xFF10080 to 0xFF110080
	.idaddr = 0xFE110000,	//GP0 Input Data Register for Dedicated [7:0]
	.peaddr = 0xFE110480,	//GP0 pull-up/down Enable Register for Dedciated GPIO [7:0] 
	.pcaddr = 0xFE1104c0,	//GP0 pull-up/down Control Register for Dedicated GPIO [7:0]
	.itbmp = 0x00008000, /* 0x01 bit 8-10: 001b: one, 000b:zero, (4) 100b:edge low level */
	.itaddr = 0xFE110308,	//GPIO interrupt request type register
	.isbmp = 0x200,
	.isaddr = 0xFE110360,	//GPIO interrupt Request Status Register [7:0]
	.irq = 5,
};

#define SET_GPIO_TS_INT() {\
	REG32_VAL(wmt_ts_gpt.ctraddr) |= wmt_ts_gpt.bitmap; \
	REG32_VAL(wmt_ts_gpt.ocaddr) &= ~wmt_ts_gpt.bitmap; \
	REG32_VAL(wmt_ts_gpt.pcaddr) &= ~wmt_ts_gpt.bitmap; \
	REG32_VAL(wmt_ts_gpt.peaddr) &= ~wmt_ts_gpt.bitmap; \
	REG32_VAL(wmt_ts_gpt.isaddr) = wmt_ts_gpt.isbmp; \
}



static void ts_set_gpio_int(void)
{
	if (wmt_ts_gpt.itbmp & 0xff  || wmt_ts_gpt.itbmp == 0) {
		REG32_VAL(wmt_ts_gpt.itaddr) &= ~0xff;
		REG32_VAL(wmt_ts_gpt.itaddr) |= (wmt_ts_gpt.itbmp & 0x7F);
	} else if (wmt_ts_gpt.itbmp & 0xff00) {
		REG32_VAL(wmt_ts_gpt.itaddr) &= ~0xff00;
		REG32_VAL(wmt_ts_gpt.itaddr) |= (wmt_ts_gpt.itbmp & 0x7F00);
	} else if (wmt_ts_gpt.itbmp & 0xff0000) {
		REG32_VAL(wmt_ts_gpt.itaddr) &= ~0xff0000;
		REG32_VAL(wmt_ts_gpt.itaddr) |= (wmt_ts_gpt.itbmp & 0x7F0000);
	} else if (wmt_ts_gpt.itbmp & 0xff000000) {
		REG32_VAL(wmt_ts_gpt.itaddr) &= ~0xff000000;
		REG32_VAL(wmt_ts_gpt.itaddr) |= (wmt_ts_gpt.itbmp & 0x7F000000);
	}
	SET_GPIO_TS_INT();
}


static void ts_clear_irq(void)
{
        REG32_VAL(wmt_ts_gpt.isaddr) |= wmt_ts_gpt.isbmp;
}

static void ts_enable_irq(void)
{
//   		REG32_VAL(wmt_ts_gpt.isaddr) |= wmt_ts_gpt.isbmp;
		if (wmt_ts_gpt.itbmp & 0xff || wmt_ts_gpt.itbmp == 0)
			REG32_VAL(wmt_ts_gpt.itaddr) |= 0x80;
		else if (wmt_ts_gpt.itbmp & 0xff00)
			REG32_VAL(wmt_ts_gpt.itaddr) |= 0x8000;
		else if (wmt_ts_gpt.itbmp & 0xff0000)
			REG32_VAL(wmt_ts_gpt.itaddr) |= 0x800000;
		else if (wmt_ts_gpt.itbmp & 0xff000000)
			REG32_VAL(wmt_ts_gpt.itaddr) |= 0x80000000;
}


static inline int ts_check_irq_enable(void)
{
	if (wmt_ts_gpt.itbmp & 0xff)
		return REG32_VAL(wmt_ts_gpt.itaddr) & 0x80;
	else if (wmt_ts_gpt.itbmp & 0xff00)
		return REG32_VAL(wmt_ts_gpt.itaddr) & 0x8000;
	else if (wmt_ts_gpt.itbmp & 0xff0000)
		return REG32_VAL(wmt_ts_gpt.itaddr) & 0x800000;
	else if (wmt_ts_gpt.itbmp & 0xff000000)
		return REG32_VAL(wmt_ts_gpt.itaddr) & 0x80000000;
	else
		return 0;
}

static int ts_disable_irq(void)
{
	if (wmt_ts_gpt.itbmp & 0xff || wmt_ts_gpt.itbmp == 0)
		REG32_VAL(wmt_ts_gpt.itaddr) &= ~0x80;
	else if (wmt_ts_gpt.itbmp & 0xff00)
		REG32_VAL(wmt_ts_gpt.itaddr) &= ~0x8000;
	else if (wmt_ts_gpt.itbmp & 0xff0000)
		REG32_VAL(wmt_ts_gpt.itaddr) &= ~0x800000;
	else if (wmt_ts_gpt.itbmp & 0xff000000)
        	REG32_VAL(wmt_ts_gpt.itaddr) &= ~0x80000000;
//	REG32_VAL(wmt_ts_gpt.isaddr) &= ~wmt_ts_gpt.isbmp;
	return 0;
}




static int  parse_gpt_arg(void)
{
/* setenv tsreset mw.b 0xd81100f1 0\;mw.b 0xd81100b1 2\;sleep 1\;mw.b 0xd81100f1 2
setenv wmt.gpt.ts mxt768:200:d8110040:d8110080:d8110000:d8110480:d81104c0:0000:d8110308:200:d8110360:5    jakie */
	char *varname = "wmt.gpt.ts";
	unsigned int varlen = 160;
	unsigned char buf[160];
	int retval;
	int i = 0;

	retval = wmt_getsyspara(varname, buf, &varlen);

	if (retval != 0)
		return 0;
	if (buf[0] != 'm')
		return 0;
	if (buf[1] != 'x')
		return 0;
	if (buf[2] != 't')
		return 0;
	if (buf[3] != '7')
		return 0;
	if (buf[4] != '6')
		return 0;
	if (buf[5] != '8')
		return 0;
	i = 7;
	sscanf((buf + i), "%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
			&wmt_ts_gpt.bitmap,
			&wmt_ts_gpt.ctraddr,
			&wmt_ts_gpt.ocaddr,
			&wmt_ts_gpt.idaddr,
			&wmt_ts_gpt.peaddr,
			&wmt_ts_gpt.pcaddr,
			&wmt_ts_gpt.itbmp,
			&wmt_ts_gpt.itaddr,
			&wmt_ts_gpt.isbmp,
			&wmt_ts_gpt.isaddr,
			&wmt_ts_gpt.irq);

	wmt_ts_gpt.ctraddr += WMT_MMAP_OFFSET;
	wmt_ts_gpt.ocaddr += WMT_MMAP_OFFSET;
	wmt_ts_gpt.idaddr += WMT_MMAP_OFFSET;
	wmt_ts_gpt.peaddr += WMT_MMAP_OFFSET;
	wmt_ts_gpt.pcaddr += WMT_MMAP_OFFSET;
	wmt_ts_gpt.itaddr += WMT_MMAP_OFFSET;
	wmt_ts_gpt.isaddr += WMT_MMAP_OFFSET;
	TOUCH_PRINTK("wmt.gpt.ts = %x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n",
		wmt_ts_gpt.bitmap,
		wmt_ts_gpt.ctraddr,
		wmt_ts_gpt.ocaddr,
		wmt_ts_gpt.idaddr,
		wmt_ts_gpt.peaddr,
		wmt_ts_gpt.pcaddr,
		wmt_ts_gpt.itbmp,
		wmt_ts_gpt.itaddr,
		wmt_ts_gpt.isbmp,
		wmt_ts_gpt.isaddr,
		wmt_ts_gpt.irq);

	return 1;

}

static int  parse_gptc_arg(void)
{
/* setenv tsreset mw.b 0xd81100f1 0\;mw.b 0xd81100b1 2\;sleep 1\;mw.b 0xd81100f1 2
setenv wmt.gpt.ts mxt768:200:d8110040:d8110080:d8110000:d8110480:d81104c0:0000:d8110308:200:d8110360:5    jakie */
	char *varname = "wmt.gpt.tsc";
	unsigned int varlen = 160;
	unsigned char buf[160];
	int retval;
	int i = 0;

	retval = wmt_getsyspara(varname, buf, &varlen);

	if (retval != 0)
		return 0;
	if (buf[0] != 'm')
		return 0;
	if (buf[1] != 'x')
		return 0;
	if (buf[2] != 't')
		return 0;
	if (buf[3] != 'c')
		return 0;
	if (buf[4] != 'f')
		return 0;
	if (buf[5] != 'g')
		return 0;
	i = 7;
	sscanf((buf + i), "%x:%x:%x:%x",
			&ac.touch_panel_config_checksum[OFILM_GFF],
			&ac.touch_panel_config_checksum[UNUSE1],
			&ac.touch_panel_config_checksum[JTOUCH],
			&ac.touch_panel_config_checksum[OFILM]);

	TOUCH_PRINTK("wmt.gpt.tsc = %x:%x:%x:%x\n",
		ac.touch_panel_config_checksum[OFILM_GFF],
		ac.touch_panel_config_checksum[UNUSE1],
		ac.touch_panel_config_checksum[JTOUCH],
		ac.touch_panel_config_checksum[OFILM]);

	return 1;

}

int set_uboot_para(int type, u32 value)
{
	char *varname = "wmt.gpt.tsc";
	unsigned int varlen = 160;
	unsigned char buf[160];
	int i = 7;

        memset(buf, 0, varlen);
	if (!wmt_getsyspara(varname, buf, &varlen) ){
                sscanf(buf+i,"%x:%x:%x:%x",
			&ac.touch_panel_config_checksum[OFILM_GFF],
			&ac.touch_panel_config_checksum[UNUSE1],
			&ac.touch_panel_config_checksum[JTOUCH],
			&ac.touch_panel_config_checksum[OFILM]);
        } else {
                ac.touch_panel_config_checksum[OFILM_GFF] = OFILM_GFF_DEFAULT;
                ac.touch_panel_config_checksum[UNUSE1] = UNUSE1_DEFAULT;
                ac.touch_panel_config_checksum[JTOUCH] = JTOUCH_DEFAULT;
                ac.touch_panel_config_checksum[OFILM]  = OFILM_DEFAULT;
        }
	TOUCH_PRINTK("wmt.gpt.tsc = %x:%x:%x:%x\n",
		ac.touch_panel_config_checksum[OFILM_GFF],
		ac.touch_panel_config_checksum[UNUSE1],
		ac.touch_panel_config_checksum[JTOUCH],
		ac.touch_panel_config_checksum[OFILM]);

	switch (type){
		case OFILM_GFF:
			ac.touch_panel_config_checksum[OFILM_GFF] = value;
		break;
		case UNUSE1:
			ac.touch_panel_config_checksum[UNUSE1] = value;
		break;
		case JTOUCH:
			ac.touch_panel_config_checksum[JTOUCH] = value;
		break;
		case OFILM:
			ac.touch_panel_config_checksum[OFILM] = value;
		break;
		default:
			ac.touch_panel_config_checksum[JTOUCH] = value;
		break;
	}

        snprintf(buf+i, varlen, "%x:%x:%x:%x",
			ac.touch_panel_config_checksum[OFILM_GFF],
			ac.touch_panel_config_checksum[UNUSE1],
			ac.touch_panel_config_checksum[JTOUCH],
			ac.touch_panel_config_checksum[OFILM]);

        wmt_setsyspara(varname, buf);
        TOUCH_PRINTK("set uboot para!\n");


	TOUCH_PRINTK("wmt.gpt.tsc = %x:%x:%x:%x\n",
		ac.touch_panel_config_checksum[OFILM_GFF],
		ac.touch_panel_config_checksum[UNUSE1],
		ac.touch_panel_config_checksum[JTOUCH],
		ac.touch_panel_config_checksum[OFILM]);

        return 0;
}

u8 asus_read_chg(void){
#if 0
	TOUCH_PRINTK("=====gpio9 Eanble Register for Dedicated 0040=%x\n",(u16)REG32_VAL(0xfe110040));
	TOUCH_PRINTK("=====gpio9 0040   =%x\n",((u16)(REG32_VAL(0xfe110040) & 0x200)));

	TOUCH_PRINTK("=====gpio9 Output Eanble Register for Dedicated 0080=%x\n",(u16)REG32_VAL(0xfe110080));
	TOUCH_PRINTK("=====gpio9 0080   =%x\n",((u16)(REG32_VAL(0xfe110080) & 0x200)));

	TOUCH_PRINTK("=====gpio9 input data Register for Dedicated 0000=%x\n",(u16)REG32_VAL(0xfe110000));
	TOUCH_PRINTK("=====gpio9 0000   =%x\n",((u16)(REG32_VAL(0xfe110000) & 0x200)));

	TOUCH_PRINTK("=====gpio9 output data Register for Dedicated 00C0=%x\n",(u16)REG32_VAL(0xfe1100C0));
	TOUCH_PRINTK("=====gpio9 00C0   =%x\n",((u16)(REG32_VAL(0xfe1100C0) & 0x200)));

	TOUCH_PRINTK("=====gpio9 interrupt request status Register 0360=%x\n",(u16)REG32_VAL(0xfe110360));
	TOUCH_PRINTK("=====gpio9 0360   =%x\n",((u16)(REG32_VAL(0xfe110360) & 0x200)));

	TOUCH_PRINTK("=====gpio9 pull-up/down enable Register 0480=%x\n",(u16)REG32_VAL(0xfe110480));
	TOUCH_PRINTK("=====gpio9 0480   =%x\n",((u16)(REG32_VAL(0xfe110480) & 0x200)));

	TOUCH_PRINTK("=====gpio9 pull-up/down control Register 04C0=%x\n",(u16)REG32_VAL(0xfe1104C0));
	TOUCH_PRINTK("=====gpio9 04C0   =%x\n",((u16)(REG32_VAL(0xfe1104C0) & 0x200)));
#endif


return ((u8)((REG32_VAL(0xfe110000) & 0x200)>>8));
}



u8 asus_valid_interrupt(void){

return (GPIO1_INT_REQ_STS_VAL & BIT1);

}

u8 readTP_ID(void){

	u8 ret=0;
	ac.tp.TP_ID0 = ((REG16_VAL(wmt_ts_gpt.idaddr) & 0x1000)==0x1000)?1:0;
	ac.tp.TP_ID1 = ((REG16_VAL(wmt_ts_gpt.idaddr) & 0x2000)==0x2000)?1:0;
	ret = ac.tp.TP_ID0 << 1 | ac.tp.TP_ID1;
	TOUCH_PRINTK("tpid.TP_ID0=%x\n",ac.tp.TP_ID0);
	TOUCH_PRINTK("tpid.TP_ID1=%x\n",ac.tp.TP_ID1);
	TOUCH_PRINTK("TP_ID=%x\n",ret);
	return ret;
}

int query_status(void){

	int ret=0;
	ret |= (ac.tp.TP_ID0<<7);
	ret |= (ac.tp.TP_ID1<<6);
	ret |= ((((REG16_VAL(0xfe110000) & 0x200)==0x200)?1:0)<<5);
	ret |= ((((REG16_VAL(0xfe110030) & 0x200)==0x200)?1:0)<<4);
	return ret;
}

static ssize_t store_d_print(struct device *dev, struct device_attribute *devattr,const char *buf, size_t count)
{
	int print_flag;

	sscanf(buf, "%d\n",&print_flag);
	d_flag=print_flag;

	return count;
}

static ssize_t show_status(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int err;
	u8 buff[0x8];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

        err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, data), 0x8, (u8 *) buff);

        if (err < 0)	TOUCH_PRINTK("show_status : mxt_read_block failure.\n");

        data->status|=query_status();
	return sprintf(buf, "Status=%x\nTP Type=%x\nCHG pin=%x\nReset pin=%x\nFirmware version = %x %x\ncalibration = %x\nFirmware change=%x\nchange TP=%x\n", (data->status & 0x01)?1:0,(data->status & 0xc0),(data->status & 0x10)?1:0,(data->status & 0x20)?1:0,mxt_firmware_id,data->device_info.build,ac.calibration_status,buff[2],(buff[1]==touch_vendor_id)?0:1);
}

//<ASUS-Darren1_Chang20121029+>
#ifdef CONFIG_ASUS_ENGINEER_MODE
static ssize_t asus_show_status(struct device *dev, struct device_attribute *devattr, char *buf)
{
	if (global_mxt == NULL)
		return sprintf(buf, "%d\n", 0);
	return sprintf(buf, "%d\n", global_mxt->status);
}
#endif
//<ASUS-Darren1_Chang20121029->

static ssize_t dump_T5(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x9];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");


	err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_GEN_MESSAGEPROCESSOR_T5, data), 0x9, (u8 *) tmp);
	if (err < 0){
		sprintf(tmpstr, "read T5 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	else{
		for (i=0; i<0x09; i++){
			sprintf(tmpstr,"T5 byte[%d] = %d\n",i,tmp[i]);
			strncat (buf,tmpstr,strlen(tmpstr));
		}
	}
	return strlen(buf);
}
static ssize_t dump_T6(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x6];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");
	err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, data), 0x6, (u8 *) tmp);
	if (err < 0){
		sprintf(tmpstr, "read T6 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	else{
		for (i=0; i<0x06; i++){
			sprintf(tmpstr,"T6 byte[%d] = %d\n",i,tmp[i]);
			strncat (buf,tmpstr,strlen(tmpstr));
		}
	}
	return strlen(buf);
}

static ssize_t dump_T7(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x4];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

	err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, data), 0x4, (u8 *) tmp);
	if (err < 0){
		sprintf(tmpstr, "read T7 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	else{
		for (i=0; i<0x04; i++){
			sprintf(tmpstr,"T7 byte[%d] = %d\n",i,tmp[i]);
			strncat (buf,tmpstr,strlen(tmpstr));
		}
	}
	return strlen(buf);
}

static ssize_t dump_T8(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0xa];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

	err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, data), 0xa, (u8 *) tmp);
	if (err < 0){
		sprintf(tmpstr, "read T8 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	else{
		for (i=0; i<0x0a; i++){
			sprintf(tmpstr,"T8 byte[%d] = %d\n",i,tmp[i]);
			strncat (buf,tmpstr,strlen(tmpstr));
		}
	}
	return strlen(buf);
}

static ssize_t dump_T9(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	int size;
	u8 tmp[0x27];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");
if (mxt_firmware_id==MXT540S_FIRMWARE_ID_10 || mxt_firmware_id==MXT540S_FIRMWARE_ID_11)	size = 0x27;
else											size = 0x24;

	err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, data), size, (u8 *) tmp);
	if (err < 0){
		sprintf(tmpstr, "read T9 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	else{
		for (i=0; i<size; i++){
			sprintf(tmpstr,"T9 byte[%d] = %d\n",i,tmp[i]);
			strncat (buf,tmpstr,strlen(tmpstr));
		}
	}
	return strlen(buf);
}

static ssize_t dump_T15(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0xb];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_TOUCH_KEYARRAY_T15, data), 0xb, (u8 *) tmp);
	if (err < 0){
		sprintf(tmpstr, "read T15 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	else{
		for (i=0; i< 0xb; i++){
			sprintf(tmpstr,"T15 byte[%d] = %d\n",i,tmp[i]);
			strncat (buf,tmpstr,strlen(tmpstr));
		}
	}
	return strlen(buf);
}

static ssize_t dump_T18(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x2];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_SPT_COMMSCONFIG_T18, data), 0x2, (u8 *) tmp);
      if (err < 0){
		sprintf(tmpstr, "read T18 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0x2; i++){
		sprintf(tmpstr,"T18 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	return strlen(buf);
}

static ssize_t dump_T19(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x6];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_SPT_GPIOPWM_T19, data), 0x6, (u8 *) tmp);
      if (err < 0){
		sprintf(tmpstr, "read T19 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0x6; i++){
		sprintf(tmpstr,"T19 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	return strlen(buf);
}

static ssize_t dump_T23(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0xf];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_TOUCH_PROXIMITY_T23, data), 0xf, (u8 *) tmp);
      if (err < 0){
		sprintf(tmpstr, "read T23 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0xf; i++){
		sprintf(tmpstr,"T23 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}

	return strlen(buf);
}

static ssize_t dump_T25(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0xf];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, data), 0xf, (u8 *) tmp);
      if (err < 0){
		sprintf(tmpstr, "read T25 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0xf; i++){
		sprintf(tmpstr,"T25 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	return strlen(buf);
}

static ssize_t dump_T37(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x82];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTIC_T37, data), 0x82, (u8 *) tmp);
      if (err < 0){
		sprintf(tmpstr, "read T37 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0x82; i++){
		sprintf(tmpstr,"T37 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	return strlen(buf);
}

static ssize_t dump_T38(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x8];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, data), 0x8, (u8 *) tmp);
      if (err < 0){
		sprintf(tmpstr, "read T38 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0x8; i++){
		sprintf(tmpstr,"T38 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}

	return strlen(buf);
}

static ssize_t dump_T40(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x5];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_PROCI_GRIPSUPPRESSION_T40, data), 0x5, (u8 *) tmp);
      if (err < 0){
		sprintf(tmpstr, "read T40 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0x5; i++){
		sprintf(tmpstr,"T40 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	return strlen(buf);
}

static ssize_t dump_T42(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0xa];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, data), 0xa, (u8 *) tmp);
	if (err < 0){
		sprintf(tmpstr, "read T42 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0xa; i++){
		sprintf(tmpstr,"T42 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	return strlen(buf);
}

static ssize_t dump_T44(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x1];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_SPT_MESSAGECOUNT_T44, data), 0x1, (u8 *) tmp);
	if (err < 0){
		sprintf(tmpstr, "read T44 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0x1; i++){
		sprintf(tmpstr,"T44 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	return strlen(buf);
}

static ssize_t dump_T46(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0xa];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, data), 0xa, (u8 *) tmp);
      if (err < 0){
		sprintf(tmpstr, "read T46 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0xa; i++){
		sprintf(tmpstr,"T46 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	return strlen(buf);
}

static ssize_t dump_T47(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x16];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_PROCI_STYLUS_T47, data), 0x16, (u8 *) tmp);
      if (err < 0){
		sprintf(tmpstr, "read T47 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0x16; i++){
		sprintf(tmpstr,"T47 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}

	return strlen(buf);
}

static ssize_t dump_T55(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x6];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_PROCI_ADAPTIVETHRESHOLD_T55, data), 0x6, (u8 *) tmp);
      if (err < 0){
		sprintf(tmpstr, "read T55 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0x6; i++){
		sprintf(tmpstr,"T55 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	return strlen(buf);
}

static ssize_t dump_T56(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x31];
	int size;
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

if (mxt_firmware_id==MXT540S_FIRMWARE_ID_10)		size = 0x31;
else if (mxt_firmware_id==MXT540S_FIRMWARE_ID_11)	size = 0x24;
else							size = 0x31;


      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, data), size, (u8 *) tmp);
      if (err < 0){
		sprintf(tmpstr, "read T56 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< size; i++){
		sprintf(tmpstr,"T56 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}

	return strlen(buf);
}

static ssize_t dump_T57(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x3];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_PROCI_EXTRATOUCHAREADATA_T57, data), 0x3, (u8 *) tmp);
      if (err < 0){
		sprintf(tmpstr, "read T57 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0x3; i++){
		sprintf(tmpstr,"T57 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}

	return strlen(buf);
}

static ssize_t dump_T61(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0xa];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_SPT_TIMER_T61, data), 0xa, (u8 *) tmp);
      if (err < 0){
		sprintf(tmpstr, "read T61 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0xa; i++){
		sprintf(tmpstr,"T61 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}

	return strlen(buf);
}

static ssize_t dump_T62(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x36];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, data), 0x36, (u8 *) tmp);
	if (err < 0){
		sprintf(tmpstr, "read T62 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0x36; i++){
		sprintf(tmpstr,"T62 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}

	return strlen(buf);
}

static ssize_t dump_T65(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x11];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");
if (mxt_firmware_id==MXT540S_FIRMWARE_ID_11){
        err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, data), 0x11, (u8 *) tmp);
	if (err < 0){
		sprintf(tmpstr, "read T65 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0x11; i++){
		sprintf(tmpstr,"T65 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
}
	return strlen(buf);
}

static ssize_t dump_T66(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x5];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");
if (mxt_firmware_id==MXT540S_FIRMWARE_ID_10 || mxt_firmware_id==MXT540S_FIRMWARE_ID_11){
        err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, data), 0x5, (u8 *) tmp);
	if (err < 0){
		sprintf(tmpstr, "read T66 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0x5; i++){
		sprintf(tmpstr,"T66 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
}
	return strlen(buf);
}

static ssize_t dump_T69(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int i;
	int err;
	u8 tmp[0x9];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");
if (mxt_firmware_id==MXT540S_FIRMWARE_ID_10 || mxt_firmware_id==MXT540S_FIRMWARE_ID_11){
      err = mxt_read_block(data->client, MXT_BASE_ADDR(MXT_PALMGESTUREPROCESSOR_T69, data), 0x9, (u8 *) tmp);
	if (err < 0){
		sprintf(tmpstr, "read T69 cfg error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}else
	for (i=0; i< 0x9; i++){
		sprintf(tmpstr,"T69 byte[%d] = %d\n",i,tmp[i]);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
}
	return strlen(buf);
}

static ssize_t dump_Family_ID(struct device *dev, struct device_attribute *devattr, char *buf)
{
//	int i;
	int err;
	unsigned int tmp[0x36];
	char tmpstr[100];
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	sprintf(buf," ");

      err = mxt_read_block(data->client, MXT_ADDR_INFO_BLOCK, MXT_ID_BLOCK_SIZE,(u8 *) tmp);
	if (err < 0){
		sprintf(tmpstr, "read Family ID error, ret %d\n",err);
		strncat (buf,tmpstr,strlen(tmpstr));
	}
	else{
		sprintf(tmpstr, "Family ID is %u configuration checksum is %lx\n",tmp[0], data->configuration_crc);
		strncat (buf,tmpstr,strlen(tmpstr));
	}


	return strlen(buf);

}

static ssize_t show_FW_version(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);
	return sprintf(buf, "Touch Firmware Version: %d.%d, checksum is %d\n", data->device_info.major, data->device_info.minor, fw_checksum);
}


static ssize_t store_mode2(struct device *dev, struct device_attribute *devattr,const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data*data = i2c_get_clientdata(client);
	int cfg[3];

	sscanf(buf, "%d%d%d\n",&cfg[0], &cfg[1], &cfg[2]);

	mxt_write_byte(data->client, MXT_BASE_ADDR(cfg[0], data)+cfg[1],cfg[2]);

	TOUCH_PRINTK("cfg[0]=%d, cfg[1]=%d, cfg[2]=%d\n",cfg[0],cfg[1],cfg[2]);
#if 0
	mxt_write_byte(data->client, MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, data) + MXT_ADR_T6_BACKUPNV,MXT_CMD_T6_BACKUP);
#endif
	return count;
}

#if defined(CONFIG_ASUS_ENGINEER_MODE) && CONFIG_ASUS_ENGINEER_MODE
DEVICE_ATTR(cfg_me172v, S_IRUGO | S_IWUSR, NULL, store_mode2);
DEVICE_ATTR(d_print, S_IRUGO | S_IWUSR, NULL, store_d_print);
DEVICE_ATTR(dump_T5_me172v,  0755, dump_T5,  NULL);
DEVICE_ATTR(dump_T6_me172v,  0755, dump_T6,  NULL);
DEVICE_ATTR(dump_T7_me172v,  0755, dump_T7,  NULL);
DEVICE_ATTR(dump_T8_me172v,  0755, dump_T8,  NULL);
DEVICE_ATTR(dump_T9_me172v,  0755, dump_T9,  NULL);
DEVICE_ATTR(dump_T15_me172v, 0755, dump_T15, NULL);
DEVICE_ATTR(dump_T18_me172v, 0755, dump_T18, NULL);
DEVICE_ATTR(dump_T19_me172v, 0755, dump_T19, NULL);
DEVICE_ATTR(dump_T23_me172v, 0755, dump_T23, NULL);
DEVICE_ATTR(dump_T25_me172v, 0755, dump_T25, NULL);
DEVICE_ATTR(dump_T37_me172v, 0755, dump_T37, NULL);
DEVICE_ATTR(dump_T38_me172v, 0755, dump_T38, NULL);
DEVICE_ATTR(dump_T40_me172v, 0755, dump_T40, NULL);
DEVICE_ATTR(dump_T42_me172v, 0755, dump_T42, NULL);
DEVICE_ATTR(dump_T44_me172v, 0755, dump_T44, NULL);
DEVICE_ATTR(dump_T46_me172v, 0755, dump_T46, NULL);
DEVICE_ATTR(dump_T47_me172v, 0755, dump_T47, NULL);
DEVICE_ATTR(dump_T55_me172v, 0755, dump_T55, NULL);
DEVICE_ATTR(dump_T56_me172v, 0755, dump_T56, NULL);
DEVICE_ATTR(dump_T57_me172v, 0755, dump_T57, NULL);
DEVICE_ATTR(dump_T61_me172v, 0755, dump_T61, NULL);
DEVICE_ATTR(dump_T62_me172v, 0755, dump_T62, NULL);
DEVICE_ATTR(dump_T66_me172v, 0755, dump_T66, NULL);
DEVICE_ATTR(dump_T65_me172v, 0755, dump_T65, NULL);
DEVICE_ATTR(dump_T69_me172v, 0755, dump_T69, NULL);
#endif

//<ASUS-Darren1_Chang20121029+>
#ifdef CONFIG_ASUS_ENGINEER_MODE
DEVICE_ATTR(asus_touchpanel_status, 0755, asus_show_status, NULL);
#endif
//<ASUS-Darren1_Chang20121029->

DEVICE_ATTR(atmel_touchpanel_status, 0755, show_status, NULL);
DEVICE_ATTR(FW_version, 0755, show_FW_version, NULL);
DEVICE_ATTR(dump_Family_ID_me172v, 0755, dump_Family_ID, NULL);




static struct attribute *mxt_attr[] = {

//<ASUS-Darren1_Chang20121029+>
#ifdef CONFIG_ASUS_ENGINEER_MODE
	&dev_attr_asus_touchpanel_status.attr,
#endif
//<ASUS-Darren1_Chang20121029->

#if defined(CONFIG_ASUS_ENGINEER_MODE) && CONFIG_ASUS_ENGINEER_MODE
	&dev_attr_dump_T5_me172v.attr,
	&dev_attr_dump_T6_me172v.attr,
	&dev_attr_dump_T7_me172v.attr,
	&dev_attr_dump_T8_me172v.attr,
	&dev_attr_dump_T9_me172v.attr,
	&dev_attr_dump_T15_me172v.attr,
	&dev_attr_dump_T18_me172v.attr,
	&dev_attr_dump_T19_me172v.attr,
	&dev_attr_dump_T23_me172v.attr,
	&dev_attr_dump_T25_me172v.attr,
	&dev_attr_dump_T37_me172v.attr,
	&dev_attr_dump_T38_me172v.attr,
	&dev_attr_dump_T40_me172v.attr,
	&dev_attr_dump_T42_me172v.attr,
	&dev_attr_dump_T44_me172v.attr,
	&dev_attr_dump_T46_me172v.attr,
	&dev_attr_dump_T47_me172v.attr,
	&dev_attr_dump_T55_me172v.attr,
	&dev_attr_dump_T56_me172v.attr,
	&dev_attr_dump_T57_me172v.attr,
	&dev_attr_dump_T61_me172v.attr,
	&dev_attr_dump_T62_me172v.attr,
	&dev_attr_dump_T65_me172v.attr,
	&dev_attr_dump_T66_me172v.attr,
	&dev_attr_dump_T69_me172v.attr,
	&dev_attr_cfg_me172v.attr,
	&dev_attr_d_print.attr,
#endif
	&dev_attr_atmel_touchpanel_status.attr,
	&dev_attr_dump_Family_ID_me172v.attr,
	&dev_attr_FW_version.attr,

	NULL
};

ssize_t debug_data_read(struct mxt_data *mxt, char *buf, size_t count,
			loff_t *ppos, u8 debug_command)
{
	int i;
	u16 *data;
	u16 diagnostics_reg;
	int offset = 0;
	int size;
	int read_size;
	int error;
	char *buf_start;
	u16 debug_data_addr;
	u16 page_address;
	u8 page;
	u8 debug_command_reg;

	data = mxt->debug_data;
	if (data == NULL)
		return -EIO;

	/* If first read after open, read all data to buffer. */
	if (mxt->current_debug_datap == 0) {

		diagnostics_reg = MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6,
						mxt) + MXT_ADR_T6_DIAGNOSTIC;
		if (count > (mxt->device_info.num_nodes * 2))
			count = mxt->device_info.num_nodes;

		debug_data_addr = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTIC_T37, mxt) +
		    MXT_ADR_T37_DATA;
		page_address = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTIC_T37, mxt) +
		    MXT_ADR_T37_PAGE;
		error = mxt_read_block(mxt->client, page_address, 1, &page);
		if (error < 0)
			return error;
		mxt_debug(DEBUG_TRACE, "[touch] debug data page = %d\n", page);
		while (page != 0) {
			error = mxt_write_byte(mxt->client,
					       diagnostics_reg,
					       MXT_CMD_T6_PAGE_DOWN);
			if (error < 0)
				return error;
			/* Wait for command to be handled; when it has, the
			   register will be cleared. */
			debug_command_reg = 1;
			while (debug_command_reg != 0) {
				error = mxt_read_block(mxt->client,
						       diagnostics_reg, 1,
						       &debug_command_reg);
				if (error < 0)
					return error;
				mxt_debug(DEBUG_TRACE,
					  "[touch] Waiting for debug diag command "
					  "to propagate...\n");

			}
			error = mxt_read_block(mxt->client, page_address, 1,
					       &page);
			if (error < 0)
				return error;
			mxt_debug(DEBUG_TRACE, "[touch] debug data page = %d\n", page);
		}

		/*
		 * Lock mutex to prevent writing some unwanted data to debug
		 * command register. User can still write through the char
		 * device interface though. TODO: fix?
		 */

		mutex_lock(&mxt->debug_mutex);
		/* Configure Debug Diagnostics object to show deltas/refs */
		error = mxt_write_byte(mxt->client, diagnostics_reg,
				       debug_command);

		/* Wait for command to be handled; when it has, the
		 * register will be cleared. */
		debug_command_reg = 1;
		while (debug_command_reg != 0) {
			error = mxt_read_block(mxt->client,
					       diagnostics_reg, 1,
					       &debug_command_reg);
			if (error < 0)
				return error;
			mxt_debug(DEBUG_TRACE, "[touch] Waiting for debug diag command "
				  "to propagate...\n");

		}

		if (error < 0) {
			printk(KERN_WARNING
			       "[touch] Error writing to maXTouch device!\n");
			return error;
		}

		size = mxt->device_info.num_nodes * sizeof(u16);

		while (size > 0) {
			read_size = size > 128 ? 128 : size;
			mxt_debug(DEBUG_TRACE,
				  "[touch] Debug data read loop, reading %d bytes...\n",
				  read_size);
			msleep(200);
			error = mxt_read_block(mxt->client,
					       debug_data_addr,
					       read_size,
					       (u8 *) &data[offset]);
			if (error < 0) {
				printk(KERN_WARNING
				       "[touch] Error reading debug data\n");
				goto error;
			}
			offset += read_size / 2;
			size -= read_size;

			/* Select next page */
			error = mxt_write_byte(mxt->client, diagnostics_reg,
					       MXT_CMD_T6_PAGE_UP);
			if (error < 0) {
				printk(KERN_WARNING
				       "[touch] Error writing to maXTouch device!\n");
				goto error;
			}
		}
		mutex_unlock(&mxt->debug_mutex);
	}

	buf_start = buf;
	i = mxt->current_debug_datap;

	while (((buf - buf_start) < (count - 6)) &&
	       (i < mxt->device_info.num_nodes)) {
//		printk(KERN_WARNING "[touch] data[%x]=%lx\n",i,data[i]);
		mxt->current_debug_datap++;
		if (debug_command == MXT_CMD_T6_REFERENCES_MODE)
			buf += sprintf(buf, "%d: %6u\n", i,
				       (u16) le16_to_cpu(data[i]));
		else if (debug_command == MXT_CMD_T6_DELTAS_MODE)
			buf += sprintf(buf, "%d: %6d\n", i,
				       (s16) le16_to_cpu(data[i]));
		i++;
	}

	return buf - buf_start;
 error:
	mutex_unlock(&mxt->debug_mutex);
	return error;
}

ssize_t deltas_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	return debug_data_read(file->private_data, buf, count, ppos,
			       MXT_CMD_T6_DELTAS_MODE);
}

ssize_t refs_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	return debug_data_read(file->private_data, buf, count, ppos,
			       MXT_CMD_T6_REFERENCES_MODE);
}

int debug_data_open(struct inode *inode, struct file *file)
{
	struct mxt_data *mxt;
	int i;
	mxt = inode->i_private;
	if (mxt == NULL)
		return -EIO;

      if( !atomic_dec_and_test(&touch_char_available) )
	{
		atomic_inc(&touch_char_available);
		return -EBUSY; /* already open */
	}

	mxt->current_debug_datap = 0;
	mxt->debug_data = kmalloc(mxt->device_info.num_nodes * sizeof(u16),
				  GFP_KERNEL);
	if (mxt->debug_data == NULL)
		return -ENOMEM;

	for (i = 0; i < mxt->device_info.num_nodes; i++)
		mxt->debug_data[i] = 7777;

	file->private_data = mxt;
	return 0;
}

int debug_data_release(struct inode *inode, struct file *file)
{
	struct mxt_data *mxt;
	mxt = file->private_data;
	kfree(mxt->debug_data);

	atomic_inc(&touch_char_available); /* release the device */
	return 0;
}

const struct file_operations delta_fops = {
	.owner = THIS_MODULE,
	.open = debug_data_open,
	.release = debug_data_release,
	.read = deltas_read,
};

const struct file_operations refs_fops = {
	.owner = THIS_MODULE,
	.open = debug_data_open,
	.release = debug_data_release,
	.read = refs_read,
};

/* Calculates the 24-bit CRC sum. */
static u32 CRC_24(u32 crc, u8 byte1, u8 byte2)
{
	static const u32 crcpoly = 0x80001B;
	u32 result;
	u32 data_word;

	data_word = ((((u16) byte2) << 8u) | byte1);
	result = ((crc << 1u) ^ data_word);
	if (result & 0x1000000)
		result ^= crcpoly;
	return result;
}

/* Returns object address in mXT chip, or zero if object is not found */
u16 get_object_address(uint8_t object_type,
			      uint8_t instance,
			      struct mxt_object *object_table, int max_objs)
{
	uint8_t object_table_index = 0;
	uint8_t address_found = 0;
	uint16_t address = 0;
	struct mxt_object *obj;

	while ((object_table_index < max_objs) && !address_found) {
		obj = &object_table[object_table_index];
		if (obj->type == object_type) {
			address_found = 1;
			/* Are there enough instances defined in the FW? */
			if (obj->instances >= instance) {
				address = obj->chip_addr +
				    (obj->size + 1) * instance;
			} else {
				return 0;
			}
		}
		object_table_index++;
	}
	return address;
}

/* Return the pointer of mxt object*/
struct mxt_object* get_object(uint8_t object_type, 
	                 const struct mxt_data *mxt){
    uint8_t object_table_index = 0;
    int max_objs = mxt->device_info.num_objs;
    struct mxt_object *object_table = mxt->object_table;
    struct mxt_object *obj;

    while(object_table_index < max_objs){
        obj = &object_table[object_table_index];
        if(obj->type == object_type)
            return obj;

        object_table_index++;
    }

    return NULL;
}

/*
 * Reads a block of bytes from given address from mXT chip. If we are
 * reading from message window, and previous read was from message window,
 * there's no need to write the address pointer: the mXT chip will
 * automatically set the address pointer back to message window start.
 */

int mxt_read_block(struct i2c_client *client,
			  u16 addr, u16 length, u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	__le16 le_addr;
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);

	if (mxt != NULL) {
		if ((mxt->last_read_addr == addr) &&
		    (addr == mxt->msg_proc_addr)) {
			if (i2c_master_recv(client, value, length) == length)
				return length;
			else
				return -EIO;
		} else {
			mxt->last_read_addr = addr;
		}
	}

	mxt_debug(DEBUG_TRACE, "[touch] Writing address pointer & reading %d bytes "
		  "in on i2c transaction...\n", length);

	le_addr = cpu_to_le16(addr);
	msg[0].addr = client->addr;
	msg[0].flags = 0x00;
	msg[0].len = 2;
	msg[0].buf = (u8 *) &le_addr;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = length;
	msg[1].buf = (u8 *) value;
	if (i2c_transfer(adapter, msg, 2) == 2)
		return length;
	else
		return -EIO;

}

/* Reads a block of bytes from current address from mXT chip. */

static int mxt_read_block_wo_addr(struct i2c_client *client,
				  u16 length, u8 *value)
{

	if (i2c_master_recv(client, value, length) == length) {
		mxt_debug(DEBUG_TRACE, "[touch] I2C block read ok\n");
		return length;
	} else {
		mxt_debug(DEBUG_INFO, "[touch] I2C block read failed\n");
		return -EIO;
	}

}

/* Writes one byte to given address in mXT chip. */

int mxt_write_byte(struct i2c_client *client, u16 addr, u8 value)
{
	struct {
		__le16 le_addr;
		u8 data;

	} i2c_byte_transfer;

	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;
	i2c_byte_transfer.le_addr = cpu_to_le16(addr);
	i2c_byte_transfer.data = value;
	if (i2c_master_send(client, (u8 *) &i2c_byte_transfer, 3) == 3)
		return 0;
	else
		return -EIO;
}

static int mxt_write_byte_bl(struct i2c_client *client, u16 addr, u16 length, u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg wmsg;
	//unsigned char wbuf[3];
	unsigned char data[I2C_MAX_SEND_LENGTH];
	int ret,i;

//	printk("Touch: length = %d\n",length);
	if(length+2 > I2C_MAX_SEND_LENGTH)
	{
		printk("[touch] [TSP][ERROR] %s() data length error\n", __FUNCTION__);
		return -ENODEV;
	}

	wmsg.addr = MXT_BL_ADDRESS;
	wmsg.flags = I2C_M_WR;
	wmsg.len = length;
	wmsg.buf = data;

	for (i = 0; i < length; i++)
	{
		data[i] = *(value+i);
	}

	//	printk("%s, %s\n",__func__,wbuf);
	ret = i2c_transfer(adapter, &wmsg, 1);
	return ret;

}

static int mxt_write_mem_bl(struct i2c_client *client, u16 start, u16 size, u8 *mem)
{
	int ret;

	ret = mxt_write_byte_bl(client, start, size, mem);
	if(ret < 0){
		printk("[touch] boot write mem fail: %d \n",ret);
		return(WRITE_MEM_FAILED);
	}
	else
		return(WRITE_MEM_OK);
}

static int mxt_read_mem_bl(struct i2c_client *client, u16 start, u16 size, u8 *mem)
{
	struct i2c_msg rmsg;
	int ret;

	rmsg.addr=MXT_BL_ADDRESS;
	rmsg.flags = I2C_M_RD;
	rmsg.len = size;
	rmsg.buf = mem;
	ret = i2c_transfer(client->adapter, &rmsg, 1);

	return ret;

}

/* Writes a block of bytes (max 256) to given address in mXT chip. */
static int mxt_write_block(struct i2c_client *client,
			   u16 addr, u16 length, u8 *value)
{
	int i;
	struct {
		__le16 le_addr;
		u8 data[256];

	} i2c_block_transfer;

	struct mxt_data *mxt;

	mxt_debug(DEBUG_TRACE, "[touch] Writing %d bytes to %d...", length, addr);
	if (length > 256)
		return -EINVAL;
	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;
	for (i = 0; i < length; i++)
		i2c_block_transfer.data[i] = *value++;
	i2c_block_transfer.le_addr = cpu_to_le16(addr);
	i = i2c_master_send(client, (u8 *) &i2c_block_transfer, length + 2);
	if (i == (length + 2))
		return length;
	else
		return -EIO;
}

/* Calculates the CRC value for mXT infoblock. */
int calculate_infoblock_crc(u32 *crc_result, u8 *data, int crc_area_size)
{
	u32 crc = 0;
	int i;

	for (i = 0; i < (crc_area_size - 1); i = i + 2)
		crc = CRC_24(crc, *(data + i), *(data + i + 1));
	/* If uneven size, pad with zero */
	if (crc_area_size & 0x0001)
		crc = CRC_24(crc, *(data + i), 0);
	/* Return only 24 bits of CRC. */
	*crc_result = (crc & 0x00FFFFFF);

	return 0;
}

void mxt_hw_reset(void){

    	TOUCH_PRINTK("Touch: mxt_hw_reset\n");
	REG32_VAL(0xFE1100f0) &= ~0x200;//gpio 25(ts_reset) Output data
	REG32_VAL(0xFE1100b0) |= 0x200;//gpio 25(ts_reset) Output data enable
//    	printk("ts_reset down\n");
	mdelay(150); // >- 1us
//    	printk("ts_reset up\n");
	REG32_VAL(0xFE1100f0) |= 0x200;//gpio 25(ts_reset) Output data
    	mdelay(1000);
}

static int mxt_init_Boot(struct mxt_data *mxt, int mode, void * fw_ptr)
{
    int ret=0;
    u8 firmware_id[MXT_ID_BLOCK_SIZE];

    dev_info(&mxt->client->dev, "[touch] Touch: start doing FW update\n");
    wake_lock(&mxt->wakelock);
//    disable_irq(mxt->irq);
    ts_disable_irq();
    ts_clear_irq();
    cancel_delayed_work(&mxt->dwork);
    if(mode == FIRMWARE_APP_UPGRADE) ret = mxt_update_firmware(mxt, FIRMWARE_APP_UPGRADE,fw_ptr, FW_SIZE);
    if(mode == FIRMWARE_BOOT_UPGRADE) ret = mxt_update_firmware(mxt, FIRMWARE_BOOT_UPGRADE,fw_ptr, FW_SIZE);
    if(ret < 0)  goto finish_init_boot;

    mdelay(500);
    dev_info(&mxt->client->dev, "[touch] Touch: mxt_hw_reset.\n");
    mxt_hw_reset();

    mxt_identify(mxt->client, mxt, firmware_id);
    mxt_read_object_table(mxt->client, mxt, firmware_id);
//    dev_info(&mxt->client->dev, "[touch] Touch: clear_touch_config.\n");
//    clear_touch_config(mxt);
//    dev_info(&mxt->client->dev, "[touch] Touch: init_touch_config.\n");
    init_touch_config(mxt, touch_vendor_id);


//    ts_enable_irq();

    //firmware change.
    mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt), 0x0);
    mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt)+2, 0x1);

    dev_info(&mxt->client->dev, "[touch] Touch: backup.\n");
    mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt)+ MXT_ADR_T6_BACKUPNV,
					MXT_CMD_T6_BACKUP);
    msleep(10);
finish_init_boot:
//    enable_irq(mxt->irq);
    schedule_delayed_work(&mxt->dwork, 0);
 //   ts_enable_irq();
    wake_unlock(&mxt->wakelock);
    return ret;
}

void force_release_pos(struct mxt_data *mxt)
{
	int i;
	TOUCH_PRINTK("Touch: force release position\n");
	for (i=0; i < 10; i++){
		if (fingerInfo[i].pressure ==0) continue;

		input_report_abs(mxt->input, ABS_MT_TRACKING_ID, i);
		input_report_abs(mxt->input, ABS_MT_TOUCH_MAJOR, 0);
		fingerInfo[i].pressure=0;
		input_mt_sync(mxt->input);
	}

	input_sync(mxt->input);
}

void process_T9_message(u8 *message, struct mxt_data *mxt, int last_touch)
{

	struct input_dev *input;
	u8 status;
	u16 xpos = 0xFFFF;
	u16 ypos = 0xFFFF;
	u8 touch_size = 255;
	u8 touch_number;
	u8 amplitude;
	u8 report_id;

	static int stored_size[10];
	static int stored_x[10];
	static int stored_y[10];
	int i;
	int active_touches = 0;

	/*
	 * If the 'last_touch' flag is set, we have received
		all the touch messages
	 * there are available in this cycle, so send the
		events for touches that are
	 * active.
	 */
	if (last_touch) {
		input_report_key(mxt->input, BTN_TOUCH, 1);
		for (i = 0; i < 10; i++) {
			if (fingerInfo[i].pressure) {
//			TOUCH_PRINTK("touch %d %d %d \n",i,fingerInfo[i].x,fingerInfo[i].y);
				active_touches++;
				input_report_abs(mxt->input, ABS_MT_TRACKING_ID,
						 i);
				input_report_abs(mxt->input, ABS_MT_TOUCH_MAJOR,
						 fingerInfo[i].pressure);
				input_report_abs(mxt->input, ABS_MT_PRESSURE,
					       fingerInfo[i].pressure);
				input_report_abs(mxt->input, ABS_MT_POSITION_X,
						 fingerInfo[i].x);
				input_report_abs(mxt->input, ABS_MT_POSITION_Y,
						 fingerInfo[i].y);
				input_mt_sync(mxt->input);
			if(unlikely(d_flag))
				printk("[touch] Touch: In last_touch decision, stored_size[%d]=%d, stored_x[%d]=%d, stored_y[%d]=%d\n",i,stored_size[i],i,stored_x[i],i,stored_y[i]);
			}
		}
		if (active_touches)
			input_sync(mxt->input);
		else {
			input_mt_sync(mxt->input);
			input_sync(mxt->input);
			if(unlikely(d_flag))
				printk("[touch] Touch: no touch and send uevent!\n");
		}

	} else {

		input = mxt->input;
		status = message[MXT_MSG_T9_STATUS];
		report_id = message[0];
                touch_number = message[MXT_MSG_REPORTID] -
                            mxt->rid_map[report_id].first_rid;

		//Atmel Jason Dai suggestion on 2012/09/06:
		//To avoid I2C read wrong message to make point leave on Screen.

		//release and press happen at the same time, it is impossible.
		if((message[1]>>4)==(MXT_MSGB_T9_DETECT|MXT_MSGB_T9_PRESS|MXT_MSGB_T9_RELEASE) ||
		//release, press and move happen at the same time, it is impossible.
		   (message[1]>>4)==(MXT_MSGB_T9_DETECT|MXT_MSGB_T9_PRESS|MXT_MSGB_T9_RELEASE|MXT_MSGB_T9_MOVE)){
		//send release message back to system.
		    message[1] = (message[1] & 0x0f) | MXT_MSGB_T9_DETECT | MXT_MSGB_T9_RELEASE;
		}
		if (status & MXT_MSGB_T9_SUPPRESS) {
			/* Touch has been suppressed by grip/face */
			/* detection                              */
                        stored_size[touch_number] = 0;
			fingerInfo[touch_number].pressure=0;
			mxt_debug(DEBUG_TRACE, "[touch] SUPRESS");
		} else {
			xpos = message[MXT_MSG_T9_XPOSMSB] * 4 +
			    ((message[MXT_MSG_T9_XYPOSLSB] >> 6) & 0x3);
			ypos = message[MXT_MSG_T9_YPOSMSB] * 4 +
			    ((message[MXT_MSG_T9_XYPOSLSB] >> 2) & 0x3);
/*			if (mxt->max_x_val < MAX_Y_SCAN)
				xpos >>= 2;
			if (mxt->max_y_val < MAX_X_SCAN)
				ypos >>= 2;
*/
			stored_x[touch_number] = xpos;
			stored_y[touch_number] = ypos;
			fingerInfo[touch_number].x=xpos;
			fingerInfo[touch_number].y=ypos;
			if (status & MXT_MSGB_T9_DETECT) {
				/*
				 * TODO: more precise touch size calculation?
				 * mXT224 reports the number of touched nodes,
				 * so the exact value for touch ellipse major
				 * axis length would be 2*sqrt(touch_size/pi)
				 * (assuming round touch shape).
				 */
				touch_size = message[MXT_MSG_T9_TCHAREA];
				if (touch_size <= 7)
					touch_size = touch_size << 5;
				else
					touch_size = 255;

				if (!touch_size)
					touch_size = 1;

				stored_size[touch_number] = touch_size;
				fingerInfo[touch_number].pressure=touch_size;

				if (status & MXT_MSGB_T9_AMP)
					/* Amplitude of touch has changed */
					amplitude =
					    message[MXT_MSG_T9_TCHAMPLITUDE];
			}

			if (status & MXT_MSGB_T9_RELEASE) {
				/* The previously reported touch has
					been removed. */

			input_report_key(mxt->input, BTN_TOUCH, 1);
			for (i = 0; i < 10; i++) {

				if (fingerInfo[i].pressure) {
					active_touches++;
//					TOUCH_PRINTK("touch %d %d %d \n",i,fingerInfo[i].x,fingerInfo[i].y);
					if(unlikely(d_flag))
					printk("[touch] Touch:  pressure\n");
					input_report_abs(mxt->input, ABS_MT_TRACKING_ID,
						 i);
					input_report_abs(mxt->input, ABS_MT_TOUCH_MAJOR,
						 fingerInfo[i].pressure);
					input_report_abs(mxt->input, ABS_MT_PRESSURE,
					       fingerInfo[i].pressure);
					input_report_abs(mxt->input, ABS_MT_POSITION_X,
						 fingerInfo[i].x);
					input_report_abs(mxt->input, ABS_MT_POSITION_Y,
						 fingerInfo[i].y);
					input_mt_sync(mxt->input);

				}
			}
				input_sync(mxt->input);

				stored_size[touch_number] = 0;
				fingerInfo[touch_number].pressure=0;


				if(unlikely(d_flag))
					printk("[touch] Touch: finger release in touch id=%d\n", touch_number);
			}
		}

		if (status & MXT_MSGB_T9_SUPPRESS) {
			mxt_debug(DEBUG_TRACE, "[touch] SUPRESS");
		} else {
			if (status & MXT_MSGB_T9_DETECT) {
				mxt_debug(DEBUG_TRACE, "[touch] DETECT:%s%s%s%s",
					  ((status & MXT_MSGB_T9_PRESS) ?
					   " PRESS" : ""),
					  ((status & MXT_MSGB_T9_MOVE) ? " MOVE"
					   : ""),
					  ((status & MXT_MSGB_T9_AMP) ? " AMP" :
					   ""),
					  ((status & MXT_MSGB_T9_VECTOR) ?
					   " VECT" : ""));

			} else if (status & MXT_MSGB_T9_RELEASE) {
				mxt_debug(DEBUG_TRACE, "[touch] RELEASE");
			}
		}
		mxt_debug(DEBUG_TRACE, "[touch] X=%d, Y=%d, TOUCHSIZE=%d",
			  xpos, ypos, touch_size);
	}
	return;
}

int process_message(u8 *message, u8 object, struct mxt_data *mxt)
{
	struct i2c_client *client;
	u8 status;
	u8 length;
	u8 report_id;
	u8 buf[3];
	unsigned long cfg_crc;

	client = mxt->client;
	length = mxt->message_size;

	report_id = message[0];
 
	if ((mxt->nontouch_msg_only == 0) || (!IS_TOUCH_OBJECT(object))) {
		mutex_lock(&mxt->msg_mutex);

		/* Copy the message to buffer */
		if (mxt->msg_buffer_startp < MXT_MESSAGE_BUFFER_SIZE)
			mxt->msg_buffer_startp++;
		else
			mxt->msg_buffer_startp = 0;

		if (mxt->msg_buffer_startp == mxt->msg_buffer_endp) {
			mxt_debug(DEBUG_TRACE,
				  "[touch] Message buf full, discarding last entry.\n");
			if (mxt->msg_buffer_endp < MXT_MESSAGE_BUFFER_SIZE)
				mxt->msg_buffer_endp++;
			else
				mxt->msg_buffer_endp = 0;
		}
		memcpy((mxt->messages + mxt->msg_buffer_startp * length),
		       message, length);
		mutex_unlock(&mxt->msg_mutex);
	}
	if(d_flag)
        	dev_info(&client->dev, "[touch] %x %x %x %x %x %x %x %x\n",message[0],message[1],message[2],message[3],message[4],message[5],message[6],message[7]);
	switch (object) {
	case MXT_GEN_COMMANDPROCESSOR_T6:
		status = message[1];
		if (status & MXT_MSGB_T6_COMSERR)
			dev_err(&client->dev, "[touch] maXTouch checksum error\n");
		if (status & MXT_MSGB_T6_CFGERR) {
			/*
			 * Configuration error. A proper configuration
			 * needs to be written to chip and backed up. Refer
			 * to protocol document for further info.
			 */
			dev_err(&client->dev, "[touch] maXTouch configuration error\n");
		}
		if (status & MXT_MSGB_T6_CAL) {
			/* Calibration in action, no need to react */
			dev_info(&client->dev,
				 "[touch] maXTouch calibration in progress\n");
			reset_flag =false;
			boot_cali_status = status;

			if ((mxt_firmware_id==MXT540S_FIRMWARE_ID_10 || mxt_firmware_id==MXT540S_FIRMWARE_ID_11)&& calibration_flag == GOLDEN_REFERENCE_GEN)
			{
				dev_info(&client->dev,"[touch] GOLDEN_REFERENCE_GEN --> CALIBRATION!\n"); 
				calibration_flag = GOLDEN_REFERENCE_CALIBRATION;
			}
		}
		if (status & MXT_MSGB_T6_SIGERR) {
			/*
			 * Signal acquisition error, something is seriously
			 * wrong, not much we can in the driver to correct
			 * this
			 */
			dev_err(&client->dev, "[touch] maXTouch acquisition error\n");
		}
		if (status & MXT_MSGB_T6_OFL) {
			/*
			 * Cycle overflow, the acquisition is too short.
			 * Can happen temporarily when there's a complex
			 * touch shape on the screen requiring lots of
			 * processing.
			 */
			dev_err(&client->dev, "[touch] maXTouch cycle overflow\n");
		}
		if (status & MXT_MSGB_T6_RESET) {
			/* Chip has reseted, no need to react. */
			reset_flag =false;
			dev_info(&client->dev, "[touch] maXTouch chip reset\n");
		}
		if (status == 0) {
			/* Chip status back to normal. */
			dev_info(&client->dev, "[touch] maXTouch status normal\n");

			if ((boot_cali_status & MXT_MSGB_T6_CAL) && mxt_firmware_id==MXT540S_FIRMWARE_ID_09){
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt), 0x1);
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt)+1, touch_vendor_id);
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt)+2, 0x0);
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt)+3, 0x0);
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt)+6, USER_DATA_MAGIC_0_FOTA_V1);
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt)+7, USER_DATA_MAGIC_1_FOTA_V1);
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) + MXT_ADR_T6_BACKUPNV,MXT_CMD_T6_BACKUP);
				ac.calibration_status = 1;
				boot_cali_status = 0;
			}
			if ((mxt_firmware_id==MXT540S_FIRMWARE_ID_10 || mxt_firmware_id==MXT540S_FIRMWARE_ID_11) && calibration_flag == GOLDEN_REFERENCE_CALIBRATION)
			{
				dev_info(&client->dev,"[touch] GOLDEN_REFERENCE_CALIBRATION --> NORMAL!\n");
				calibration_flag = GOLDEN_REFERENCE_NORMAL;

			}
			if(calibration_flag == SELFTEST_CALIBRATION_START){
				ac.selftest_calibration_status = 1;
			}

		}

             if (cfg_flag){
		    buf[0]=message[2];
		    buf[1]=message[3];
		    buf[2]=message[4];
		    cfg_crc= buf[2] << 16 | buf[1] <<8 | buf[0];
		    mxt->configuration_crc = cfg_crc;
		    dev_info(&client->dev,"[touch] Touch: configuration checksum is %lx\n",cfg_crc);

		    if (cfg_crc != ac.touch_panel_config_checksum[touch_vendor_id]){

		        dev_info(&client->dev,"[touch] Touch: start uboot parameter BACKUP\n");
			set_uboot_para(touch_vendor_id,cfg_crc);
#if 0
	              mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) + MXT_ADR_T6_BACKUPNV,MXT_CMD_T6_BACKUP);
#endif
		    }else
	              dev_info(&client->dev,"[touch] Touch: uboot parameter is BACKUP already\n");

		    cfg_flag =false;
	      }

		break;

	case MXT_TOUCH_MULTITOUCHSCREEN_T9:
		mxt->hasTouch = true;
		process_T9_message(message, mxt, 0);
		break;

	case MXT_SPT_GPIOPWM_T19:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev, "[touch] Receiving GPIO message\n");
		break;

	case MXT_SPT_SELFTEST_T25:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev, "[touch] Receiving Self-Test msg\n");

		if (message[MXT_MSG_T25_STATUS] == MXT_MSGR_T25_OK) {
				dev_err(&client->dev,
					 "[touch] maXTouch: Self-Test OK\n");
				selftest_flag = true;
		} else {
			dev_err(&client->dev,
				"maXTouch: Self-Test Failed [%02x]:"
				"{%02x,%02x,%02x,%02x,%02x}\n",
				message[MXT_MSG_T25_STATUS],
				message[MXT_MSG_T25_STATUS + 0],
				message[MXT_MSG_T25_STATUS + 1],
				message[MXT_MSG_T25_STATUS + 2],
				message[MXT_MSG_T25_STATUS + 3],
				message[MXT_MSG_T25_STATUS + 4]
			    );
			selftest_flag = false;
		}
		break;

	case MXT_PROCG_NOISESUPPRESSION_T62: // the nosie message
		dev_info(&client->dev, "[touch] Noise Message with bytes: 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\n",
			      message[1], message[2], message[3], message[4], message[5]);
		break;

	case MXT_GOLDENREFERENCES_T66:
        	dev_info(&client->dev, "[touch] %x %x %x %x %x %x %x %x\n",message[0],message[1],message[2],message[3],message[4],message[5],message[6],message[7]);
		status = message[1];
		if(status  & MXT_MSGB_T66_GEN){
			if(calibration_flag == GOLDEN_REFERENCE_PRIME)
			{
				dev_info(&client->dev,"[touch] GOLDEN_REFERENCE_PRIME --> GEN!\n");
				calibration_flag = GOLDEN_REFERENCE_GEN;
				mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 0, 0x1b);//generate
			}
		}
		if(status  & MXT_MSGB_T66_ERROR){
			if(calibration_flag == GOLDEN_REFERENCE_NORMAL)
			{
				dev_info(&client->dev,"[touch] GOLDEN_REFERENCE_ERROR --> STORE!\n");
				calibration_flag = GOLDEN_REFERENCE_ERROR;
				mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 0, 0x1f);//store

			}
		}
		if(status  & MXT_MSGB_T66_STORE){
			if(calibration_flag == GOLDEN_REFERENCE_NORMAL)
			{
				dev_info(&client->dev,"[touch] GOLDEN_REFERENCE_NORMAL --> STORE!\n");
				calibration_flag = GOLDEN_REFERENCE_STORE;
				mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 0, 0x1f);//store

			}
		}
		if(status  & MXT_MSGB_T66_FINISH){
			if(calibration_flag == GOLDEN_REFERENCE_STORE)
			{
//				del_timer(&ts_timer_calibration);
				dev_info(&client->dev,"[touch] GOLDEN_REFERENCE_STORE -->END!\n");
				if (mxt_firmware_id==MXT540S_FIRMWARE_ID_10)
					mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x13);
				else if (mxt_firmware_id==MXT540S_FIRMWARE_ID_11)
					mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x03);
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt), 0x1);
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt)+1, touch_vendor_id);
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt)+2, 0x0);
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt)+3, 0x0);
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt)+6, USER_DATA_MAGIC_0_FOTA_V1);
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt)+7, USER_DATA_MAGIC_1_FOTA_V1);
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) + MXT_ADR_T6_BACKUPNV,MXT_CMD_T6_BACKUP);
				ac.calibration_status = 1;
				calibration_flag = GOLDEN_REFERENCE_FINISH;
			}
			if(calibration_flag == GOLDEN_REFERENCE_ERROR)
			{
				dev_info(&client->dev,"[touch] GOLDEN_REFERENCE_STORE (ERROR) --> END!\n");
			}
		}
		break;

	default:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev, "[touch] maXTouch: Type=%d Unknown message!\n", object);

		break;
	}

	return 0;
}

static void setInterruptable(struct i2c_client *client, bool interrupt){
    struct mxt_data *mxt = i2c_get_clientdata(client);
    down(&mxt->sem);
    mxt->interruptable = interrupt;
    up(&mxt->sem);
}
/*
 * Processes messages when the interrupt line (CHG) is asserted. Keeps
 * reading messages until a message with report ID 0xFF is received,
 * which indicates that there is no more new messages.
 *
 */

static void mxt_worker(struct work_struct *work)
{
	struct mxt_data *mxt;
	struct i2c_client *client;

	u8 *message;
	u16 message_length;
	u16 message_addr;
	u8 report_id;
	u8 object;
	int error;
	int i;
	char *message_string;
	char *message_start;

	message = NULL;
	mxt = container_of(work, struct mxt_data, dwork.work);
	client = mxt->client;
	message_addr = mxt->msg_proc_addr;
	message_length = mxt->message_size;


	if (message_length < 256) {
		message = kmalloc(message_length, GFP_KERNEL);
		if (message == NULL) {
			dev_err(&client->dev, "Error allocating memory\n");
			setInterruptable(client, false);
			return;
		}
	} else {
		dev_err(&client->dev,
			"[touch] Message length larger than 256 bytes not supported\n");
		setInterruptable(client, false);
		return;
	}

	mxt_debug(DEBUG_TRACE, "[touch] maXTouch worker active: \n");
	do {
		/* Read next message, reread on failure. */
		/* -1 TO WORK AROUND A BUG ON 0.9 FW MESSAGING, needs */
		/* to be changed back if checksum is read */
		mxt->message_counter++;
		for (i = 1; i < I2C_RETRY_COUNT; i++) {
			error = mxt_read_block(client,
					       message_addr,
					       message_length - 1, message);
			if (error >= 0)
				break;
			mxt->read_fail_counter++;
			dev_err(&client->dev,
				"[touch] Failure reading maxTouch device\n");
		}
		if (error < 0) {
			kfree(message);
			setInterruptable(client, false);
			return;
		}

		if (mxt->address_pointer != message_addr)
			mxt->valid_ap = 0;
		report_id = message[0];

		if (debug >= DEBUG_RAW) {
			mxt_debug(DEBUG_RAW, "[touch] %s message [msg count: %08x]:",
				  REPORT_ID_TO_OBJECT_NAME(report_id, mxt),
				  mxt->message_counter);
			/* 5 characters per one byte */
			message_string = kmalloc(message_length * 5,
						 GFP_KERNEL);
			if (message_string == NULL) {
				dev_err(&client->dev,
					"[touch] Error allocating memory\n");
				kfree(message);
				setInterruptable(client, false);
				return;
			}
			message_start = message_string;
			for (i = 0; i < message_length; i++) {
				message_string +=
				    sprintf(message_string,
					    "0x%02X ", message[i]);
			}
			mxt_debug(DEBUG_RAW, "%s", message_start);
			kfree(message_start);
		}

		if ((report_id != MXT_END_OF_MESSAGES) && (report_id != 0)) {
			memcpy(mxt->last_message, message, message_length);
			mxt->new_msgs = 1;
			smp_wmb();
			/* Get type of object and process the message */
			object = mxt->rid_map[report_id].object;
			process_message(message, object, mxt);
		}
		if(mxt->read_chg!=NULL)
		mxt_debug(DEBUG_TRACE, "[touch] chgline: %d\n", mxt->read_chg());
	} while (comms ? (mxt->read_chg() == 0) :
		 ((report_id != MXT_END_OF_MESSAGES) && (report_id != 0)));

	/* All messages processed, send the events) */
	if(mxt->hasTouch){
	    process_T9_message(NULL, mxt, 1);
	    mxt->hasTouch = false;
	}

	kfree(message);

#if 0 //test low level trigger
	if (mxt->read_chg() == 0){
		schedule_delayed_work(&mxt->dwork, 0);
	}else{

		ts_enable_irq();
//printk("[touch] mxt_irq_end\n");
	}
#else
		ts_clear_irq();
		ts_enable_irq();
#endif

}

/*
 * The maXTouch device will signal the host about a new message by asserting
 * the CHG line. This ISR schedules a worker routine to read the message when
 * that happens.
 */

static irqreturn_t mxt_irq_handler(int irq, void *_mxt)
{

	struct mxt_data *mxt = _mxt;

	mxt->irq_counter++;

	if(!ts_check_irq_enable()) {
		return IRQ_NONE;
	}

	if (mxt->valid_interrupt()) {
//    		disable_irq(mxt->irq);
		ts_disable_irq();
		ts_clear_irq();
		/* Send the signal only if falling edge generated the irq. */
		cancel_delayed_work(&mxt->dwork);
		schedule_delayed_work(&mxt->dwork, 0);
		mxt->valid_irq_counter++;
	} else {
		mxt->invalid_irq_counter++;
//		ts_enable_irq();
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static int recovery_from_bootMode(struct i2c_client *client){
    u8 buf[MXT_ID_BLOCK_SIZE];
    int ret;
    int retry = 40;
    int times;
    unsigned char data[] = {0x01, 0x01};
    struct i2c_msg wmsg;

    wmsg.addr = MXT_BL_ADDRESS;
    wmsg.flags = I2C_M_WR;
    wmsg.len = 2;
    wmsg.buf = data;
    dev_err(&client->dev, "[touch] ---------Touch: Try to leave the bootloader mode!\n");
	/*Write two nosense bytes to I2C address "0x35" in order to force touch to leave the bootloader mode.*/
    i2c_transfer(client->adapter, &wmsg, 1);
    mdelay(10);

    /* Read Device info to check if chip is valid */
    for(times = 0; times < retry; times++ ){
        ret = mxt_read_block(client, MXT_ADDR_INFO_BLOCK, MXT_ID_BLOCK_SIZE, (u8 *) buf);
	  if(ret >= 0)
	      break;

	  dev_err(&client->dev, "[touch] Retry addressing I2C address 0x%02X with %d times\n", client->addr,times+1);
	  msleep(25);
    }

    if(ret >= 0){
        dev_err(&client->dev, "[touch] ---------Touch: Successfully leave the bootloader mode!\n");
		ret = 0;
    }
    return ret;
}
static bool isInBootLoaderMode(struct i2c_client *client){
    u8 buf[2];
	int ret;
	int retry = 2;
	int times;
	struct i2c_msg rmsg;

	rmsg.addr=MXT_BL_ADDRESS;
	rmsg.flags = I2C_M_RD;
	rmsg.len = 2;
	rmsg.buf = buf;

    /* Read 2 byte from boot loader I2C address to make sure touch chip is in bootloader mode */
	for(times = 0; times < retry; times++ ){
	     ret = i2c_transfer(client->adapter, &rmsg, 1);
	     if(ret >= 0)
		 	break;

	     mdelay(25);
	}
	dev_err(&client->dev, "[touch] The touch is %s in bootloader mode.\n", (ret < 0 ? "not" : "indeed"));
	return ret >= 0;
}

static bool isNeedUpgradeFirmware(u8 * id_data){
//	TOUCH_PRINTK("isNeedUpgradeFirmware\n");
//	TOUCH_PRINTK("*(id_data+2)=%x\n",*(id_data+2));
//	TOUCH_PRINTK("MXT540S_FIRMWARE_ID_10=%x\n",MXT540S_FIRMWARE_ID_10);
//	TOUCH_PRINTK("*(id_data+3)=%x\n",*(id_data+3));
    	if(*(id_data+2) < MXT540S_FINAL_FIRMWARE_ID){
		return 1;
    	}else if(*(id_data+2) == MXT540S_FINAL_FIRMWARE_ID){

		if(*(id_data+3) != MXT540S_FINAL_BUILD_ID)
			return 1;
		else
			return 0;
	}else
		return 0;
}


static void backup_T7_config(struct mxt_data *mxt){

	int err;
	u8 buf[0x4];
	TOUCH_PRINTK("backup_T7_config\n");
        err = mxt_read_block(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt), 0x4, (u8 *) buf);
        if (err < 0)	TOUCH_PRINTK("backup_T7_config() : mxt_read_block failure.\n");

	T7_BYTE_0_IDLEACQINT = buf[0];
	T7_BYTE_1_ACTVACQINT = buf[1];
}

static bool isNeedLoadDefaultConfig(struct mxt_data *mxt){

	int err;
	u8 buf[0x8];
	TOUCH_PRINTK("isNeedLoadDefaultConfig\n");
        err = mxt_read_block(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt), 0x8, (u8 *) buf);
        if (err < 0)	TOUCH_PRINTK("isNeedLoadDefaultConfig() : mxt_read_block failure.\n");

	if((buf[6]== USER_DATA_MAGIC_0 && buf[7]==USER_DATA_MAGIC_1) ||
	   (buf[6]== USER_DATA_MAGIC_0_FOTA_V1 && buf[7]==USER_DATA_MAGIC_1_FOTA_V1)){
		if(buf[3] == 1){
			TOUCH_PRINTK("Use backup configuration.\n");
			return 0;
		}else{
			TOUCH_PRINTK("load default configuration\n");
			return 1;
		}
	}else{
		TOUCH_PRINTK("load default configuration\n");
		return 1;
	}
}




static void checkGR(struct mxt_data *mxt, u8 vendor_id){

	int err;
	u8 buf[0x8];

	TOUCH_PRINTK("checkGR\n");

        err = mxt_read_block(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt), 0x8, (u8 *) buf);

        if (err < 0)	TOUCH_PRINTK("checkGR() : mxt_read_block failure.\n");

	//buf[0] : is calibration?
	//buf[1] : Does ME172v use the same TP?
	//buf[2] : does Firmware change?
	//buf[3] : Do not use default configuration.
	//buf[4] :
	//buf[5] :
	//buf[6] : magic 0
	//buf[7] : magic 1

	if((buf[6]== USER_DATA_MAGIC_0 && buf[7]==USER_DATA_MAGIC_1) ||
	   (buf[6]== USER_DATA_MAGIC_0_FOTA_V1 && buf[7]==USER_DATA_MAGIC_1_FOTA_V1)){
		//firmware change
		if(buf[2]==1){
			if (mxt_firmware_id==MXT540S_FIRMWARE_ID_10 )
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x12);
			else if (mxt_firmware_id==MXT540S_FIRMWARE_ID_11)
				mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x02);
			ac.calibration_status = 0;
			selftest_flag = false;
			TOUCH_PRINTK("firmware change\n");
		}else{
			//calibration: no, the same TP : no
			if(buf[0]==0 && vendor_id != buf[1]){
				if (mxt_firmware_id==MXT540S_FIRMWARE_ID_10)
					mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x12);
				else if (mxt_firmware_id==MXT540S_FIRMWARE_ID_11)
					mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x02);
				ac.calibration_status = 0;
				selftest_flag = false;
				TOUCH_PRINTK("calibration: no, the same TP : no\n");
			//calibration: no, the same TP : yes
			}else if(buf[0]==0 && vendor_id == buf[1]){
				if (mxt_firmware_id==MXT540S_FIRMWARE_ID_10)
					mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x12);
				else if (mxt_firmware_id==MXT540S_FIRMWARE_ID_11)
					mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x02);
				ac.calibration_status = 0;
				selftest_flag = false;
				TOUCH_PRINTK("calibration: no, the same TP : yes\n");
			//calibration: yes, the same TP : no
			}else if(buf[0]!=0 && vendor_id != buf[1]){
				if (mxt_firmware_id==MXT540S_FIRMWARE_ID_10)
					mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x12);
				else if (mxt_firmware_id==MXT540S_FIRMWARE_ID_11)
					mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x02);
				ac.calibration_status = 0;
				selftest_flag = false;
				TOUCH_PRINTK("calibration: yes, the same TP : no\n");
			//calibration: yes, the same TP : yes
			}else{
	/*			if (mxt_firmware_id==MXT540S_FIRMWARE_ID_10)
					mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x13);
				else if (mxt_firmware_id==MXT540S_FIRMWARE_ID_11)
					mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x03);*/
				ac.calibration_status = 1;
				selftest_flag = false;
				TOUCH_PRINTK("calibration: yes, the same TP : yes\n");
			}
		}

		TOUCH_PRINTK("checkGR end\n");
		return;
	}else{
		if (mxt_firmware_id==MXT540S_FIRMWARE_ID_10)
			mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x12);
		else if (mxt_firmware_id==MXT540S_FIRMWARE_ID_11)
			mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x02);
		ac.calibration_status = 0;
		selftest_flag = false;
		TOUCH_PRINTK("magic number error!!\n");
		TOUCH_PRINTK("Goldne calibration = NO!!\n");
		TOUCH_PRINTK("checkGR end\n");
		return;
	}
}



int check_refs_before_selftest(struct mxt_data *mxt)
{
	int i, j;
	u16 *data;
	u16 diagnostics_reg;
	int offset = 0;
	int size;
	int read_size;
	int error;
	char *buf_start;
	u16 debug_data_addr;
	u16 page_address;
	u8 page;
	u8 debug_command_reg;
	size_t count;
	char *buf;

	u32 x_sum_base = 0;
	u32 y_sum_base = 0;
	u32 x_sum = 0;
	u32 y_sum = 0;


	data = kmalloc(mxt->device_info.num_nodes * sizeof(u16),
				  GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

		diagnostics_reg = MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6,
						mxt) + MXT_ADR_T6_DIAGNOSTIC;
//		if (count > (mxt->device_info.num_nodes * 2))
			count = mxt->device_info.num_nodes;

		debug_data_addr = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTIC_T37, mxt) +
		    MXT_ADR_T37_DATA;
		page_address = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTIC_T37, mxt) +
		    MXT_ADR_T37_PAGE;
		error = mxt_read_block(mxt->client, page_address, 1, &page);
		if (error < 0)
			return error;
		mxt_debug(DEBUG_TRACE, "[touch] debug data page = %d\n", page);
		while (page != 0) {
			error = mxt_write_byte(mxt->client,
					       diagnostics_reg,
					       MXT_CMD_T6_PAGE_DOWN);
			if (error < 0)
				return error;
			/* Wait for command to be handled; when it has, the
			   register will be cleared. */
			debug_command_reg = 1;
			while (debug_command_reg != 0) {
				error = mxt_read_block(mxt->client,
						       diagnostics_reg, 1,
						       &debug_command_reg);
				if (error < 0)
					return error;
				mxt_debug(DEBUG_TRACE,
					  "[touch] Waiting for debug diag command "
					  "to propagate...\n");

			}
			error = mxt_read_block(mxt->client, page_address, 1,
					       &page);
			if (error < 0)
				return error;
			mxt_debug(DEBUG_TRACE, "[touch] debug data page = %d\n", page);
		}

		/*
		 * Lock mutex to prevent writing some unwanted data to debug
		 * command register. User can still write through the char
		 * device interface though. TODO: fix?
		 */

		mutex_lock(&mxt->debug_mutex);
		/* Configure Debug Diagnostics object to show deltas/refs */
		error = mxt_write_byte(mxt->client, diagnostics_reg,
				       MXT_CMD_T6_REFERENCES_MODE);

		/* Wait for command to be handled; when it has, the
		 * register will be cleared. */
		debug_command_reg = 1;
		while (debug_command_reg != 0) {
			error = mxt_read_block(mxt->client,
					       diagnostics_reg, 1,
					       &debug_command_reg);
			if (error < 0)
				return error;
			mxt_debug(DEBUG_TRACE, "[touch] Waiting for debug diag command "
				  "to propagate...\n");

		}

		if (error < 0) {
			printk(KERN_WARNING
			       "[touch] Error writing to maXTouch device!\n");
			return error;
		}

		size = mxt->device_info.num_nodes * sizeof(u16);

		while (size > 0) {
			read_size = size > 128 ? 128 : size;
			mxt_debug(DEBUG_TRACE,
				  "[touch] Debug data read loop, reading %d bytes...\n",
				  read_size);
			msleep(200);
			error = mxt_read_block(mxt->client,
					       debug_data_addr,
					       read_size,
					       (u8 *) &data[offset]);
			if (error < 0) {
				printk(KERN_WARNING
				       "[touch] Error reading debug data\n");
				goto error;
			}
			offset += read_size / 2;
			size -= read_size;

			/* Select next page */
			error = mxt_write_byte(mxt->client, diagnostics_reg,
					       MXT_CMD_T6_PAGE_UP);
			if (error < 0) {
				printk(KERN_WARNING
				       "[touch] Error writing to maXTouch device!\n");
				goto error;
			}
		}
		mutex_unlock(&mxt->debug_mutex);

	buf_start = buf;
	i = 0;
	j = 0;
	while (((buf - buf_start) < (count - 6)) &&
	       (i < mxt->device_info.num_nodes)) {

		x_sum = x_sum + ((u16) le16_to_cpu(data[i]) - 16384);

		if(((i+1) % (mxt->device_info.y_size)) == 0 && i!=0){

			x_sum = x_sum / mxt->device_info.y_size;

			if(x_sum_base == 0) x_sum_base = x_sum;
#if 0
			printk(KERN_WARNING "[touch] x_sum=%ld\n",x_sum);
			printk(KERN_WARNING "[touch] x_sum_base=%ld\n",x_sum_base);
			printk(KERN_WARNING "[touch] (x_sum - x_sum_base)=%ld\n",abs(x_sum - x_sum_base));
			printk(KERN_WARNING "[touch] ((x_sum - x_sum_base)/x_sum_base)=%ld\n",(abs(x_sum - x_sum_base)/x_sum_base));
			printk(KERN_WARNING "[touch] (((x_sum - x_sum_base)/x_sum_base)*100)=%ld\n",((abs(x_sum - x_sum_base)*100/x_sum_base)));
			printk(KERN_WARNING "[touch] (((x_sum - x_sum_base)/x_sum_base)*100)>11=%ld\n",((abs(x_sum - x_sum_base)*100/x_sum_base))>11);
#endif
//for debug			if(j==5)x_sum_base=19000;
			if(((abs(x_sum - x_sum_base)*100/x_sum_base))>11){
				printk(KERN_WARNING "[touch] ERROR X%d : average=%d precent=%d\n",j,x_sum, ((abs(x_sum - x_sum_base)*100/x_sum_base)));

				if(touch_vendor_id == OFILM){
					//skip edge and corner
					if(j==1 || j==mxt->device_info.x_size-1){
						printk(KERN_WARNING "O-FILM XSENSE PASS!!!\n");
					}else{
						return -1;
					}

				}else{
					return -1;
				}

			}
			printk(KERN_WARNING "[touch] X%d : average=%d precent=%d\n",j,x_sum, ((abs(x_sum - x_sum_base)*100/x_sum_base)));
			x_sum_base = x_sum;
			x_sum = 0;
			j++;
		}
		i++;
	}

	i = 0;
	j = 0;
	while (((buf - buf_start) < (count - 6)) &&
	       (j < mxt->device_info.y_size)) {

		for(;i<mxt->device_info.num_nodes;i=i+18)
			y_sum = y_sum + ((u16) le16_to_cpu(data[i])-16384);

		y_sum = y_sum / mxt->device_info.x_size;

		if(y_sum_base == 0) y_sum_base = y_sum;
#if 0
			printk(KERN_WARNING "[touch] y_sum=%ld\n",y_sum);
			printk(KERN_WARNING "[touch] y_sum_base=%ld\n",y_sum_base);
			printk(KERN_WARNING "[touch] (y_sum - y_sum_base)=%ld\n",abs(y_sum - y_sum_base));
			printk(KERN_WARNING "[touch] ((y_sum - y_sum_base)/y_sum_base)=%ld\n",(abs(y_sum - y_sum_base)/y_sum_base));
			printk(KERN_WARNING "[touch] (((y_sum - y_sum_base)/y_sum_base)*100)=%ld\n",((abs(y_sum - y_sum_base)*100/y_sum_base)));
			printk(KERN_WARNING "[touch] (((y_sum - y_sum_base)/y_sum_base)*100)>11=%ld\n",((abs(y_sum - y_sum_base)*100/y_sum_base))>11);
#endif

		if(((abs(y_sum - y_sum_base)*100/y_sum_base))>11){
			printk(KERN_WARNING "[touch] ERROR Y%d : average=%d precent=%d\n",j,y_sum, ((abs(y_sum - y_sum_base)*100/y_sum_base)));

			if(touch_vendor_id == OFILM){
				//skip edge and corner
				if(j==1 || j==mxt->device_info.y_size-1){
					printk(KERN_WARNING "O-FILM XSENSE PASS!!!\n");
				}else{
					return -1;
				}

			}else{
				return -1;
			}
		}
		printk(KERN_WARNING "[touch] Y%d : average=%d precent=%d\n",j,y_sum, ((abs(y_sum - y_sum_base)*100/y_sum_base)));
		y_sum_base = y_sum;
		y_sum = 0;
		i=++j;
	}
	kfree(data);
	return 0;
 error:
	kfree(data);
	mutex_unlock(&mxt->debug_mutex);
	return error;
}


/******************************************************************************/
/* Initialization of driver                                                   */
/******************************************************************************/

int __devinit mxt_identify(struct i2c_client *client,
				  struct mxt_data *mxt, u8 * id_block_data)
{
	u8 buf[7];
	int error;
	int identified;
      int retry = 2;
      int times;

	identified = 0;
	/* Read Device info to check if chip is valid */
       for(times = 0; times < retry; times++ ){
	     error = mxt_read_block(client, MXT_ADDR_INFO_BLOCK, MXT_ID_BLOCK_SIZE,
			       (u8 *) buf);
	     if(error >= 0)
		 	break;

	     mdelay(25);
	 }

	if (error < 0) {
		mxt->read_fail_counter++;
		dev_err(&client->dev, "[touch] Failure accessing maXTouch device\n");
		return -EIO;
	}

	memcpy(id_block_data, buf, MXT_ID_BLOCK_SIZE);

	mxt->device_info.family_id = buf[0];
	mxt->device_info.variant_id = buf[1];
	mxt->device_info.major = ((buf[2] >> 4) & 0x0F);
	mxt->device_info.minor = (buf[2] & 0x0F);
	mxt->device_info.build = buf[3];
	mxt->device_info.x_size = buf[4];
	mxt->device_info.y_size = buf[5];
	mxt->device_info.num_objs = buf[6];
	mxt->device_info.num_nodes = mxt->device_info.x_size *
	    mxt->device_info.y_size;

	mxt_firmware_id = mxt->device_info.major << 4 | mxt->device_info.minor;
	/*
	 * Check Family & Variant Info; warn if not recognized but
	 * still continue.
	 */

	/* MXT224 */
	if (mxt->device_info.family_id == MXT224_FAMILYID) {
		strcpy(mxt->device_info.family_name, "mXT224");

		if (mxt->device_info.variant_id == MXT224_CAL_VARIANTID) {
			strcpy(mxt->device_info.variant_name, "Calibrated");
		} else if (mxt->device_info.variant_id ==
			   MXT224_UNCAL_VARIANTID) {
			strcpy(mxt->device_info.variant_name, "Uncalibrated");
		} else {
			dev_err(&client->dev,
				"[touch] Warning: maXTouch Variant ID [%d] not "
				"supported\n", mxt->device_info.variant_id);
			strcpy(mxt->device_info.variant_name, "UNKNOWN");
			/* identified = -ENXIO; */
		}

		/* MXT1386 */
	} else if (mxt->device_info.family_id == MXT1386_FAMILYID) {
		strcpy(mxt->device_info.family_name, "mXT1386");

		if (mxt->device_info.variant_id == MXT1386_CAL_VARIANTID) {
			strcpy(mxt->device_info.variant_name, "Calibrated");
		} else {
			dev_err(&client->dev,
				"[touch] Warning: maXTouch Variant ID [%d] not "
				"supported\n", mxt->device_info.variant_id);
			strcpy(mxt->device_info.variant_name, "UNKNOWN");
			/* identified = -ENXIO; */
		}
		/* MXT540S */
	} else if (mxt->device_info.family_id == MXT540S_FAMILYID) {
		strcpy(mxt->device_info.family_name, "mXT540S");

		if (mxt->device_info.variant_id == MXT540S_CAL_VARIANTID) {
			strcpy(mxt->device_info.variant_name, "Calibrated");
		} else {
			dev_err(&client->dev,
				"[touch] Warning: maXTouch Variant ID [%d] not "
				"supported\n", mxt->device_info.variant_id);
			strcpy(mxt->device_info.variant_name, "UNKNOWN");
			/* identified = -ENXIO; */
		}
		/* Unknown family ID! */
	} else {
		dev_err(&client->dev,
			"[touch] Warning: maXTouch Family ID [%d] not supported\n",
			mxt->device_info.family_id);
		strcpy(mxt->device_info.family_name, "UNKNOWN");
		strcpy(mxt->device_info.variant_name, "UNKNOWN");
		/* identified = -ENXIO; */
	}

	dev_info(&client->dev,
		 "[touch] Atmel maXTouch (Family %s (%X), Variant %s (%X)) Firmware "
		 "version [%d.%d] Build %d\n",
		 mxt->device_info.family_name,
		 mxt->device_info.family_id,
		 mxt->device_info.variant_name,
		 mxt->device_info.variant_id,
		 mxt->device_info.major,
		 mxt->device_info.minor, mxt->device_info.build);
	dev_info(&client->dev,
		 "[touch] Atmel maXTouch Configuration "
		 "[X: %d] x [Y: %d]\n",
		 mxt->device_info.x_size, mxt->device_info.y_size);
	return identified;
}

/*
 * Reads the object table from maXTouch chip to get object data like
 * address, size, report id. For Info Block CRC calculation, already read
 * id data is passed to this function too (Info Block consists of the ID
 * block and object table).
 *
 */
static int __devinit mxt_read_object_table(struct i2c_client *client,
					   struct mxt_data *mxt,
					   u8 *raw_id_data)
{
	u16 report_id_count;
	u8 buf[MXT_OBJECT_TABLE_ELEMENT_SIZE];
	u8 *raw_ib_data;
	u8 object_type;
	u16 object_address;
	u16 object_size;
	u8 object_instances;
	u8 object_report_ids;
	u16 object_info_address;
	u32 crc;
	u32 calculated_crc;
	int i;
	int error;

	u8 object_instance;
	u8 object_report_id;
	u8 report_id;
	int first_report_id;
	int ib_pointer;
	struct mxt_object *object_table;

	mxt_debug(DEBUG_TRACE, "[touch] maXTouch driver reading configuration\n");

	object_table = kzalloc(sizeof(struct mxt_object) *
			       mxt->device_info.num_objs, GFP_KERNEL);
	if (object_table == NULL) {
		printk(KERN_WARNING "[touch] maXTouch: Memory allocation failed!\n");
		error = -ENOMEM;
		goto err_object_table_alloc;
	}

	raw_ib_data = kmalloc(MXT_OBJECT_TABLE_ELEMENT_SIZE *
			      mxt->device_info.num_objs + MXT_ID_BLOCK_SIZE,
			      GFP_KERNEL);
	if (raw_ib_data == NULL) {
		printk(KERN_WARNING "[touch] maXTouch: Memory allocation failed!\n");
		error = -ENOMEM;
		goto err_ib_alloc;
	}

	/* Copy the ID data for CRC calculation. */
	memcpy(raw_ib_data, raw_id_data, MXT_ID_BLOCK_SIZE);
	ib_pointer = MXT_ID_BLOCK_SIZE;

	mxt->object_table = object_table;

	mxt_debug(DEBUG_TRACE, "[touch] maXTouch driver Memory allocated\n");

	object_info_address = MXT_ADDR_OBJECT_TABLE;

	report_id_count = 0;
	for (i = 0; i < mxt->device_info.num_objs; i++) {
		mxt_debug(DEBUG_TRACE, "[touch] Reading maXTouch at [0x%04x]: ",
			  object_info_address);

		error = mxt_read_block(client, object_info_address,
				       MXT_OBJECT_TABLE_ELEMENT_SIZE, buf);

		if (error < 0) {
			mxt->read_fail_counter++;
			dev_err(&client->dev,
				"[touch] maXTouch Object %d could not be read\n", i);
			error = -EIO;
			goto err_object_read;
		}

		memcpy(raw_ib_data + ib_pointer, buf,
		       MXT_OBJECT_TABLE_ELEMENT_SIZE);
		ib_pointer += MXT_OBJECT_TABLE_ELEMENT_SIZE;

		object_type = buf[0];
		object_address = (buf[2] << 8) + buf[1];
		object_size = buf[3] + 1;
		object_instances = buf[4] + 1;
		object_report_ids = buf[5];
		mxt_debug(DEBUG_TRACE, "[touch] Type=%03d, Address=0x%04x, "
			  "Size=0x%02x, %d instances, %d report id's\n",
			  object_type,
			  object_address,
			  object_size, object_instances, object_report_ids);
#if 0
		TOUCH_PRINTK("Type=%03d, Address=0x%04x, "
			  "Size=0x%02x, %d instances, %d report id's\n",
			  object_type,
			  object_address,
			  object_size, object_instances, object_report_ids);
#endif
		/* TODO: check whether object is known and supported? */

		/* Save frequently needed info. */
		if (object_type == MXT_GEN_MESSAGEPROCESSOR_T5) {
			mxt->msg_proc_addr = object_address;
			mxt->message_size = object_size;
			printk(KERN_ALERT "[touch] message length: %d", object_size);
		}

		object_table[i].type = object_type;
		object_table[i].chip_addr = object_address;
		object_table[i].size = object_size;
		object_table[i].instances = object_instances;
		object_table[i].num_report_ids = object_report_ids;
		report_id_count += object_instances * object_report_ids;

		object_info_address += MXT_OBJECT_TABLE_ELEMENT_SIZE;
	}

	mxt->rid_map =
	    kzalloc(sizeof(struct report_id_map) * (report_id_count + 1),
		    /* allocate for report_id 0, even if not used */
		    GFP_KERNEL);
	if (mxt->rid_map == NULL) {
		printk(KERN_WARNING "[touch] maXTouch: Can't allocate memory!\n");
		error = -ENOMEM;
		goto err_rid_map_alloc;
	}

	mxt->messages = kzalloc(mxt->message_size * MXT_MESSAGE_BUFFER_SIZE,
				GFP_KERNEL);
	if (mxt->messages == NULL) {
		printk(KERN_WARNING "[touch] maXTouch: Can't allocate memory!\n");
		error = -ENOMEM;
		goto err_msg_alloc;
	}

	mxt->last_message = kzalloc(mxt->message_size, GFP_KERNEL);
	if (mxt->last_message == NULL) {
		printk(KERN_WARNING "[touch] maXTouch: Can't allocate memory!\n");
		error = -ENOMEM;
		goto err_msg_alloc;
	}

	mxt->report_id_count = report_id_count;
	if (report_id_count > 254) {	/* 0 & 255 are reserved */
		dev_err(&client->dev,
			"[touch] Too many maXTouch report id's [%d]\n",
			report_id_count);
		error = -ENXIO;
		goto err_max_rid;
	}

	/* Create a mapping from report id to object type */
	report_id = 1;		/* Start from 1, 0 is reserved. */

	/* Create table associating report id's with objects & instances */
	for (i = 0; i < mxt->device_info.num_objs; i++) {
		for (object_instance = 0;
		     object_instance < object_table[i].instances;
		     object_instance++) {
			first_report_id = report_id;
			for (object_report_id = 0;
			     object_report_id < object_table[i].num_report_ids;
			     object_report_id++) {
				mxt->rid_map[report_id].object =
				    object_table[i].type;
				mxt->rid_map[report_id].instance =
				    object_instance;
				mxt->rid_map[report_id].first_rid =
				    first_report_id;
				report_id++;
			}
		}
	}

	/* Read 3 byte CRC */
	error = mxt_read_block(client, object_info_address, 3, buf);
	if (error < 0) {
		mxt->read_fail_counter++;
		dev_err(&client->dev, "[touch] Error reading CRC\n");
	}

	crc = (buf[2] << 16) | (buf[1] << 8) | buf[0];

	if (calculate_infoblock_crc(&calculated_crc, raw_ib_data, ib_pointer)) {
		printk(KERN_WARNING "[touch] Error while calculating CRC!\n");
		calculated_crc = 0;
	}
	kfree(raw_ib_data);

	mxt_debug(DEBUG_TRACE, "\n[touch] Reported info block CRC = 0x%6X\n", crc);
	mxt_debug(DEBUG_TRACE, "[touch] Calculated info block CRC = 0x%6X\n\n",
		  calculated_crc);

	if (crc == calculated_crc) {
		mxt->info_block_crc = crc;
		fw_checksum=mxt->info_block_crc;
	} else {
		mxt->info_block_crc = 0;
		printk(KERN_ALERT "[touch] maXTouch: Info block CRC invalid!\n");
	}

	if (debug >= DEBUG_VERBOSE) {

		dev_info(&client->dev, "[touch] maXTouch: %d Objects\n",
			 mxt->device_info.num_objs);

		for (i = 0; i < mxt->device_info.num_objs; i++) {
			dev_info(&client->dev, "[touch] Type:\t\t\t[%d]: %s\n",
				 object_table[i].type,
				 object_type_name[object_table[i].type]);
			dev_info(&client->dev, "\t[touch] Address:\t0x%04X\n",
				 object_table[i].chip_addr);
			dev_info(&client->dev, "\t[touch] Size:\t\t%d Bytes\n",
				 object_table[i].size);
			dev_info(&client->dev, "\t[touch] Instances:\t%d\n",
				 object_table[i].instances);
			dev_info(&client->dev, "\t[touch] Report Id's:\t%d\n",
				 object_table[i].num_report_ids);
#if 0
			TOUCH_PRINTK("Type:\t\t\t[%d]: %s\n",
				 object_table[i].type,
				 object_type_name[object_table[i].type]);
			TOUCH_PRINTK("\tAddress:\t0x%04X\n",
				 object_table[i].chip_addr);
			TOUCH_PRINTK("\tSize:\t\t%d Bytes\n",
				 object_table[i].size);
			TOUCH_PRINTK("\tInstances:\t%d\n",
				 object_table[i].instances);
			TOUCH_PRINTK("\tReport Id's:\t%d\n",
				 object_table[i].num_report_ids);
#endif
		}
	}

	return 0;

 err_max_rid:
	kfree(mxt->last_message);
 err_msg_alloc:
	kfree(mxt->rid_map);
 err_rid_map_alloc:
 err_object_read:
	kfree(raw_ib_data);
 err_ib_alloc:
	kfree(object_table);
 err_object_table_alloc:
	return error;
}

#if defined(CONFIG_PM)
static void mxt_start(struct mxt_data *mxt)
{
      char buf[2];
      int wait_count = 10;
      int error;
      force_release_pos(mxt);


	//dummy read to wake up from deep sleep mode
	mxt_read_block(mxt->client, MXT_BASE_ADDR(MXT_SPT_COMMSCONFIG_T18, mxt), 1, buf);
	mdelay(25);

	error = mxt_read_block(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt), 1, buf);

      if(error < 0){ // start hardware reset

	mxt_hw_reset();

          do{
	        mdelay(30);
	        dev_info(&mxt->client->dev, "[touch] Wait CHG count=%d\n", wait_count);
          }while(--wait_count && mxt->read_chg() != 0);
	    error = mxt_read_block(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt), 1, buf);
	}

	if(error < 0 && isInBootLoaderMode(mxt->client)){ // start boot loader recovery mode
		TOUCH_PRINTK("recovery_from_bootMode \n");
            recovery_from_bootMode(mxt->client);
	}

	schedule_delayed_work(&mxt->dwork, 0);

	if(!mxt->interruptable){
	    	ts_enable_irq();
	    	setInterruptable(mxt->client, true);
	}else{
		ts_enable_irq();
	}


	// 50ms for Idle, free run for active, 25*200ms for active to idle
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt), T7_BYTE_0_IDLEACQINT);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt)+1, T7_BYTE_1_ACTVACQINT);

}




static void mxt_stop(struct mxt_data *mxt)
{

//	REG32_VAL(0xd81100f0) &= ~0x200; //gpio 25(ts_reset) Output data
 //   	disable_irq(mxt->irq);
	ts_disable_irq();
	ts_clear_irq();
	cancel_delayed_work(&mxt->dwork);
 //     	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt), 0x0);
	// Go into Deep sleep Mode
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt), 0);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt) + 1, 0);
}


static int mxt_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct mxt_data *mxt = i2c_get_clientdata(client);
	//init irq, just for in case other driver change irq hehavior
	ts_set_gpio_int();
	if (device_may_wakeup(&client->dev)){
        	REG32_VAL(wmt_ts_gpt.isaddr) |= wmt_ts_gpt.isbmp;
		enable_irq_wake(mxt->irq);
	}else
		mxt_stop(mxt);

	return 0;
}

static int mxt_resume(struct i2c_client *client)
{
	struct mxt_data *mxt = i2c_get_clientdata(client);

	if (device_may_wakeup(&client->dev)){
		disable_irq_wake(mxt->irq);
        	REG32_VAL(wmt_ts_gpt.isaddr) &= ~wmt_ts_gpt.isbmp;
	}else
		mxt_start(mxt);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxt_early_suspend(struct early_suspend *es)
{
	struct mxt_data *mxt;
	mxt = container_of(es, struct mxt_data, early_suspend);

	if (mxt_suspend(mxt->client, PMSG_SUSPEND) != 0)
		dev_err(&mxt->client->dev, "[touch] %s: failed\n", __func__);
//	printk(KERN_WARNING "[touch] MXT Early Suspended\n");
}

static void mxt_early_resume(struct early_suspend *es)
{
	struct mxt_data *mxt;
	mxt = container_of(es, struct mxt_data, early_suspend);

	if (mxt_resume(mxt->client) != 0)
		dev_err(&mxt->client->dev, "[touch] %s: failed\n", __func__);
//	printk(KERN_WARNING "[touch] MXT Early Resumed\n");
}
#endif

#else
#define mxt_suspend NULL
#define mxt_resume NULL
#endif


static void mxt_shutdown(struct i2c_client *client)
{
	struct mxt_data *mxt;
	mxt = i2c_get_clientdata(client);

	/* Remove debug dir entries */
	debugfs_remove_recursive(mxt->debug_dir);

	if (mxt != NULL) {

		if (mxt->exit_hw != NULL)
			mxt->exit_hw();

		if (mxt->irq){
    			ts_disable_irq();
			ts_clear_irq();
			free_irq(mxt->irq, mxt);
		}
//		unregister_chrdev_region(mxt->dev_num, 2);
//		device_destroy(mxt->mxt_class, MKDEV(MAJOR(mxt->dev_num), 0));
//		device_destroy(mxt->mxt_class, MKDEV(MAJOR(mxt->dev_num), 1));
//		cdev_del(&mxt->cdev);
//		cdev_del(&mxt->cdev_messages);
		cancel_delayed_work_sync(&mxt->dwork);
		input_unregister_device(mxt->input);
//		class_destroy(mxt->mxt_class);
		wake_lock_destroy(&mxt->wakelock);

		kfree(mxt->rid_map);
		kfree(mxt->object_table);
		kfree(mxt->last_message);
	}
	sysfs_remove_group(&client->dev.kobj, &mxt->attrs);
	kfree(mxt);

	i2c_set_clientdata(client, NULL);
	if (debug >= DEBUG_TRACE)
		dev_info(&client->dev, "[touch] Touchscreen unregistered\n");
	del_timer(&ts_timer_read_dummy);

}


static inline void ts_read_dummy_func(unsigned long fcontext)
{
	if(dummy_count < READ_DUMMY_COUNT){
		printk("[touch] ts_read_dummy_func %x\n",dummy_count);
		schedule_delayed_work(&global_mxt->dwork, 0);
	    	mod_timer(&ts_timer_read_dummy, jiffies + (HZ/100)*READ_DUMMY_DELAY);//1 second
		dummy_count++;
	}else{
		printk("[touch] del timer\n");
		del_timer(&ts_timer_read_dummy);
	}
}

#if 0
static void  mxt_poll_data(struct work_struct * work)
{
	bool status;
	u8 count[7];
	status = mxt_read_block(mxt_client, 0, 7, (u8 *)count);
	if(status < 0)
		printk("[touch] Read touch sensor data fail\n");

	if(poll_mode ==0)
		msleep(5);

	queue_delayed_work(sensor_work_queue, &mxt_poll_data_work, poll_mode);
}
#endif
int mxt_stress_open(struct inode *inode, struct file *filp)
{
//	printk("[touch] %s\n", __func__);
	  if( !atomic_dec_and_test(&touch_char_available) )
	{
		atomic_inc(&touch_char_available);
		return -EBUSY; /* already open */
	}
	return 0;          /* success */
}


int mxt_stress_release(struct inode *inode, struct file *filp)
{
//	printk("[touch] %s\n", __func__);
	atomic_inc(&touch_char_available); /* release the device */
	return 0;          /* success */
}


long mxt_stress_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct mxt_data *mxt;
	u8 firmware_id[MXT_ID_BLOCK_SIZE];
	void * fw_mem=NULL;
	int err = 1;
	int ret=0;
	u8 cmd_code;
	u8 buf[0x8];

	mxt = global_mxt;
//	printk("[touch] %s\n", __func__, cmd);
	if (_IOC_TYPE(cmd) != MXT_IOC_MAGIC)
	return -ENOTTY;
	if (_IOC_NR(cmd) > MXT_IOC_MAXNR)
	return -ENOTTY;
	if (_IOC_DIR(cmd) & _IOC_READ)
	err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
	err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;
	switch (cmd) {
#if 0
		case MXT_POLL_DATA:
			if (arg == MXT_IOCTL_START_HEAVY){
//				TOUCH_PRINTK("touch sensor heavey\n");
			poll_mode = START_HEAVY;
			queue_delayed_work(sensor_work_queue, &mxt_poll_data_work, poll_mode);
			}
			else if (arg == MXT_IOCTL_START_NORMAL){
//				TOUCH_PRINTK("touch sensor normal\n");
				poll_mode = START_NORMAL;
				queue_delayed_work(sensor_work_queue, &mxt_poll_data_work, poll_mode);
			}
			else if  (arg == MXT_IOCTL_END){
//			TOUCH_PRINTK("touch sensor end\n");
			cancel_delayed_work_sync(&mxt_poll_data_work);
			}
			else
				return -ENOTTY;
		break;
#endif
		case MXT_FW_UPDATE:
			//copy_from_user
			if(FW_SIZE==0) return -EINVAL;
			fw_mem = kmalloc(FW_SIZE, GFP_KERNEL);
			if(fw_mem==NULL) return -EINVAL;
			if (copy_from_user(fw_mem, (char __user *)arg, FW_SIZE)) {
				kfree(fw_mem);
				return -EFAULT;
			}
			TOUCH_PRINTK("+MXT_FW_UPDATE\n");
			ret = mxt_init_Boot(mxt, FIRMWARE_APP_UPGRADE, fw_mem);

			msleep(1500);
			checkGR(mxt, touch_vendor_id);
			msleep(1500);
   			mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) + MXT_ADR_T6_RESET, 1);

			kfree(fw_mem);

			TOUCH_PRINTK("-MXT_FW_UPDATE\n");
		return ret;

		case MXT_CLEAR_CONFIG:
			TOUCH_PRINTK("+MXT_CLEAR_CONFIG byte %d\n", arg);
			clear_touch_config(mxt, arg);
			TOUCH_PRINTK("-MXT_CLEAR_CONFIG\n");
		break;
		case MXT_GET_CONFIG_CHECKOSUM:
			err = copy_to_user((unsigned long __user *)arg, &mxt->configuration_crc, sizeof(unsigned long));
			TOUCH_PRINTK("Checksum = %lx\n",mxt->configuration_crc);
			return err;
		break;
		case MXT_FW_DEFAULT_UPDATE:

			TOUCH_PRINTK("+MXT_FW_DEFAULT_UPDATE\n");
	    		FW_SIZE = sizeof(mXT540S_1_1_firmware);
			ret = mxt_init_Boot(mxt, FIRMWARE_BOOT_UPGRADE, mXT540S_1_1_firmware);
			msleep(1500);
			checkGR(mxt, touch_vendor_id);
			msleep(1500);
			mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt)+3, 0x0);
   			mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) + MXT_ADR_T6_RESET, 1);
			TOUCH_PRINTK("-MXT_FW_DEFAULT_UPDATE\n");
		return ret;

		case MXT_SET_FW_SIZE:
			FW_SIZE = arg;
			TOUCH_PRINTK("FW_SIZE=%lx\n", FW_SIZE);
		break;

		case MXT_FW_VERSION:
			dev_info(&mxt->client->dev, "[touch] Get Firmware firmware version!\n");
			mxt_identify(mxt->client, mxt, firmware_id);
			err = copy_to_user((char __user *)arg, firmware_id + 2, 1);
			return err;
		break;

		case MXT_GOLDEN_CALIBRATION:
			switch(arg){
			    case AP_FORCE_CALIBRATION:
			    case AP_CALIBRATION:
				if(mxt_firmware_id==MXT540S_FIRMWARE_ID_09){
					ac.calibration_status = 0;
					mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) + MXT_ADR_T6_CALIBRATE, 1);

				}else{
					if(ac.calibration_status == 0 || arg == AP_FORCE_CALIBRATION){
//						del_timer(&ts_timer_calibration);
//	    					mod_timer(&ts_timer_calibration, jiffies + (HZ/100)*4000);

						calibration_flag = GOLDEN_REFERENCE_PRIME;
						mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 0, 0x17);
						selftest_flag=0;
						TOUCH_PRINTK("maXTouch xxxxxx: GOLDEN_CALIBRATION START --> PRIME!\n");
					}else
						TOUCH_PRINTK("do nothing!\n");
				}
			    break;
			    case AP_SELFTEST_CALIBRATION:
				ac.selftest_calibration_status = 0;
				calibration_flag = SELFTEST_CALIBRATION_START;
				mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) + MXT_ADR_T6_CALIBRATE, 1);
			    break;


			}
		break;
		case MXT_CALIBRATION_STATUS:
				if(mxt_firmware_id==MXT540S_FIRMWARE_ID_09)
					err = copy_to_user((char __user *)arg, &ac.calibration_status, 1);
				else{
					if(calibration_flag == GOLDEN_REFERENCE_FINISH || calibration_flag == GOLDEN_REFERENCE_INACTIVE)
						err = copy_to_user((char __user *)arg, &ac.calibration_status, 1);
					else if(calibration_flag == SELFTEST_CALIBRATION_START)
						err = copy_to_user((char __user *)arg, &ac.selftest_calibration_status, 1);
					else{
						err = copy_to_user((char __user *)arg, &calibration_flag, 1);
					}
				}
				return err;
		break;
		case MXT_HW_RESET:
				TOUCH_PRINTK("MXT HW RESET!\n");
				calibration_flag = GOLDEN_REFERENCE_INACTIVE;
				mxt_hw_reset();
      				force_release_pos(global_mxt);
				cancel_delayed_work(&global_mxt->dwork);
				ts_disable_irq();
				ts_clear_irq();
				schedule_delayed_work(&global_mxt->dwork, 0);
		break;
		case MXT_SELFTEST:

			// start the test
		       	TOUCH_PRINTK("do refs check!\n");
			if(!check_refs_before_selftest(mxt)){
				TOUCH_PRINTK("PASS!\n");
			}else{
				TOUCH_PRINTK("FAIL!\n");
				break;
			}
			cmd_code = arg & 0xFFUL;
			selftest_flag = false;
			mxt_write_byte(global_mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, global_mxt), 0x0);
//			mxt_write_byte(global_mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, global_mxt)+1, 0x0);
//			mxt_write_byte(global_mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, global_mxt)+2, 0x0);
//			mxt_write_byte(global_mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, global_mxt)+3, 0x0);
//			mxt_write_byte(global_mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, global_mxt)+4, 0x0);
//			mxt_write_byte(global_mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, global_mxt)+5, 0x0);
//			mxt_write_byte(global_mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, global_mxt)+6, 0x0);
//			mxt_write_byte(global_mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, global_mxt)+7, 0x0);
			// don't need to backup
	                msleep(50);
			mxt_write_byte(global_mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, global_mxt) + 0, 0x03);
			mxt_write_byte(global_mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, global_mxt) + 1, cmd_code);
		       	TOUCH_PRINTK("Start the self test with byte: %X\n", cmd_code);
		break;

		case MXT_SELFTEST_STATUS:
			TOUCH_PRINTK("selftest_flag=%x\n",selftest_flag);
			err = copy_to_user((char __user *)arg, &selftest_flag, 1);
		break;

		case MXT_SW_RESET:
			calibration_flag = GOLDEN_REFERENCE_INACTIVE;
			mxt_write_byte(mxt->client,
					MXT_BASE_ADDR
					(MXT_GEN_COMMANDPROCESSOR_T6,
					 mxt) + MXT_ADR_T6_RESET, 1);
	                msleep(500);
      			force_release_pos(global_mxt);
			cancel_delayed_work(&global_mxt->dwork);
			ts_disable_irq();
			ts_clear_irq();
			schedule_delayed_work(&global_mxt->dwork, 0);

		break;

		case MXT_CALIBRATE:
			mxt_write_byte(mxt->client,
					MXT_BASE_ADDR
					(MXT_GEN_COMMANDPROCESSOR_T6,
					 mxt) + MXT_ADR_T6_CALIBRATE, 1);

		break;

		case MXT_BACKUP:
			mxt_write_byte(mxt->client,
					MXT_BASE_ADDR
					(MXT_GEN_COMMANDPROCESSOR_T6,
					 mxt) + MXT_ADR_T6_BACKUPNV,
					MXT_CMD_T6_BACKUP);
		break;

		case MXT_LOAD_NEWER_CONFIG:

			//for maintain purpose
			TOUCH_PRINTK("load newer configuration!\n");
			mxt_write_byte(global_mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, global_mxt)+6, USER_DATA_MAGIC_0_FOTA_V1);
			mxt_write_byte(global_mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, global_mxt)+7, USER_DATA_MAGIC_1_FOTA_V1);
			if (init_touch_config(global_mxt, touch_vendor_id)){
				printk(KERN_WARNING "[touch] touch configuration error!\n");
				return -EINVAL;
			}
			ac.calibration_status = 0;
    			mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt), 0x0);
			mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x02);
    			mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt)+ MXT_ADR_T6_BACKUPNV,
					MXT_CMD_T6_BACKUP);
	                msleep(500);
			mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) + MXT_ADR_T6_RESET, 1);

		break;

#if 0
	case MXT_NONTOUCH_MSG:
		mxt->nontouch_msg_only = 1;
		break;
	case MXT_ALL_MSG:
		mxt->nontouch_msg_only = 0;
		break;
#endif
		default: /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}

	return 0;
}

	struct file_operations mxt_fops = {
		.owner =    THIS_MODULE,
		.unlocked_ioctl =	mxt_stress_ioctl,
		.open =		mxt_stress_open,
		.release =	mxt_stress_release,
		};

static ssize_t mxt_touch_switch_name(struct switch_dev *sdev, char *buf)
{
      struct mxt_data *mxt = global_mxt;
	return sprintf(buf, "MXT-%d.%d build-%u\n",
		        mxt->device_info.major, mxt->device_info.minor, mxt->device_info.build);
}

static ssize_t mxt_touch_switch_state(struct switch_dev *sdev, char *buf)
{ 
      	return sprintf(buf, "%s\n", "0");
}


static int __devinit mxt_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
	struct mxt_data *mxt;
	struct mxt_platform_data *pdata;
	struct input_dev *input;
	u8 *id_data;
	int error;
	int err;


	cfg_flag = true;
	d_flag=0;
	reset_flag = true;
	calibration_flag = GOLDEN_REFERENCE_INACTIVE;
	boot_cali_status = 0;
	selftest_flag = false;
	mxt_firmware_id = 0;
	dummy_count = 0;

	mxt_debug(DEBUG_INFO, "[touch] mXT540S: mxt_probe\n");

	if (client == NULL) {
		pr_debug("[touch] maXTouch: client == NULL\n");
		return -EINVAL;
	} else if (client->adapter == NULL) {
		pr_debug("[touch] maXTouch: client->adapter == NULL\n");
		return -EINVAL;
	} else if (&client->dev == NULL) {
		pr_debug("[touch] maXTouch: client->dev == NULL\n");
		return -EINVAL;
	} else if (&client->adapter->dev == NULL) {
		pr_debug("[touch] maXTouch: client->adapter->dev == NULL\n");
		return -EINVAL;
	} else if (id == NULL) {
		pr_debug("[touch] maXTouch: id == NULL\n");
		return -EINVAL;
	}

	mxt_debug(DEBUG_INFO, "[touch] maXTouch driver\n");
	mxt_debug(DEBUG_INFO, "\t[touch]  \"%s\"\n", client->name);
	mxt_debug(DEBUG_INFO, "\t[touch] addr:\t0x%04x\n", client->addr);
	mxt_debug(DEBUG_INFO, "\t[touch] irq:\t%d\n", client->irq);
	mxt_debug(DEBUG_INFO, "\t[touch] flags:\t0x%04x\n", client->flags);
	mxt_debug(DEBUG_INFO, "\t[touch] adapter:\"%s\"\n", client->adapter->name);
	mxt_debug(DEBUG_INFO, "\t[touch] device:\t\"%s\"\n", client->dev.init_name);

	/* Check if the I2C bus supports BYTE transfer */
	error = i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE);
	dev_info(&client->dev, "[touch] RRC:  i2c_check_functionality = %i\n", error);
	error = 0xff;
/*
	if (!error) {
		dev_err(&client->dev, "maXTouch driver\n");
		dev_err(&client->dev, "\t \"%s\"\n", client->name);
		dev_err(&client->dev, "\taddr:\t0x%04x\n", client->addr);
		dev_err(&client->dev, "\tirq:\t%d\n", client->irq);
		dev_err(&client->dev, "\tflags:\t0x%04x\n", client->flags);
		dev_err(&client->dev, "\tadapter:\"%s\"\n",
			client->adapter->name);
		dev_err(&client->dev, "\tdevice:\t\"%s\"\n",
			client->dev.init_name);
		dev_err(&client->dev, "%s adapter not supported\n",
				dev_driver_string(&client->adapter->dev));
		return -ENODEV;
	}
*/
	mxt_debug(DEBUG_TRACE, "[touch] maXTouch driver functionality OK\n");

	/* Allocate structure - we need it to identify device */
	mxt = kzalloc(sizeof(struct mxt_data), GFP_KERNEL);
	if (mxt == NULL) {
		dev_err(&client->dev, "[touch] insufficient memory\n");
		error = -ENOMEM;
		goto err_mxt_alloc;
	}
	id_data = kmalloc(MXT_ID_BLOCK_SIZE, GFP_KERNEL);
	if (id_data == NULL) {
		dev_err(&client->dev, "[touch] insufficient memory\n");
		error = -ENOMEM;
		goto err_id_alloc;
	}

	input = input_allocate_device();
	if (!input) {
		dev_err(&client->dev, "[touch] error allocating input device\n");
		error = -ENOMEM;
		goto err_input_dev_alloc;
	}

	/* Initialize Platform data */

	pdata = client->dev.platform_data;
	if (pdata == NULL) {
		dev_err(&client->dev, "[touch] platform data is required!\n");
		error = -EINVAL;
		goto err_pdata;
	}
	if (debug >= DEBUG_TRACE)
		printk(KERN_INFO "[touch] Platform OK: pdata = 0x%08x\n",
		       (unsigned int)pdata);

	mxt->read_fail_counter = 0;
	mxt->message_counter = 0;
	mxt->max_x_val = pdata->max_x;
	mxt->max_y_val = pdata->max_y;

	/* Get data that is defined in board specific code. */
	mxt->init_hw = pdata->init_platform_hw;
	mxt->exit_hw = pdata->exit_platform_hw;
	mxt->read_chg = pdata->read_chg;

	if (pdata->valid_interrupt != NULL)
		mxt->valid_interrupt = pdata->valid_interrupt;
	else
		mxt->valid_interrupt = mxt_valid_interrupt_dummy;

	if (mxt->init_hw != NULL)
		mxt->init_hw();

	if (debug >= DEBUG_TRACE)
		printk(KERN_INFO "[touch] maXTouch driver identifying chip\n");

	mxt->client = client;

	if(isInBootLoaderMode(client)){
	    mxt_update_firmware(mxt, FIRMWARE_BOOT_RECOVERY, mXT540S_1_1_firmware, sizeof(mXT540S_1_1_firmware));
    	    //firmware change.
    	    mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt)+2, 0x1);
    	    mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt)+ MXT_ADR_T6_BACKUPNV,
					MXT_CMD_T6_BACKUP);
	    msleep(200);
	}

	if (mxt_identify(client, mxt, id_data) < 0) {
		dev_err(&client->dev, "[touch] Chip could not be identified\n");
		error = -ENODEV;
		goto err_identify;
	}

	/* Chip is valid and active. */
	if (debug >= DEBUG_TRACE)
		printk(KERN_INFO "[touch] maXTouch driver allocating input device\n");

	mxt->client = client;
	mxt->input = input;
	mxt->hasTouch = false;

	INIT_DELAYED_WORK(&mxt->dwork, mxt_worker);
	mutex_init(&mxt->debug_mutex);
	mutex_init(&mxt->msg_mutex);
	mxt_debug(DEBUG_TRACE, "[touch] maXTouch driver creating device name\n");

	snprintf(mxt->phys_name,
		 sizeof(mxt->phys_name), "%s/input0", dev_name(&client->dev)
	    );
	input->name = "atmel-maxtouch";
	input->phys = mxt->phys_name;
	input->id.bustype = BUS_I2C;
	input->dev.parent = &client->dev;

	mxt_debug(DEBUG_INFO, "[touch] maXTouch name: \"%s\"\n", input->name);
	mxt_debug(DEBUG_INFO, "[touch] maXTouch phys: \"%s\"\n", input->phys);
	mxt_debug(DEBUG_INFO, "[touch] maXTouch driver setting abs parameters\n");

	set_bit(BTN_TOUCH, input->keybit);

	/* Single touch */
	input_set_abs_params(input, ABS_X, 0, mxt->max_y_val, 0, 0);
	input_set_abs_params(input, ABS_Y, 0, mxt->max_x_val, 0, 0);
	input_set_abs_params(input, ABS_PRESSURE, 0, MXT_MAX_REPORTED_PRESSURE,
			     0, 0);
	input_set_abs_params(input, ABS_TOOL_WIDTH, 0, MXT_MAX_REPORTED_WIDTH,
			     0, 0);

	/* Multitouch */
	input_set_abs_params(input, ABS_MT_POSITION_X, 0, mxt->max_y_val, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, mxt->max_x_val, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, MXT_MAX_TOUCH_SIZE,
			     0, 0);
	input_set_abs_params(input, ABS_MT_PRESSURE, 0, MXT_MAX_REPORTED_PRESSURE,
		           0, 0);
	input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, MXT_MAX_NUM_TOUCHES,
			     0, 0);

	__set_bit(EV_ABS, input->evbit);
	__set_bit(EV_SYN, input->evbit);
	__set_bit(EV_KEY, input->evbit);
/* jakie 	__set_bit(EV_TOUCH, input->evbit); */

	mxt_debug(DEBUG_TRACE, "[touch] maXTouch driver setting client data\n");
	sema_init(&mxt->sem, 1);
	mxt->interruptable = true;
	i2c_set_clientdata(client, mxt);
	mxt->status = 0;
	mxt_debug(DEBUG_TRACE, "[touch] maXTouch driver setting drv data\n");
	input_set_drvdata(input, mxt);
	mxt_debug(DEBUG_TRACE, "[touch] maXTouch driver input register device\n");
	error = input_register_device(mxt->input);
	if (error < 0) {
		dev_err(&client->dev, "[touch] Failed to register input device\n");
		goto err_register_device;
	}

	error = mxt_read_object_table(client, mxt, id_data);
	if (error < 0)
		goto err_read_ot;

        touch_vendor_id = readTP_ID();

	dev_info(&client->dev, "[touch] The touch vendor is 0x%02X\n", touch_vendor_id);

	if(touch_vendor_id==OFILM_GFF) 		dev_info(&client->dev, "[touch] OFILM_GFF configuration!\n");
	else if(touch_vendor_id==UNUSE1) 	dev_info(&client->dev, "[touch] UNUSE1 configuration-->use JTouch configuration!\n");
	else if(touch_vendor_id==JTOUCH) 	dev_info(&client->dev, "[touch] JTouch configuration!\n");
	else					dev_info(&client->dev, "[touch] O-film configuration!\n");

	if(isNeedLoadDefaultConfig(mxt)){
		error = init_touch_config(mxt, touch_vendor_id); // start config
		if (error){
			printk(KERN_WARNING "[touch] touch configuration error!\n");
			goto err_config;
		}
	}

	checkGR(mxt, touch_vendor_id);

	/* Create debugfs entries. */
	mxt->debug_dir = debugfs_create_dir("maXTouch", NULL);
	if (mxt->debug_dir == ERR_PTR(-ENODEV)) {
		/* debugfs is not enabled. */
		printk(KERN_WARNING "[touch] debugfs not enabled in kernel\n");
	} else if (mxt->debug_dir == NULL) {
		printk(KERN_WARNING "[touch] error creating debugfs dir\n");
	} else {
		mxt_debug(DEBUG_TRACE, "[touch] created \"maXTouch\" debugfs dir\n");

		debugfs_create_file("deltas", S_IRUSR, mxt->debug_dir, mxt,
				    &delta_fops);
		debugfs_create_file("refs", S_IRUSR, mxt->debug_dir, mxt,
				    &refs_fops);
	}

	mxt->msg_buffer_startp = 0;
	mxt->msg_buffer_endp = 0;


	/* Allocate the interrupt */
	mxt_debug(DEBUG_TRACE, "[touch] maXTouch driver allocating interrupt...\n");
	client->irq = wmt_ts_gpt.irq;
	mxt->irq = client->irq;
	printk(KERN_INFO "[touch] mxt->irq = %x\n",mxt->irq);
	mxt->valid_irq_counter = 0;
	mxt->invalid_irq_counter = 0;
	mxt->irq_counter = 0;
	if (mxt->irq) {
		/* Try to request IRQ with falling edge first. This is
		 * not always supported. If it fails, try with any edge. */
		ts_clear_irq();
		error = request_irq(mxt->irq,
				    mxt_irq_handler,
				    IRQF_SHARED,
				    client->dev.driver->name, mxt);
		// client->irq = 5;

//	printk(KERN_INFO "wmt_ts_gpt.irq = %x\n",wmt_ts_gpt.irq);
//	error = request_irq(wmt_ts_gpt.irq , mxt_irq_handler, IRQF_TRIGGER_FALLING, client->name, mxt);
//	printk(KERN_INFO "error = %x\n",error);
//	printk(KERN_INFO "client->name = %s\n",client->name);
//	printk(KERN_INFO "client->dev.driver->name = %s\n",client->dev.driver->name);

		if (error < 0) {
			/* TODO: why only 0 works on STK1000? */
			error = request_irq(mxt->irq,
					    mxt_irq_handler,
					    0, client->dev.driver->name, mxt);
		}

		if (error < 0) {
			dev_err(&client->dev,
				"[touch] failed to allocate irq %d\n", mxt->irq);
			goto err_irq;
		}
	}


	wake_lock_init(&mxt->wakelock, WAKE_LOCK_SUSPEND, "mxt_touch");
#ifdef CONFIG_HAS_EARLYSUSPEND
	mxt->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 21;
	mxt->early_suspend.suspend = mxt_early_suspend;
	mxt->early_suspend.resume = mxt_early_resume;
	register_early_suspend(&mxt->early_suspend);
#endif

#if 0
	sensor_work_queue = create_singlethread_workqueue("i2c_touchsensor_wq");
	if(!sensor_work_queue){
		pr_err("[touch] touch_probe: Unable to create workqueue");
		goto err_irq;
		}

//	INIT_DELAYED_WORK(&mxt_poll_data_work, mxt_poll_data);
#endif
	mxt->misc_dev.minor  = MISC_DYNAMIC_MINOR;
	mxt->misc_dev.name = "touchpanel";
	mxt->misc_dev.fops = &mxt_fops;
	err = misc_register(&mxt->misc_dev);
		if (err) {
			pr_err("[touch] tegra_acc_probe: Unable to register %s \\misc device\n", mxt->misc_dev.name);
		goto misc_register_device_failed;
			}

      mxt->touch_sdev.name = "touch";
      mxt->touch_sdev.print_name = mxt_touch_switch_name;
	mxt->touch_sdev.print_state = mxt_touch_switch_state;
	if(switch_dev_register(&mxt->touch_sdev) < 0){
		dev_info(&client->dev, "[touch] switch_dev_register for dock failed!\n");
		//goto exit;
	}
	switch_set_state(&mxt->touch_sdev, 0);

	if (debug > DEBUG_INFO)
		dev_info(&client->dev, "[touch] touchscreen, irq %d\n", mxt->irq);

	/* Schedule a worker routine to read any messages that might have
	 * been sent before interrupts were enabled. */
	cancel_delayed_work(&mxt->dwork);
	ts_disable_irq();
	ts_clear_irq();
	schedule_delayed_work(&mxt->dwork, 0);


	mxt->attrs.attrs = mxt_attr;
	error = sysfs_create_group(&client->dev.kobj, &mxt->attrs);
	if (error) {
		dev_err(&client->dev, "[touch] Not able to create the sysfs\n");
		goto exit_remove;
	}
	mxt->status=1;
	global_mxt = mxt;

#if defined(CONFIG_ASUS_ENGINEER_MODE) && CONFIG_ASUS_ENGINEER_MODE
	    if(isNeedUpgradeFirmware(id_data)){
	    	cfg_flag = false;
		//wait calibration completely
	    	msleep(1000);
	    	FW_SIZE = sizeof(mXT540S_1_1_firmware);
	    	mxt_init_Boot(mxt, FIRMWARE_BOOT_UPGRADE, mXT540S_1_1_firmware);
	    	msleep(1500);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt)+3, 0x0);
	    	checkGR(mxt, touch_vendor_id);
	    	msleep(1500);
   	    	mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) + MXT_ADR_T6_RESET, 1);
	    }
#endif

	backup_T7_config(mxt);

	if(reset_flag)
	    mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) + MXT_ADR_T6_CALIBRATE, 1);

	reset_flag =false;
	kfree(id_data);

#if 0
	//init timer
	init_timer(&ts_timer_read_dummy);
	ts_timer_read_dummy.function = ts_read_dummy_func;
	mod_timer(&ts_timer_read_dummy, jiffies + (HZ/100)*READ_DUMMY_DELAY);//1 second
#endif


	return 0;

exit_remove:
		sysfs_remove_group(&client->dev.kobj, &mxt->attrs);
err_irq:
	kfree(mxt->rid_map);
	kfree(mxt->object_table);
	kfree(mxt->last_message);
misc_register_device_failed:
	misc_deregister(&mxt->misc_dev);
err_read_ot:
err_config:
err_register_device:
err_identify:
err_pdata:
	input_free_device(input);
err_input_dev_alloc:
	kfree(id_data);
err_id_alloc:
	if (mxt->exit_hw != NULL)
		mxt->exit_hw();
	kfree(mxt);
err_mxt_alloc:
	return error;
}

static int __devexit mxt_remove(struct i2c_client *client)
{
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);

	/* Remove debug dir entries */
	debugfs_remove_recursive(mxt->debug_dir);

	if (mxt != NULL) {

		if (mxt->exit_hw != NULL)
			mxt->exit_hw();

		if (mxt->irq){
    			ts_disable_irq();
			ts_clear_irq();
			free_irq(mxt->irq, mxt);
		}

		cancel_delayed_work_sync(&mxt->dwork);
		input_unregister_device(mxt->input);
		wake_lock_destroy(&mxt->wakelock);

		kfree(mxt->rid_map);
		kfree(mxt->object_table);
		kfree(mxt->last_message);
	}
	sysfs_remove_group(&client->dev.kobj, &mxt->attrs);
	kfree(mxt);

	i2c_set_clientdata(client, NULL);
	if (debug >= DEBUG_TRACE)
		dev_info(&client->dev, "[touch] Touchscreen unregistered\n");
	del_timer(&ts_timer_read_dummy);
	return 0;
}

static const struct i2c_device_id mxt_idtable[] = {
	{"maXTouch", 0,},
	{}
};

MODULE_DEVICE_TABLE(i2c, mxt_idtable);

static struct i2c_driver mxt_driver = {
	.driver = {
		   .name = "maXTouch",
		   .owner = THIS_MODULE,
		   },

	.id_table = mxt_idtable,
	.probe = mxt_probe,
	.remove = __devexit_p(mxt_remove),
	.shutdown = mxt_shutdown,
#if !defined(CONFIG_HAS_EARLYSUSPEND)
	.suspend = mxt_suspend,
	.resume = mxt_resume,
#endif
};

static struct mxt_ts_platform_data ts_mxt_data = {
    .panel_type		= 0,
    .sclk_div 		= 0x04,
    .i2c_bus_id 	= 1,
//    .soc_gpio_irq;
    .gpio_num		= 2,
//    enum gpio_irq_trigger_type irq_type;
    .abs_x_min	= 0,
    .abs_x_max	= MAX_X_SCAN-1,
    .abs_y_min	= 0,
    .abs_y_max	= MAX_Y_SCAN-1,
};

static struct mxt_platform_data mxt_ts_pdata = {
    .numtouch	  = MXT_MAX_NUM_TOUCHES,
    .max_x	  = MAX_X_SCAN-1,
    .max_y	  = MAX_Y_SCAN-1,
    .init_platform_hw 	= NULL,
    .exit_platform_hw 	= NULL,
    .valid_interrupt	= asus_valid_interrupt,
    .read_chg 		= asus_read_chg,
};


static struct i2c_board_info __initdata  mxt_ts_i2c_board_info= {
	.type          = "maXTouch",
	.flags         = 0x00,
	.addr          = MXT540S_I2C_ADDR1,// 4b is 540S
	.platform_data = &mxt_ts_pdata,
	.archdata      = NULL,
	.irq           = 5,
};


static int __init mxt_init(void)
{
	int err;
	int ret = 0;
	struct i2c_adapter *adapter = NULL;
	struct i2c_client *client   = NULL;
	struct i2c_board_info *mxt_i2c_bi;

	printk(KERN_INFO "[touch] %s+ #####\n", __func__);


	mxt_i2c_bi = &mxt_ts_i2c_board_info;

	if(parse_gpt_arg()==0){
             ret = -ENODEV;
             goto release_dev;
	}
	if(parse_gptc_arg()==0){
             ret = -ENODEV;
             goto release_dev;
	}

	ts_set_gpio_int();
	adapter = i2c_get_adapter(ts_mxt_data.i2c_bus_id);
	if (NULL == adapter) {
       		 printk("[touch] can not get i2c adapter, client address error\n");
        		ret = -ENODEV;
	        	goto release_dev;
	    }
	client = i2c_new_device(adapter, mxt_i2c_bi);
	if (client == NULL) {
		        printk("[touch] allocate i2c client failed\n");
		        ret = -ENOMEM;
	        goto release_dev;
	  }

	err = i2c_add_driver(&mxt_driver);
	if (err) {
		printk(KERN_WARNING "[touch] Adding maXTouch driver failed "
		       "(errno = %d)\n", err);
	} else {
		mxt_debug(DEBUG_TRACE, "[touch] Successfully added driver %s\n",
			  mxt_driver.driver.name);
	}
	printk(KERN_INFO "[touch] %s- #####\n", __func__);
	return err;
release_dev:
	printk("[touch] mxt fail \n");
	return ret;
}

static void __exit mxt_cleanup(void)
{
	i2c_del_driver(&mxt_driver);
}

module_init(mxt_init);
module_exit(mxt_cleanup);

MODULE_AUTHOR("Iiro Valkonen");
MODULE_DESCRIPTION("Driver for Atmel maXTouch Touchscreen Controller");
MODULE_LICENSE("GPL");
