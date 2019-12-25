#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include "./atmel_maxtouch.h"


u8 version = 0;



int init_touch_config(struct mxt_data *mxt, u8 touch_vendor_id){

       	int i, object_size;
       	u16 address;
       	struct mxt_object *object;
	int err;
	u8 buf[0x8];

        version = mxt->device_info.major << 4 | mxt->device_info.minor;
        dev_info(&mxt->client->dev,"[touch] Touch: init register in function:%s, vendor=%2.2X version=0x%2.2X\n", __func__, touch_vendor_id, version);

       	if(version == MXT540S_FIRMWARE_ID_09){
                dev_info(&mxt->client->dev,"[touch] Touch: init_touch_config Firmware ID=%X\n",version);

	}else if(version == MXT540S_FIRMWARE_ID_10){
                dev_info(&mxt->client->dev,"[touch] Touch: init_touch_config_v2 Firmware ID=%X\n",version);
          	return init_touch_config_v2(mxt, touch_vendor_id);
	}else{
        	err = mxt_read_block(mxt->client, MXT_BASE_ADDR(MXT_SPT_USER_DATA_T38, mxt), 0x8, (u8 *) buf);
        	if (err < 0)	dev_info(&mxt->client->dev,"init_touch_config() : mxt_read_block failure.\n");
		if(buf[6]==USER_DATA_MAGIC_0 && buf[7]==USER_DATA_MAGIC_1){
                	dev_info(&mxt->client->dev,"[touch] Touch: init_touch_config_v3 Firmware ID=%X\n",version);
          		return init_touch_config_v3(mxt, touch_vendor_id);
		}else{
                	dev_info(&mxt->client->dev,"[touch] Touch: init_touch_config_v3_FOTA_v1 Firmware ID=%X\n",version);
          		return init_touch_config_v3_FOTA_v1(mxt, touch_vendor_id);
		}
	}
//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt),   0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt)+1, 0xFF);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt)+2, 0x19);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt)+3, 0x43);

//=================================================================================================

      	switch(touch_vendor_id){
      	case OFILM_GFF:
      	case UNUSE1:
      	case JTOUCH:
          	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt), 0x50);
          break;
      	case OFILM:
          	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt), 0x2d);
          break;
      	}

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+1, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+2, 0x1E);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+3, 0x1E);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+4, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+5, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+6, 0xff);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+7, 0x01);
 	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+8, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+9, 0x00);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt),    0x8F);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+1,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+2,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+3,  0x1E);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+4,  0x12);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+5,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+6,  0x60);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+7,  0x2D);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+8,  0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+9,  0x04);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+10, 0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+11, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+12, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+13, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+14, 0x0A);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+15, 0x0A);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+16, 0x19);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+17, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+18, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+19, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+20, 0x57);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+21, 0x02);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+22, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+23, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+24, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+25, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+26, 0x40);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+27, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+28, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+29, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+30, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+31, 0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+32, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+33, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+34, 0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+35, 0x00);
//=================================================================================================

      	object = get_object(MXT_TOUCH_KEYARRAY_T15, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_SPT_COMMSCONFIG_T18, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_SPT_GPIOPWM_T19, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_TOUCH_PROXIMITY_T23, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt),    0x03);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+1,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+2,  0x78);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+3,  0x69);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+4,  0x38);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+5,  0x4a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+6,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+7,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+8,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+9,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+10, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+11, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+12, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+13, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+14, 0x00);

//=================================================================================================

      	object = get_object(MXT_PROCI_GRIPSUPPRESSION_T40, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt),   0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+1, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+2, 0x10);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+3, 0x18);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+4, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+5, 0x00);
        mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+6, 0x04);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+7, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+8, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+9, 0x01);

//=================================================================================================

      	object = get_object(MXT_PROCI_STYLUS_T47, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_PROCI_ADAPTIVETHRESHOLD_T55, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt),      0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 1,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 2,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 3,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 4,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 5,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 6,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 7,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 8,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 9,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 10, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 11, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 12, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 13, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 14, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 15, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 16, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 17, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 18, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 19, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 20, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 21, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 22, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 23, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 24, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 25, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 26, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 27, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 28, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 29, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 30, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 31, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 32, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 33, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 34, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 35, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 36, 0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 37, 0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 38, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 39, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 40, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 41, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 42, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 43, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 44, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 45, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 46, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 47, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 48, 0x00);

//=================================================================================================

      	object = get_object(MXT_PROCI_EXTRATOUCHAREADATA_T57, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      object = get_object(MXT_SPT_TIMER_T61, mxt);
      address = object->chip_addr;
      object_size = object->size * object->instances;
      for(i = 0; i < object_size; i++)
          mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt),      0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 1,  0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 2,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 3,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 4,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 5,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 6,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 7,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 8,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 9,  0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 10, 0x19);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 11, 0x1e);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 12, 0x23);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 13, 0x28);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 14, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 15, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 16, 0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 17, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 18, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 19, 0x60);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 20, 0x10);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 21, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 22, 0x2d);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 23, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 24, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 25, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 26, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 27, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 28, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 29, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 30, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 31, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 32, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 33, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 34, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 35, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 36, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 37, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 38, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 39, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 40, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 41, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 42, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 43, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 44, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 45, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 46, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 47, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 48, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 49, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 50, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 51, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 52, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 53, 0x00);



//=================================================================================================
	return 0;

}

int init_touch_config_v2(struct mxt_data *mxt, u8 touch_vendor_id)
{

      	int i, object_size;
       	u16 address;
       	struct mxt_object *object;


//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt),   0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt)+1, 0xFF);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt)+2, 0x19);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt)+3, 0x43);

//=================================================================================================

      	switch(touch_vendor_id){
      	case OFILM_GFF:
      	case UNUSE1:
      	case JTOUCH:
          	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt), 0x50);
          break;
      	case OFILM:
          	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt), 0x2d);
          break;
      	}

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+1, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+2, 0x1E);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+3, 0x1E);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+4, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+5, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+6, 0xff);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+7, 0x01);
 	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+8, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+9, 0x00);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt),    0x8F);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+1,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+2,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+3,  0x1E);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+4,  0x12);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+5,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+6,  0x60);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+7,  0x2D);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+8,  0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+9,  0x04);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+10, 0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+11, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+12, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+13, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+14, 0x0A);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+15, 0x0A);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+16, 0x19);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+17, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+18, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+19, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+20, 0x57);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+21, 0x02);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+22, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+23, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+24, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+25, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+26, 0x40);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+27, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+28, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+29, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+30, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+31, 0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+32, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+33, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+34, 0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+35, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+36, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+37, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+38, 0x00);

//=================================================================================================

      	object = get_object(MXT_TOUCH_KEYARRAY_T15, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_SPT_COMMSCONFIG_T18, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_SPT_GPIOPWM_T19, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_TOUCH_PROXIMITY_T23, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt),    0x03);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+1,  0x00);
      	switch(touch_vendor_id){
      	case OFILM_GFF:
      	case UNUSE1:
      	case JTOUCH:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+2,  0x78);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+3,  0x69);
          break;
      	case OFILM:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+2,  0x4a);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+3,  0x71);
          break;
      	}
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+4,  0x38);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+5,  0x4a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+6,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+7,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+8,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+9,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+10, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+11, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+12, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+13, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+14, 0x00);

//=================================================================================================

      	object = get_object(MXT_PROCI_GRIPSUPPRESSION_T40, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt),   0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+1, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+2, 0x10);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+3, 0x18);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+4, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+5, 0x00);
        mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+6, 0x04);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+7, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+8, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+9, 0x01);

//=================================================================================================

      	object = get_object(MXT_PROCI_STYLUS_T47, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_PROCI_ADAPTIVETHRESHOLD_T55, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt),      0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 1,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 2,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 3,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 4,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 5,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 6,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 7,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 8,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 9,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 10, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 11, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 12, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 13, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 14, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 15, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 16, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 17, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 18, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 19, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 20, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 21, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 22, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 23, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 24, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 25, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 26, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 27, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 28, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 29, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 30, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 31, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 32, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 33, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 34, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 35, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 36, 0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 37, 0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 38, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 39, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 40, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 41, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 42, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 43, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 44, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 45, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 46, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 47, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 48, 0x00);

//=================================================================================================

      	object = get_object(MXT_PROCI_EXTRATOUCHAREADATA_T57, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      object = get_object(MXT_SPT_TIMER_T61, mxt);
      address = object->chip_addr;
      object_size = object->size * object->instances;
      for(i = 0; i < object_size; i++)
          mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt),      0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 1,  0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 2,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 3,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 4,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 5,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 6,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 7,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 8,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 9,  0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 10, 0x19);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 11, 0x1e);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 12, 0x23);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 13, 0x28);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 14, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 15, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 16, 0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 17, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 18, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 19, 0x60);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 20, 0x10);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 21, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 22, 0x2d);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 23, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 24, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 25, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 26, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 27, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 28, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 29, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 30, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 31, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 32, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 33, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 34, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 35, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 36, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 37, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 38, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 39, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 40, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 41, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 42, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 43, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 44, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 45, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 46, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 47, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 48, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 49, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 50, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 51, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 52, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 53, 0x00);


//=================================================================================================
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x13);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 1, 0x1e);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 2, 0xff);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 3, 0x03);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 4, 0x10);


      	object = get_object(MXT_PALMGESTUREPROCESSOR_T69, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================
	return 0;
}

int init_touch_config_v3(struct mxt_data *mxt, u8 touch_vendor_id)
{

       	int i, object_size;
       	u16 address;
       	struct mxt_object *object;

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt),   0x1E);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt)+1, 0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt)+2, 0x19);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt)+3, 0x43);

//=================================================================================================

      	switch(touch_vendor_id){
      	case OFILM_GFF:
      	case UNUSE1:
      	case JTOUCH:
          	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt), 0x50);
          break;
      	case OFILM:
          	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt), 0x00);
          break;
      	}

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+1, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+2, 0xFF);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+3, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+4, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+5, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+6, 0xff);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+7, 0x01);
 	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+8, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+9, 0x00);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt),    0x8F);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+1,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+2,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+3,  0x1E);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+4,  0x12);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+5,  0x00);
      	switch(touch_vendor_id){
      	case OFILM_GFF:
      	case UNUSE1:
      	case JTOUCH:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+6,  0x60);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+7,  0x2D);
          break;
      	case OFILM:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+6,  0x50);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+7,  0x23);
          break;
      	}

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+8,  0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+9,  0x04);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+10, 0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+11, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+12, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+13, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+14, 0x0A);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+15, 0x0A);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+16, 0x19);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+17, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+18, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+19, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+20, 0x57);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+21, 0x02);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+22, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+23, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+24, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+25, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+26, 0x40);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+27, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+28, 0x40);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+29, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+30, 0x12);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+31, 0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+32, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+33, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+34, 0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+35, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+36, 0x45);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+37, 0xDC);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+38, 0x00);

//=================================================================================================

      	object = get_object(MXT_TOUCH_KEYARRAY_T15, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_SPT_COMMSCONFIG_T18, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_SPT_GPIOPWM_T19, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_TOUCH_PROXIMITY_T23, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt),    0x03);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+1,  0x00);


      	switch(touch_vendor_id){
      	case OFILM_GFF:
      	case UNUSE1:
      	case JTOUCH:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+2,  0x90);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+3,  0x65);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+4,  0xF0);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+5,  0x55);
          break;
      	case OFILM:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+2,  0xA8);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+3,  0x61);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+4,  0x20);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+5,  0x4E);
          break;
      	}
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+6,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+7,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+8,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+9,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+10, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+11, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+12, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+13, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+14, 0x64);


//=================================================================================================

      	object = get_object(MXT_PROCI_GRIPSUPPRESSION_T40, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt),   0x01);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+1, 0x14);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+2, 0x14);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+3, 0x14);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+4, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+5, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+6, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+7, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+8, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+9, 0x00);

//=================================================================================================

      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt),   0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+1, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+2, 0x0C);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+3, 0x18);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+4, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+5, 0x00);
      	switch(touch_vendor_id){
      	case OFILM_GFF:
      	case UNUSE1:
      	case JTOUCH:
       	 	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+6, 0x04);
          break;
      	case OFILM:
        	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+6, 0x03);
          break;
      	}
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+7, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+8, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+9, 0x01);

//=================================================================================================

      	object = get_object(MXT_PROCI_STYLUS_T47, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_PROCI_ADAPTIVETHRESHOLD_T55, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt),      0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 1,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 2,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 3,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 4,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 5,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 6,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 7,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 8,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 9,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 10, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 11, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 12, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 13, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 14, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 15, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 16, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 17, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 18, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 19, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 20, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 21, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 22, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 23, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 24, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 25, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 26, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 27, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 28, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 29, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 30, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 31, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 32, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 33, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 34, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 35, 0x00);


//=================================================================================================

      	object = get_object(MXT_PROCI_EXTRATOUCHAREADATA_T57, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      object = get_object(MXT_SPT_TIMER_T61, mxt);
      address = object->chip_addr;
      object_size = object->size * object->instances;
      for(i = 0; i < object_size; i++)
          mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt),      0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 1,  0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 2,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 3,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 4,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 5,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 6,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 7,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 8,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 9,  0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 10, 0x19);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 11, 0x1e);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 12, 0x23);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 13, 0x28);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 14, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 15, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 16, 0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 17, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 18, 0x05);
      	switch(touch_vendor_id){
      	case OFILM_GFF:
      	case UNUSE1:
      	case JTOUCH:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 19, 0x60);
          break;
      	case OFILM:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 19, 0x50);
          break;
      	}
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 20, 0x10);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 21, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 22, 0x2d);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 23, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 24, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 25, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 26, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 27, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 28, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 29, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 30, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 31, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 32, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 33, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 34, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 35, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 36, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 37, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 38, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 39, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 40, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 41, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 42, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 43, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 44, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 45, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 46, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 47, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 48, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 49, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 50, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 51, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 52, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 53, 0x00);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt),      0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 1,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 2,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 3,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 4,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 5,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 6,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 7,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 8,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 9,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 10, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 11, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 12, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 13, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 14, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 15, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 16, 0x00);

//=================================================================================================
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x03);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 1, 0x32);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 2, 0x8C);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 3, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 4, 0x00);


      	object = get_object(MXT_PALMGESTUREPROCESSOR_T69, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================
	return 0;
}

int init_touch_config_v3_FOTA_v1(struct mxt_data *mxt, u8 touch_vendor_id)
{

       	int i, object_size;
       	u16 address;
       	struct mxt_object *object;

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt),   0x1E);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt)+1, 0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt)+2, 0x19);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7, mxt)+3, 0x43);

//=================================================================================================

      	switch(touch_vendor_id){
      	case OFILM_GFF:
          	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt), 0x50);
          break;
      	case UNUSE1:
      	case JTOUCH:
          	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt), 0x50);
          break;
      	case OFILM:
          	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt), 0x00);
          break;
      	}

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+1, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+2, 0xFF);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+3, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+4, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+5, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+6, 0xff);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+7, 0x01);
 	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+8, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8, mxt)+9, 0x00);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt),    0x8F);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+1,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+2,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+3,  0x1E);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+4,  0x12);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+5,  0x00);
      	switch(touch_vendor_id){
      	case OFILM_GFF:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+6,  0x70);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+7,  0x28);
          break;
      	case UNUSE1:
      	case JTOUCH:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+6,  0x70);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+7,  0x35);
          break;
      	case OFILM:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+6,  0x60);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+7,  0x23);
          break;
      	}

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+8,  0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+9,  0x04);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+10, 0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+11, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+12, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+13, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+14, 0x0A);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+15, 0x0A);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+16, 0x19);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+17, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+18, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+19, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+20, 0x57);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+21, 0x02);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+22, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+23, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+24, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+25, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+26, 0x40);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+27, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+28, 0x40);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+29, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+30, 0x12);
      	switch(touch_vendor_id){
      	case OFILM_GFF:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+31, 0x08);
          break;
      	case UNUSE1:
      	case JTOUCH:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+31, 0x0a);
          break;
      	case OFILM:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+31, 0x0a);
          break;
      	}
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+32, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+33, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+34, 0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+35, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+36, 0x45);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+37, 0xDC);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9, mxt)+38, 0x00);

//=================================================================================================

      	object = get_object(MXT_TOUCH_KEYARRAY_T15, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_SPT_COMMSCONFIG_T18, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_SPT_GPIOPWM_T19, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_TOUCH_PROXIMITY_T23, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt),    0x03);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+1,  0x00);


      	switch(touch_vendor_id){
      	case OFILM_GFF:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+2,  0x60);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+3,  0x6D);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+4,  0xC0);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+5,  0x5D);
          break;
      	case UNUSE1:
      	case JTOUCH:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+2,  0x54);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+3,  0x6F);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+4,  0xC0);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+5,  0x5D);
          break;
      	case OFILM:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+2,  0x58);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+3,  0x66);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+4,  0x14);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+5,  0x50);
          break;
      	}
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+6,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+7,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+8,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+9,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+10, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+11, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+12, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+13, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25, mxt)+14, 0x64);


//=================================================================================================

      	object = get_object(MXT_PROCI_GRIPSUPPRESSION_T40, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt),   0x01);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+1, 0x14);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+2, 0x14);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+3, 0x14);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+4, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+5, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+6, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+7, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+8, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42, mxt)+9, 0x00);

//=================================================================================================

      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt),   0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+1, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+2, 0x0C);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+3, 0x18);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+4, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+5, 0x00);
      	switch(touch_vendor_id){
      	case OFILM_GFF:
       	 	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+6, 0x04);
          break;
      	case UNUSE1:
      	case JTOUCH:
       	 	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+6, 0x04);
          break;
      	case OFILM:
        	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+6, 0x03);
          break;
      	}
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+7, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+8, 0x00);
      	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46, mxt)+9, 0x01);

//=================================================================================================

      	object = get_object(MXT_PROCI_STYLUS_T47, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      	object = get_object(MXT_PROCI_ADAPTIVETHRESHOLD_T55, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================
      	switch(touch_vendor_id){
      	case OFILM_GFF:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt),      0x01);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 1,  0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 2,  0x01);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 3,  0x46);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 4,  0x09);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 5,  0x09);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 6,  0x09);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 7,  0x09);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 8,  0x09);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 9,  0x09);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 10, 0x09);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 11, 0x09);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 12, 0x09);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 13, 0x09);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 14, 0x09);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 15, 0x08);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 16, 0x08);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 17, 0x08);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 18, 0x08);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 19, 0x08);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 20, 0x08);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 21, 0x08);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 22, 0x08);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 23, 0x08);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 24, 0x06);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 25, 0x06);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 26, 0x06);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 27, 0x06);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 28, 0x06);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 29, 0x06);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 30, 0x06);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 31, 0x06);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 32, 0x06);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 33, 0x06);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 34, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 35, 0x00);
          break;
      	case UNUSE1:
      	case JTOUCH:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt),      0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 1,  0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 2,  0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 3,  0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 4,  0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 5,  0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 6,  0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 7,  0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 8,  0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 9,  0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 10, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 11, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 12, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 13, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 14, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 15, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 16, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 17, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 18, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 19, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 20, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 21, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 22, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 23, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 24, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 25, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 26, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 27, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 28, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 29, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 30, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 31, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 32, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 33, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 34, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 35, 0x00);
          break;
      	case OFILM:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt),      0x01);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 1,  0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 2,  0x01);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 3,  0x18);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 4,  0x0C);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 5,  0x0C);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 6,  0x0C);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 7,  0x0C);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 8,  0x0C);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 9,  0x0C);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 10, 0x0C);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 11, 0x0C);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 12, 0x0C);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 13, 0x0C);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 14, 0x0C);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 15, 0x0C);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 16, 0x0C);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 17, 0x0C);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 18, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 19, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 20, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 21, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 22, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 23, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 24, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 25, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 26, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 27, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 28, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 29, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 30, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 31, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 32, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 33, 0x0A);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 34, 0x00);
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCI_SHIELDLESS_T56, mxt) + 35, 0x00);
          break;
      	}

//=================================================================================================

      	object = get_object(MXT_PROCI_EXTRATOUCHAREADATA_T57, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

      object = get_object(MXT_SPT_TIMER_T61, mxt);
      address = object->chip_addr;
      object_size = object->size * object->instances;
      for(i = 0; i < object_size; i++)
          mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt),      0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 1,  0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 2,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 3,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 4,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 5,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 6,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 7,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 8,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 9,  0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 10, 0x19);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 11, 0x1e);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 12, 0x23);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 13, 0x28);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 14, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 15, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 16, 0x0a);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 17, 0x05);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 18, 0x05);
      	switch(touch_vendor_id){
      	case OFILM_GFF:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 19, 0x70);
          break;
      	case UNUSE1:
      	case JTOUCH:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 19, 0x70);
          break;
      	case OFILM:
		mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 19, 0x50);
          break;
      	}
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 20, 0x10);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 21, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 22, 0x2d);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 23, 0x14);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 24, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 25, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 26, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 27, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 28, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 29, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 30, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 31, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 32, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 33, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 34, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 35, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 36, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 37, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 38, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 39, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 40, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 41, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 42, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 43, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 44, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 45, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 46, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 47, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 48, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 49, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 50, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 51, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 52, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62, mxt) + 53, 0x00);

//=================================================================================================

	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt),      0x01);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 1,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 2,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 3,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 4,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 5,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 6,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 7,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 8,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 9,  0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 10, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 11, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 12, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 13, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 14, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 15, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_LENSBENDING_T65, mxt) + 16, 0x00);

//=================================================================================================
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt),     0x03);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 1, 0x32);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 2, 0x46);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 3, 0x00);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GOLDENREFERENCES_T66, mxt) + 4, 0x00);


      	object = get_object(MXT_PALMGESTUREPROCESSOR_T69, mxt);
      	address = object->chip_addr;
      	object_size = object->size * object->instances;
      	for(i = 0; i < object_size; i++)
          	mxt_write_byte(mxt->client, address + i, 0);

//=================================================================================================
	return 0;
}

int clear_touch_config(struct mxt_data *mxt, int index)
{

      int i, object_size;
      struct mxt_object *object;
      u16 address;
      object = get_object(index, mxt);
      address = object->chip_addr;
      object_size = object->size * object->instances;
      for(i = 0; i < object_size; i++)
          mxt_write_byte(mxt->client, address + i, 0);

	return 0;
}


static int mxt_unlock_bootloader(struct i2c_client *client)
{
	u8 buf[2];

	buf[0] = MXT_UNLOCK_CMD_LSB;
	buf[1] = MXT_UNLOCK_CMD_MSB;

	if (i2c_master_send(client, buf, 2) != 2) {
		dev_err(&client->dev, "[touch] %s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int mxt_check_boot_status(struct i2c_client *client, u8 state){
      u8 val;

      msleep(20);
recheck:
	if (i2c_master_recv(client, &val, 1) != 1) {
		dev_err(&client->dev, "[touch] %s: i2c recv failed\n", __func__);
		return -EIO;
	}
//		dev_err(&client->dev, "bootloader mode state %x\n",state);
//		dev_err(&client->dev, "bootloader mode val %x\n",val);

	switch (state) {
	case MXT_WAITING_BOOTLOAD_COMMAND:
	case MXT_WAITING_FRAME_DATA:
		val &= ~MXT_BOOT_STATUS_MASK;
//		dev_err(&client->dev, "bootloader mode val %x\n",val);
		break;

	case MXT_FRAME_CRC_PASS:
//		dev_err(&client->dev, "MXT_FRAME_CRC_PASS val %x\n",val);
//		dev_err(&client->dev, "MXT_FRAME_CRC_PASS state %x\n",state);
		if (val == MXT_FRAME_CRC_CHECK)
			goto recheck;
		break;
	default:
		return -EINVAL;
	}

	if (val != state) {
		dev_err(&client->dev, "[touch] Unvalid bootloader mode state %x\n",state);
		return -EINVAL;
	}

	return 0;
}

int mxt_update_firmware(struct mxt_data *mxt, int inBoot, void * fw_ptr, long fw_size)
{
    u8 firmware_id[MXT_ID_BLOCK_SIZE];
    int ret, retry = 0;
    unsigned char *firmware_data=NULL;
    unsigned long pos = 0;
    unsigned long frame_size = 0, firmware_size=0;
    struct i2c_client *client;


    client = mxt->client;

    switch(inBoot){

    	case FIRMWARE_BOOT_RECOVERY:
    	    firmware_data = (unsigned char *)fw_ptr;
    	    firmware_size = fw_size;
	    goto mxt_boot_start; // if already in boot loader mode
    	break;
    	case FIRMWARE_BOOT_UPGRADE:
    	case FIRMWARE_APP_UPGRADE:
    	    firmware_data = (unsigned char *)fw_ptr;
    	    firmware_size = fw_size;
            dev_info(&client->dev, "[touch] Touch: fw_size=%lx\n", fw_size);
            dev_info(&client->dev, "[touch] Touch: firmware_size=%lx\n", firmware_size);
    	break;

	default:

	break;
    }
    /*checkout firmware version and don't update if the version is equal to firmware ID in header file */
    mxt_identify(mxt->client, mxt, firmware_id);
    dev_info(&client->dev, "[touch] firmware_id =%x  MXT540S_FIRMWARE_ID_10=%x\n", firmware_id[2], MXT540S_FIRMWARE_ID_10);
#if 0
    if(firmware_id[2] == MXT540S_FIRMWARE_ID_10){

        dev_info(&client->dev, "[touch] Firware is the latest version %d.%d. No need to update!\n",
			mxt->device_info.major, mxt->device_info.major);
	  return FW_VERSION_HAS_UPDATED;
    }
#endif
//            dev_info(&client->dev, "[touch] Touch: mxt->client=%x\n", mxt->client);
//            dev_info(&client->dev, "[touch] Touch: client=%x\n", client);
    /* start get into boot loader mode*/
    mxt_write_byte(client, MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6, mxt) + MXT_ADR_T6_RESET,
                            0xA5);

    msleep(250);
mxt_boot_start:
    if (client->addr == MXT_I2C_ADDRESS) {
		dev_info(&client->dev,"[touch] Change I2C address from 0x%02X to 0x%02X\n",
			MXT_I2C_ADDRESS, MXT_BL_ADDRESS);
		client->addr = MXT_BL_ADDRESS;
    }

    switch(inBoot){
    	case FIRMWARE_BOOT_RECOVERY:
    	case FIRMWARE_BOOT_UPGRADE:
    	ret = mxt_check_boot_status(client,  MXT_WAITING_BOOTLOAD_COMMAND);
    	if(ret < 0){
        	dev_info(&client->dev, "[touch] Error to get into state MXT_WAITING_BOOTLOAD_COMMAND\n");
        	goto mxt_boot_exit;
    	}
    	break;
    	case FIRMWARE_APP_UPGRADE:
	default:
	break;
    }

    /*unlock the bootloader*/
    ret = mxt_unlock_bootloader(client);
    while(pos < firmware_size){
        retry = 10;
        ret = mxt_check_boot_status(client, MXT_WAITING_FRAME_DATA);
        if (ret < 0){
            dev_err(&client->dev, "[touch] Error to get into state: MXT_WAITING_FRAME_DATA\n");
            goto mxt_boot_exit;
        }

        frame_size = ((*(firmware_data + pos) << 8) | *(firmware_data + pos + 1));
        dev_info(&client->dev, "[touch] Touch: frame_size=%lx\n", frame_size);
        /* We should add 2 at frame size as the the firmware data is not
        * included the CRC bytes.*/
        frame_size += 2;
mxt_send_boot_frame:
        /* Write one frame to device */
        i2c_master_send(client, (unsigned char *)(firmware_data + pos), frame_size);
        ret = mxt_check_boot_status(client, MXT_FRAME_CRC_PASS);
        if (ret < 0){
            if(retry-- > 0){
                dev_info(&client->dev, "[touch] #########Try send frame again!\n ");
                goto mxt_send_boot_frame;
            }
            goto mxt_boot_exit;
	  }

         pos += frame_size;
         dev_info(&client->dev, "[touch] Updated %lx bytes / %lx bytes\n", pos, firmware_size);
    }
    msleep(1000);
    while(mxt->read_chg()!=0){
   	dev_info(&mxt->client->dev, "[touch] Touch: wait for chg pin pull low.\n");
	msleep(500);
    }
mxt_boot_exit:
    if (client->addr == MXT_BL_ADDRESS) {
        dev_info(&client->dev,"[touch] Change I2C address from 0x%02X to 0x%02X\n", MXT_BL_ADDRESS, MXT_I2C_ADDRESS);
        client->addr = MXT_I2C_ADDRESS;
    }
    dev_info(&client->dev, "[touch] QT_Boot end\n");
    return ret;
}