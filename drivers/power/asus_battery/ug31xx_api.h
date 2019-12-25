/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#ifndef _UG31XX_API_H_
#define _UG31XX_API_H_

#include "ug31xx_typeDefine.h"
//#include "ug31xx.h"

/* data struct */
typedef enum _GGSTATUS{
        UG_SUCCESS                    = 0x00,
        UG_FAIL		                  = 0x01,
        UG_NOT_DEF                    = 0x02,
        UG_INIT_OCV_FAIL	          = 0x03,

        UG_READ_GGB_FAIL              = 0x04,
        UG_ACTIVE_FAIL                = 0x05,
        UG_INIT_SUCCESS               = 0x06,

        UG_INIT_I2C_FAIL              = 0x10,
        UG_I2C_READ_SUCCESS           = 0x11,
        UG_I2C_READ_FAIL              = 0x12,
        UG_I2C_WRITE_SUCCESS          = 0x13,
        UG_I2C_WRITE_FAIL             = 0x14,

        UG_READ_REG_SUCCESS           = 0x20,
        UG_READ_REG_FAIL              = 0x21,

        UG_READ_DEVICE_INFO_SUCCESS   = 0x22,
        UG_READ_DEVICE_INFO_FAIL      = 0x23,
        UG_READ_DEVICE_ALARM_SUCCESS  = 0x24,
        UG_READ_DEVICE_ALARM_FAIL     = 0x25,
        UG_READ_DEVICE_RID_SUCCESS	  = 0x26,
        UG_READ_DEVICE_RID_FAIL		  = 0x27,
        UG_READ_ADC_FAIL			  = 0x28
}GGSTATUS;

/* define the register of uG31xx */
typedef struct _GG_USER_REG{
        u8 	regMode;				//0x00
        u8 	regCtrl1;				//0x01
        s16	regCharge;				//0x02
        u16	regCounter;				//0x04
        s16	regCurrentAve;			//0x06 AVE current
        s16	regVbat1Ave;			//0x08 AVE vBat1
        u16	regITAve;				//0x0a AVE IT

        s16 regOffsetCurrentAve;	//0x0c AVE offset current
        u16 regETAve;				//0x0e
        u16 regRidAve;				//0x10
        u8  regAlarm1Status;		//0x12
        u8  regAlarm2Status;		//0x13
        u8  regIntrStatus;			//0x14
        u8  regAlarmEnable;			//0x15
        u8  regCtrl2;				//0x16
} __attribute__((packed)) GG_USER_REG, *PGG_USER_REG;

/* define the register of uG31xx */
typedef struct _GG_USER2_REG{
        s16 regVbat2;				//0x40,vBat2
        s16 regVbat3;				//0x42,vBat3
        s16 regVbat1;				//0x44,vBat1 Average
        s16 regVbat2Ave;			//0x46,vBat2 Average
        s16 regVbat3Ave;			//0x48,vBat3 Average
        u16 regV1;					//0x4a,cell 1 Voltage
        u16 regV2;					//0x4c,0xcell 2 Voltage
        u16 regV3;					//0x4e,cell 3 Voltage
        s16 regIT;					//0x50
        s16 regET;					//0x52
        u16 regRID;					//0x54
        s16	regCurrent;				//0x56
		s16 regAdc1Offset;			//0x58ADC1 offset
}__attribute__((packed)) GG_USER2_REG, *PGG_USER2_REG;
//
/* define device information */
typedef struct _GG_DEVICE_INFO{
        s16	   oldRegCurrent;				//for skip the ADC code Error
        s16	   oldRegVbat1;					//for skip the ADC code Error

        s16	   vBat1_AdcCode;				//debug use
        s16	   vBat1_AveAdcCode;

        //xxxx for readDeviceInfo each 10 secs
        s16	   fwCalAveCurrent_mA;			//f/w calculate average current
        u32	   lastTime;					//
        s16	   chargeRegister;				//coulomb counter register
        u16	   AdcCounter;					//ADC convert counter

        s16	   preChargeRegister;			//coulomb counter register
        s16        aveCurrentRegister;			//2012/0711/jacky
        u16	   preAdcCounter;
        s32	   fwChargeData_mAH;			//fw calculate maH (Q= I * T)

        s32        chargeData_mAh;				//maH calculate from charge register
        u16        voltage_mV;		            //total voltage
        s16        current_mA;			        // now current
        s16	   AveCurrent_mA;				// average current
        s16        IT;							// internal temperature
        s16        ET;							// external temperature

        u16	   v1_mV;						//v1   from hw register
        u16	   v2_mV;						//v2
        u16	   v3_mV;						//v3

        u16	   vBat1Average_mV;				//vbat1
        u16	   vBat2Average_mV;
        u16	   vBat3Average_mV;

        //	u16	   vBat1_mV;					//v Bat1 from hw register
        //	u16	   vBat2_mV;					//v Bat2
        //	u16    vBat3_mV;					//v Bat3

        u16        vCell1_mV;					//v Cell1
        u16	   vCell2_mV;					//v Cell2
        u16	   vCell3_mV;					//v Cell3

	s16 CaliAdc1Code;				//####2012/08/29#####
	s16 CaliAdc2Code;				//####2012/08/29#####

	s16 CoulombCounter;
} __attribute__((packed)) GG_DEVICE_INFO, PGG_DEVICE_INFO;

/* define battery capacity */
typedef struct _GG_CAPACITY {
        u16     LMD;						    //battery Qmax (maH)
        u16     NAC;						    //battery NAC(maH)
        u16     RSOC;					    //Battery Current RSOC(%)
} __attribute__((packed)) GG_CAPACITY, *PGG_CAPACITY;

GGSTATUS upiGG_Initial(GGBX_FILE_HEADER *pGGBBuf);
//extern GGSTATUS upiGG_ReadAllRegister(GG_USER_REG* pExtUserReg, GG_USER2_REG* pExtUserReg2);
//extern void upiGG_SetI2CSpeedAndBitMode(bool higtSpeed, bool tenBitMode);
GGSTATUS upiGG_ReadDeviceInfo(GG_DEVICE_INFO* pExtDeviceInfo);
void upiGG_ReadCapacity(GG_CAPACITY *pExtCapacity);
GGSTATUS upiGG_PreSuspend(void);
GGSTATUS upiGG_Wakeup(void);
//extern GGSTATUS upiGG_GetAlarmStatus(ALARM_STATUS*  alarmStatus);
//
//extern bool GetDeviceAllRegister(GG_USER_REG* pUserReg,GG_USER2_REG* pUser2Reg);
//extern unsigned long GetTickCount();
//extern void upiGG_CountInitQmax(void);
//extern bool ReadGGBFileToCellDataAndInitSetting(const char* GGBFilename, CELL_TABLE* pCellTable, CELL_PARAMETER* pCellParameter);
//extern void InitialBatteryInformation(GG_BATTERY_INFO* pBatteryInfo);
//extern u8 CheckIcType(void);
//extern void FixKconfigTable(void);
//extern void GetOtpSetting(void);
//extern bool IsOtpEmpty(void);
//extern void InitGpioSetting(void);
//extern u8 ConfigGpioFunction(u8 setting);
//extern void InitAlarmSetting(void);
//extern void SetupAdc2Quene(void);
//extern void SetupOscTuneJK(void);
//extern void SetupAdc1Queue(void);
//extern bool ActiveuG31xx(void);
//extern void InitialDeltaQ(GG_REAL_DATA* pRealData);
//extern short CalculateCurrentFromUserReg(s16 current);
//extern s16 CalibrationIT(s16 s16ITCode);
//extern void GetITtargetCode(s16* iTdeltaCode25, s16* iTdeltaCode80);
//extern s16 CalculateIT(const u16 IT_AdcCode);
//extern u16 CalibrationOSC(u8 CurrentIT_HiByte);
//extern s16 CalculateET(const u16 ET_AdcCode);
//extern short CalculateChargeFromUserReg(const s16 chargeAdcCode);
//extern u16 CalculateV123FromUser2Reg(u16 vAdcCode);
//extern u16 CalculateVbat1FromUserReg(void);
//extern u16 CalculateVbat2FromUser2Reg(void);
//extern u16 CalculateVbat2Voltage_mV(u8 Scale,u16 vBat2_6V_IdealAdcCode,u16 vBat2_8V_IdealAdcCode,s16 vBat2RealAdcCode);
//extern u16 CalculateVbat3FromUser2Reg(void);
//extern u16 CalculateVbat3Voltage_mV(u8 Scale,u16 vBat3_9V_IdealAdcCode,u16 vBat3_12V_IdealAdcCode,s16 vBat3RealAdcCode);
//extern u16 CalculateVoltageFromUserReg(const s16 voltageAdcCode, const short current, const u16 offsetR, const u16 deltaR);
//extern void FindInitQ(const u8 targetTemp);
//extern u8 FindIndexRangeInTable(const u8 target, const u8* pTable, const u8 maxNum);
//extern u16 Interpolation(const u16 fx1, const u16 fx0, const u8 x0, const u8 x, const u8 x1);
//extern void FindInitSOC(const u8 targetCurrent, const u8 targetTemp, const u16 targetVoltage);
//extern u8 CalculateTargetSOC(const short* pOcvTable, short targetVoltage);
//extern void CalculateDischarge(const unsigned int offsetCharge, const int percentQ);
//extern u8 CalculateAverageCurrent(void);
//extern void UpdateRSOC_T(u8 cRateNow);
//extern void CalculateDeltaQ(short* deltaQ,const u16 offsetCharge);
//extern void DoSovDeltaQCheck(u8 cRateNow,const u16 offsetCharge,const u16 precentQ);
//extern void CalculateBatteryIndex(u8 targetCrate);
//extern u16 FindInitSOV(u8 tempIndex, u8 cIndex);
//extern u16 CalculateTargetSOV(const short* pTargetTable, const short targetVoltage);
//extern u8 FindSovCheckPoint(const u16 currentSOV);
//extern void BuildCheckVoltageTable(void);
//extern void BuildDeltaQTable(void);
//extern void UpdateQmax(u8 targetCrate,const u16 offsetCharge,const u16 percentQ);
//extern short  CheckDeltaQmax(short deltaQC, const short deltaQmax, const int percentQ);
//extern void CalculateCharge(const unsigned int offsetCharge);
//extern void BuildEmptyTimeTable(void);
//extern u16 ExtraPolation(const u16 fx1, const u16 fx0, const u8 x0,const u8 x, const u8 x1);
//extern s16 CalDeltaqEach10secs(u32 totalTime);
//extern GGSTATUS upiGG_ReadAllRegister(GG_USER_REG* pUserReg,GG_USER2_REG* pUser2Reg);
//extern s16 CalculateRsocDeltaQ(u32 totalTime);
//extern s16 CalculateFwDeltaQ(u32 totalTime,s16* aveCurrent);
//extern void  HandleChargeFull(void);
typedef struct GG_ADC_CALI_INFO_ST
{
  CaliAdcDataType Current;
  CaliAdcDataType VCell1;
  CaliAdcDataType VCell2;
  CaliAdcDataType VCell3;
  CaliAdcDataType ExtTemp;

  s16 RawCodeCurrent;
  s16 RawCodeVCell1;
  s16 RawCodeVCell2;
  s16 RawCodeVCell3;
  u16 RawCodeExtTemp;

  u16 CurrITCode;

  u16 Adc1CodeT25V100;
  u16 Adc1CodeT25V200;
  u16 Adc1CodeT80V100;
  u16 Adc1CodeT80V200;

  u16 Adc2CodeT25V100;
  u16 Adc2CodeT25V200;
  u16 Adc2CodeT80V100;
  u16 Adc2CodeT80V200;

  u16 CodeT25IT;
  u16 CodeT80IT;

  u16 MeasCurr;
  u16 MeasExtTemp;
  u16 MeasVCell1;
  u16 MeasVCell2;
  u16 MeasVCell3;

  u16 Adc1INHigh;
  u16 Adc1INLow;
  u16 Adc2INHigh;
  u16 Adc2INLow;
} GG_ADC_CALI_INFO_TYPE;

#endif //_UG31XX_API_H_

