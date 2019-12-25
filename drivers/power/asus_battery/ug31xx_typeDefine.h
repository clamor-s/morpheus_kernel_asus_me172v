/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#ifndef _UG31XX_TYPE_DEFINE_H_
#define _UG31XX_TYPE_DEFINE_H_

#ifdef __KERNEL__
#include <linux/module.h>

#else //android execute binary

#endif

#include "ug31xx_global.h"

//#define I2C_ADDRESS    0x70
//#define I2C_CLOCK      0x100

//UG3100 register address mapping
#define REG_MODE_ADDR			0x00
#define REG_CTRL_ADDR			0x01

//const for CELL_TABLE table
#define TEMPERATURE_NUMS        4
#define C_RATE_NUMS		5	//0.1C, 0.2C, 0.5C, 0.7C, 1.0C
#define OCV_C_RATE_NUMS         2 //<ASUS-WAD+>
#define OCV_NUMS		21	//include the 0% & 100%

#define SOV_NUMS		16

typedef struct CELL_PARAMETER
{
	u16 totalSize;     //Total struct size
	u8 fw_ver;            //CellParameter struct version
	
	char customer[16];   //Customer name defined by uPI   //####2012/08/29#####
	char project[16];    //Project name defined by uPI
	u16 ggb_version;       //0x0101 => 1.1

	char  customerSelfDef[16];  //Customer name record by customer 
	char  projectSelfDef[16];   //Project name record by customer
        u16 cell_type_code; 

	u8 ICType;/*[2:0]=000 -> uG3100 [2:0]=001 -> uG3101
			    [2:0]=010 -> uG3102 [2:0]=100 -> uG3103_2
			    [2:0]=101 -> uG3103_3 */
	u8 gpio1;/*bit[4] cbc_en32
               bit[3] cbc_en21
               bit[2] pwm
               bit[1] alarm
               bit[0] gpio */
	u8 gpio2;/*bit[4] cbc_en32
               bit[3] cbc_en21
               bit[2] pwm
               bit[1] alarm
               bit[0] gpio */
	u8 gpio34;			//11/22/2011 -->reg92
	u8 chopCtrl;		//11/22/2011 -->regC1
	s16 adc1Offset;		//11/22/2011 -->reg58/59

	u8  cellNumber;
	u8  assignCellOneTo;
	u8  assignCellTwoTo;
	u8  assignCellThreeTo;

	u16 i2cAddress;    //I2C Address(Hex)
	u8  tenBitAddressMode;
	u8  highSpeedMode;
	u16 clock;

	u8  rSense;
	u16 ILMD;
	u16 edv1Voltage;
	u16 standbyCurrent;
	u16 TPCurrent;
	u16 TPVoltage;
	u16 TPTime;

	u16 offsetR;
	u16 deltaR;
	u8  maxDeltaQ;
	u16 offsetmaH;

	u8  timeInterval;

	s16 COC;			//2012/06/04
	s16 DOC;			//
	s16 UC;				//
	u16 OV1;
	u16 UV1;
	u16 OV2;
	u16 UV2;			
	u16 OV3;
	u16 UV3;
	u16 OVP;
	u16 UVP;
	u16 OIT;
	u16 UIT;
	u16 OET;
	u16 UET;
	u16 CBC21;
	u16 CBC32;

	s16 oscTuneJ;
	s16 oscTuneK;

	u8 alarm_timer;		//11/22/2011  00:*5,01:*10,10:*15,11:*20
	u8 clkDivA;			//11/22/2011
	u8 clkDivB;			//11/22/2011	

	u8 pwm_timer;    /*[1:0]=00:32k [1:0]=01:16k
					   [1:0]=10:8k  [1:0]=11: 4k */

	u8 alarmEnable;  /*[7]:COC [6]:DOC [5]:IT [4]:ET
					   [3]:VP  [2]:V3  [1]:V2 [0]:V1 */
	u8 cbcEnable;    /*[1]:CBC_EN32  [0]:CBC_EN21 */
//	u8 pullupSetting;  /*[4]:SDA pull up, [3]:SCL pull up
//                       [2]:gpio2 pull up,[1]:gpio1 pull up */
	u8 otp1Scale;
	u16 vBat2_8V_IdealAdcCode;	//ideal ADC Code
	u16 vBat2_6V_IdealAdcCode;
	u16 vBat3_12V_IdealAdcCode;
	u16 vBat3_9V_IdealAdcCode;

//	float vBat2Ra;		
//	float vBat2Rb;
//	float vBat3Ra;
//	float vBat3Rb; 

	u16 adc1_pgain;
	u16 adc1_ngain;
	s16 adc1_pos_offset;

	u16 adc2_gain;
	s16 adc2_offset;

	u16 R;
	u16 rtTable[19];

	// SOV_TABLE %
    u16 SOV_TABLE[SOV_NUMS];

	s16 adc_d1;				//2012/06/06/update for IT25
	s16 adc_d2;				//2012/06/06/update for IT80

	s16 adc_d3;
	s16 adc_d4;
	s16 adc_d5;

}  __attribute__((packed)) CELL_PARAMETER;

typedef struct CELL_TABLE
{
	s16 INIT_OCV[TEMPERATURE_NUMS][OCV_C_RATE_NUMS][OCV_NUMS];							//initial OCV Table,0.1C/0.2C OCV
	s16 CELL_VOLTAGE_TABLE[TEMPERATURE_NUMS][C_RATE_NUMS][OCV_NUMS];		//cell behavior Model,the voltage data
	s16 CELL_NAC_TABLE[TEMPERATURE_NUMS][C_RATE_NUMS][OCV_NUMS];			//cell behavior Model,the deltaQ
}  __attribute__((packed)) CELL_TABLE;

//<ASUS-WAD+>
typedef struct _GGBX_FILE_HEADER
{
        u32             ggb_tag;        //'_GG_'
        u32             sum16;          //16 bits checksum, but store as 4 bytes
        u64             time_stamp;     //seconds pass since 1970 year, 00:00:00
        u64             length;         //size that not only include ggb content. (multi-file support)
        u64             num_ggb;        //number of ggb files. 
}  __attribute__((packed)) GGBX_FILE_HEADER;
//<ASUS-WAD->

#define GGBX_FILE_TAG 0x5F47475F // _GG_
#define GGBX_FACTORY_FILE_TAG 0x5F67675F // _gg_

typedef struct CELL_DATA
{
	CELL_PARAMETER cellParameter;
	CELL_TABLE cellTable1;
//	CELL_TABLE cellTable2;    ////####2012/08/29#####
//	CELL_TABLE cellTable3;
//	u8 checkSum;
}  __attribute__((packed)) CELL_DATA;

typedef struct USER_REGISTER
{
	u8 mode;
	u8 ctrl1;
	u8 charge_low;
	u8 charge_high;
	u8 counter_low;
	u8 counter_high;
	u8 current_low;
        u8 current_high;
	u8 vbat1_low;
	u8 vbat1_high;
	u8 intr_temper_low;
	u8 intr_temper_high;
	u8 ave_current_low;
	u8 ave_current_high;
	u8 extr_temper_low;
	u8 extr_temper_high;
	u8 rid_low;
	u8 rid_high;
	u8 alarm1_status;
	u8 alarm2_status;
	u8 intr_status;
	u8 alram_en;
	u8 ctrl2;
}  __attribute__((packed)) USER_REGISTER;

//ALARM_STATUS mapping to user register alarm1_status and alarm2_status
typedef struct ALARM_STATUS {
	u8
		uet:1,	//bit0
		oet:1,	//bit1
		uit:1,
		oit:1,
		doc:1,
		coc:1,
		rev1:2;  //bit 78
	u8
		uv1:1,
		ov1:1,
		uv2:1,
		ov2:1,
		uv3:1,
		ov3:1,
		uvp:1,
		ovp:1;
	u8
		rev2:5,
		uc:1,
		cbc21:1,
        cbc32:1;
} __attribute__((packed)) ALARM_STATUS;
typedef struct CaliAdcDataST
{
  /// [AT] : ADC code in CP (Read from OTP array) ; 07/03/2012
  u16 CodeT25V100;
  u16 CodeT25V200;
  u16 CodeT80V100;
  u16 CodeT80V200;

  /// [AT] : Internal temperature code in CP ; 07/03/2012
  u16 CodeT25IT;
  u16 CodeT80IT;

  /// [AT] : Slope and factor b of ADC gain ; 07/26/2012
  s32 GainSlope;
  s32 GainFactorB;

  /// [AT] : Slope and factor o of ADC gain ; 07/26/2012
  s32 OffsetSlope;
  s32 OffsetFactorO;

  /// [AT] : ADC gain and offset ; 07/26/2012
  s32 AdcGain;
  s32 AdcOffset;

  /// [AT] : ADC conversion factor ; 07/03/2012
	u32 InputVoltLow;
	u32 InputVoltHigh;
  u16 IdealCodeLow;
  u16 IdealCodeHigh;

  /// [AT] : Calibrated code ; 07/03/2012
  u16 CaliCode;
} __attribute__((packed)) CaliAdcDataType;
#endif //_UG31XX_TYPE_DEFINE_H_

