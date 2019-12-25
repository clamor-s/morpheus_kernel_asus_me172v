/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#ifndef _UG31XX_REG_DEF_H_
#define _UG31XX_REG_DEF_H_

#include "ug31xx_global.h"
#include "ug31xx_typeDefine.h"

#define CALI_WITH_IT_OTP_PARA      ///< Default: ON
#ifdef  CALI_WITH_IT_OTP_PARA
#define FEATURE_IT_CALIBRATION        ///< DEFAULT : ON
#endif  ///< end of CALI_WITH_IT_OTP_PARA
#define FEATURE_ADC_CALIBRATION         ///< DEFAULT : ON
#ifdef FEATURE_ADC_CALIBRATION
#define	FEATURE_ADC_CALI_KAPIK          ///< DEFAULT : ON
//#define FEATURE_ADC_CALI_DEBUG          ///< DEFAULT : OFF
#define FEATURE_ADC_CALI_AT_VBAT1       ///< DEFAULT : ON
#define FEATURE_ADC_CALI_AT_CURRENT     ///< DEFAULT : ON
#define FEATURE_ADC_CALI_AT_V123        ///< DEFAULT : ON
#define FEATURE_ADC_CALI_AT_VOLTAGE     ///< DEFAULT : ON
//#define FEATURE_ADC_CALI_USE_CALIITCODE ///< DEFAULT : OFF
//#define FEATURE_ADC_CALI_FT             ///< DEFAULT : OFF
//#define FEATURE_ADC_MANUAL_CP       ///< DEFAULT : OFF
#ifdef  FEATURE_ADC_CALI_FT
#define ADC1_200_GAIN     (110224)
#define ADC1_100_GAIN     (77356)
#define ADC2_200_GAIN     (-108280)
#define ADC2_100_GAIN     (-141732)
#define ADC_GAIN_CONST    (1000)
#endif  ///< end of FEATURE_ADC_CALI_FT
#endif ///< end of FEATURE_ADC_CALIBRATION
//#define FEATURE_LOAD_OTP_FROM_FILE      ///< DEFAULT : OFF
//#define FEATURE_EMPLOY_COULOMB_COUNTER      ///< DEFAULT : OFF

//<ASUS-Wade1_Chen20120910+>
#define BIT_CALI_WITH_IT_OTP_PARA               0x00000001
#define BIT_FEATURE_IT_CALIBRATION              0x00000002
struct ug31_hw_config {
        u32 feature_flag;
};
//<ASUS-Wade1_Chen20120910->

#define  SECURITY			1		//Security Mode enable
#define  NORMAL				0		//Security Mode OFF

#define  HIGH_SPEED			1		//HS Mode
#define  FULL_SPEED			0		//FIL speed

#define  TEN_BIT_ADDR		1		//10-bit address Mode
#define	 SEVEN_BIT_ADDR		0		//7-bit address Mode

#define  I2C_SUCESS			1		//
#define  I2C_FAIL			0

/// ===========================================================================
/// Constant for Calibration
/// ===========================================================================

#define IT_TARGET_CODE25	(12155)
#define IT_TARGET_CODE80	(14306)
//####2012/08/29#####
#define CALI_ADC_1  (0)
#define CALI_ADC_2  (1)

#define ADC_1_SIGN_BIT_CODE_V200  (1<<1)
#define ADC_1_SIGN_BIT_CODE_V100  (1<<0)
#define ADC_1_SIGN_BIT_CODE_IT    (1<<2)
#define ADC_1_CODE_V200_CP        (24576)
#define ADC_1_CODE_V200_POSITIVE  (0x0000)
#define ADC_1_CODE_V200_NEGATIVE  (0xFE00)
#define ADC_1_CODE_V100_CP        (12288)
#define ADC_1_CODE_V100_POSITIVE  (0x0000)
#define ADC_1_CODE_V100_NEGATIVE  (0xFF00)
#define ADC_1_CODE_IT_25_IDEAL    (IT_TARGET_CODE25)
#define ADC_1_CODE_IT_80_IDEAL    (IT_TARGET_CODE80)
#define ADC_1_CODE_IT_25_OFFSET   (0x0004)
#define ADC_1_CODE_IT_80_OFFSET   (0x0004)
#define ADC_1_CODE_IT_CALI_OFFSET (0x0004)
#define ADC_1_CODE_IT_POSITIVE    (0x0000)
#define ADC_1_CODE_IT_NEGATIVE    (0xFC00)
#define ADC_1_CODE_CURR_CONST     (10000)

#define ADC_2_SIGN_BIT_CODE_V200  (1<<7)
#define ADC_2_SIGN_BIT_CODE_V100  (1<<6)
#define ADC_2_CODE_V200_CP        (6144)
#define ADC_2_CODE_V200_POSITIVE  (0x0000)
#define ADC_2_CODE_V200_NEGATIVE  (0xFF80)
#define ADC_2_CODE_V100_CP        (3072)
#define ADC_2_CODE_V100_POSITIVE  (0x0000)
#define ADC_2_CODE_V100_NEGATIVE  (0xFFC0)
#define ADC_2_CALI_CONSTANT       (1000)

#ifdef  FEATURE_ADC_MANUAL_CP
#define ADC_1_CODE_V200_IDEAL     (ADC_1_CODE_V200_CP)
#define ADC_1_CODE_V100_IDEAL     (ADC_1_CODE_V100_CP)
#define ADC_2_CODE_V200_IDEAL     (ADC_2_CODE_V200_CP)
#define ADC_2_CODE_V100_IDEAL     (ADC_2_CODE_V100_CP)
#else   ///< else of FEATURE_ADC_MANUAL_CP
#define ADC_1_CODE_V200_IDEAL     (27361)        ///< [AT-PM] : 32768/250*ADC_1_VOLTAGE_V100/1000 ; 08/17/2012
#define ADC_1_CODE_V100_IDEAL     (13926)        ///< [AT-PM] : 32768/250*ADC_1_VOLTAGE_V100/1000 ; 08/17/2012
#define ADC_2_CODE_V200_IDEAL     (6515)        ///< [AT-PM] : 8192/5250*ADC_2_VOLTAGE_V100 ; 08/17/2012
#define ADC_2_CODE_V100_IDEAL     (3316)        ///< [AT-PM] : 8192/5250*ADC_2_VOLTAGE_V100 ; 08/17/2012
#endif  ///<  end of FEATURE_ADC_MANUAL_CP

#define ADC_1_VOLTAGE_V100        (106250)      ///< [AT-PM] : Unit in uV ; 08/08/2012
#define ADC_1_VOLTAGE_V200        (208750)      ///< [AT-PM] : Unit in uV ; 08/08/2012
#define ADC_2_VOLTAGE_V100        (2125)        ///< [At-PM] : Unit in mV ; 08/08/2012
#define ADC_2_VOLTAGE_V200        (4175)        ///< [AT-PM] : Unit in mV ; 08/08/2012

#define CONVERT_ADC2_TO_VBAT1     (1)

//constant
//define IC type
#define uG3100		0
#define uG3101		1
#define uG3102		2
#define uG3103_2	4
#define uG3103_3	5

//constant
//GPIO1/2 define
#define FUN_GPIO				0x01
#define FUN_ALARM				0x02
#define FUN_PWM					0x04
#define FUN_CBC_EN21			0x08
#define FUN_CBC_EN32			0x10

#define BIT_MACRO(x)		((u8)1 << (x))

					
//=================================================================================================//
//uG31xx register definitions 
//=================================================================================================//
#define REG_MODE				0x00
#define REG_CTRL1				0x01
#define REG_CTRL1_RSVD7           (1<<7)
#define REG_CTRL1_RSVD6           (1<<6)
#define REG_CTRL1_RSVD5           (1<<5)
#define REG_CTRL1_PORDET          (1<<4)
#define REG_CTRL1_VTM_EOC         (1<<3)
#define REG_CTRL1_GG_EOC          (1<<2)
#define REG_CTRL1_GG_RST          (1<<1)
#define REG_CTRL1_IO1DATA         (1<<0)
#define REG_CTRL1_IO1DATA_OPEN  (1<<0)

#define REG_RID_AVE_LOW			0x10
#define REG_RID_AVE_HIGH		0x11
#define REG_ALARM1_STATUS		0x12
#define REG_ALARM2_STATUS		0x13
#define REG_ALARM_EN			0x15
#define REG_CTRL2				0x16
#define REG_RAM_AREA		0x20

//user Area
//GROUP D register definitions
#define GROUP_D_BASE					0x40
#define REG_VBAT2_LOW					GROUP_D_BASE + 0		//0x40
#define REG_VBAT2_HIGH					GROUP_D_BASE + 1		//0x41
#define REG_VBAT3_LOW					GROUP_D_BASE + 2		//0x42
#define REG_VBAT3_HIGH					GROUP_D_BASE + 3		//0x43

#define REG_VBAT1_LOW					GROUP_D_BASE + 4		//0x44
#define REG_VBAT1_HIGH					GROUP_D_BASE + 5		//0x45

#define REG_VBAT2_AVE_LOW				GROUP_D_BASE + 6		//0x46
#define REG_VBAT2_AVE_HIGH				GROUP_D_BASE + 7		//0x47

#define REG_VBAT3_AVE_LOW				GROUP_D_BASE + 8		//0x48
#define REG_VBAT3_AVE_HIGH				GROUP_D_BASE + 9		//0x49

#define REG_V1_LOW						GROUP_D_BASE +10		//0x4a		
#define REG_V1_HIGH						GROUP_D_BASE +11		//0x4b		
#define REG_V2_LOW						GROUP_D_BASE +12		//0x4c		
#define REG_V2_HIGH						GROUP_D_BASE +13		//0x4d			
#define REG_V3_LOW						GROUP_D_BASE +14		//0x4e		
#define REG_V3_HIGH						GROUP_D_BASE +15		//0x4f			

#define REG_INTR_TEMPER_LOW				GROUP_D_BASE +16		//0x50
#define REG_INTR_TEMPER_HIGH			GROUP_D_BASE +17		//0x51

#define REG_EXTR_TEMPER_LOW				GROUP_D_BASE +18		//0x52
#define REG_EXTR_TEMPER_HIGH			GROUP_D_BASE +19		//0x53

#define REG_RID_LOW						GROUP_D_BASE +20		//0x54
#define REG_RID_HIGH					GROUP_D_BASE +21		//0x55
#define REG_CURRENT_LOW					GROUP_D_BASE +22		//0x56
#define REG_CURRENT_HIGH				GROUP_D_BASE +23		//0x57
#define REG_ADC1_OFFSET_LOW				GROUP_D_BASE +24		//0x58
#define REG_ADC1_OFFSET_HIGH			GROUP_D_BASE +25		//0x59
//Engineering Register(need password)
//Group E
#define GROUP_E_BASE					0x80
#define REG_LOAD_OTP1					GROUP_E_BASE + 0		//0x80
#define REG_LOAD_OTP2					GROUP_E_BASE + 1		//0x81
#define REG_LOAD_OTP3					GROUP_E_BASE + 2		//0x82
#define REG_LOAD_OTP4					GROUP_E_BASE + 3		//0x83
#define REG_LOAD_OTP5					GROUP_E_BASE + 4		//0x84

#define REG_TIMER						GROUP_E_BASE + 0x0f		//0x8f
#define REG_CLK_DIVA					GROUP_E_BASE + 0x10		//0x90
#define REG_CLK_DIVB					GROUP_E_BASE + 0x11		//0x91

#define REG_INTR_CTRL_D					GROUP_E_BASE + 0x12		//0x92 
#define REG_OSCTUNE_J1					GROUP_E_BASE + 0x13		//0x93
#define REG_OSCTUNE_J2					GROUP_E_BASE + 0x14		//0x94
#define REG_OSCTUNE_K1					GROUP_E_BASE + 0x15		//0x95
#define REG_OSCTUNE_K2					GROUP_E_BASE + 0x16		//0x96

#define REG_OSCTUNE_CNTA				GROUP_E_BASE + 0x17		//0x97
#define REG_OSCTUNE_CNTB				GROUP_E_BASE + 0x18		//0x98
#define REG_ADC1_TARGET_A				GROUP_E_BASE + 0x19		//0x99
#define REG_ADC1_TARGET_B				GROUP_E_BASE + 0x1a		//0x9a
#define REG_INTR_CTRL_A					GROUP_E_BASE + 0x1b		//0x9b
#define REG_INTR_CTRL_B					GROUP_E_BASE + 0x1c		//0x9c
		#define OSC_CNT_EN_BIT			0x80
		#define PWM_EN_BIT				0x40
		#define ET_EN_BIT				0x04
		#define IT_EN_BIT				0x02
		#define RID_EN_BIT				0x01

#define REG_INTR_CTRL_C				GROUP_E_BASE + 0x1d		//0x9d
#define REG_CELL_EN						GROUP_E_BASE + 0x1e		//0x9e


//Group E register(need password)
//define the struct of Internal compare Register
#define GROUP_F_BASE					0x9f
#define REG_INT_COMP_BASE				0x9f	
#define REG_COC_LOW						0x9f
#define REG_UC_LOW							0xa3
#define REG_OV1_LOW						0xa5
#define REG_OIT_LOW						0xb5
#define REG_OET_LOW						0xb9
#define REG_CBC_LOW						0xbd


//Register group G
#define GROUP_G_BASE					0xc1

#define REG_FW_CTRL						0xc1    //chop ctrl
#define REG_OTP_CTRL2					0xc2
#define REG_OTP_PPROG_ON				0xc3
#define REG_OTP_PPROG_OFF				0xc4

//Register group H
#define REG_ADC_CTRL_A					0xc5
#define REG_ADC_CTRL_B					0xc6
#define REG_ADC_CTRL_C					0xc7
#define REG_ADC_CTRL_D					0xc8
#define REG_ADC_V1						0xc9
#define REG_ADC_V2						0xca
#define REG_ADC_V3						0xcb

//Register Group I
#define REG_KCONFIG_D					0xcc	//D1/D2	
#define REG_KCONFIG_D1					0xcc
#define REG_KCONFIG_D2					0xcd

//Register Group J
#define REG_KCONFIG_A					0xce	//0xce ~ 0xd6
//A1~A9
#define REG_KCONFIG_H					0xd7	//0xd7 ~ 0xdb
//H1~H5
#define REG_KCONFIG_CAL					0xdc	//0xdc ~ 0xdf

#define REG_OTP1						0xe0	
#define REG_OTP1_BYTE1					0xe0
#define REG_OTP1_BYTE2					0xe1
#define REG_OTP1_BYTE3					0xe2
#define REG_OTP1_BYTE4					0xe3

#define REG_OTP2						0xf0
#define REG_OTP2_BYTE1					0xf0
#define REG_OTP2_BYTE2					0xf1
#define REG_OTP2_BYTE3					0xf2
#define REG_OTP2_BYTE4					0xf3

#define REG_OTP3						0xf4
#define REG_OTP3_BYTE1					0xf4
#define REG_OTP3_BYTE2					0xf5
#define REG_OTP3_BYTE3					0xf6
#define REG_OTP3_BYTE4					0xf7

#define REG_OTP4						0xf8
#define REG_OTP4_BYTE1					0xf8
#define REG_OTP4_BYTE2					0xf9
#define REG_OTP4_BYTE3					0xfa
#define REG_OTP4_BYTE4					0xfb

#define REG_OTP5						0xfc
#define REG_OTP5_BYTE1					0xfc
#define REG_OTP5_BYTE2					0xfd
#define REG_OTP5_BYTE3					0xfe
#define REG_OTP5_BYTE4					0xff

#define REG_OTP6						0x70


//<ASUS-WAD+>
#define SECURITY_KEY            0x5A
#define ONE_BYTE                0x1
#define TWO_BYTE                0x0
//<ASUS-WAD->

//Define  struct not release to customer

// real cell data(units: maH)
typedef struct _GG_REAL_DATA{
	s16     deltaQ[TEMPERATURE_NUMS][C_RATE_NUMS][OCV_NUMS];	//to keep the deltaQ update each SOV voltage Range
} __attribute__((packed)) GG_REAL_DATA;	
//2012/08/24/new add for system suspend 
//####2012/08/29#####
typedef struct _GG_SUSPEND_INFO{	
	u16     LMD;						//battery Qmax (maH)
	u16     NAC;						//battery NAC(maH)
	u16     RSOC;						//Battery Current RSOC(%)
	s32		currentTime;			//the time tick
}GG_SUSPEND_INFO,*PGG_SUSPEND_INFO;

typedef struct _GG_BATTERY_INFO{	
	u16     LMD;				//battery Qmax (maH)
	u16     NAC;				//battery NAC(maH)
	u16     RSOC;				//Battery Current RSOC(%)
//
 //   u16     LMD10;			//2012/07/15/Jacky 10*LMD
	//u16     NAC10;
	//u16     RSOC10;
//for updateQmax() use
	u16		startAdcCounter;		//for update Qmax use
	s16		startChargeRegister;	//to keep the charge Register
	s16		startChargeData;		//to keep the start charge maH
	u32		startTime;				//

//for calculate deltaQ use
	s16		previousChargeData;
	s16		previousChargeRegister;
	u16		previousAdcCounter;		//to keep the last convert counter	
	u32		previousTime;			//to keep the last time
//for calculate average current between 2 SOV
	

	u16     startChargeVoltage;		//to keep the voltage form  discharge to charge 
	u16     startCheckmV;				//keep the start check voltage(mV)
	u16     endCheckmV;					//keep the end check V % @ each C-rate(mV),start from 1
	u16     deltaQT[21];		//delta Q from V table					  ,start from 1	
	u8      deltaTimeEDV[21];	//to keep delta time from edv1(SOV 40% to edv (3000mV)
	u16     startChargeNAC;		//
	u16     previousVoltageData;//to keep the last voltage
	u16     SOV;				//to keep the SOV
	u16     rSocCounter;		//to keep the timer counter 

	u8      vIndex;		    	//voltage index
	u8      cIndex;		    	//current index
	u8      tempIndex;			//temperature index	
//charge Full use
	bool      bTpConditionFlag;	//1:
	u8		tpTimer;			//for keep the time for charge full from TP condition
	u8      BatteryFullCounter;	//to keep the battery full counter to avoid the 
//	for DisCharge empty 
	bool      startEdvTimerFlag;	//
//	u16		emptyTimer;			//for keep the time from the EDVF to EDV1
	u16     edvRSOC;			//

	bool      bChargeFlag;		//1:charge
	bool      bAlarmFlag;			//1:enable Alarm  06/17/2011/Jacky
	bool      bVoltageInverseFlag;	//voltage discharge to charge
	u8      delayCnt;			//to keep the dvdt delay counter
	
}  __attribute__((packed)) GG_BATTERY_INFO;
/*define the register of OTP   */
/// [AT] : Used for ADC1/2 delta code in OTP ; 07/19/2012
typedef struct _GG_OTP1_REG{
  u8  regAdc1Index25_200mV:4,     ///< 0xE0[3:0]
      regAdc1Index80_200mV:4;     ///< 0xE0[7:4]
  u8  regAdc1Index25_100mV:4,     ///< 0xE1[3:0]
      regAdc1Index80_100mV:4;     ///< 0xE1[7:4]
  u8  regAdc2Index25_200mV:4,     ///< 0xE2[3:0]
      regAdc2Index80_200mV:4;     ///< 0xE2[7:4]
  u8  regAdc2Index25_100mV:4,     ///< 0xE3[3:0]
      regAdc2Index80_100mV:4;     ///< 0xE3[7:4]
}GG_OTP1_REG, *PGG_OTP1_REG;

//define the OTP 0xf0 ~ 
//Adc1DeltaCode80_200mv -->10 bits
//Adc1DeltaCode25_200mv -->10 bits
//Adc1DeltaCode25_100mv --> 9 bits
//Adc1DeltaCode80_100mv --> 9 bits

//Adc2DeltaCode80_200mv --> 8 bits
//Adc2DeltaCode25_200mv --> 8 bits
//Adc2DeltaCode25_100mv --> 7 bits
//Adc2DeltaCode80_100mv --> 7 bits
// ITdeltaCode25-----------> 10 bits
// ITdeltaCode80-----------> 10 bits
//####2012/08/29#####
typedef struct _GG_OTP2_REG{
	u8	regITdeltaCode25_Bit1098:3,			//0xf0[2:0]
regProductType:2,               ///< [AT-PM] : 0xf0[4:3] -> Product_Type ; 08/08/2012
regRsvdF0:1,                    ///< [AT-PM] : 0xf0[5] -> Reserved ; 08/08/2012
regAdc2Index25_100mV_Bit4:1,    ///< [AT-PM] : 0xf0[6] -> Index_ADC2_100_25[4] ; 08/08/2012
regAdc2Index80_100mV_Bit4:1;    ///< [AT-PM] : 0xf0[7] -> Index_ADC2_100_80[4] ; 08/08/2012
	u8	regITdeltaCode25;								//0xf1

	u8	regITdeltaCode80_Bit1098:3,			//0xf2[2:0]		//ITdeltaCode80	
			regOtpCellEn:5;										//0xf2[7:3]	   //OTP_CELL_EN

	u8	regITdeltaCode80;								//0xf3

	u8	regAdc1DeltaCode25_200mv_Bit98:2,	//0xf4[1:0] 2 bits Adc1DeltaCode80_200mv[9:8]
			regDevAddr_Bit210:3,									//0xf4[4;2] 3 bits
			regDevAddr_Bit987:3;									//0xf4[7:5] 3 bits

	u8	regAdc1DeltaCode80_200mv_Bit98:2,	//0xf5[1:0] 2 bits Adc1DeltaCode80_200mv[9:8]
			regBgrTune:6;													//0xf5[7;2] 6 bits

	s8	regOscDeltaCode25;										//0xf6
	s8	regOscDeltaCode80;										//0xf7	
	u8	regAdc1DeltaCode25_200mv;					//0xf8
	u8	regAdc1DeltaCode80_200mv;					//0xf9
	u8	regAdc1DeltaCode25_100mv;					//0xfa
	u8	regAdc1DeltaCode80_100mv;					//0xfb

	u8	regAdc1DeltaCode25_100mv_Bit8:1,				//0xfc[0]
			regAdc2DeltaCode25_100mv:7;						//0xfc[7:1]

	u8	regAdc1DeltaCode80_100mv_Bit8:1,				//0xfd[0]	
			regAdc2DeltaCode80_100mv:7;						//0xfd[7:1]

	u8 regAdc2DeltaCode25_200mv;							//0xfe
	u8 regAdc2DeltaCode80_200mv;							//0xff

	
}  __attribute__((packed)) GG_OTP2_REG, *PGG_OTP2_REG;

//####2012/08/29#####
typedef struct _GG_OTP3_REG{
	u8  regAdc2Index80_200mV_Bit4:1,      ///< [AT-PM] : 0x70[0] -> Index_ADC2_200_80[4] ; 08/08/2012
regAdc1Index80_100mV_Bit4:1,      ///< [AT-PM] : 0x70[1] -> Index_ADC1_100_80[4] ; 08/08/2012
regAdc1Index80_200mV_Bit4:1,      ///< [AT-PM] : 0x70[2] -> Index_ADC1_200_80[4] ; 08/08/2012
regAveIT25_LowByte_Bit76543:5;    ///< [AT-PM] : 0x70[7:3] -> AVE_IT25[7:3] ; 08/08/2012
	u8  regAveIT25_HighByte;    ///< 0x71
	u8  regAdc2Index25_200mV_Bit4:1,      ///< [AT-PM] : 0x72[0] -> Index_ADC2_200_25[4] ; 08/08/2012
regAdc1Index25_100mV_Bit4:1,      ///< [AT-PM] : 0x72[1] -> Index_ADC1_100_25[4] ; 08/08/2012
regAdc1Index25_200mV_Bit4:1,      ///< [AT-PM] : 0x72[2] -> Index_ADC1_200_25[4] ; 08/08/2012
regAveIT80_LowByte_Bit76543:5;    ///< [AT-PM] : 0x72[7:3] -> AVE_IT80[7:3] ; 08/08/2012
	u8  regAveIT80_HighByte;    ///< 0x73
} __attribute__((packed)) GG_OTP3_REG, *PGG_OTP3_REG;
//####2012/08/29#####
typedef struct GGAdcDeltaCodeMappingST {
	s32 Adc1V100;
	s32 Adc1V200;
	s32 Adc2V100;
	s32 Adc2V200;
} GGAdcDeltaCodeMappingType;
// Function declare 
//u8 ReadGGBFileToCellDataAndInitSetting(const wchar_t *GGBFilename, CELL_TABLE* pCellTable, CELL_PARAMETER* pCellParameter);
//void InitialBatteryInformation(GG_BATTERY_INFO* pBatteryInfo);
//bool ActiveuG31xx(void);
//void InitialDeltaQ(GG_REAL_DATA* pRealData);
//void UpdateQmax(u8 targetCrate,const u16 offsetCharge,const u16 percentQ);
//u8 FindIndexRangeInTable(const u8 target, const u8* pTable, const u8 maxLength);
//u16 FindInitSOV(u8 tempIndex, u8 cIndex);
//u16 CalculateTargetSOV(const short* pTargetTable, const short targetVoltage);
//u8 FindSovCheckPoint(const u16 currentSOV);
//u16 Interpolation(const u16 fx1, const u16 fx0, const u8 x1, const u8 x, const u8 x0);
//u16 ExtraPolation(const u16 fx1, const u16 fx0, const u8 x0,const u8 x, const u8 x1);
//void BuildCheckVoltageTable(void);
//void BuildDeltaQTable(void);
//void BuildEmptyTimeTable(void);
//s16 CheckDeltaQmax(short deltaQC, const short deltaQmax, const int percentQ);
//void FindInitQ(const u8 targetTemp);
//void FindInitSOC(const u8 targetCurrent, const u8 targetTemp, const u16 targetVoltage);
//u8 CalculateTargetSOC(const short* pOcvTable, short targetVoltage);
//s16 CalculateCurrentFromUserReg(s16 current);
//u16 CalculateVoltageFromUserReg(const s16 voltage, const short current, const u16 offsetR, const u16 deltaR);
//s16 CalculateIT(const u16 iTcode);
//s16 CalculateET(const u16 eTCode);
//s16 CalculateChargeFromUserReg(const s16 charge);
//void CalculateDischarge(const int percent);
//void CalculateCharge(void);
//u8 CalculateAverageCurrent(void);

//void CalculateDeltaQ(short &deltaQ,const u16 offsetCharge);
//void CalculateBatteryIndex(u8 targetCrate);
//void DoSovDeltaQCheck(u8 cRateNow,const u16 offsetCharge,const u16 precentQ);
//s16 CalculateRsocDeltaQ(u32 totalTime);
//u16 CalculateV123FromUser2Reg(u16 vAdcCode);
//u16 CalculateVbat1FromUserReg(void);
//u16 CalculateVbat2FromUser2Reg(void);
//u16 CalculateVbat3FromUser2Reg(void);

//void upiGG_CountInitQmax(void);
//setup the Reg93/94/95/96
/// [AT-PM] : Column definition of OTP file  ; 08/08/2012
//####2012/08/29#####
enum COLUMN_OTP_FILE {
	OTP_FILE_INDEX = 0,
	OTP_FILE_ADC1_GAIN,
	OTP_FILE_ADC1_OFFSET,
	OTP_FILE_ADC2_GAIN,
	OTP_FILE_ADC2_OFFSET,
	OTP_FILE_BOARD_OFFSET,
	OTP_FILE_BOARD_RATIO,
	OTP_FILE_IT25,
	OTP_FILE_IT80,
	OTP_FILE_SETTING1,
	OTP_FILE_SETTING2,
	OTP_FILE_VBAT1_25_4175MV,
	OTP_FILE_VBAT1_25_2125MV,
	OTP_FILE_CURR_25_208MV,
	OTP_FILE_CURR_25_106MV,
	OTP_FILE_VBAT1_80_4175MV,
	OTP_FILE_VBAT1_80_2125MV,
	OTP_FILE_CURR_80_208MV,
	OTP_FILE_CURR_80_106MV,
	OTP_FILE_PRODUCT_TYPE,
};

/// [AT-PM] : Type definition for converting value from OTP file to register ; 08/08/2012
//####2012/08/29#####
enum TYPE_CONVERT_OTP_FILE {
	TYPE_OTP_FILE_IT_25 = 0,
	TYPE_OTP_FILE_IT_80,
	TYPE_OTP_FILE_ADC1_25_200MV,
	TYPE_OTP_FILE_ADC1_25_100MV,
	TYPE_OTP_FILE_ADC1_80_200MV,
	TYPE_OTP_FILE_ADC1_80_100MV,
	TYPE_OTP_FILE_ADC2_25_200MV,
	TYPE_OTP_FILE_ADC2_25_100MV,
	TYPE_OTP_FILE_ADC2_80_200MV,
	TYPE_OTP_FILE_ADC2_80_100MV,
};

/// [AT-PM] : Product Type definition in OTP ; 08/08/2012
//####2012/08/29#####
enum PRODUCT_TYPE {
	PRODUCT_TYPE_VBAT2_VBAT3 = 0,
	PRODUCT_TYPE_INDEX_14 = 1,
	PRODUCT_TYPE_INDEX_30 = 2,
};
#endif //_UG31XX_REG_DEF_H_
