/*
 *  Atmel maXTouch header file
 *
 *  Copyright (c) 2010 Atmel Corporation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 or 3 as
 *  published by the Free Software Foundation.
 *  See the file "COPYING" in the main directory of this archive
 *  for more details.
 *
 */

#include <linux/cdev.h>
#include <linux/earlysuspend.h>
#include <linux/input.h>
#include <linux/miscdevice.h>
#include <linux/switch.h>
#include <linux/wakelock.h>

#define	MXT224_I2C_ADDR1				0x4A
#define	MXT224_I2C_ADDR2				0x4B
#define	MXT1386_I2C_ADDR1				0x4C
#define	MXT1386_I2C_ADDR2				0x4D
#define	MXT1386_I2C_ADDR3				0x5A
#define	MXT1386_I2C_ADDR4				0x5B
// additional for MXT768E
#define	MXT768E_I2C_ADDR1				0x4C
#define	MXT768E_I2C_ADDR2				0x4D
//additional for MXT540S
#define	MXT540S_I2C_ADDR1				0x4B
#define	MXT540S_I2C_ADDR2				0x4D

/*
 * Select this address from above depending on what maXTouch
 * chip you have and how it's address pins are configured;
 * see datasheet.
 */

#define MXT_I2C_ADDRESS                                 MXT540S_I2C_ADDR1

#define MXT_BL_ADDRESS                                  0x25

#define	MXT224_FAMILYID				        0x80
#define MXT1386_FAMILYID                                0xA0
#define MXT540S_FAMILYID                                0x82

#define	MXT224_CAL_VARIANTID				0x01
#define MXT224_UNCAL_VARIANTID                          0x00
#define MXT1386_CAL_VARIANTID                           0x00
#define MXT540S_CAL_VARIANTID                           0x1C

#define MXT_MAX_REPORTED_WIDTH                          255
#define MXT_MAX_REPORTED_PRESSURE                       255
#define MXT_MAX_TOUCH_SIZE                              255
#define MXT_MAX_NUM_TOUCHES                             10

/* Fixed addresses inside maXTouch device */
#define	MXT_ADDR_INFO_BLOCK				0
#define	MXT_ADDR_OBJECT_TABLE				7
#define MXT_ID_BLOCK_SIZE                               7
#define	MXT_OBJECT_TABLE_ELEMENT_SIZE			6

/* Object types */
#define	MXT_DEBUG_DELTAS_T2				2
#define	MXT_DEBUG_REFERENCES_T3				3
#define	MXT_GEN_MESSAGEPROCESSOR_T5			5
#define	MXT_GEN_COMMANDPROCESSOR_T6			6
#define	MXT_GEN_POWERCONFIG_T7				7
#define	MXT_GEN_ACQUIRECONFIG_T8			8
#define	MXT_TOUCH_MULTITOUCHSCREEN_T9			9
#define MXT_TOUCH_SINGLETOUCHSCREEN_T10                 10
#define MXT_TOUCH_XSLIDER_T11                           11
#define MXT_TOUCH_YSLIDER_T12                           12
#define MXT_TOUCH_XWHEEL_T13                            13
#define MXT_TOUCH_YWHEEL_T14                            14
#define	MXT_TOUCH_KEYARRAY_T15				15
#define	MXT_SPT_COMMSCONFIG_T18				18
#define	MXT_SPT_GPIOPWM_T19				19
#define	MXT_PROCI_GRIPFACESUPPRESSION_T20		20
#define	MXT_PROCG_NOISESUPPRESSION_T22			22
#define	MXT_TOUCH_PROXIMITY_T23				23
#define	MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24		24
#define	MXT_SPT_SELFTEST_T25				25
#define MXT_DEBUG_CTERANGE_T26				26
#define	MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27		27
#define	MXT_SPT_CTECONFIG_T28				28
#define	MXT_TOUCH_KEYSET_T31				31
#define	MXT_TOUCH_XSLIDERSET_T32			32
#define	MXT_DEBUG_DIAGNOSTIC_T37			37
#define	MXT_SPT_USER_DATA_T38				38
#define	MXT_PROCI_GRIPSUPPRESSION_T40		40
#define	MXT_PALMSUPPRESSION_T41		41
#define	MXT_SPT_DIGITIZER_T43			43
#define	MXT_SPT_MESSAGECOUNT_T44		44
// MXT 768E additional data source
#define   MXT_PROCI_TOUCHSUPPRESSION_T42 42
#define   MXT_SPT_CTECONFIG_T46 46
#define   MXT_PROCI_STYLUS_T47  47
#define   MXT_PROCG_NOISESUPPRESSION_T48 48
#define   MXT_TOUCH_PROXKEY_T52 52
#define   MXT_GEN_DATASOURCE_T53 53
#define   MXT_PROCI_ADAPTIVETHRESHOLD_T55 55
#define   MXT_PROCI_SHIELDLESS_T56 56
#define   MXT_PROCI_EXTRATOUCHAREADATA_T57 57
#define   MXT_SPT_TIMER_T61 61
#define   MXT_PROCG_NOISESUPPRESSION_T62 62
#define   MXT_LENSBENDING_T65 65
#define   MXT_GOLDENREFERENCES_T66 66
#define   MXT_PALMGESTUREPROCESSOR_T69 69



/*
 * If a message is read from mXT when there's no new messages available,
 * the report ID of the message will be 0xFF.
 */
#define	MXT_END_OF_MESSAGES				0xFF

/* GEN_COMMANDPROCESSOR_T6 Register offsets from T6 base address */
#define	MXT_ADR_T6_RESET				0x00
#define	MXT_ADR_T6_BACKUPNV				0x01
#define	MXT_ADR_T6_CALIBRATE				0x02
#define	MXT_ADR_T6_REPORTALL				0x03
#define	MXT_ADR_T6_RESERVED				0x04
#define	MXT_ADR_T6_DIAGNOSTIC				0x05

/* T6 Debug Diagnostics Commands */
#define	MXT_CMD_T6_PAGE_UP          			0x01
#define	MXT_CMD_T6_PAGE_DOWN        			0x02
#define	MXT_CMD_T6_DELTAS_MODE      			0x10
#define	MXT_CMD_T6_REFERENCES_MODE  			0x11
#define	MXT_CMD_T6_DEVICE_IDENTIFICATION_MODE         	0x80
#define	MXT_CMD_T6_TOUCH_THRESHOLD_MODE         	0xF4

/* T6 Backup Command */
#define MXT_CMD_T6_BACKUP           0x55

/* SPT_DEBUG_DIAGNOSTIC_T37 Register offsets from T37 base address */
#define MXT_ADR_T37_PAGE                                0x01
#define	MXT_ADR_T37_DATA				0x02

/************************************************************************
 * MESSAGE OBJECTS ADDRESS FIELDS
 *
 ************************************************************************/
#define MXT_MSG_REPORTID                                0x00

/* MXT_GEN_MESSAGEPROCESSOR_T5 Message address definitions		*/
#define	MXT_MSG_T5_REPORTID				0x00
#define	MXT_MSG_T5_MESSAGE				0x01
#define	MXT_MSG_T5_CHECKSUM				0x08

/* MXT_GEN_COMMANDPROCESSOR_T6 Message address definitions		*/
#define	MXT_MSG_T6_STATUS				0x01
#define		MXT_MSGB_T6_COMSERR		0x04
#define		MXT_MSGB_T6_CFGERR		0x08
#define		MXT_MSGB_T6_CAL			0x10
#define		MXT_MSGB_T6_SIGERR		0x20
#define		MXT_MSGB_T6_OFL			0x40
#define		MXT_MSGB_T6_RESET		0x80
/* Three bytes */
#define	MXT_MSG_T6_CHECKSUM				0x02

/* MXT_GEN_POWERCONFIG_T7 NO Message address definitions		*/
/* MXT_GEN_ACQUIRECONFIG_T8 Message address definitions			*/
/* MXT_TOUCH_MULTITOUCHSCREEN_T9 Message address definitions		*/

#define	MXT_MSG_T9_STATUS				0x01
/* Status bit field */
#define		MXT_MSGB_T9_SUPPRESS		0x02
#define		MXT_MSGB_T9_AMP			0x04
#define		MXT_MSGB_T9_VECTOR		0x08
#define		MXT_MSGB_T9_MOVE		0x10
#define		MXT_MSGB_T9_RELEASE		0x20
#define		MXT_MSGB_T9_PRESS		0x40
#define		MXT_MSGB_T9_DETECT		0x80

#define	MXT_MSG_T9_XPOSMSB				0x02
#define	MXT_MSG_T9_YPOSMSB				0x03
#define	MXT_MSG_T9_XYPOSLSB				0x04
#define	MXT_MSG_T9_TCHAREA				0x05
#define	MXT_MSG_T9_TCHAMPLITUDE				0x06
#define	MXT_MSG_T9_TCHVECTOR				0x07

/* MXT_SPT_GPIOPWM_T19 Message address definitions			*/
#define	MXT_MSG_T19_STATUS				0x01

/* MXT_PROCI_GRIPFACESUPPRESSION_T20 Message address definitions	*/
#define	MXT_MSG_T20_STATUS				0x01
#define		MXT_MSGB_T20_FACE_SUPPRESS	0x01
/* MXT_PROCG_NOISESUPPRESSION_T22 Message address definitions		*/
#define	MXT_MSG_T22_STATUS				0x01
#define		MXT_MSGB_T22_FHCHG		0x01
#define		MXT_MSGB_T22_GCAFERR		0x04
#define		MXT_MSGB_T22_FHERR		0x08
#define	MXT_MSG_T22_GCAFDEPTH				0x02

/* MXT_TOUCH_PROXIMITY_T23 Message address definitions			*/
#define	MXT_MSG_T23_STATUS				0x01
#define		MXT_MSGB_T23_FALL		0x20
#define		MXT_MSGB_T23_RISE		0x40
#define		MXT_MSGB_T23_DETECT		0x80
/* 16 bit */
#define	MXT_MSG_T23_PROXDELTA				0x02

/* MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24 Message address definitions	*/
#define	MXT_MSG_T24_STATUS				0x01
#define	MXT_MSG_T24_XPOSMSB				0x02
#define	MXT_MSG_T24_YPOSMSB				0x03
#define	MXT_MSG_T24_XYPOSLSB				0x04
#define	MXT_MSG_T24_DIR					0x05
/* 16 bit */
#define	MXT_MSG_T24_DIST				0x06

/* MXT_SPT_SELFTEST_T25 Message address definitions			*/
#define	MXT_MSG_T25_STATUS				0x01
/* 5 Bytes */
#define		MXT_MSGR_T25_OK				0xFE
#define		MXT_MSGR_T25_INVALID_TEST		0xFD
#define		MXT_MSGR_T25_UNRELATED_FAULT		0xFC
#define		MXT_MSGR_T25_AVDD_IS_NOT_PRESENT	0x01
#define		MXT_MSGR_T25_PIN_FAULT			0x12
#define		MXT_MSGR_T25_SIGNAL_LIMIT_FAULT		0x17

#define	MXT_MSG_T25_INFO				0x02

/* MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27 Message address definitions	*/
#define	MXT_MSG_T27_STATUS			0x01
#define		MXT_MSGB_T27_ROTATEDIR		0x10
#define		MXT_MSGB_T27_PINCH		0x20
#define		MXT_MSGB_T27_ROTATE		0x40
#define		MXT_MSGB_T27_STRETCH		0x80
#define	MXT_MSG_T27_XPOSMSB			0x02
#define	MXT_MSG_T27_YPOSMSB			0x03
#define	MXT_MSG_T27_XYPOSLSB			0x04
#define	MXT_MSG_T27_ANGLE			0x05

/* 16 bit */
#define	MXT_MSG_T27_SEPARATION				0x06

/* MXT_SPT_CTECONFIG_T28 Message address definitions			*/
#define	MXT_MSG_T28_STATUS				0x01
#define	MXT_MSGB_T28_CHKERR		0x01

/* MXT_GOLDENREFERENCES_T66 Message address definitions		*/
#define		MXT_MSGB_T66_GEN		0x02
#define		MXT_MSGB_T66_STORE		0x44
#define		MXT_MSGB_T66_FINISH		0x20
#define		MXT_MSGB_T66_ERROR		0x80

/* One Touch Events */
#define	MT_GESTURE_RESERVED				0x00
#define	MT_GESTURE_PRESS				0x01
#define	MT_GESTURE_RELEASE				0x02
#define	MT_GESTURE_TAP					0x03
#define	MT_GESTURE_DOUBLE_TAP				0x04
#define	MT_GESTURE_FLICK				0x05
#define	MT_GESTURE_DRAG					0x06
#define	MT_GESTURE_SHORT_PRESS				0x07
#define	MT_GESTURE_LONG_PRESS				0x08
#define	MT_GESTURE_REPEAT_PRESS				0x09
#define	MT_GESTURE_TAP_AND_PRESS			0x0a
#define	MT_GESTURE_THROW				0x0b

/* Bootloader states */
#define WAITING_BOOTLOAD_COMMAND   0xC0
#define WAITING_FRAME_DATA         0x80
#define APP_CRC_FAIL               0x40
#define FRAME_CRC_CHECK            0x02
#define FRAME_CRC_PASS             0x04
#define FRAME_CRC_FAIL             0x03

#define MXT_MAX_FRAME_SIZE         276

/* Debug levels */
#define NO_DEBUG       0
#define DEBUG_INFO     1
#define DEBUG_VERBOSE  2
#define DEBUG_MESSAGES 5
#define DEBUG_RAW      8
#define DEBUG_TRACE   10


/* Message buffer size. This is a ring buffer, and when full, the oldest entry
   will be overwritten. */
#define MXT_MESSAGE_BUFFER_SIZE  128


/*
 * vt1603_ts_platform_data - vt1603 configuration data
 * @panel_type:              touch panel type: 4-wired or 5-wired
 * @sclk_div:                initial value of sclk dividor
 *    if mclk = 12.288MHZ 0x04 = 200ksps 0x08 = 100ksps
 * @soc_gpio_irq:            soc gpio interrupts, connect with vt1603 gpio1
 */
struct mxt_ts_platform_data {
    u8  panel_type;
    u8  sclk_div;
    int i2c_bus_id;
//    int soc_gpio_irq;
    int gpio_num;
//    enum gpio_irq_trigger_type irq_type;
    u16 abs_x_min;
    u16 abs_x_max;
    u16 abs_y_min;
    u16 abs_y_max;
};

/**
 * struct mxt_platform_data - includes platform specific informatio
 * related to Atmel maXTouch touchscreen controller.
 *
 * @numtouch:           Number of simultaneous touches supported
 * @init_platform_hw(): Initialization function, which can for example
 *                      trigger a hardware reset by toggling a GPIO pin
 * @exit_platform_hw(): Function to run when the driver is unloaded.
 * @valid_interrupt():  Function that checks the validity of the interrupt -
 *                      function that check the validity of a interrupt (by
 *                      reading the changeline interrupt pin and checking that
 *                      it really is low for example).
 * @max_x:              Reported X range
 * @max_y:              Reported Y range
 */

struct mxt_platform_data {
	u8 numtouch;		/* Number of touches to report  */
	void (*init_platform_hw) (void);
	void (*exit_platform_hw) (void);
	int max_x;		/* The default reported X range   */
	int max_y;		/* The default reported Y range   */
	 u8(*valid_interrupt) (void);
	 u8(*read_chg) (void);
};

/* Device Info descriptor */
/* Parsed from maXTouch "Id information" inside device */
struct mxt_device_info {
	u8 family_id;
	u8 variant_id;
	u8 major;
	u8 minor;
	u8 build;
	u8 num_objs;
	u8 x_size;
	u8 y_size;
	char family_name[16];	/* Family name */
	char variant_name[16];	/* Variant name */
	u16 num_nodes;		/* Number of sensor nodes */
};

/* object descriptor table, parsed from maXTouch "object table" */
struct mxt_object {
	u16 chip_addr;
	u8 type;
	u8 size;
	u8 instances;
	u8 num_report_ids;
};

/* Driver datastructure */
struct mxt_data {
	struct i2c_client *client;
	struct input_dev *input;
	struct miscdevice  misc_dev;
	char phys_name[32];
	int irq;

	u16 last_read_addr;
	bool new_msgs;
	u8 *last_message;

	int valid_irq_counter;
	int invalid_irq_counter;
	int irq_counter;
	int message_counter;
	int read_fail_counter;

	int bytes_to_read;

	struct delayed_work dwork;
	u8 xpos_format;
	u8 ypos_format;

	u8 numtouch;

	struct mxt_device_info device_info;

	u32 info_block_crc;
	unsigned long configuration_crc;
	u16 report_id_count;
	struct report_id_map *rid_map;
	struct mxt_object *object_table;
      struct wake_lock wakelock;
	u16 msg_proc_addr;
	u8 message_size;

	u16 max_x_val;
	u16 max_y_val;

	void (*init_hw) (void);
	void (*exit_hw) (void);
	 u8(*valid_interrupt) (void);
	 u8(*read_chg) (void);

	/* debugfs variables */
	struct dentry *debug_dir;
	int current_debug_datap;

	struct mutex debug_mutex;
	u16 *debug_data;

	/* Character device variables */

	u16 address_pointer;
	bool valid_ap;

	/* Message buffer & pointers */
	char *messages;
	int msg_buffer_startp, msg_buffer_endp;
	/* Put only non-touch messages to buffer if this is set */
	char nontouch_msg_only;
	struct mutex msg_mutex;
	struct early_suspend early_suspend;
	struct attribute_group attrs;
	int status;
	struct semaphore sem;
	bool interruptable;
	bool hasTouch;
	struct switch_dev touch_sdev;
};



struct mxt_TP_ID {
	int TP_ID0;
	int TP_ID1;
};


struct asus_config{
        unsigned int selftest_calibration_status;
	unsigned int calibration_status;
	unsigned long touch_panel_config_checksum[4];
	struct mxt_TP_ID tp;
};


/* Bootloader mode status */
#define MXT_WAITING_BOOTLOAD_COMMAND   0xC0
#define MXT_WAITING_FRAME_DATA         0x80
#define MXT_FRAME_CRC_CHECK            0x02
#define MXT_FRAME_CRC_PASS             0x04
#define MXT_FRAME_CRC_FAIL             0x03
#define MXT_APP_CRC_FAIL	0x40
#define MXT_BOOT_STATUS_MASK	0x3f

/* Command to unlock bootloader */
#define MXT_UNLOCK_CMD_MSB	0xaa
#define MXT_UNLOCK_CMD_LSB	0xdc

#define OFILM_GFF	0
#define UNUSE1 		1
#define JTOUCH 		2
#define OFILM 		3

#define OFILM_GFF_DEFAULT 0xa4e8b
#define UNUSE1_DEFAULT 0xa4e8b
#define JTOUCH_DEFAULT 0xa4e8b
#define OFILM_DEFAULT  0xe10b89

#define MXT540S_FIRMWARE_ID_11 		0x11
#define MXT540S_FIRMWARE_ID_10 		0x10
#define MXT540S_FIRMWARE_ID_09 		0x09

#define MXT540S_FINAL_FIRMWARE_ID	MXT540S_FIRMWARE_ID_11
#define MXT540S_FINAL_BUILD_ID 		0xAA

#define FW_VERSION_HAS_UPDATED 0x02

#define FIRMWARE_BOOT_RECOVERY	0x00
#define FIRMWARE_BOOT_UPGRADE	0x01
#define FIRMWARE_APP_UPGRADE	0x02

#define GOLDEN_REFERENCE_INACTIVE		0x00
#define GOLDEN_REFERENCE_PRIME			0x02
#define GOLDEN_REFERENCE_GEN			0x04
#define GOLDEN_REFERENCE_CALIBRATION		0x08
#define GOLDEN_REFERENCE_NORMAL			0x10
#define GOLDEN_REFERENCE_STORE			0x20
#define GOLDEN_REFERENCE_FINISH			0x40
#define SELFTEST_CALIBRATION_START		0x80
#define GOLDEN_REFERENCE_ERROR			0x81

#define AP_CALIBRATION				0x00
#define AP_FORCE_CALIBRATION			0x01
#define AP_SELFTEST_CALIBRATION			0x02

#define USER_DATA_MAGIC_0			0x38
#define USER_DATA_MAGIC_1			0x72
#define USER_DATA_MAGIC_0_FOTA_V1		0x69
#define USER_DATA_MAGIC_1_FOTA_V1		0x42

#define X_LINE					0x1E
#define Y_LINE					0x12

#define READ_DUMMY_COUNT	0x3
#define READ_DUMMY_DELAY	100

u16 get_object_address(uint8_t object_type,
			      uint8_t instance,
			      struct mxt_object *object_table, int max_objs);

/* Returns the start address of object in mXT memory. */
#define	MXT_BASE_ADDR(object_type, mxt)					\
	get_object_address(object_type, 0, mxt->object_table,           \
			   mxt->device_info.num_objs)

struct mxt_object* get_object(uint8_t object_type,
	                 const struct mxt_data *mxt);

int __devinit mxt_identify(struct i2c_client *client,
				  struct mxt_data *mxt, u8 * id_block_data);

void mxt_hw_reset(void);
int mxt_write_byte(struct i2c_client *client, u16 addr, u8 value);
int mxt_read_block(struct i2c_client *client, u16 addr, u16 length, u8 *value);
int init_touch_config(struct mxt_data *mxt, u8 touch_vendor_id);
int init_touch_config_v2(struct mxt_data *mxt, u8 touch_vendor_id);
int init_touch_config_v3(struct mxt_data *mxt, u8 touch_vendor_id);
int init_touch_config_v3_FOTA_v1(struct mxt_data *mxt, u8 touch_vendor_id);
int clear_touch_config(struct mxt_data *mxt, int index);
int mxt_update_firmware(struct mxt_data *mxt, int inBoot, void * fw_ptr, long fw_size);
u8 readTP_ID(void);
int set_uboot_para(int type, u32 value);
int query_status(void);
int check_refs_before_selftest(struct mxt_data *mxt);