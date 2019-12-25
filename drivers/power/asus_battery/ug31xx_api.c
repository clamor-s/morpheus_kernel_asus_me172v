/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#include <linux/delay.h>
#include <linux/time.h>
#include <asm/div64.h>

#include "ug31xx_global.h"
#include "ug31xx_output.h"
#include "ug31xx_typeDefine.h"
#include "ug31xx_reg_def.h"
#include "ug31xx_api.h"
#include "ug31xx_i2c.h"
#include "ug31xx_adc_convert.h"


#define SleepMiniSecond(x) mdelay(x)

struct ug31_hw_config ug31_config; //<ASUS-Wade1_Chen20120910+>

const bool IS_HIGH_SPEED_MODE = false;
const bool IS_TEN_BIT_MODE = false;
u8  totalCellNums = 0;
u8  deltaQmaxInRsoc = 0;
bool bOTPisEmpty = false; 
bool bStartCheckUpdateQ = false;
bool bFirstData;

// Global variable
CELL_TABLE              cellTable;     // data from .GGB file
CELL_PARAMETER          cellParameter;  // data from .GGB file
GG_BATTERY_INFO         batteryInfo;
GG_DEVICE_INFO          deviceInfo;
GG_USER_REG             userReg;                  //user register 0x00 ~0x10
GG_USER2_REG            user2Reg;            //user register 0x40 ~0x4f

//GG_USER_REG        oldUserReg;		//to keep the last register data
//GG_USER2_REG	   oldUser2Reg;		//to keep the last register data for avoid ADC abnormal Data

GG_REAL_DATA            realData;
GG_OTP1_REG             otp1Reg;             //otp block 1 0xe0~0xe3
GG_OTP2_REG             otp2Reg;             //otp block 2 0xf0~0xff
GG_OTP3_REG             otp3Reg;             //otp block 3 0x70~0x73


// Global table
const u8  TEMPERATURE_TABLE[TEMPERATURE_NUMS] = {65,45,25,5};		//
const u8  CRATE_TABLE[C_RATE_NUMS] = {10,7,5,2,1};									//0.2c/0.5c/0.7c/1.0Cn
const u8  SOC_TABLE[OCV_NUMS] = {100,95,90,85,80,75,70,65,60,55,50,45,40,35,30,25,20,15,10,5,0};
const u8  OCV_CRATE_TABLE[2] = {2,1};

//####2012/08/29#####
GGAdcDeltaCodeMappingType const ADC_DELTA_CODE_MAPPING[] =
{
#ifdef  FEATURE_ADC_CALI_FT
	/// ADC1 V100,    ADC1 V200,    ADC2 V100,    ADC2 V200,
	{   -12800,     -30720,     0,          0,    },      ///< Index = 0
	{   -12544,     -30208,     64,         128,  },      ///< Index = 1
	{   -13056,     -31232,     -64,        -128, },      ///< Index = 2
	{   -12288,     -29696,     128,        256,  },      ///< Index = 3
	{   -13312,     -31744,     -128,       -256, },      ///< Index = 4
	{   -12032,     -29184,     192,        384,  },      ///< Index = 5
	{   -13568,     -32256,     -192,       -384, },      ///< Index = 6
	{   -11776,     -28672,     256,        512,  },      ///< Index = 7
	{   -13824,     -32768,     -256,       -512, },      ///< Index = 8
	{   -11520,     -28160,     320,        640,  },      ///< Index = 9
	{   -14080,     -33280,     -320,       -640, },      ///< Index = 10
	{   -11264,     -27648,     384,        768,  },      ///< Index = 11
	{   -14336,     -33792,     -384,       -768, },      ///< Index = 12
	{   -11008,     -27136,     448,        896,  },      ///< Index = 13
	{   -14592,     -34304,     -448,       -896, },      ///< Index = 14
	{   -10752,     -26624,     512,        1024, },      ///< Index = 15
	{   -14848,     -34816,     -512,       -1024,},      ///< Index = 16
	{   -10496,     -26112,     576,        1152, },      ///< Index = 17
	{   -15104,     -35328,     -576,       -1152,},      ///< Index = 18
	{   -10240,     -25600,     640,        1280, },      ///< Index = 19
	{   -15360,     -35840,     -640,       -1280,},      ///< Index = 20
	{   -9984,      -25088,     704,        1408, },      ///< Index = 21
	{   -15616,     -36352,     -704,       -1408,},      ///< Index = 22
	{   -9728,      -24576,     768,        1536, },      ///< Index = 23
	{   -15872,     -36864,     -768,       -1536,},      ///< Index = 24
	{   -9472,      -24064,     832,        1664, },      ///< Index = 25
	{   -16128,     -37376,     -832,       -1664,},      ///< Index = 26
	{   -9216,      -23552,     896,        1792, },      ///< Index = 27
	{   -16384,     -37888,     -896,       -1792,},      ///< Index = 28
	{   -8960,      -23040,     960,        1920, },      ///< Index = 29
	{   -16640,     -38400,     -960,       -1920,},      ///< Index = 30
	{   0,          0,          0,          0,    },      ///< Index = 31
#else   ///< else of FEATURE_ADC_CALI_FT
	/// ADC1 V100,    ADC1 V200,    ADC2 V100,    ADC2 V200,
	{   0,          0,          0,          0,    },      ///< Index = 0
	{   256,        512,        64,         128,  },      ///< Index = 1
	{   -256,       -512,       -64,        -128, },      ///< Index = 2
	{   512,        1024,       128,        256,  },      ///< Index = 3
	{   -512,       -1024,      -128,       -256, },      ///< Index = 4
	{   768,        1536,       192,        384,  },      ///< Index = 5
	{   -768,       -1536,      -192,       -384, },      ///< Index = 6
	{   1024,       2048,       256,        512,  },      ///< Index = 7
	{   -1024,      -2048,      -256,       -512, },      ///< Index = 8
	{   1280,       2560,       320,        640,  },      ///< Index = 9
	{   -1280,      -2560,      -320,       -640, },      ///< Index = 10
	{   1536,       3072,       384,        768,  },      ///< Index = 11
	{   -1536,      -3072,      -384,       -768, },      ///< Index = 12
	{   1792,       3584,       448,        896,  },      ///< Index = 13
	{   -1792,      -3584,      -448,       -896, },      ///< Index = 14
	{   2048,       4096,       512,        1024, },      ///< Index = 15
	{   -2048,      -4096,      -512,       -1024,},      ///< Index = 16
	{   2304,       4608,       576,        1152, },      ///< Index = 17
	{   -2304,      -4608,      -576,       -1152,},      ///< Index = 18
	{   2560,       5120,       640,        1280, },      ///< Index = 19
	{   -2560,      -5120,      -640,       -1280,},      ///< Index = 20
	{   2816,       5632,       704,        1408, },      ///< Index = 21
	{   -2816,      -5632,      -704,       -1408,},      ///< Index = 22
	{   3072,       6144,       768,        1536, },      ///< Index = 23
	{   -3072,      -6144,      -768,       -1536,},      ///< Index = 24
	{   3328,       6656,       832,        1664, },      ///< Index = 25
	{   -3328,      -6656,      -832,       -1664,},      ///< Index = 26
	{   3584,       7168,       896,        1792, },      ///< Index = 27
	{   -3584,      -7168,      -896,       -1792,},      ///< Index = 28
	{   3840,       7680,       960,        1920, },      ///< Index = 29
	{   -3840,      -7680,      -960,       -1920,},      ///< Index = 30
	{   0,          0,          0,          0,    },      ///< Index = 31
#endif  ///< end of FEATURE_ADC_CALI_FT
};
u16 const ADC_IDEAL_CODE_CONVERT_OTP[] = {
	ADC_1_CODE_IT_25_IDEAL,
	ADC_1_CODE_IT_80_IDEAL,
	ADC_1_CODE_V200_IDEAL,
	ADC_1_CODE_V100_IDEAL,
	ADC_1_CODE_V200_IDEAL,
	ADC_1_CODE_V100_IDEAL,
	ADC_2_CODE_V200_IDEAL,
	ADC_2_CODE_V100_IDEAL,
	ADC_2_CODE_V200_IDEAL,
	ADC_2_CODE_V100_IDEAL,
};

u32 GetTickCount(void) 
{
	struct timeval current_tick;

	do_gettimeofday(&current_tick);

	return current_tick.tv_sec * 1000 + current_tick.tv_usec/1000;
}
//IT:11
//C: 00
//RID:10
//ET:01
//####2012/08/29#####
void SetupAdc1Queue(void)
{
        s16 AdcOffset;
#ifdef  FEATURE_ADC_CALIBRATION
	u8 ADC_QUEUE[4] = {0x05, 0x00, 0x0F, 0x00};     ///< [AT-PM] : C,IT,C,C,C,ET,C,C ; 08/08/2012
#else   ///< else of FEATURE_ADC_CALIBRATION
	u8 ADC_QUEUE[4]={0x00,0x00,0x00,0x00};		//all C
#endif  ///< end of FEATURE_ADC_CALIBRATION
        AdcOffset = 0;

        API_I2C_Write(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, 
                        REG_ADC_CTRL_A, 4, &ADC_QUEUE[0]);                 //write Reg9C
        API_I2C_Write(NORMAL, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, 
                        REG_ADC1_OFFSET_LOW, 2, (u8 *)&AdcOffset);	//write Reg9C
}

//check IC type
//return the cell number 1/2/3
u8 CheckIcType(void)
{
        u8 cellNum = 0;

        const u8  IC_CELL_NUM[6]={1,1,2,0,2,3};
        cellNum = IC_CELL_NUM[cellParameter.ICType];

        return cellNum;

}

//check the OTP1 & OTP2
//if OTP1(0xe0~0xe3) OTP2(0xf0~0xff) all 0(Empty) return 0
//else return 1
bool IsOtpEmpty(void)
{
        u8* u8Ptr;
        u8 i=0;

        u8Ptr =(u8* )&otp1Reg;  //check OTP1 content
        for(i=0;i<sizeof(GG_OTP1_REG);i++)
        {
                if(u8Ptr[i] !=0)  return false;
        }

        u8Ptr =(u8* )&otp2Reg;  //check OTP2 content
        for(i=0;i<sizeof(GG_OTP2_REG);i++)
        {
                if(u8Ptr[i] !=0)  return false;
        }
        return true;

}

//function to get otp1/otp2 register
void GetOtpSetting(void)
{
        memset(&otp1Reg,0x00,sizeof(GG_OTP1_REG));
        memset(&otp2Reg,0x00,sizeof(GG_OTP2_REG));
        memset(&otp3Reg,0x00,sizeof(GG_OTP3_REG));
        API_I2C_Read(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,REG_OTP1, 4, (u8* )&otp1Reg);      //4 bytes, 0xe0~0xe3
  #ifdef  FEATURE_ADC_CALIBRATION
        API_I2C_Read(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,REG_OTP2, 16, (u8* )&otp2Reg);   //16 bytes, 0xf0~0xff
  #endif  ///< end of FEATURE_ADC_CALIBRATION
        API_I2C_Read(NORMAL, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,REG_OTP6, 4, (u8* )&otp3Reg);     //4 bytes, 0x70~0x73
        bOTPisEmpty = IsOtpEmpty();
        //debug XXXX
        bOTPisEmpty = true;                             //2012/09/28
        //debug XXX

}

/**
 * @brief GetMappingIndex
 *
 *  Convert delta code mapping index from OTP file
 *
 * @para  _in_deltaCode delta code
 * @para  _in_type  TYPE_CONVERT_OTP_FILE
 * @return  mapping index
 */
//####2012/08/29#####
u8 ConvertMappingIndex(s16 _in_deltaCode, u8 _in_type)
{
        u8 Idx;

        Idx = 0;
        while(1)
        {    
                if((_in_type == TYPE_OTP_FILE_ADC1_25_200MV) || (_in_type == TYPE_OTP_FILE_ADC1_80_200MV))
                {
                        if(Idx == 0)
                        {
                                if((_in_deltaCode < ADC_DELTA_CODE_MAPPING[Idx + 1].Adc1V200) &&
                                                (_in_deltaCode > ADC_DELTA_CODE_MAPPING[Idx + 2].Adc1V200))
                                {
                                        break;
                                }
                        }
                        else
                        {
                                if((_in_deltaCode > 0) &&
                                                (ADC_DELTA_CODE_MAPPING[Idx + 1].Adc1V200 > 0) &&
                                                (_in_deltaCode < ADC_DELTA_CODE_MAPPING[Idx + 1].Adc1V200))
                                {
                                        break;
                                }
                                if((_in_deltaCode < 0) &&
                                                (ADC_DELTA_CODE_MAPPING[Idx + 2].Adc1V200 < 0) &&
                                                (_in_deltaCode > ADC_DELTA_CODE_MAPPING[Idx + 2].Adc1V200))
                                {
                                        break;
                                }
                        }
                }
                else if((_in_type == TYPE_OTP_FILE_ADC1_25_100MV) || (_in_type == TYPE_OTP_FILE_ADC1_80_100MV))
                {
                        if(Idx == 0)
                        {
                                if((_in_deltaCode < ADC_DELTA_CODE_MAPPING[Idx + 1].Adc1V100) &&
                                                (_in_deltaCode > ADC_DELTA_CODE_MAPPING[Idx + 2].Adc1V100))
                                {
                                        break;
                                }
                        }
                        else
                        {
                                if((_in_deltaCode > 0) &&
                                                (ADC_DELTA_CODE_MAPPING[Idx + 1].Adc1V100 > 0) &&
                                                (_in_deltaCode < ADC_DELTA_CODE_MAPPING[Idx + 1].Adc1V100))
                                {
                                        break;
                                }
                                if((_in_deltaCode < 0) &&
                                                (ADC_DELTA_CODE_MAPPING[Idx + 2].Adc1V100 < 0) &&
                                                (_in_deltaCode > ADC_DELTA_CODE_MAPPING[Idx + 2].Adc1V100))
                                {
                                        break;
                                }
                        }
                }
                else if((_in_type == TYPE_OTP_FILE_ADC2_25_200MV) || (_in_type == TYPE_OTP_FILE_ADC2_80_200MV))
                {
                        if(Idx == 0)
                        {
                                if((_in_deltaCode < ADC_DELTA_CODE_MAPPING[Idx + 1].Adc2V200) &&
                                                (_in_deltaCode > ADC_DELTA_CODE_MAPPING[Idx + 2].Adc2V200))
                                {
                                        break;
                                }
                        }
                        else
                        {
                                if((_in_deltaCode > 0) &&
                                                (ADC_DELTA_CODE_MAPPING[Idx + 1].Adc2V200 > 0) &&
                                                (_in_deltaCode < ADC_DELTA_CODE_MAPPING[Idx + 1].Adc2V200))
                                {
                                        break;
                                }
                                if((_in_deltaCode < 0) &&
                                                (ADC_DELTA_CODE_MAPPING[Idx + 2].Adc2V200 < 0) &&
                                                (_in_deltaCode > ADC_DELTA_CODE_MAPPING[Idx + 2].Adc2V200))
                                {
                                        break;
                                }
                        }
                }
                else if((_in_type == TYPE_OTP_FILE_ADC2_25_100MV) || (_in_type == TYPE_OTP_FILE_ADC2_80_100MV))
                {
                        if(Idx == 0)
                        {
                                if((_in_deltaCode < ADC_DELTA_CODE_MAPPING[Idx + 1].Adc2V100) &&
                                                (_in_deltaCode > ADC_DELTA_CODE_MAPPING[Idx + 2].Adc2V100))
                                {
                                        break;
                                }
                        }
                        else
                        {
                                if((_in_deltaCode > 0) &&
                                                (ADC_DELTA_CODE_MAPPING[Idx + 1].Adc2V100 > 0) &&
                                                (_in_deltaCode < ADC_DELTA_CODE_MAPPING[Idx + 1].Adc2V100))
                                {
                                        break;
                                }
                                if((_in_deltaCode < 0) &&
                                                (ADC_DELTA_CODE_MAPPING[Idx + 2].Adc2V100 < 0) &&
                                                (_in_deltaCode > ADC_DELTA_CODE_MAPPING[Idx + 2].Adc2V100))
                                {
                                        break;
                                }
                        }
                }
                else
                {
                        break;
                }

                Idx += 1;
                if((ADC_DELTA_CODE_MAPPING[Idx].Adc1V100 == 0) || 
                                (ADC_DELTA_CODE_MAPPING[Idx].Adc1V200 == 0) || 
                                (ADC_DELTA_CODE_MAPPING[Idx].Adc2V100 == 0) || 
                                (ADC_DELTA_CODE_MAPPING[Idx].Adc2V200 == 0))
                {
                        break;
                }

                if((otp2Reg.regProductType == PRODUCT_TYPE_INDEX_14) && (Idx > 14))
                {
                        break;
                }
        }

        return (Idx);
}

//to get ITcode25/ITcode80
void GetITtargetCode(s16* iTdeltaCode25, s16* iTdeltaCode80)
{

	//Step 1.read ITdeltacode25/80
	if(otp2Reg.regITdeltaCode25_Bit1098 & BIT_MACRO(2))	//check sign Bit
	{
		*iTdeltaCode25 =((otp2Reg.regITdeltaCode25_Bit1098&0x03)<<8)+(s16)otp2Reg.regITdeltaCode25;
		*iTdeltaCode25 = (*iTdeltaCode25 ^0x3ff)+1;
		*iTdeltaCode25 = (0 - *iTdeltaCode25);
	} else{
		*iTdeltaCode25 = (otp2Reg.regITdeltaCode25_Bit1098<<8)+ (s16) otp2Reg.regITdeltaCode25;
	}

	if(otp2Reg.regITdeltaCode80_Bit1098 & BIT_MACRO(2) )	//check sign Bit
	{
		*iTdeltaCode80 = ((otp2Reg.regITdeltaCode80_Bit1098&0x03)<<8) +(s16) otp2Reg.regITdeltaCode80 ;
		*iTdeltaCode80 = (*iTdeltaCode80 ^0x3ff)+1;
		*iTdeltaCode80 = (0 - *iTdeltaCode80);
	} else{
		*iTdeltaCode80 =(otp2Reg.regITdeltaCode80_Bit1098<<8) +  (s16)otp2Reg.regITdeltaCode80;
	}

	//	*iTdeltaCode25 = 411;
	//	*iTdeltaCode80 = 492;


}
//set OV/UV/OC/UC/OIT/UIT/OET/UET Alarm initial setting of Reg9F ~ RegBC
//set alarm enable Reg15
//convert  alarm parameter to real ADC Code
int Convert2AdcCode(int source,int gain, int offset)
{	
	//return(((source - offset)*1000)/gain);
	return((source * gain )/1000 + offset);
}
/*
int tempC;
t1 = (IT_AdcCode>>1) - 11172;
tempC = (t1*100)/392;				//2012/02/21
return((s16)tempC);
s32 it1,it2;
//
it1=  ( ((s32)IT_TARGET_CODE80<<1) - (IT_TARGET_CODE25<<1)) /(iTcode80 - iTcode25);
it3 = (s16ITCode - iTcode25)*it1;
it2 = it3 + (IT_TARGET_CODE25<<1);


*/

int  ConvertIT2AdcCode(int source)
{
        int it2,it1,it3;
        int realItCode;

        it2 = (392*source)/10 + 11172;
        it2 =it2*2;

        it3 = it2 -(IT_TARGET_CODE25<<1);
        it1=  ( ((s32)IT_TARGET_CODE80<<1) - (IT_TARGET_CODE25<<1)) /(cellParameter.adc_d2 - cellParameter.adc_d1);
        realItCode = it3/it1 +  cellParameter.adc_d1;

        return(realItCode);	
}

//convert ET to ADC code
int  ConvertET2AdcCode(int source)
{
        const unsigned char  ET_TABLE[19]={-10,-5,0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80};
        int i;

        for(i=0; i < 19; i++)
        {
                if(source == ET_TABLE[i])   //found the temperature
                {
                        break;
                } 
        }
        return  (cellParameter.rtTable[i]);
}

void InitAlarmSetting(void)
{
	s16* ptr;
	u8	u8Temp,i;

	ptr = (s16*)&cellParameter.COC;
	for(i=0;i<2;i++)
	{
		u16 AdcCode =Convert2AdcCode(*(ptr+i),cellParameter.adc1_ngain,cellParameter.adc1_pos_offset);
		API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                                REG_COC_LOW+i*2, (u8)AdcCode);						//write Reg9f
		API_I2C_SingleWrite(SECURITY,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                                REG_COC_LOW+i*2+1, (u8)(AdcCode >>8));			//write Reg9f
	}
	ptr = (s16*)&cellParameter.UC;
	{
		//s16 temp = *ptr;
		s16 AdcCode =Convert2AdcCode(*ptr,cellParameter.adc1_ngain,cellParameter.adc1_pos_offset);
		AdcCode += cellParameter.adc1Offset;
		API_I2C_SingleWrite(SECURITY,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                                REG_UC_LOW, (u8)AdcCode);						//write Reg9f
		API_I2C_SingleWrite(SECURITY,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                                REG_UC_LOW+1, (u8)(AdcCode >>8));			//write Reg9f
	}

	ptr = (s16*)&cellParameter.OV1;
	for(i=0;i<8;i++) //from OV1 to UVP
	{
		u16 AdcCode =Convert2AdcCode(*(ptr+i),cellParameter.adc2_gain,cellParameter.adc2_offset);
		API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                        REG_OV1_LOW+i*2, (u8)AdcCode);			//write Reg9f
		API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                        REG_OV1_LOW+i*2+1, (u8)(AdcCode >>8));			//write Reg9f
	}
	ptr = (s16*)&cellParameter.OIT;
	for(i=0;i<2;i++)
	{
		u16 AdcCode =ConvertIT2AdcCode(*(ptr+i));
		API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                                REG_OIT_LOW+i*2, (u8)AdcCode);			//write Reg9f
		API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, 
                                REG_OIT_LOW+i*2+1, (u8)(AdcCode >>8));			//write Reg9f
	}
	ptr = (s16*)&cellParameter.OET;
	for(i=0;i<2;i++)
	{
		u16 AdcCode =ConvertET2AdcCode(*(ptr+i));
		API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                                REG_OET_LOW+i*2, (u8)AdcCode);			//write Reg9f
		API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                                REG_OET_LOW+i*2+1, (u8)(AdcCode >>8));			//write Reg9f
	}

	API_I2C_SingleWrite(SECURITY,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                        REG_CBC_LOW, (u8)cellParameter.CBC21);			//write Reg9f
	API_I2C_SingleWrite(SECURITY,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                        REG_CBC_LOW+1, (u8)(cellParameter.CBC21 >>8));			//write Reg9f
	API_I2C_SingleWrite(SECURITY,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                        REG_CBC_LOW+2, (u8)cellParameter.CBC32);			//write Reg9f
	API_I2C_SingleWrite(SECURITY,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                        REG_CBC_LOW+3, (u8)(cellParameter.CBC32 >>8));			//write Reg9f
	
	// get the cell parameter
	API_I2C_SingleWrite(NORMAL,  IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                        REG_ALARM_EN,cellParameter.alarmEnable);	//write reg0x15
    
	API_I2C_SingleRead(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,
                        REG_INTR_CTRL_B,&u8Temp);		//write Reg9c
	u8Temp &= 0xf9;
	if(cellParameter.alarmEnable & 0x20)	 /*[7]:COC [6]:DOC [5]:IT [4]:ET [3]:VP  [2]:V3  [1]:V2 [0]:V1 */
		u8Temp |= IT_EN_BIT;
	if(cellParameter.alarmEnable & 0x10)	 /*[7]:COC [6]:DOC [5]:IT [4]:ET [3]:VP  [2]:V3  [1]:V2 [0]:V1 */
		u8Temp |= ET_EN_BIT;
	API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE,IS_TEN_BIT_MODE,
                        REG_INTR_CTRL_B,u8Temp);		//write Reg9C
}

/**
 * @brief GetAdc1T25V200
 *
 *  Get ADC1 code at 25oC/200mV in CP
 *  10 bits
 *
 * @para  pCaliAdcData  pointer of structure CaliAdcDataType
 * @return  NULL
 */
//####2012/08/29#####
void GetAdc1T25V200(CaliAdcDataType *pCaliAdcData)
{
        u8 Idx;

        pCaliAdcData->CodeT25V200 = ADC_1_CODE_V200_CP;
        pCaliAdcData->CodeT25V200 += (((otp2Reg.regAdc1DeltaCode25_200mv_Bit98 & ADC_1_SIGN_BIT_CODE_V200) ?
                                ADC_1_CODE_V200_NEGATIVE : ADC_1_CODE_V200_POSITIVE) + 
                        ((otp2Reg.regAdc1DeltaCode25_200mv_Bit98 & (~ADC_1_SIGN_BIT_CODE_V200)) << 8) +
                        otp2Reg.regAdc1DeltaCode25_200mv);

        if((otp2Reg.regProductType == PRODUCT_TYPE_INDEX_14) || (otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30))
        {
                Idx = otp1Reg.regAdc1Index25_200mV;
                if(otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30)
                {
                        Idx = Idx + (otp3Reg.regAdc1Index25_200mV_Bit4 << 4);
                }
                pCaliAdcData->CodeT25V200 += ADC_DELTA_CODE_MAPPING[Idx].Adc1V200;
        }
}

/**
 * @brief GetAdc1T80V200
 *
 *  Get ADC1 code at 80oC/200mV in CP
 *  10 bits
 *
 * @para  pCaliAdcData  pointer of structure CaliAdcDataType
 * @return  NULL
 *///####2012/08/29#####
void GetAdc1T80V200(CaliAdcDataType *pCaliAdcData)
{
  #ifdef  FEATURE_ADC_CALI_FT
  
    s32 Code;

    Code = (s32)pCaliAdcData->CodeT80IT;
    Code = Code - pCaliAdcData->CodeT25IT;
    Code = Code*ADC_GAIN_CONST;
    Code = Code/ADC1_200_GAIN;
    Code = Code + pCaliAdcData->CodeT25V200;
    pCaliAdcData->CodeT80V200 = (u16)Code;
    
  #else   ///< else of FEATURE_ADC_CALI_FT
  
    u8 Idx;
    
    pCaliAdcData->CodeT80V200 = ADC_1_CODE_V200_CP;
    pCaliAdcData->CodeT80V200 += (((otp2Reg.regAdc1DeltaCode80_200mv_Bit98 & ADC_1_SIGN_BIT_CODE_V200) ?
                                   ADC_1_CODE_V200_NEGATIVE : ADC_1_CODE_V200_POSITIVE) + 
                                  ((otp2Reg.regAdc1DeltaCode80_200mv_Bit98 & (~ADC_1_SIGN_BIT_CODE_V200)) << 8) +
                                  otp2Reg.regAdc1DeltaCode80_200mv);

    if((otp2Reg.regProductType == PRODUCT_TYPE_INDEX_14) || (otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30))
    {
      Idx = otp1Reg.regAdc1Index80_200mV;
      if(otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30)
      {
        Idx = Idx + (otp3Reg.regAdc1Index80_200mV_Bit4 << 4);
      }
      pCaliAdcData->CodeT80V200 += ADC_DELTA_CODE_MAPPING[Idx].Adc1V200;
    }
    
  #endif  ///< end of FEATURE_ADC_CALI_FT
}

/**
 * @brief GetAdc1T25V100
 *
 *  Get ADC1 code at 25oC/100mV in CP
 *  9 bits
 *
 * @para  pCaliAdcData  pointer of structure CaliAdcDataType
 * @return  NULL
 *///####2012/08/29#####
void GetAdc1T25V100(CaliAdcDataType *pCaliAdcData)
{
        u8 Idx;

        pCaliAdcData->CodeT25V100 = ADC_1_CODE_V100_CP;
        pCaliAdcData->CodeT25V100 += (((otp2Reg.regAdc1DeltaCode25_100mv_Bit8 & ADC_1_SIGN_BIT_CODE_V100) ? 
                                ADC_1_CODE_V100_NEGATIVE : ADC_1_CODE_V100_POSITIVE) + 
                        otp2Reg.regAdc1DeltaCode25_100mv);

        if((otp2Reg.regProductType == PRODUCT_TYPE_INDEX_14) || (otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30))
        {
                Idx = otp1Reg.regAdc1Index25_100mV;
                if(otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30)
                {
                        Idx = Idx + (otp3Reg.regAdc1Index25_100mV_Bit4 << 4);
                }
                pCaliAdcData->CodeT25V100 += ADC_DELTA_CODE_MAPPING[Idx].Adc1V100;
        }
}
/**
 * @brief GetAdc1T80V100
 *
 *  Get ADC1 code at 80oC/100mV in CP
 *  10 bits
 *
 * @para  pCaliAdcData  pointer of structure CaliAdcDataType
 * @return  NULL
 *///####2012/08/29#####
void GetAdc1T80V100(CaliAdcDataType *pCaliAdcData)
{
#ifdef  FEATURE_ADC_CALI_FT

        s32 Code;

        Code = (s32)pCaliAdcData->CodeT80IT;
        Code = Code - pCaliAdcData->CodeT25IT;
        Code = Code*ADC_GAIN_CONST;
        Code = Code/ADC1_100_GAIN;
        Code = Code + pCaliAdcData->CodeT25V100;
        pCaliAdcData->CodeT80V100 = (u16)Code;

#else   ///< else of FEATURE_ADC_CALI_FT

        u8 Idx;

        pCaliAdcData->CodeT80V100 = ADC_1_CODE_V100_CP;
        pCaliAdcData->CodeT80V100 += (((otp2Reg.regAdc1DeltaCode80_100mv_Bit8 & ADC_1_SIGN_BIT_CODE_V100) ? 
                                ADC_1_CODE_V100_NEGATIVE : ADC_1_CODE_V100_POSITIVE) + 
                        otp2Reg.regAdc1DeltaCode80_100mv);

        if((otp2Reg.regProductType == PRODUCT_TYPE_INDEX_14) || (otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30))
        {
                Idx = otp1Reg.regAdc1Index80_100mV;
                if(otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30)
                {
                        Idx = Idx + (otp3Reg.regAdc1Index80_100mV_Bit4 << 4);
                }
                pCaliAdcData->CodeT80V100 += ADC_DELTA_CODE_MAPPING[Idx].Adc1V100;
        }

#endif  ///< end of FEATURE_ADC_CALI_FT
}
/**
 * @brief GetAdc2T80V200
 *
 *  Get ADC2 code at 80oC/200mV in CP
 *  8 bits
 *
 * @para  pCaliAdcData  pointer of structure CaliAdcDataType
 * @return  NULL
 *///####2012/08/29#####
void GetAdc2T80V200(CaliAdcDataType *pCaliAdcData)
{
#ifdef  FEATURE_ADC_CALI_FT

        s32 Code;

        Code = (s32)pCaliAdcData->CodeT80IT;
        Code = Code - pCaliAdcData->CodeT25IT;
        Code = Code*ADC_GAIN_CONST;
        Code = Code/ADC2_200_GAIN;
        Code = Code + pCaliAdcData->CodeT25V200;
        pCaliAdcData->CodeT80V200 = (u16)Code;

#else   ///< else of FEATURE_ADC_CALI_FT

        u8 Idx;

        pCaliAdcData->CodeT80V200 = ADC_2_CODE_V200_CP;
        pCaliAdcData->CodeT80V200 += (((otp2Reg.regAdc2DeltaCode80_200mv & ADC_2_SIGN_BIT_CODE_V200) ? 
                                ADC_2_CODE_V200_NEGATIVE : ADC_2_CODE_V200_POSITIVE) + 
                        (otp2Reg.regAdc2DeltaCode80_200mv & (~ADC_2_SIGN_BIT_CODE_V200)));

        if((otp2Reg.regProductType == PRODUCT_TYPE_INDEX_14) || (otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30))
        {
                Idx = otp1Reg.regAdc2Index80_200mV;
                if(otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30)
                {
                        Idx = Idx + (otp3Reg.regAdc2Index80_200mV_Bit4 << 4);
                }
                pCaliAdcData->CodeT80V200 += ADC_DELTA_CODE_MAPPING[Idx].Adc2V200;
        }

#endif  ///< end of FEATURE_ADC_CALI_FT
}
/**
 * @brief GetAdc2T25V200
 *
 *  Get ADC2 code at 25oC/200mV in CP
 *  8 bits
 *
 * @para  pCaliAdcData  pointer of structure CaliAdcDataType
 * @return  NULL
 *///####2012/08/29#####
void GetAdc2T25V200(CaliAdcDataType *pCaliAdcData)
{
        u8 Idx;

        pCaliAdcData->CodeT25V200 = ADC_2_CODE_V200_CP;
        pCaliAdcData->CodeT25V200 += (((otp2Reg.regAdc2DeltaCode25_200mv & ADC_2_SIGN_BIT_CODE_V200) ? 
                                ADC_2_CODE_V200_NEGATIVE : ADC_2_CODE_V200_POSITIVE) + 
                        (otp2Reg.regAdc2DeltaCode25_200mv & (~ADC_2_SIGN_BIT_CODE_V200)));

        if((otp2Reg.regProductType == PRODUCT_TYPE_INDEX_14) || (otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30))
        {
                Idx = otp1Reg.regAdc2Index25_200mV;
                if(otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30)
                {
                        Idx = Idx + (otp3Reg.regAdc2Index25_200mV_Bit4 << 4);
                }
                pCaliAdcData->CodeT25V200 += ADC_DELTA_CODE_MAPPING[Idx].Adc2V200;
        }
}

/**
 * @brief GetAdc2T25V100
 *
 *  Get ADC2 code at 25oC/100mV in CP
 *  7 bits
 *
 * @para  pCaliAdcData  pointer of structure CaliAdcDataType
 * @return  NULL
 *///####2012/08/29#####
void GetAdc2T25V100(CaliAdcDataType *pCaliAdcData)
{
        u8 Idx;

        pCaliAdcData->CodeT25V100 = ADC_2_CODE_V100_CP;
        pCaliAdcData->CodeT25V100 += (((otp2Reg.regAdc2DeltaCode25_100mv & ADC_2_SIGN_BIT_CODE_V100) ? 
                                ADC_2_CODE_V100_NEGATIVE : ADC_2_CODE_V100_POSITIVE) + 
                        (otp2Reg.regAdc2DeltaCode25_100mv & (~ADC_2_SIGN_BIT_CODE_V100)));

        if((otp2Reg.regProductType == PRODUCT_TYPE_INDEX_14) || (otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30))
        {
                Idx = otp1Reg.regAdc2Index25_100mV;
                if(otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30)
                {
                        Idx = Idx + (otp2Reg.regAdc2Index25_100mV_Bit4 << 4);
                }
                pCaliAdcData->CodeT25V100 += ADC_DELTA_CODE_MAPPING[Idx].Adc2V100;
        }
}
/**
 * @brief GetAdc2T80V100
 *
 *  Get ADC2 code at 80oC/100mV in CP
 *  7 bits
 *
 * @para  pCaliAdcData  pointer of structure CaliAdcDataType
 * @return  NULL
 *///####2012/08/29#####
void GetAdc2T80V100(CaliAdcDataType *pCaliAdcData)
{
#ifdef  FEATURE_ADC_CALI_FT

        s32 Code;

        Code = (s32)pCaliAdcData->CodeT80IT;
        Code = Code - pCaliAdcData->CodeT25IT;
        Code = Code*ADC_GAIN_CONST;
        Code = Code/ADC2_100_GAIN;
        Code = Code + pCaliAdcData->CodeT25V100;
        pCaliAdcData->CodeT80V100 = (u16)Code;

#else   ///< else of FEATURE_ADC_CALI_FT

        u8 Idx;

        pCaliAdcData->CodeT80V100 = ADC_2_CODE_V100_CP;
        pCaliAdcData->CodeT80V100 += (((otp2Reg.regAdc2DeltaCode80_100mv & ADC_2_SIGN_BIT_CODE_V100) ? 
                                ADC_2_CODE_V100_NEGATIVE : ADC_2_CODE_V100_POSITIVE) + 
                        (otp2Reg.regAdc2DeltaCode80_100mv & (~ADC_2_SIGN_BIT_CODE_V100)));

        if((otp2Reg.regProductType == PRODUCT_TYPE_INDEX_14) || (otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30))
        {
                Idx = otp1Reg.regAdc2Index80_100mV;
                if(otp2Reg.regProductType == PRODUCT_TYPE_INDEX_30)
                {
                        Idx = Idx + (otp2Reg.regAdc2Index80_100mV_Bit4 << 4);
                }
                pCaliAdcData->CodeT80V100 += ADC_DELTA_CODE_MAPPING[Idx].Adc2V100;
        }

#endif  ///< end of FEATURE_ADC_CALI_FT
}

/**
 * @brief GetIT
 *
 *  Get internal temperature code at 25 and 80oC in CP
 *  7 bits
 *
 * @para  pCaliAdcData  pointer of structure CaliAdcDataType
 * @return  NULL
 *///####2012/08/29#####
void GetIT(CaliAdcDataType *pCaliAdcData)
{
        if (ug31_config.feature_flag & BIT_CALI_WITH_IT_OTP_PARA) {
                pCaliAdcData->CodeT25IT = ((u16)otp3Reg.regAveIT25_HighByte << 8) +   //20120923/jacky
                        ((u16)otp3Reg.regAveIT25_LowByte_Bit76543 << 3) + 
                        ADC_1_CODE_IT_25_OFFSET;
                pCaliAdcData->CodeT25IT = pCaliAdcData->CodeT25IT >> 1;

                pCaliAdcData->CodeT80IT = ((u16)otp3Reg.regAveIT80_HighByte << 8) +   //20120923/jacky type convertion error?
                        ((u16)otp3Reg.regAveIT80_LowByte_Bit76543 << 3) + 
                        ADC_1_CODE_IT_80_OFFSET;
                pCaliAdcData->CodeT80IT = pCaliAdcData->CodeT80IT >> 1;              

        } else {
#ifdef JACKY_DEBUG			
 UG31_LOGE("[%s]-FAIL !!! \n",  __func__); 			
#endif
                                     
                pCaliAdcData->CodeT25IT = ADC_1_CODE_IT_25_IDEAL;
                pCaliAdcData->CodeT25IT += ((((otp2Reg.regITdeltaCode25_Bit1098 & ADC_1_SIGN_BIT_CODE_IT) ? 
                                                ADC_1_CODE_IT_NEGATIVE : ADC_1_CODE_IT_POSITIVE) + 
                                        ((otp2Reg.regITdeltaCode25_Bit1098 & (~ADC_1_SIGN_BIT_CODE_IT)) << 8)) + 
                                otp2Reg.regITdeltaCode25);

                pCaliAdcData->CodeT80IT = ADC_1_CODE_IT_80_IDEAL;
                pCaliAdcData->CodeT80IT += ((((otp2Reg.regITdeltaCode80_Bit1098 & ADC_1_SIGN_BIT_CODE_IT) ? 
                                                ADC_1_CODE_IT_NEGATIVE : ADC_1_CODE_IT_POSITIVE) + 
                                        ((otp2Reg.regITdeltaCode80_Bit1098 & (~ADC_1_SIGN_BIT_CODE_IT)) << 8)) + 
                                otp2Reg.regITdeltaCode80);

        }
}

/**
 * @brief CalGainFactor
 *
 *  Calculate slope and factor b of ADC gain
 *
 * @para  pCaliAdcData  pointer of structure CaliAdcDataType
 * @return  NULL
 *///####2012/08/29#####
void CalGainFactor(CaliAdcDataType *pCaliAdcData)
{
#ifdef  FEATURE_ADC_CALI_KAPIK

        u16 Delta25;
        u16 Delta80;

        Delta25 = pCaliAdcData->CodeT25V200 - pCaliAdcData->CodeT25V100;
        Delta80 = pCaliAdcData->CodeT80V200 - pCaliAdcData->CodeT80V100;

        pCaliAdcData->GainSlope = (s32)Delta80;
        pCaliAdcData->GainSlope = (pCaliAdcData->GainSlope - Delta25);

        pCaliAdcData->GainFactorB = (s32)Delta25;
        pCaliAdcData->GainFactorB = pCaliAdcData->GainFactorB*pCaliAdcData->CodeT80IT - Delta80*pCaliAdcData->CodeT25IT;

#endif  ///< end of FEATURE_ADC_CALI_KAPIK
}

/**
 * @brief CalOffsetFactor
 *
 *  Calculate slope and factor o of ADC offset
 *
 * @para  pCaliAdcData  pointer of structure CaliAdcDataType
 * @return  NULL
 *///####2012/08/29#####
void CalOffsetFactor(CaliAdcDataType *pCaliAdcData)
{
#ifdef  FEATURE_ADC_CALI_KAPIK

        s32 Offset25;
        s32 Offset80;

        Offset25 = (s32)pCaliAdcData->CodeT25V100;
        Offset25 = Offset25*2 - pCaliAdcData->CodeT25V200;
        Offset80 = (s32)pCaliAdcData->CodeT80V100;
        Offset80 = Offset80*2 - pCaliAdcData->CodeT80V200;

        pCaliAdcData->OffsetSlope = Offset80 - Offset25;

        pCaliAdcData->OffsetFactorO = Offset25*pCaliAdcData->CodeT80IT - Offset80*pCaliAdcData->CodeT25IT;

#endif  ///< end of FEATURE_ADC_CALI_KAPIK
}

/**
 * @brief CalAdcGainOffset
 *
 *  Calculate ADC gain and offset at current temperature
 *
 * @para  pCaliAdcData  pointer of structure CaliAdcDataType
 * @para  CurrITCode  current internal temperature code
 * @return  NULL
 *///####2012/08/29#####
void CalAdcGainOffset(CaliAdcDataType *pCaliAdcData, s16 CurrITCode)
{
        pCaliAdcData->AdcGain = pCaliAdcData->GainSlope*CurrITCode + pCaliAdcData->GainFactorB;
        pCaliAdcData->AdcOffset = pCaliAdcData->OffsetSlope*CurrITCode + pCaliAdcData->OffsetFactorO;
        /*
        UG31_LOGE("[%s] AdcGain 0x%08X, offset 0x%08X\n", 
                __func__,
                pCaliAdcData->AdcGain, 
                pCaliAdcData->AdcOffset
        );
        */
}

/**
 * @brief CalibrateCode
 *
 *  Calibrate raw code
 *
 * @para  pCaliAdcData  pointer of structure CaliAdcDataType
 * @para  TargetCode  code to be calibrated
 * @return  NULL
 *///####2012/08/29#####
void CalibrateCode(CaliAdcDataType *pCaliAdcData, s32 TargetCode)
{
#ifdef  FEATURE_ADC_CALI_KAPIK

        s64 CaliCode;
        s32 DeltaT;

        DeltaT = (s32)pCaliAdcData->CodeT80IT;
        //UG31_ADC_DBG(testAdc1, "[%s] DeltaT 0x%08X\n", __func__, DeltaT); 
        DeltaT = DeltaT - pCaliAdcData->CodeT25IT;
        //UG31_ADC_DBG(testAdc1,"[%s] DeltaT 0x%08X, CodeT25IT %d\n", __func__, DeltaT, pCaliAdcData->CodeT25IT); 

        CaliCode = (s64)TargetCode;
        //UG31_ADC_DBG(testAdc1,"[%s] CaliCode 0x%llX TargetCode %d\n", __func__, CaliCode, TargetCode); 
        CaliCode = DeltaT*CaliCode - pCaliAdcData->AdcOffset;
        //UG31_ADC_DBG(testAdc1,"[%s] CaliCode 0x%llX, AdcOffset 0x%08X\n", __func__, CaliCode, pCaliAdcData->AdcOffset); 
        CaliCode = CaliCode*(pCaliAdcData->IdealCodeHigh - pCaliAdcData->IdealCodeLow);
        //UG31_ADC_DBG(testAdc1,"[%s] CaliCode 0x%llX\n", __func__, CaliCode);
//<ASUS-WAD+>        
//      CaliCode = CaliCode/pCaliAdcData->AdcGain;
        CaliCode = div64_s64( CaliCode, pCaliAdcData->AdcGain);        
//<ASUS-WAD->
        pCaliAdcData->CaliCode = (u16)CaliCode;
        //UG31_ADC_DBG(testAdc1, "[%s] CalidCode %d, AdcGain %d\n", __func__, (s32)CaliCode, pCaliAdcData->AdcGain);
#endif  ///< end of FEATURE_ADC_CALI_KAPIK
}

/**
 * @brief CalibrationAdc
 *
 *  ADC calibration procedure according to the "kpk_gg1.docx"
 *  1. Get CP information from OTP array
 *  2. Calculate slope and factor b of ADC gain
 *  3. Calculate slope and factor o of ADC offset
 *  4. Calculate ADC gain and offset at current temperature
 *  5. Calculate calibrated code
 *
 * @para  AdcMode target ADC, CALI_ADC_1/CALI_ADC_2
 * @para  CurrITCode  current internal temperature code
 * @para  TargetCode  target code for compensation
 * @return  compensated code
 *///####2012/08/29#####

/*
void dumpCaliAdcDataType(const char *pStr, CaliAdcDataType *p)
{
        UG31_ADC_DBG(testAdc1, "[%s] AdcGain 0x%08X, %d\n", pStr, p->AdcGain, p->AdcGain)
        UG31_ADC_DBG(testAdc1, "[%s] AdcOffset 0x%08X, %d\n", pStr, p->AdcOffset, p->AdcOffset)
}
*/

s32 CalibrationAdc(u8 AdcMode, s16 CurrITCode, s32 TargetCode, CaliAdcDataType *pAdcCaliData)
{
        CaliAdcDataType CaliAdcData;

        memcpy(&CaliAdcData, pAdcCaliData, sizeof(CaliAdcDataType));

        /*
        UG31_ADC_DBG(testAdc1, "[%s] AdcMode 0x%02X\n"
                "CurrITCode 0x%04X\n"
                "TargetCode 0x%08X\n",
                __func__,
                AdcMode,
                CurrITCode,
                TargetCode);
        dumpCaliAdcDataType(__func__, &CaliAdcData);
        */

        /// [AT] : 1. Get CP information from OTP array ; 07/03/2012
        if(AdcMode == CALI_ADC_1)
        {
                GetAdc1T25V100(&CaliAdcData);
                //dumpCaliAdcDataType("1", &CaliAdcData);
                GetAdc1T25V200(&CaliAdcData);
                //dumpCaliAdcDataType("2", &CaliAdcData);
                GetAdc1T80V100(&CaliAdcData);
                //dumpCaliAdcDataType("3", &CaliAdcData);
                GetAdc1T80V200(&CaliAdcData);
                //dumpCaliAdcDataType("4", &CaliAdcData);
        }
        else
        {
                GetAdc2T25V100(&CaliAdcData);
                //dumpCaliAdcDataType("5", &CaliAdcData);
                GetAdc2T25V200(&CaliAdcData);
                //dumpCaliAdcDataType("6", &CaliAdcData);
                GetAdc2T80V100(&CaliAdcData);
                //dumpCaliAdcDataType("7", &CaliAdcData);
                GetAdc2T80V200(&CaliAdcData);
                //dumpCaliAdcDataType("8", &CaliAdcData);
        }
        //BAT_DBG_E("%s %d", __func__, __LINE__);
        GetIT(&CaliAdcData);
        //dumpCaliAdcDataType("9", &CaliAdcData);

        /// [AT] : 2. Calculate slope and factor b of ADC gain ; 07/03/2012
        CalGainFactor(&CaliAdcData);
        //dumpCaliAdcDataType("10", &CaliAdcData);

        /// [AT] : 3. Calculate slope and factor o of ADC offset ; 07/03/2012
        CalOffsetFactor(&CaliAdcData);
        //dumpCaliAdcDataType("11", &CaliAdcData);

        /// [AT] : 4. Calculate ADC gain and offset at current temperature ; 07/03/2012
        CalAdcGainOffset(&CaliAdcData, CurrITCode);
        //dumpCaliAdcDataType("12", &CaliAdcData);

        /// [AT] : 5. Calculate calibrated code ; 07/03/2012
        CalibrateCode(&CaliAdcData, TargetCode);
        //dumpCaliAdcDataType("13", &CaliAdcData);

        memcpy(pAdcCaliData, &CaliAdcData, sizeof(CaliAdcDataType));

        return (CaliAdcData.CaliCode);
}

#ifdef  FEATURE_ADC_CALI_USE_CALIITCODE

/**
 * @brief GetCaliRefIT
 *
 *  Get the IT code for calibration
 *
 * @return  IT code for calibration
 */
s16 GetCaliRefIT(void)
{
        CaliAdcDataType CaliAdcData;
        s32 RefITCode;

        //BAT_DBG_E("%s %d", __func__, __LINE__);
        GetIT(&CaliAdcData);

        if ((CaliAdcData.CodeT80IT - CaliAdcData.CodeT25IT) != 0) {
                UG31_LOGE("Error!! May divide by 0. %s %d\n", __func__, __LINE__);
                return 0;
        }

        RefITCode = (s32)cellParameter.adc_d3;
        RefITCode = RefITCode - ADC_1_CODE_IT_25_IDEAL;
        RefITCode = RefITCode*(CaliAdcData.CodeT80IT - CaliAdcData.CodeT25IT);
        RefITCode = RefITCode/(ADC_1_CODE_IT_80_IDEAL - ADC_1_CODE_IT_25_IDEAL);
        RefITCode = RefITCode + CaliAdcData.CodeT25IT;

        return ((s16)RefITCode);
}

#endif  ///< end of FEATURE_ADC_CALI_USE_CALIITCODE
/**
 * @brief CalibrateAdc
 *
 *  Calibrate ADC code according to the algorithm from Kapik
 *  1. Current
 *      -> REG_AVE_CURRENT_HIGH, REG_AVE_CURRENT_LOW (0x07, 0x06)
 *  2. Cell1 voltage
 *      -> REG_AVE_VBAT1_HIGH, REG_AVE_VBAT1_LOW (0x09, 0x08)
 *  3. Cell2 voltage
 *      -> REG_VBAT2_AVE_HIGH, REG_VBAT2_AVE_LOW (0x47, 0x46)
 *  4. Cell3 voltage
 *      -> REG_VBAT3_AVE_HIGH, REG_VBAT3_AVE_LOW (0x49, 0x48)
 *  5. Internal temperature
 *      -> REG_AVE_IT_HIGH, REG_AVE_IT_LOW (0x0B, 0x0A)
 *  6. External temperature
 *      -> REG_AVE_ET_HIGH, REG_AVE_ET_LOW (0x0F, 0x0E)
 *
 * @return  NULL
 *///####2012/08/29#####
void CalibrateAdc(void)
{
        s16 CurrITCode;
        GG_ADC_CALI_INFO_TYPE AdcCaliData;

        CurrITCode = userReg.regITAve >> 1;

        /// 1. Current
        AdcCaliData.Current.IdealCodeLow = ADC_1_CODE_V100_IDEAL;
        AdcCaliData.Current.IdealCodeHigh = ADC_1_CODE_V200_IDEAL;
        AdcCaliData.Current.InputVoltLow = ADC_1_VOLTAGE_V100;
        AdcCaliData.Current.InputVoltHigh = ADC_1_VOLTAGE_V200;

        userReg.regCurrentAve =(s16)CalibrationAdc(CALI_ADC_1,CurrITCode, (s32)userReg.regCurrentAve, &AdcCaliData.Current);
        

        /*
        BAT_DBG_E("[%s] IdealCodeLow 0x%08X\n"
                "IdealCodeHigh 0x%08X\n"
                "InputVoltLow 0x%08X\n"
                "InputVoltHigh 0x%08X\n"
                "regCurrentAve 0x%08X\n",
                __func__,
                AdcCaliData.Current.IdealCodeLow,
                AdcCaliData.Current.IdealCodeHigh,
                AdcCaliData.Current.InputVoltLow,
                AdcCaliData.Current.InputVoltHigh,
                userReg.regCurrentAve
                );
                */
        deviceInfo.CaliAdc1Code = userReg.regCurrentAve;
        user2Reg.regCurrent = (s16)CalibrationAdc(CALI_ADC_1, CurrITCode, (s32)user2Reg.regCurrent, &AdcCaliData.Current);

        /// 2. Cell1 voltage
        AdcCaliData.VCell1.IdealCodeLow = ADC_2_CODE_V100_IDEAL;
        AdcCaliData.VCell1.IdealCodeHigh = ADC_2_CODE_V200_IDEAL;
        AdcCaliData.VCell1.InputVoltLow = ADC_2_VOLTAGE_V100;
        AdcCaliData.VCell1.InputVoltHigh = ADC_2_VOLTAGE_V200;
        userReg.regVbat1Ave = (s16)CalibrationAdc(CALI_ADC_2, CurrITCode, (s32)userReg.regVbat1Ave, &AdcCaliData.VCell1);
        deviceInfo.CaliAdc2Code = userReg.regVbat1Ave;
        user2Reg.regV1 = (s16)CalibrationAdc(CALI_ADC_2, CurrITCode, (s32)user2Reg.regV1, &AdcCaliData.VCell1);
        user2Reg.regV2 = (s16)CalibrationAdc(CALI_ADC_2, CurrITCode, (s32)user2Reg.regV2, &AdcCaliData.VCell1);
        user2Reg.regV3 = (s16)CalibrationAdc(CALI_ADC_2, CurrITCode, (s32)user2Reg.regV3, &AdcCaliData.VCell1);

        /// 3. Cell2 voltage
        AdcCaliData.VCell2.IdealCodeLow = ADC_2_CODE_V100_IDEAL;
        AdcCaliData.VCell2.IdealCodeHigh = ADC_2_CODE_V200_IDEAL;
        AdcCaliData.VCell2.InputVoltLow = ADC_2_VOLTAGE_V100;
        AdcCaliData.VCell2.InputVoltHigh = ADC_2_VOLTAGE_V200;
        user2Reg.regVbat2Ave = (s16)CalibrationAdc(CALI_ADC_2, CurrITCode, (s32)user2Reg.regVbat2Ave, &AdcCaliData.VCell2);

        /// 4. Cell3 voltage
        AdcCaliData.VCell3.IdealCodeLow = ADC_2_CODE_V100_IDEAL;
        AdcCaliData.VCell3.IdealCodeHigh = ADC_2_CODE_V200_IDEAL;
        AdcCaliData.VCell3.InputVoltLow = ADC_2_VOLTAGE_V100;
        AdcCaliData.VCell3.InputVoltHigh = ADC_2_VOLTAGE_V200;
        user2Reg.regVbat3Ave = (s16)CalibrationAdc(CALI_ADC_2, CurrITCode, (s32)user2Reg.regVbat3Ave, &AdcCaliData.VCell3);

        /// 5. Internal temperature
        /// Not calibrated here !!!

        /// 6. External temperature
        AdcCaliData.ExtTemp.IdealCodeLow = ADC_1_CODE_V100_IDEAL;
        AdcCaliData.ExtTemp.IdealCodeHigh = ADC_1_CODE_V200_IDEAL;
        AdcCaliData.ExtTemp.InputVoltLow = ADC_1_VOLTAGE_V100;
        AdcCaliData.ExtTemp.InputVoltHigh = ADC_1_VOLTAGE_V200;
        userReg.regETAve = (u16)CalibrationAdc(CALI_ADC_2, CurrITCode, (s32)userReg.regETAve, &AdcCaliData.ExtTemp);
}
//20120927  Jacky add for fix the bugs of qmax error
/*
new add for fix bugs
now the average current is calculate by average ADC code
so it need to calibrate again before transfer to real current
2012/09/27/Jacky
*/
s16 CalibrateCapacityAdcCode(s32 inputCode)
{
	s16  updateCode;
	s16 CurrITCode;
	GG_ADC_CALI_INFO_TYPE AdcCaliData;

	CurrITCode = userReg.regITAve >> 1;

	/// 1. Current
	AdcCaliData.Current.IdealCodeLow = ADC_1_CODE_V100_IDEAL;
	AdcCaliData.Current.IdealCodeHigh = ADC_1_CODE_V200_IDEAL;
	AdcCaliData.Current.InputVoltLow = ADC_1_VOLTAGE_V100;
	AdcCaliData.Current.InputVoltHigh = ADC_1_VOLTAGE_V200;
	updateCode = (s16)CalibrationAdc(CALI_ADC_1, CurrITCode, (s32)inputCode, &AdcCaliData.Current);
	return(updateCode);
}


//OSC calibration
// input: IT Temperature High byte
// output: oscCnt[9:0] (need to write to Reg97/98
// return: u16[9:0]
// oscCnt25[9:0] = oscCntTarg[9:0] + oscDeltaCode25[7:0]
// oscCnt80[9:0] = oscCntTarg[9:0] + oscDeltaCode80[7:0]
// oscCnt[9:0] = m*ITcode[15:8] + C[9:0]
// m = (oscCnt80[9:0]-oscCnt25[9:0])/(iTcode80[7:0]-iTcode25[7:0])
// c = oscCnt25[9:0] - m*ITcode25[7:0]


#define OSC_CNT_TARG 512	//oscCntTarg[9:0]
u16 CalibrationOSC(u8 CurrentIT_HiByte)
{
	u16 u16Temp;
    
	s16 oscCnt25,oscCnt80;					//10 bits
	s16 oscDeltaCode25,oscDeltaCode80;		//
	s16 targetOscCnt;				//target osc

	s16 varM;
	s16 varC;

  	CaliAdcDataType CaliAdcData;

	if(bOTPisEmpty == false)
	{
		//Step 1.read ITdeltacode25/80
                //BAT_DBG_E("%s %d", __func__, __LINE__);
		GetIT(&CaliAdcData);
        
		//step2. calculate m & C
		oscDeltaCode25 = (s16)otp2Reg.regOscDeltaCode25;	//expand to s16
		oscDeltaCode80 = (s16)otp2Reg.regOscDeltaCode80;

		oscCnt25 = OSC_CNT_TARG + oscDeltaCode25;			//oscCnt25[9:0] = oscCntTarg[9:0] + oscDeltaCode25[7:0]
		oscCnt80 = OSC_CNT_TARG + oscDeltaCode80;			//oscCnt80[9:0] = oscCntTarg[9:0] + oscDeltaCode80[7:0]

		//assert((iTcode80 - iTcode25) != 0);
		if (!(CaliAdcData.CodeT80IT - CaliAdcData.CodeT25IT)) {
			UG31_LOGE("%s, divide 0.\n", __func__);
                        return 0;
		}

		varM = (oscCnt80 - oscCnt25)/(CaliAdcData.CodeT80IT - CaliAdcData.CodeT25IT);
		varC = oscCnt25 - varM*CaliAdcData.CodeT25IT;
		targetOscCnt = varM * CurrentIT_HiByte + varC;  //16 bits sign value
		if(targetOscCnt & 0x8000)		//check +/-
		{
			u16Temp = (u16 ) (targetOscCnt & 0x1fff);
			u16Temp |= 0x0200;			// minus
		} else{
			u16Temp = (u16 )targetOscCnt;  //positive data
		}
		return(u16Temp);   //only bit [9:0]
	} else{
		u16Temp = 0;
	}
	return(u16Temp);   //only bit [9:0]

}

//config GPIO I/II function
u8 ConfigGpioFunction(u8 setting)
{
        unsigned char u8Temp;

        u8 gpioSelData = 0;

        if(setting & FUN_GPIO)
        {
                gpioSelData = 0;
        }
        if(setting & FUN_ALARM)   //select Alarm function
        {
                u8Temp = cellParameter.alarm_timer;             //0x8f
                API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_TIMER, u8Temp);   //set alarm watch dog timer
                u8Temp = cellParameter.clkDivA;                 //0x90
                API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_CLK_DIVA, u8Temp);
                u8Temp = cellParameter.clkDivB;                 //0x91
                API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_CLK_DIVB, u8Temp);
                gpioSelData = 1;
        }
        if(setting & FUN_CBC_EN21)      //cbc21 enable
        {
                gpioSelData = 2;
        }if(setting & FUN_CBC_EN32)             //cbc32 Enable
        {
                gpioSelData = 3;
        }
        if(setting & FUN_PWM)  //PWM function, set PWM cycle
        {
                API_I2C_SingleRead(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_INTR_CTRL_C,&u8Temp);             //write Reg9d
                u8Temp &= 0xf3;
                u8Temp|= (cellParameter.pwm_timer << 2);
                API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE,REG_INTR_CTRL_C,u8Temp);              //set PWM output cycle
                gpioSelData = 4;
                API_I2C_SingleRead(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_INTR_CTRL_B,&u8Temp);             //write Reg9c
                u8Temp |= 0x40;
                API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_INTR_CTRL_B,u8Temp);              //write Reg9C
        }
        return(gpioSelData);
}

/*u8 ICType;
  /                 [2:0]=000 -> uG3100 [2:0]=001 -> uG3101
  /                 [2:0]=010 -> uG3102 [2:0]=100 -> uG3103_2
  /                 [2:0]=101 -> uG3103_3
  */
//function to set the REG_ADC_V1/2/3
////to set Reg C9/CA/CB
//// 00: GND  01:VBAT1 10:VAB2 11:VBAT3
void SetupAdc2Quene(void)
{
        u8  reg9cData;

        u8  ADC2_1CELL_TABLE[3] = {0x55,0x55,0x55};     //01010101 01010101 01010101 (0x55 55 55 )
        u8  ADC2_2CELL_TABLE[3] = {0x5a,0x5a,0x5a};     //01000100 01001000 10001000 (0x5a 5a 5a)
        u8  ADC2_3CELL_TABLE[3] = {0x5a,0xf5,0xa5}; //01011010 11110101 10101111
        u8* startPrt=NULL;

        API_I2C_SingleRead(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, 
                        REG_INTR_CTRL_B,&reg9cData); //Read reg9C
        if((cellParameter.ICType == 0) || (cellParameter.ICType == 1))  //0/1 means 1 cell
        {
                startPrt = &ADC2_1CELL_TABLE[0];
                reg9cData |= 0x07;
        }else if((cellParameter.ICType == 2) || (cellParameter.ICType == 4)){   //2/4 means 2-cell
                startPrt = &ADC2_2CELL_TABLE[0];
                reg9cData |= 0x17;
        }else if(cellParameter.ICType == 5){
                startPrt = &ADC2_3CELL_TABLE[0];        //3_cell
                reg9cData |= 0x37;
        }
        //new add for check ebcEnble   07/18/2011/Jacky add for cbc enable
        if(cellParameter.cbcEnable)
        {
                reg9cData &= 0xcf;              //clear bit4/5
                reg9cData |= (cellParameter.cbcEnable <<4);
        }

        API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE,  IS_TEN_BIT_MODE, REG_INTR_CTRL_B,reg9cData); //Read reg9C
        API_I2C_Write(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_ADC_V1, 3,startPrt);                   //write Reg9C~RegC0
}
//get the GPIO initial setting
////config GPIO1/ GPIO2
////input cellParameter
void InitGpioSetting(void)
{
        u8 cellEnableValue=0;
        u8 icType=0;
        u8 gpioSel = 0;         //

        icType = cellParameter.ICType & 0x07;

        //write Reg9E (cell enable)
        API_I2C_SingleRead(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_CELL_EN, &cellEnableValue);  //0x9e
        cellEnableValue = (cellEnableValue & 0xe3) + (icType <<2);
        API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_CELL_EN,cellEnableValue);    //write the REG_CELL_EN[4:2]

        API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_INTR_CTRL_D, cellParameter.gpio34);                //write Reg92
        // write  Reg9B(gpio1 sel/gpio2 sel/global enable)
        gpioSel = (ConfigGpioFunction(cellParameter.gpio2) << 5);       //set bit 765
        gpioSel |= (ConfigGpioFunction(cellParameter.gpio1) << 2);      //set bit 432
        gpioSel |= 0x03;                                                                                        //enable ADC2_EN / ADC1_EN
        API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_INTR_CTRL_A,gpioSel);             //write Reg9B

    	InitAlarmSetting();
        API_I2C_SingleRead(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_CELL_EN,&cellEnableValue);  //write Reg9E
        cellEnableValue |=0x03;                 //enable
        API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_CELL_EN,cellEnableValue);    //write the REG_CELL_EN[4:2]
}

bool ReadGGBXFileToCellDataAndInitSetting(GGBX_FILE_HEADER *pGGBXFileBuf, 
                CELL_TABLE* pCellTable, CELL_PARAMETER* pCellParameter)
{
        u8 *p_start = NULL ;
        u8 *p_end = NULL;
        u16 sum16=0;
        int i=0;

        /*
         * check GGBX_FILE tag
         */
        if (pGGBXFileBuf->ggb_tag != GGBX_FILE_TAG) {
                UG31_LOGE("[%s] GGBX file tag not correct. tag: %08X\n", 
                        __func__, pGGBXFileBuf->ggb_tag);
                return false;
        }

        /*
         * check GGBX_FILE checksum
         */
        p_start = (u8 *)pGGBXFileBuf + sizeof(GGBX_FILE_HEADER);
        p_end = p_start + pGGBXFileBuf->length - 1;
        for (; p_start <= p_end; p_start++) 
                sum16 += *p_start;

        if (sum16 != pGGBXFileBuf->sum16) {
                UG31_LOGE("[%s] GGBX file checksum incorrect. checksum: %04X\n", 
                                __func__, pGGBXFileBuf->sum16);
                return false;
        }
        
        /* check done. prepare copy data */
        memset(pCellTable, 0x00, sizeof(CELL_TABLE));
        memset(pCellParameter, 0x00, sizeof(CELL_PARAMETER));

        p_start = (u8 *)pGGBXFileBuf + sizeof(GGBX_FILE_HEADER);
        for (i=0; i<pGGBXFileBuf->num_ggb; i++) {
                /* TODO: boundary checking */
                /* TODO: select right ggb content by sku */
                memcpy(pCellParameter, p_start, sizeof(CELL_PARAMETER)); 
                memcpy(pCellTable, p_start + sizeof(CELL_PARAMETER), sizeof(CELL_TABLE)); 
                p_start += (sizeof(CELL_PARAMETER) + sizeof(CELL_TABLE));
        }
        return true;
}

// Initial battery global variable
void InitialBatteryInformation(GG_BATTERY_INFO* pBatteryInfo)
{
        memset(pBatteryInfo, 0x00, sizeof(GG_BATTERY_INFO));
        pBatteryInfo->SOV = 100;
}

// Initial GG_REAL_DATA' deltaQ array to 0xaaa
void InitialDeltaQ(GG_REAL_DATA* pRealData)
{
        int i, j, k;
        memset(pRealData, 0x00, sizeof(GG_REAL_DATA));
        for(i=0;i<TEMPERATURE_NUMS;i++)
                for(j=0;j<C_RATE_NUMS;j++)
                        for(k=0;k<OCV_NUMS;k++)
                                pRealData->deltaQ[i][j][k]=0x0aaa;
}

// Read GG_USER_REG from device to global variable and output
GGSTATUS upiGG_ReadAllRegister(GG_USER_REG* pUserReg,GG_USER2_REG* pUser2Reg)
{

	if(!API_I2C_Read(0, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, 0, 16, &pUserReg->regMode))  //
		return UG_READ_REG_FAIL;
	else{
		if(!API_I2C_Read(0, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, 0x15, 2, &pUserReg->regAlarmEnable))
			return UG_READ_REG_FAIL;
		if(!API_I2C_Read(0, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_VBAT2_LOW, sizeof(GG_USER2_REG), (u8* )&pUser2Reg->regVbat2))		//read
			return UG_READ_REG_FAIL;

                //pUserReg->regCurrentAve = -1326;
                //pUserReg->regITAve = 24104;
                //pUserReg->regVbat1Ave = 5516;
	}
        return UG_READ_REG_SUCCESS;
}

s16 _CalibrationITOtp(s16 s16ITCode)
{
	s32 iCaliItCode;
        CaliAdcDataType CaliAdcData;
    
	//Step 1.read ITdeltacode25/80
        //BAT_DBG_E("%s %d", __func__, __LINE__);
	GetIT(&CaliAdcData);
      
        if ( (CaliAdcData.CodeT80IT - CaliAdcData.CodeT25IT) == 0) {
                UG31_LOGE("Error!! May divide by 0 %s %d\n", __func__, __LINE__);
                return 0;
        }
      
	iCaliItCode = ((((s32)s16ITCode - (CaliAdcData.CodeT25IT<<1))*
	                (ADC_1_CODE_IT_80_IDEAL - ADC_1_CODE_IT_25_IDEAL))/
	               (CaliAdcData.CodeT80IT - CaliAdcData.CodeT25IT)) + 
	              (ADC_1_CODE_IT_25_IDEAL<<1);

#ifdef  JACKY_DEBUG
 UG31_LOGE( "[%s] CaliAdcData.CodeT80IT = 0x%08X CaliAdcData.CodeT25IT = 0x%08X\n", 
                                     __func__, CaliAdcData.CodeT80IT ,  CaliAdcData.CodeT25IT ); 
                                     
UG31_LOGE("[%s] s16ITCode = 0x%08X iCaliItcode = 0x%08X\n",  __func__, s16ITCode, iCaliItCode); 

#endif        


        return (iCaliItCode);
}

s16 _CalibrationIT(s16 s16ITCode)
{

	s16 iTcode25,iTcode80;				//16 bits
	s32 it1,it2;
	
  	iTcode25 = cellParameter.adc_d1;
	iTcode80 = cellParameter.adc_d2;

	if (!(iTcode80 - iTcode25)) {
                UG31_LOGE("%s, may divide by 0.\n", __func__);
                return 0;
        }
	//
	it1=  ( ((s32)IT_TARGET_CODE80<<1) - (IT_TARGET_CODE25<<1)) /(iTcode80 - iTcode25);
	it2 = (s16ITCode - iTcode25)*it1;
	it2 = it2 + (IT_TARGET_CODE25<<1);

	return it2;
}

s16 CalibrationIT(s16 s16ITCode)
{
        if (ug31_config.feature_flag & BIT_CALI_WITH_IT_OTP_PARA) {
                return _CalibrationITOtp(s16ITCode);
        } else {
                return _CalibrationIT(s16ITCode);

        }
}
//
// Calculate current(mA) = raw_data * 7.63 / rSense
//####2012/08/29#####
s16 CalculateCurrentFromUserReg(s16 currentAdcCode)
{
#ifdef  FEATURE_ADC_CALI_AT_CURRENT

        s64 Curr;

        //UG31_LOGE("[%s] currentAdcCode %d\n", __func__, currentAdcCode);
        Curr = (s32)currentAdcCode;
        //UG31_LOGE("[%s] Curr 1 %d, %d\n", __func__, (s32)Curr, cellParameter.adc1_pos_offset);
        Curr = Curr - cellParameter.adc1_pos_offset;
        //UG31_LOGE("[%s] Curr 2 %d, %d\n", __func__, (s32)Curr, cellParameter.adc1_ngain);
        Curr =  (s16)div64_s64(((s64)Curr * 1000L), cellParameter.adc1_ngain);	//convert to 10*maH
        //UG31_LOGE("[%s] Result Curr %d\n", __func__, (s32)Curr);

        return ((s16)Curr);

#else   ///< else of FEATURE_ADC_CALI_AT_CURRENT
        u16 iRawData;  //overflow?
        assert(cellParameter.rSense != 0);
        short result;

        currentAdcCode -= cellParameter.adc1_pos_offset;

        //	if(currentAdcCode < 0)
        //	{
        iRawData = ((s32)currentAdcCode*1000)/cellParameter.adc1_ngain;   //mA
        result = iRawData;
        //	} else{
        //		iRawData = ((s32)currentAdcCode*1000)/cellParameter.adc1_pgain;
        //		result = iRawData;
        //	}
        return(result);
#endif  ///< end of FEATURE_ADC_CALI_AT_CURRENT
}

//####2012/08/29#####
#ifdef  FEATURE_ADC_CALI_AT_VBAT1

/**
 * @brief GetVoltFromCaliCode
 *
 *  Convert voltage from calibrated ADC code
 *
 * @para  _in_voltCode  calibrated ADC code
 * @return  battery voltage in mV
 */
u16 GetVoltFromCaliCode(s16 _in_voltCode)
{
  s32 Volt;

  Volt = (s32)_in_voltCode;
  #ifdef  FEATURE_ADC_MANUAL_CP
    Volt = Volt - ADC_2_CODE_V100_IDEAL;
    Volt = Volt*(ADC_2_VOLTAGE_V200 - ADC_2_VOLTAGE_V100);
    Volt = Volt/(ADC_2_CODE_V200_IDEAL - ADC_2_CODE_V100_IDEAL);
    Volt = Volt + ADC_2_VOLTAGE_V100;
  #else   ///< else of FEATURE_ADC_MANUAL_CP
    Volt = Volt - cellParameter.adc2_offset;
    Volt = Volt*1000/cellParameter.adc2_gain;
  #endif  ///< end of FEATURE_ADC_MANUAL_CP
  
  return ((u16)Volt);
}

/**
 * @brief GetVoltConvertFactor
 *
 *  Get the voltage conversion factor
 *
 * @return  conversion factor
 *///####2012/08/29#####
u16 GetVoltConvertFactor(void)
{
  u32 Volt = 1;
  u32 Factor;

//  Volt = (u32)GetVoltFromCaliCode((s16)deviceInfo.CaliAdc2Gain);

  Factor = (u32)cellParameter.adc_d4;
  Factor = Factor*ADC_2_CALI_CONSTANT/Volt;
  
  Factor = ADC_2_CALI_CONSTANT;
  return ((u16)Factor);
}
#endif  ///< end of FEATURE_ADC_CALI_AT_VBAT1

//####2012/08/29#####
u16 CalculateVbat1FromUserReg(void)
{
#ifdef  FEATURE_ADC_CALI_AT_VBAT1

	u32 Volt;
	u16 Factor;

	Volt = (u32)GetVoltFromCaliCode(userReg.regVbat1Ave);
	Factor = GetVoltConvertFactor();
#ifndef FEATURE_ADC_MANUAL_CP
	Volt = Volt*Factor/ADC_2_CALI_CONSTANT;
#endif  ///< end of FEATURE_ADC_MANUAL_CP

	return ((u16)Volt);

#else   ///< else of FEATURE_ADC_CALI_AT_VBAT1

	u16 iRawData;
	u16 voltage_return;

	iRawData = userReg.regVbat1Ave - cellParameter.adc2_offset;
	voltage_return = ((u32)iRawData*1000)/cellParameter.adc2_gain;

#if 0
	iRawData = userReg.regVbat1 & 0x1fff;	  //14 bits(bit13 is Sign Bit)
	voltage_return = ((u32 )iRawData*UG_RAW_VBAT1_LSB)/100;//voltage_code * 0.65mV
#endif

	return voltage_return;

#endif  ///< end of FEATURE_ADC_CALI_AT_VBAT1
}
// Calculate IT Temperature
// 11 bits ADC code
// y = 2.4505*X + 697.78
// tempC =  (IT_AdcCode[10:0] - 697.78)/2.4505;
// return tempC*100
//####2012/08/29#####
s16 CalculateIT(const u16 IT_AdcCode)
{
	int tempC;

	tempC = (IT_AdcCode>>1) - 11172;

	tempC = (tempC*100)/392;				//2012/02/21
#ifdef JACKY_DEBUG
UG31_LOGE("[%s] tempC before calibration = 0x%08X \n", __func__, tempC); 
#endif
        if (ug31_config.feature_flag & BIT_CALI_WITH_IT_OTP_PARA) {
                tempC = tempC - cellParameter.adc_d5;
  #ifdef JACKY_DEBUG
UG31_LOGE("[%s] tempC after calibration = 0x%08X \n", __func__, tempC); 
#endif              
        }

	return((s16)tempC);
}

// Calculate ET Temperature
s16 CalculateET(const u16 ET_AdcCode)
{
	u16 eTdata = 0;
	u8 i;
	u8 u8Index = 0;

	s16 ET_TABLE[19]={-10,-5,0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80};
    //Large to Small
        if(ET_AdcCode >= cellParameter.rtTable[0])  
                return ET_TABLE[0]*10;			//upper boundary
        if(ET_AdcCode <= cellParameter.rtTable[18]) 
                return ET_TABLE[18]*10;		//lower boundary

	for(i=0; i < 19; i++)
	{
		if(ET_AdcCode == cellParameter.rtTable[i])
		{
			return ET_TABLE[u8Index]*10;
		} else {
			if(ET_AdcCode > cellParameter.rtTable[i])
				break;
			u8Index++;
		}
	}
	//cellParameter.rtTable[u8Index-1] < ET_AdcCode < cellParameter.rtTable[u8Index]
	//ET_TABLE[u8Index-1] > eTdata > ET_TABLE[u8Index]
	eTdata = 10*ET_TABLE[u8Index] - 5*10*(ET_AdcCode -cellParameter.rtTable[u8Index])/(cellParameter.rtTable[u8Index-1] -cellParameter.rtTable[u8Index]);

	return eTdata;
}

// Calculate the charge data(mA.h) by raw_data * 0.432 uV/ R-sense
s16 CalculateChargeFromUserReg(const s16 chargeAdcCode)
{
	s32 temp;

        if (!cellParameter.rSense) {
                UG31_LOGE("%s: rSense is 0", __func__);
                return 0;
        }

	temp = (s32)chargeAdcCode*UG_RAW_CHARGE_LSB;
	temp /=(cellParameter.rSense*100);

	return (((s32)chargeAdcCode*UG_RAW_CHARGE_LSB)/(cellParameter.rSense*100));
}

s16 CalDeltaqEach10secs(u32 totalTime)
{
	s16 deltaQC = 0;		//coulomb counter's deltaQ

	s16 deltaOfCharge = 0;
	u16 totalConvertCnt = 0;
	s16 averageAdcCode = 0;

	deltaOfCharge = deviceInfo.chargeRegister - deviceInfo.preChargeRegister;	//s16
	totalConvertCnt =  deviceInfo.AdcCounter - deviceInfo.preAdcCounter;		//u16-u16

	if(totalConvertCnt)
	{
		averageAdcCode = (s16) (((s32)deltaOfCharge << 12)/totalConvertCnt);	//average current from  deltaQ/adc convet cnt
		averageAdcCode = CalibrateCapacityAdcCode(averageAdcCode);            //20120927/jacky
		deviceInfo.fwCalAveCurrent_mA =(s16) ((((s32)averageAdcCode - cellParameter.adc1_pos_offset)*1000)/cellParameter.adc1_pgain);  //mA
                //<ASUS-WAD+> 64 bit division MUST use function
		deltaQC =  (s16)div64_s64(((s64)deviceInfo.fwCalAveCurrent_mA * (s64)totalTime), (1000*360));	//convert to 10*maH
                //<ASUS-WAD->
	}
	return deltaQC;
}

//calculate Cell 1
//####2012/08/29#####
u16 CalculateV123FromUser2Reg(u16 vAdcCode)
{
#ifdef  FEATURE_ADC_CALI_AT_V123

	u32 Volt;
	u16 Factor;

	Volt = (u32)GetVoltFromCaliCode((s16)vAdcCode);
	Factor = GetVoltConvertFactor();
#ifndef FEATURE_ADC_MANUAL_CP
	Volt = Volt*Factor/ADC_2_CALI_CONSTANT;
#endif  ///< end of FEATURE_ADC_MANUAL_CP

	return ((u16)Volt);

#else   ///< else of FEATURE_ADC_CALI_AT_V123

	int iRawData = vAdcCode;	  //14 bits(bit13 is Sign Bit)
	u16 voltage_return;

	voltage_return = ((u32 )iRawData*UG_RAW_VBAT1_LSB)/100; 	//voltage_code * 0.032 mV
	return voltage_return;

#endif  ///< end of FEATURE_ADC_CALI_AT_V123
}


// Calculate voltage by raw_data * 0.032
//####2012/08/29#####
u16 CalculateVoltageFromUserReg(const s16 voltageAdcCode, const s16 Current, const u16 offsetR, const u16 deltaR)
{
	int iRawData;
	u16 voltage_return;

#ifdef  FEATURE_ADC_CALI_AT_VOLTAGE

	u32 Volt;
	u16 Factor;
	
	Volt = (u32)GetVoltFromCaliCode(voltageAdcCode);
	Factor = GetVoltConvertFactor();
	
#ifndef FEATURE_ADC_MANUAL_CP
	Volt = Volt*Factor/ADC_2_CALI_CONSTANT;
#endif  ///< end of FEATURE_ADC_MANUAL_CP
	voltage_return = (u16)Volt;

#else   ///< else of FEATURE_ADC_CALI_AT_VOLTAGE
	iRawData = userReg.regVbat1Ave - cellParameter.adc2_offset;
	voltage_return = ((u32)iRawData*1000)/cellParameter.adc2_gain;

#endif  ///< end of FEATURE_ADC_CALI_AT_VOLTAGE
	if(Current < 0) {
		voltage_return =voltage_return + offsetR*abs(Current)/1000+deltaR;
	} else{
		voltage_return =voltage_return - offsetR*abs(Current)/1000 + deltaR;
	}
	return voltage_return;

#if 0
	int iRawData = voltageAdcCode & 0x1fff;	  //14 bits(bit13 is Sign Bit)
	u16 voltage_return;
	if(current < 0) {
		voltage_return = ((u32 )iRawData*UG_RAW_VOLTAGE_LSB)/1000; 	//voltage_code * 0.032 mV
		//XXX consider the gap between the EL & the Gauge
		//		voltage_return += ((offsetR*abs(current))/1000);
	} else { //+ plus(charge)
		voltage_return = ((u32 )iRawData*UG_RAW_VOLTAGE_LSB)/1000;	//voltage_code * 0.032 mV
		//	voltage_return -= ((offsetR*abs(current))/1000);
		//	if(voltage_return >= deltaR)
		//		voltage_return -= deltaR;
	}
	return voltage_return;
#endif
}
//use Vbat2 ADC Code/0x40 0x41
//OTP1 vbat2_8v_cal to calculate the ADC Code
u16 CalculateVbat2Voltage_mV(
                u8 Scale,
                u16 vBat2_6V_IdealAdcCode,
                u16 vBat2_8V_IdealAdcCode,
                s16 vBat2RealAdcCode
)
{

	u16 vBat2_mV;
	u16 vBat2_8V_RealAdcCode,vBat2_6V_RealAdcCode;
    
  	vBat2_8V_RealAdcCode = vBat2_8V_IdealAdcCode;
  	vBat2_6V_RealAdcCode = vBat2_6V_IdealAdcCode;

  	vBat2_mV = 6000+((8000-6000)*(vBat2RealAdcCode - vBat2_6V_RealAdcCode))/(vBat2_8V_RealAdcCode - vBat2_6V_RealAdcCode);
	return(vBat2_mV);
}

// Read GG_USER_REG from device and calculate GG_DEVICE_INFO, then write to global variable and output
// TODO: offsetR and deltaR will input from .GGB in the future modify
GGSTATUS upiGG_ReadDeviceInfo(GG_DEVICE_INFO* pExtDeviceInfo)
{
	// Get current user register data
	s16 realITcode;
	u16 oscCaliData;
	GGSTATUS status = UG_READ_DEVICE_INFO_SUCCESS;
    
	if(upiGG_ReadAllRegister(&userReg, &user2Reg) != UG_READ_REG_SUCCESS)  //found data error
	{
		status = UG_READ_ADC_FAIL;
	}
  //put Calibrations function Here  //####2012/08/29#####
  #ifdef FEATURE_ADC_CALIBRATION
    CalibrateAdc();     ///< [AT] : ADC calibration from Kapik ; 07/04/2012
  #endif ///< end of FEATURE_ADC_CALIBRATION
  
  //xxxxx debug use
	//deviceInfo.vBat1_AveAdcCode = userReg.regVbat1Ave;
	//deviceInfo.vBat1_AdcCode = user2Reg.regVbat1;
    //xxxxx debug use
	deviceInfo.chargeRegister = userReg.regCharge;					//coulomb counter
	deviceInfo.AdcCounter = userReg.regCounter;						//adc1 convert counter
	deviceInfo.aveCurrentRegister = userReg.regCurrentAve;    //2012/07/11
	deviceInfo.current_mA = CalculateCurrentFromUserReg(user2Reg.regCurrent);					//20110714/
	deviceInfo.AveCurrent_mA = CalculateCurrentFromUserReg(userReg.regCurrentAve);	//average current
	
     if(abs(deviceInfo.AveCurrent_mA ) < cellParameter.standbyCurrent)		//20120927/jacky
		deviceInfo.AveCurrent_mA = 0;
    
	realITcode = CalibrationIT(userReg.regITAve);	//average IT
	deviceInfo.IT = CalculateIT(realITcode);	//calculate IT
	deviceInfo.ET = CalculateET(userReg.regETAve);		//calculate ET
	//deviceInfo.IT =  250;	//debug use for fix in 25C //####2012/08/29#####

      
	oscCaliData = CalibrationOSC((u8) (realITcode >>8));			//osc calibration
	if(bOTPisEmpty == false)
	{
		API_I2C_SingleWrite(SECURITY,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_OSCTUNE_CNTB, (u8)(oscCaliData >>8) );
		API_I2C_SingleWrite(SECURITY,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_OSCTUNE_CNTA, (u8)oscCaliData );
	}
	deviceInfo.chargeData_mAh = CalculateChargeFromUserReg(userReg.regCharge);
    
	if(deviceInfo.lastTime == 0)			//1st
	{
		//for count average current use(each 10 secs)
		deviceInfo.preAdcCounter = deviceInfo.AdcCounter;			//keep the ADC counter
		deviceInfo.preChargeRegister = deviceInfo.chargeRegister;	//keep the charge register
		deviceInfo.fwChargeData_mAH = 0;			//rest to 0
		deviceInfo.fwCalAveCurrent_mA = 0;
		deviceInfo.lastTime = GetTickCount();		//keep the start time
	} else{
		deviceInfo.fwChargeData_mAH += CalDeltaqEach10secs((GetTickCount()-deviceInfo.lastTime));
		deviceInfo.lastTime = GetTickCount();						//keep the start time
		deviceInfo.preAdcCounter = deviceInfo.AdcCounter;			//
		deviceInfo.preChargeRegister = deviceInfo.chargeRegister;	//
	}
  
	deviceInfo.v1_mV = CalculateV123FromUser2Reg(user2Reg.regV1);
	deviceInfo.vCell1_mV = deviceInfo.vBat1Average_mV = CalculateVbat1FromUserReg();
	deviceInfo.vBat1Average_mV = CalculateVoltageFromUserReg(
                        userReg.regVbat1Ave, 
                        deviceInfo.AveCurrent_mA, 
                        cellParameter.offsetR, 
                        cellParameter.deltaR
                );
	deviceInfo.voltage_mV = deviceInfo.vBat1Average_mV;
	//	deviceInfo.voltage_mV = CalculateVoltageFromUserReg(userReg.regVbat1Ave, deviceInfo.AveCurrent_mA, cellParameter.offsetR, cellParameter.deltaR);
	// Output Result
	memcpy(pExtDeviceInfo, &deviceInfo, sizeof(GG_DEVICE_INFO));

	return status;
}


bool ActiveuG31xx(void)
{ 
//	userReg.regCtrl1 = 0x10;  //set PORDET(soft reset)			//2012/08/29/Jacky
//	if(!API_I2C_Write(0,isHighSpeedMode, isTenBitMode, REG_CTRL_ADDR, 1,  &userReg.regCtrl1))
//		return false;

        // Sleep function by different OS define
//	SleepMiniSecond(500);

        userReg.regCtrl1 = 0x02;  //set GG_RST = 1
        if(!API_I2C_Write(0,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_CTRL_ADDR, 1,  &userReg.regCtrl1))
                return false;

	SleepMiniSecond(500);		//####2012/08/29#####
        userReg.regMode = 0x10;     //set GG_RUN=1
        if(!API_I2C_Write(0,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_MODE_ADDR, 1, &userReg.regMode))
                return false;

        return true;
}

void dumpInfo(void)
{
        int i=0, j=0, k=0;

        BAT_DBG("/// ====================================\n");
        BAT_DBG("/// Cell Table\n");
        BAT_DBG("/// ====================================\n");
        for (i=0; i<TEMPERATURE_NUMS; i++) {
                for (j=0; j<OCV_C_RATE_NUMS; j++) {
                        for (k=0; k<OCV_NUMS; k++) {
                                BAT_DBG("INIT_OCV %02doC 0.%dC %d%% %d\n", 
                                        TEMPERATURE_TABLE[i], OCV_CRATE_TABLE[j], 
                                        SOC_TABLE[k], cellTable.INIT_OCV[i][j][k]);
                        }// for k
                        printk("\n");
                }// for j
        } //for i

        for (i=0; i<TEMPERATURE_NUMS; i++) {
                for (j=0; j<C_RATE_NUMS; j++) {
                        for (k=0; k<SOV_NUMS; k++) {
                                BAT_DBG("CELL_VOLTAGE_TABLE %02doC 0.%dC %d%% %d\n", 
                                        TEMPERATURE_TABLE[i], CRATE_TABLE[j], 
                                        cellParameter.SOV_TABLE[k]/10, cellTable.CELL_VOLTAGE_TABLE[i][j][k]);
                        }// for k
                        printk("\n");
                }// for j
        } //for i

        for (i=0; i<TEMPERATURE_NUMS; i++) {
                for (j=0; j<C_RATE_NUMS; j++) {
                        for (k=0; k<SOV_NUMS; k++) {
                                BAT_DBG("CELL_NAC_TABLE %02doC 0.%dC %d%% %d\n", 
                                        TEMPERATURE_TABLE[i], CRATE_TABLE[j], 
                                        cellParameter.SOV_TABLE[k]/10, cellTable.CELL_NAC_TABLE[i][j][k]);
                        }// for k
                        printk("\n");
                }// for j
        } //for i

        BAT_DBG("/// ====================================\n");
        BAT_DBG("/// CELL_PARAMETER\n");
        BAT_DBG("/// ====================================\n");
        BAT_DBG("Total struct size: %d\n", cellParameter.totalSize);
        BAT_DBG("firmware version: 0x%02X\n", cellParameter.fw_ver)
        BAT_DBG("customer: %s\n", cellParameter.customer)
        BAT_DBG("project: %s\n", cellParameter.project)
        BAT_DBG("ggb version: 0x%02X\n", cellParameter.ggb_version)
        BAT_DBG("customer self-define: %s\n", cellParameter.customerSelfDef)
        BAT_DBG("project self-define: %s\n", cellParameter.projectSelfDef)
        BAT_DBG("cell type : 0x%04X\n", cellParameter.cell_type_code);
        BAT_DBG("ICType: 0x%02X\n", cellParameter.ICType);
        BAT_DBG("gpio1: 0x%02X\n", cellParameter.gpio1);
        BAT_DBG("gpio2: 0x%02X\n", cellParameter.gpio2);
        BAT_DBG("gpio34: 0x%02X\n", cellParameter.gpio34);
        BAT_DBG("Chop control ?? : 0x%02X\n", cellParameter.chopCtrl);
        BAT_DBG("ADC1 offset ?? : %d\n", cellParameter.adc1Offset);
        BAT_DBG("Cell number ?? : %d\n", cellParameter.cellNumber);
        BAT_DBG("Assign cell one to: %d\n", cellParameter.assignCellOneTo);
        BAT_DBG("Assign cell two to: %d\n", cellParameter.assignCellTwoTo);
        BAT_DBG("Assign cell three to: %d\n", cellParameter.assignCellThreeTo);
        BAT_DBG("I2C Address: : 0x%02X\n", cellParameter.i2cAddress);
        BAT_DBG("I2C 10bit address: : 0x%02X\n", cellParameter.tenBitAddressMode); 
        BAT_DBG("I2C high speed: 0x%02X\n", cellParameter.highSpeedMode);
        BAT_DBG("clock(kHz): %d\n", cellParameter.clock);
        BAT_DBG("RSense(m ohm): %d\n", cellParameter.rSense);
        BAT_DBG("ILMD(mAH) ?? : %d\n", cellParameter.ILMD);
        BAT_DBG("EDV1 Voltage(mV): %d\n", cellParameter.edv1Voltage);
        BAT_DBG("Standby current ?? : %d\n", cellParameter.standbyCurrent);
        BAT_DBG("TP Current(mA)?? : %d\n", cellParameter.TPCurrent);
        BAT_DBG("TP Voltage(mV)?? : %d\n", cellParameter.TPVoltage);
        BAT_DBG("TP Time ?? : %d\n", cellParameter.TPTime);
        BAT_DBG("Offset R ?? : %d\n", cellParameter.offsetR);
        BAT_DBG("Delta R ?? : %d\n", cellParameter.deltaR);
        BAT_DBG("max delta Q(%%)  ?? : %d\n", cellParameter.maxDeltaQ);
        BAT_DBG("Offset mAH  ?? : %d\n", cellParameter.offsetmaH);
        BAT_DBG("time interval (s) : %d\n", cellParameter.timeInterval);
        BAT_DBG("COC ?? : %d\n", cellParameter.COC);
        BAT_DBG("DOC ?? : %d\n", cellParameter.DOC);
        BAT_DBG("UC ?? : %d\n", cellParameter.UC);
        BAT_DBG("OV1 (mV) ?? : %d\n", cellParameter.OV1);
        BAT_DBG("UV1 (mV) ?? : %d\n", cellParameter.UV1);
        BAT_DBG("OV2 (mV) ?? : %d\n", cellParameter.UV2);
        BAT_DBG("UV2 (mV) ?? : %d\n", cellParameter.UV2);
        BAT_DBG("OV3 (mV) ?? : %d\n", cellParameter.UV3);
        BAT_DBG("UV3 (mV) ?? : %d\n", cellParameter.UV3);
        BAT_DBG("OVP (mV) ?? : %d\n", cellParameter.OVP);
        BAT_DBG("UVP (mV) ?? : %d\n", cellParameter.UVP);
        BAT_DBG("OIT (C) ?? : %d\n", cellParameter.OIT);
        BAT_DBG("UIT (C) ?? : %d\n", cellParameter.UIT);
        BAT_DBG("OET (C) ?? : %d\n", cellParameter.OET);
        BAT_DBG("UET (C) ?? : %d\n", cellParameter.UET);
        BAT_DBG("CBC_21 ?? : %d\n", cellParameter.CBC21);
        BAT_DBG("CBC_32 ?? : %d\n", cellParameter.CBC32);
        BAT_DBG("OSC tune J?? : %d\n", cellParameter.oscTuneJ);
        BAT_DBG("OSC tune K?? : %d\n", cellParameter.oscTuneK);
        BAT_DBG("Alarm timer ?? : %d\n", cellParameter.alarm_timer);
        BAT_DBG("Clock div. A?? : %d\n", cellParameter.clkDivA);
        BAT_DBG("Clock div. B?? : %d\n", cellParameter.clkDivB);
        BAT_DBG("PWM timer ?? : %d\n", cellParameter.pwm_timer);
        BAT_DBG("Alarm enable?? : 0x%02X\n", cellParameter.alarmEnable);
        BAT_DBG("CBC enable?? : 0x%02X\n", cellParameter.cbcEnable);
        BAT_DBG("OTP1 scale?? : %d\n", cellParameter.otp1Scale);
        BAT_DBG("BAT2 8V ideal ADC code?? : %d\n", cellParameter.vBat2_8V_IdealAdcCode);
        BAT_DBG("BAT2 6V ideal ADC code?? : %d\n", cellParameter.vBat2_6V_IdealAdcCode);
        BAT_DBG("BAT3 12V ideal ADC code?? : %d\n", cellParameter.vBat3_12V_IdealAdcCode);
        BAT_DBG("BAT3 9V ideal ADC code?? : %d\n", cellParameter.vBat3_9V_IdealAdcCode);
        //BAT_DBG("BAT2 Ra : %d\n", cellParameter.vBat2Ra);
        //BAT_DBG("BAT2 Rb : %d\n", cellParameter.vBat2Rb);
        //BAT_DBG("BAT3 Ra : %d\n", cellParameter.vBat3Ra);
        //BAT_DBG("BAT3 Rb : %d\n", cellParameter.vBat3Rb);
        BAT_DBG("ADC1 pgain: %d\n", cellParameter.adc1_pgain);
        BAT_DBG("ADC1 ngain: %d\n", cellParameter.adc1_ngain);
        BAT_DBG("ADC1 pos. offset: %d\n", cellParameter.adc1_pos_offset);
        BAT_DBG("ADC2 gain: %d\n", cellParameter.adc2_gain);
        BAT_DBG("ADC2 offset: %d\n", cellParameter.adc2_offset);
        BAT_DBG("R ?? : %d\n", cellParameter.R);
        for (i=0; i<sizeof(cellParameter.rtTable)/sizeof(u16); i++) {
                BAT_DBG("RTTable[%02d]: %d\n", i, cellParameter.rtTable[i]);
        }
        for (i=0; i<sizeof(cellParameter.SOV_TABLE)/sizeof(u16); i++) {
                BAT_DBG("SOV Table[%02d]: %d\n", i, cellParameter.SOV_TABLE[i]/10);
        }
        BAT_DBG("ADC d1: %d\n", cellParameter.adc_d1);
        BAT_DBG("ADC d2: %d\n", cellParameter.adc_d2);
        BAT_DBG("ADC d3: %d\n", cellParameter.adc_d3);
        BAT_DBG("ADC d4: %d\n", cellParameter.adc_d4);
        BAT_DBG("ADC d5: %d\n", cellParameter.adc_d5);

        //Some registers
        BAT_DBG("[OTP_REG1] regAdc1Index25_200mV: 0x%02X\n", otp1Reg.regAdc1Index25_200mV);
        BAT_DBG("[OTP_REG1] regAdc1Index80_200mV: 0x%02X\n", otp1Reg.regAdc1Index80_200mV);
        BAT_DBG("[OTP_REG1] regAdc1Index25_100mV: 0x%02X\n", otp1Reg.regAdc1Index25_100mV);
        BAT_DBG("[OTP_REG1] regAdc1Index80_100mV: 0x%02X\n", otp1Reg.regAdc1Index80_100mV);
        BAT_DBG("[OTP_REG1] regAdc2Index25_200mV: 0x%02X\n", otp1Reg.regAdc2Index25_200mV);
        BAT_DBG("[OTP_REG1] regAdc2Index80_200mV: 0x%02X\n", otp1Reg.regAdc2Index80_200mV);
        BAT_DBG("[OTP_REG1] regAdc2Index25_100mV: 0x%02X\n", otp1Reg.regAdc2Index25_100mV);
        BAT_DBG("[OTP_REG1] regAdc2Index80_100mV: 0x%02X\n", otp1Reg.regAdc2Index80_100mV);

        BAT_DBG("[OTP_REG2] regITdeltaCode25_Bit1098: 0x%02X\n", otp2Reg.regITdeltaCode25_Bit1098);
        BAT_DBG("[OTP_REG2] regProductType: 0x%02X\n", otp2Reg.regProductType);
        BAT_DBG("[OTP_REG2] regRsvdF0: 0x%02X\n", otp2Reg.regRsvdF0);
        BAT_DBG("[OTP_REG2] regAdc2Index25_100mV_Bit4: 0x%02X\n", otp2Reg.regAdc2Index25_100mV_Bit4);
        BAT_DBG("[OTP_REG2] regAdc2Index80_100mV_Bit4: 0x%02X\n", otp2Reg.regAdc2Index80_100mV_Bit4);
        BAT_DBG("[OTP_REG2] regITdeltaCode25: 0x%02X\n", otp2Reg.regITdeltaCode25);
        BAT_DBG("[OTP_REG2] regITdeltaCode80_Bit1098: 0x%02X\n", otp2Reg.regITdeltaCode80_Bit1098);
        BAT_DBG("[OTP_REG2] regDevAddr_Bit210: 0x%02X\n", otp2Reg.regDevAddr_Bit210);
        BAT_DBG("[OTP_REG2] regDevAddr_Bit987: 0x%02X\n", otp2Reg.regDevAddr_Bit987);
        BAT_DBG("[OTP_REG2] regAdc1DeltaCode80_200mv_Bit98: 0x%02X\n", otp2Reg.regAdc1DeltaCode80_200mv_Bit98);
        BAT_DBG("[OTP_REG2] regBgrTune: 0x%02X\n", otp2Reg.regBgrTune);
        BAT_DBG("[OTP_REG2] regOscDeltaCode25: 0x%02X\n", otp2Reg.regOscDeltaCode25);
        BAT_DBG("[OTP_REG2] regOscDeltaCode80: 0x%02X\n", otp2Reg.regOscDeltaCode80);
        BAT_DBG("[OTP_REG2] regAdc1DeltaCode25_200mv: 0x%02X\n", otp2Reg.regAdc1DeltaCode25_200mv);
        BAT_DBG("[OTP_REG2] regAdc1DeltaCode80_200mv: 0x%02X\n", otp2Reg.regAdc1DeltaCode80_200mv);
        BAT_DBG("[OTP_REG2] regAdc1DeltaCode25_100mv: 0x%02X\n", otp2Reg.regAdc1DeltaCode25_100mv);
        BAT_DBG("[OTP_REG2] regAdc1DeltaCode80_100mv: 0x%02X\n", otp2Reg.regAdc1DeltaCode80_100mv);
        BAT_DBG("[OTP_REG2] regAdc1DeltaCode25_100mv_Bit8: 0x%02X\n", otp2Reg.regAdc1DeltaCode25_100mv_Bit8);
        BAT_DBG("[OTP_REG2] regAdc2DeltaCode25_100mv: 0x%02X\n", otp2Reg.regAdc2DeltaCode25_100mv);
        BAT_DBG("[OTP_REG2] regAdc1DeltaCode80_100mv_Bit8: 0x%02X\n", otp2Reg.regAdc1DeltaCode80_100mv_Bit8);
        BAT_DBG("[OTP_REG2] regAdc2DeltaCode80_100mv: 0x%02X\n", otp2Reg.regAdc2DeltaCode80_100mv);
        BAT_DBG("[OTP_REG2] regAdc2DeltaCode25_200mv: 0x%02X\n", otp2Reg.regAdc2DeltaCode25_200mv);
        BAT_DBG("[OTP_REG2] regAdc2DeltaCode80_200mv: 0x%02X\n", otp2Reg.regAdc2DeltaCode80_200mv);

        BAT_DBG("[OTP_REG3] regAdc2Index80_200mV_Bit4: 0x%02X\n", otp3Reg.regAdc2Index80_200mV_Bit4);
        BAT_DBG("[OTP_REG3] regAdc1Index80_100mV_Bit4: 0x%02X\n", otp3Reg.regAdc1Index80_100mV_Bit4);
        BAT_DBG("[OTP_REG3] regAdc1Index80_100mV_Bit4: 0x%02X\n", otp3Reg.regAdc1Index80_200mV_Bit4);
        BAT_DBG("[OTP_REG3] regAveIT25_LowByte_Bit76543: 0x%02X\n", otp3Reg.regAveIT25_LowByte_Bit76543);
        BAT_DBG("[OTP_REG3] regAveIT25_HighByte: 0x%02X\n", otp3Reg.regAveIT25_HighByte);           //20120923/jacky
        BAT_DBG("[OTP_REG3] regAdc1Index25_100mV_Bit4: 0x%02X\n", otp3Reg.regAdc1Index25_100mV_Bit4);
        BAT_DBG("[OTP_REG3] regAdc1Index25_200mV_Bit4: 0x%02X\n", otp3Reg.regAdc1Index25_200mV_Bit4);
        BAT_DBG("[OTP_REG3] regAveIT80_LowByte_Bit76543: 0x%02X\n", otp3Reg.regAveIT80_LowByte_Bit76543);
        BAT_DBG("[OTP_REG3] regAveIT80_HighByte: 0x%02X\n", otp3Reg.regAveIT80_HighByte);

        //more debug
        {
                u32 *pOTP1 = (u32 *)&otp1Reg;
                u32 *pOTP2 = (u32 *)&otp2Reg;
                u32 *pOTP3 = (u32 *)&otp3Reg;

                BAT_DBG("OTP1: 0x%08X\n", *pOTP1);
                BAT_DBG("OTP2: 0x%08X\n", pOTP2[0]);
                BAT_DBG("OTP3: 0x%08X\n", pOTP2[1]);
                BAT_DBG("OTP4: 0x%08X\n", pOTP2[2]);
                BAT_DBG("OTP5: 0x%08X\n", pOTP2[3]);
                BAT_DBG("OTP6: 0x%08X\n", *pOTP3);
        }


}

// Scan the pTable & find out the target range
u8 FindIndexRangeInTable(const u8 target, const u8* pTable, const u8 maxNum)
{
	u8 tempIndex=0;
	int i;

	if(target == 0) {
		tempIndex = maxNum-1;
		return(tempIndex | 0x80);
	}
	for(i=0 ; i<maxNum; i++) {		//
		tempIndex = i;
		if(target  ==  pTable[i]) {
			tempIndex |= 0x80;			//Bit7 =1 means match the TEMPERATURE_TABLE[], for example= 5/15/25/....
			break;
		}
		if(target > pTable[i])
			break;
	}
	return tempIndex;
}

//interpolation:
// x1 >  x >  x0
// f(x1) > f(x) > f(x0)
u16 Interpolation(const u16 fx1, const u16 fx0, const u8 x0, const u8 x, const u8 x1)
{
	if(fx1 > fx0)
		return fx0 + (x-x0)*(fx1 - fx0)/(x1 - x0);
	else //fx1 < fx < fx0,  x0 < x <x1
		return fx0 - (x-x0)*(fx0 - fx1)/(x1-x0);
}
//
//x>x1>x0
u16 ExtraPolation(const u16 fx1, const u16 fx0, const u8 x0,const u8 x, const u8 x1)
{
	return( fx1 + (x-x1)*(fx1-fx0)/(x1-x0) );
}
//
//=============================================================================
//unsigned char searchAndFindRoot(unsigned short *pTable, unsigned char MaxNums, unsigned short seed)
//Descriptions: search the tempC/c-rate/SOC
//========================================================== ==================
u16 CalculateTargetSOV(const short* pTargetTable, const short targetVoltage)
{
	u16  sovData = 0;
	int i;

	if(targetVoltage >= pTargetTable[0])  //> 100%
		return cellParameter.SOV_TABLE[0];
    
	if(targetVoltage <= pTargetTable[SOV_NUMS - 1])		//< 0%
		return cellParameter.SOV_TABLE[SOV_NUMS - 1];	//0 %
    
	for(i=1; i< SOV_NUMS; i++) {		//search Table & find out the target SOV
		u16 ocv1= pTargetTable[i-1];		//upper
		u16 ocv2= pTargetTable[i];		//lower
		if((targetVoltage <= ocv1) && (targetVoltage >= ocv2)) {
			u16 soc1 = cellParameter.SOV_TABLE[i-1];		//higher
			u16 soc2 = cellParameter.SOV_TABLE[i];		//Lower
			sovData = soc2 + ((soc1-soc2) * (targetVoltage - ocv2))/(ocv1 - ocv2);
			break;
		}
	}
	return(sovData);
}


//=============================================================================
//lookup table: CELL_NAC_TABLE[]
// use 0.1C to get the initQmax
// get the Battery LMD(Qmax)
// 2012/08/29
// update the LMD use 0.5C instead of 0.1C
//=============================================================================
//####2012/08/29#####
void FindInitQ(const u8 targetTemp)
{
 //  batteryInfo.LMD =cellParameter.ILMD;		//09262011, testing
   //batteryInfo.LMD = 992;							//testing 2012/07/13/testing
	u8 tempIndex;
	u16 QmaxLowT,QmaxHighT;			//
	u16 Qc1;
    
	tempIndex = FindIndexRangeInTable(deviceInfo.IT/10, TEMPERATURE_TABLE, sizeof(TEMPERATURE_TABLE));
    
	//T2 < T now  < T1
	//step 2, build the  END_CHECK_VOLTAGE TABLE, T1(
	if(tempIndex & 0x80)	//
	{
		tempIndex &= 0x7f;
		batteryInfo.LMD = cellTable.CELL_NAC_TABLE[tempIndex][2][0];			//2012/08/29 use 0.5C as initial LMD//####2012/08/29#####
	} else{
		if(tempIndex == 0)	//temperature > 75
		{
			batteryInfo.LMD = cellTable.CELL_NAC_TABLE[0][2][0];		// > 75 use 75//####2012/08/29#####
			Qc1 = cellTable.CELL_NAC_TABLE[0][2][0];		//####2012/08/29#####
		} else{		// 0< temp < 75,  1 <= tempIndex < TEMPERATURE_NUMS
			QmaxLowT = cellTable.CELL_NAC_TABLE[tempIndex][2][0];				//0.5c//####2012/08/29#####
			QmaxHighT = cellTable.CELL_NAC_TABLE[tempIndex-1][2][0];       //####2012/08/29#####
			//lowT < T < highT
			//QmaxLowT < Q < QmaxHighT
			batteryInfo.LMD = Interpolation(QmaxLowT, QmaxHighT,TEMPERATURE_TABLE[tempIndex-1],targetTemp,TEMPERATURE_TABLE[tempIndex]);
		}
	}

        UG31_LOGE("[%s]:LMD:%d\n", __func__, batteryInfo.LMD);

}

//to calculate the delta Q from SOV to the Voltage of the checkpoint
//2012/07/09 
void BuildDeltaQTable(void)
{
	u8 i;
	u8 vIndex = batteryInfo.vIndex;
	u8 tempIndex = batteryInfo.tempIndex;

	for(i=0; i<C_RATE_NUMS; i++) {	// 100 mA ~1C
		if(tempIndex & 0x80) {	//1T
			tempIndex &= 0x7f;
			batteryInfo.deltaQT[CRATE_TABLE[i]] = cellTable.CELL_NAC_TABLE[tempIndex][i][vIndex];
		} else{					//2T
			u16 deltaQ1 = cellTable.CELL_NAC_TABLE[tempIndex-1][i][vIndex] ;
			u16 deltaQ2 = cellTable.CELL_NAC_TABLE[tempIndex][i][vIndex] ;
			batteryInfo.deltaQT[CRATE_TABLE[i]] = Interpolation(deltaQ1, deltaQ2, TEMPERATURE_TABLE[tempIndex],
                                                                abs(deviceInfo.IT/10), TEMPERATURE_TABLE[tempIndex-1]);
		}
	}
	batteryInfo.deltaQT[3] = Interpolation(batteryInfo.deltaQT[5], batteryInfo.deltaQT[2],2,3,5);
	batteryInfo.deltaQT[4] = Interpolation(batteryInfo.deltaQT[5], batteryInfo.deltaQT[2],2,4,5);
	batteryInfo.deltaQT[6] = Interpolation(batteryInfo.deltaQT[7], batteryInfo.deltaQT[5],5,6,7);
	batteryInfo.deltaQT[8] = Interpolation(batteryInfo.deltaQT[10], batteryInfo.deltaQT[7],7,8,10);
	batteryInfo.deltaQT[9] = Interpolation(batteryInfo.deltaQT[10], batteryInfo.deltaQT[7],7,9,10);
    
	for(i=11;i<20;i++)		//calculate 1.1C 1.2C ~2C
	{
		batteryInfo.deltaQT[i] = ExtraPolation(batteryInfo.deltaQT[10], batteryInfo.deltaQT[7],7,i,10);
	}
}

//to count the % of V @low range of C-rate
//tempIndexd = 5, cIndex = 0x84;
u16 FindInitSOV(u8 tempIndex, u8 cIndex)
{
	u16 sovData;
	cIndex &= 0x7f;

	if(tempIndex & 0x80) {	//only 1 T
		tempIndex &= 0x7f;
		sovData =  CalculateTargetSOV(cellTable.CELL_VOLTAGE_TABLE[tempIndex][cIndex], batteryInfo.startCheckmV);
	} else{
		if(tempIndex == 0) {	//temperature > 75
			sovData = CalculateTargetSOV(cellTable.CELL_VOLTAGE_TABLE[0][cIndex], batteryInfo.startCheckmV);		//lower  C-rate
		} else {				// 0< temp < 75,  1 <= tempIndex < TEMPERATURE_NUMS
			u16 sov1 = CalculateTargetSOV(cellTable.CELL_VOLTAGE_TABLE[tempIndex][cIndex], batteryInfo.startCheckmV);		//lower  C-rate
			u16 sov2 = CalculateTargetSOV(cellTable.CELL_VOLTAGE_TABLE[tempIndex-1][cIndex], batteryInfo.startCheckmV);	//higher C-rate
			sovData = Interpolation(sov2, sov1, TEMPERATURE_TABLE[tempIndex], abs(deviceInfo.IT/10), TEMPERATURE_TABLE[tempIndex-1]);
		}
	}
	return sovData;		//return the % of V
}

//to calculate the time period from edv1(SOV 40%) to edv(3000 mV)
void BuildEmptyTimeTable(void)
{
	u8 i;
	u8 tempIndex = batteryInfo.tempIndex;
	for(i=0; i<C_RATE_NUMS; i++) {
		if(tempIndex & 0x80) {	//1T
			tempIndex &= 0x7f;
			batteryInfo.deltaTimeEDV[CRATE_TABLE[i]] = (u8)cellTable.CELL_NAC_TABLE[tempIndex][i][SOV_NUMS];
		} else {					//2T
			u16 deltaT1 = cellTable.CELL_NAC_TABLE[tempIndex-1][i][SOV_NUMS];
			u16 deltaT2 = cellTable.CELL_NAC_TABLE[tempIndex][i][SOV_NUMS];
			batteryInfo.deltaTimeEDV[CRATE_TABLE[i]] = (u8)Interpolation(deltaT1, deltaT2,
                                                                         TEMPERATURE_TABLE[tempIndex], abs(deviceInfo.IT/10), TEMPERATURE_TABLE[tempIndex-1]);
		}
	}
	batteryInfo.deltaTimeEDV[3] = (u8)Interpolation(batteryInfo.deltaTimeEDV[5], batteryInfo.deltaTimeEDV[2], 2, 3, 5);
	batteryInfo.deltaTimeEDV[4] = (u8)Interpolation(batteryInfo.deltaTimeEDV[5], batteryInfo.deltaTimeEDV[2], 2, 4, 5);
	batteryInfo.deltaTimeEDV[6] = (u8)Interpolation(batteryInfo.deltaTimeEDV[7], batteryInfo.deltaTimeEDV[5], 5, 6, 7);
	batteryInfo.deltaTimeEDV[8] = (u8)Interpolation(batteryInfo.deltaTimeEDV[10], batteryInfo.deltaTimeEDV[7], 7, 8, 10);
	batteryInfo.deltaTimeEDV[9] = (u8)Interpolation(batteryInfo.deltaTimeEDV[10], batteryInfo.deltaTimeEDV[7], 7, 9, 10);
    
	for(i=11;i<20;i++)		//calculate 1.1C 1.2C ~2C
	{
		batteryInfo.deltaTimeEDV[i] = (u8)ExtraPolation(batteryInfo.deltaTimeEDV[10], batteryInfo.deltaTimeEDV[7],7,i,10);
	}
}

//return the SOV CheckPoint Voltage Index
u8 FindSovCheckPoint(const u16 currentSOV)
{
	u8 sovIndex;
	int i;

	for(i=0; i < SOV_NUMS; i++) {		//
		sovIndex = i;
		if(currentSOV  ==  cellParameter.SOV_TABLE[i]) {
			sovIndex++;
			break;
		}
		if(currentSOV > cellParameter.SOV_TABLE[i])
			break;
	}
	return sovIndex;
}

void CalculateBatteryIndex(u8 targetCrate)
{
    
	batteryInfo.startCheckmV = deviceInfo.voltage_mV; //keep the start check voltage
	batteryInfo.startChargeData = deviceInfo.chargeData_mAh;
    
	batteryInfo.startChargeRegister = deviceInfo.chargeRegister;		//keep the
	batteryInfo.startAdcCounter = deviceInfo.AdcCounter;
	batteryInfo.startTime = GetTickCount();			//keep the SOV check start Time
    
	batteryInfo.tempIndex = FindIndexRangeInTable(abs(deviceInfo.IT/10), TEMPERATURE_TABLE, sizeof(TEMPERATURE_TABLE));	//find the Temperature Range
	batteryInfo.cIndex = FindIndexRangeInTable(targetCrate, CRATE_TABLE, sizeof(CRATE_TABLE));
	batteryInfo.SOV = FindInitSOV(batteryInfo.tempIndex, batteryInfo.cIndex);		   //count the SOV(%)
}

//function to calculate the average ADC code/Average current/ deltaQ
//return f/w calculate Charge/Discharge Q(maH) *10
//s16 CalculateFwDeltaQ(u32 startTime,u32 endTime)
s16 CalculateFwDeltaQ(u32 totalTime,s16* aveCurrent)
{
	s16 deltaQC = 0;		//coulomb counter's deltaQ
    
	s16 deltaOfCharge = 0;
	u16 totalConvertCnt = 0;
	s16 averageAdcCode = 0;
   
	deltaOfCharge = deviceInfo.chargeRegister - batteryInfo.startChargeRegister;				//s16
	totalConvertCnt =  deviceInfo.AdcCounter - batteryInfo.startAdcCounter;						//u16-u16
	if(totalConvertCnt)
	{
		averageAdcCode = (s16) (((s32)deltaOfCharge << 12)/totalConvertCnt);	//average current from  deltaQ/adc convet cnt
		averageAdcCode = CalibrateCapacityAdcCode(averageAdcCode);            //20120927/jacky
		*aveCurrent =(s16) ((((s32)averageAdcCode - cellParameter.adc1_pos_offset)*1000)/cellParameter.adc1_pgain);  //mA
		deltaQC =  (s16)div64_s64(((s64)*aveCurrent * (s64)totalTime), (1000*360));	//convert to 10*maH
// testing 
                UG31_LOGE("[%s]: Total time:%d ms\n", __func__, totalTime);
                UG31_LOGE("[%s]: Charge Register Start:%d,End:%d\n", __func__, batteryInfo.startChargeRegister,deviceInfo.chargeRegister);
                UG31_LOGE("[%s]: Adc counter:%d,End:%d\n", __func__, deviceInfo.AdcCounter,batteryInfo.startAdcCounter);
                UG31_LOGE("[%s]: Average Adc code:%d,current:%d DeltaQC;%d\n", __func__, averageAdcCode,*aveCurrent ,deltaQC);

                return(deltaQC);
        }
        return(0);
}

//return the new QT, & update the deltaQ table
//deltaQ now is the new QT Table
int  GetUpdataQmax(s16 QC,u8 targetCrate)
{
	u8 tempIndex = (batteryInfo.tempIndex & 0x7f);
	u8 cIndex = (batteryInfo.cIndex & 0x7f);
	u8 vIndex = (batteryInfo.vIndex);
	s16 deltaQ= 0;
	s16 oldQT = 0;
	u16 sov = batteryInfo.SOV;
	u16 deltaP = (sov - cellParameter.SOV_TABLE[vIndex]);				  //count the delta % of V   547 - 500 = 47(means 4.7%)
	u16 totalP = cellParameter.SOV_TABLE[vIndex-1] - cellParameter.SOV_TABLE[vIndex]; //count the total %        550 - 500
        if (!totalP) {
                BAT_DBG_E("%s error, totalP is 0.\n", __func__);
                return 0;
        }
	QC = abs(QC)*totalP/deltaP;			//

        UG31_LOGE("[%s] tempIndex-vIndex-cIndex-crate:%d-%d-%d-%d\n",
                        __func__, tempIndex,vIndex,cIndex,targetCrate);

	if(realData.deltaQ[tempIndex][cIndex][vIndex] == 0x0aaa) {		//empty marker
		oldQT = batteryInfo.deltaQT[targetCrate];								//1st time, get data from deltaQT Table
		deltaQ = QC - oldQT;
		realData.deltaQ[tempIndex][cIndex][vIndex] =  QC;     //save new QT

                UG31_LOGE("[%s]  1st update\n", __func__);

	} else{
		oldQT = realData.deltaQ[tempIndex][cIndex][vIndex];
		deltaQ = QC - oldQT;	
		realData.deltaQ[tempIndex][cIndex][vIndex] =  QC;     //save new QT

                UG31_LOGE("[%s]  2nd update\n", __func__);
	}
        UG31_LOGE("[%s]:QC-oldQT-newQT-updateQ:%d-%d-%d-%d\n", __func__, QC, oldQT, QC,deltaQ);

	return (deltaQ);
}

void UpdateQmax(u8 targetCrate,const u16 percentQ)
{
	s16 deltaQC = 0;			//coulomb counter's deltaQ
	s16 updateQ = 0;			//the delta Q between QC & QT
	s16 aveCurrent = 0;
	s32 deltaTime=0;

        //
        batteryInfo.cIndex = FindIndexRangeInTable(targetCrate, CRATE_TABLE, sizeof(CRATE_TABLE));

        deltaTime = GetTickCount()-batteryInfo.startTime;   //ms

        UG31_LOGE("[%s]:delta Time:%d ms\n", __func__, deltaTime);

        if(deltaTime/1000 >=  cellParameter.timeInterval*2)		//check deltaTime is reasonable?
        {	//each table should > 10 secs		
                deltaQC = CalculateFwDeltaQ(deltaTime,&aveCurrent);
                updateQ =  (abs(deltaQC)/10);
                if(abs(deltaQC) - updateQ*10 >=5)  //Round to integer
                        updateQ++;				//=>QC

                batteryInfo.LMD += GetUpdataQmax(updateQ,targetCrate);
        }
}

u8 calculateAverageCrateBetweenSOV(void)
{
	s16 aveCurrent = 0;
	s16 deltaOfCharge = 0;
	u8 target = 0;
	u16 averageCurrent = 0;
	u16 totalConvertCnt = 0;
	s16 averageAdcCode = 0;
	u16 range = 0;

	deltaOfCharge = deviceInfo.chargeRegister - batteryInfo.startChargeRegister;				//s16
	totalConvertCnt =  deviceInfo.AdcCounter - batteryInfo.startAdcCounter;						//u16-u16
	if(totalConvertCnt)
	{
		averageAdcCode = (s16) (((s32)deltaOfCharge << 12)/totalConvertCnt);	//average current from  deltaQ/adc convet cnt
		averageAdcCode = CalibrateCapacityAdcCode(averageAdcCode);            //20120927/jacky
          aveCurrent =(s16) ((((s32)averageAdcCode - cellParameter.adc1_pos_offset)*1000)/cellParameter.adc1_pgain);  //mA
	}
        UG31_LOGE("[%s]:average current between 2 SOV=%d\n", __func__, aveCurrent);
        UG31_LOGE("[%s]:Charge Register Start:%d,End:%d\n", __func__, batteryInfo.startChargeRegister,deviceInfo.chargeRegister);
        UG31_LOGE("[%s]:Adc counter:%d,End:%d\n", __func__, deviceInfo.AdcCounter,batteryInfo.startAdcCounter);

	range = cellParameter.ILMD/20;									//0.05C 
	averageCurrent = abs(aveCurrent);			//
	target = (averageCurrent*10)/cellParameter.ILMD;				//get the C-rate
	if(averageCurrent - ((target * cellParameter.ILMD)/10) >=range)	
		target++;
	if(target > 20) target = 20;		//maximum to support form 0.1c to  2C
	return(target);
}

void DoSovDeltaQCheck(u8 cRateNow,const u16 precentQ)
{
	u8 sovDeltaQ=0;
	u16 deltaV = 0;
	u8 averageCrate = 0;

	if(bStartCheckUpdateQ) {				// check start point
		CalculateBatteryIndex(cRateNow);
		if((batteryInfo.SOV <= cellParameter.SOV_TABLE[SOV_NUMS-2]) && (bFirstData ==true))		//SOV <=35 %
		{
			bFirstData = false;
			batteryInfo.edvRSOC = batteryInfo.RSOC*100;		//keep the RSOC
			batteryInfo.startEdvTimerFlag = 1;			//
			BuildEmptyTimeTable();						
           //	batteryInfo.emptyTimer = batteryInfo.deltaTimeEDV[cRateNow];    
		} else{  // 40% < SOV < 100%       */
			if((batteryInfo.SOV <= cellParameter.SOV_TABLE[0]) && (batteryInfo.SOV > cellParameter.SOV_TABLE[SOV_NUMS-2]) )	
			{
					batteryInfo.vIndex =  FindSovCheckPoint(batteryInfo.SOV);
					sovDeltaQ = ((cellParameter.SOV_TABLE[batteryInfo.vIndex-1]- cellParameter.SOV_TABLE[batteryInfo.vIndex])*6)/10;   //60%
					if((batteryInfo.SOV - cellParameter.SOV_TABLE[batteryInfo.vIndex]) >= sovDeltaQ) {  
						batteryInfo.endCheckmV = cellTable.CELL_VOLTAGE_TABLE[0][0][batteryInfo.vIndex];
						BuildDeltaQTable();		
						bStartCheckUpdateQ = false;			//clear bStartCheckUpdateQ flag

                                                UG31_LOGE("[%s]:SOVStart=%d\n", __func__, batteryInfo.SOV);
					}
			}
		}
        } else{	
                averageCrate =  calculateAverageCrateBetweenSOV();   //from SOV start count each average current

                UG31_LOGE("[AT-PM][%s]:current voltage = %d, check voltage = %d, average c-rate = %d\n", 
                                __func__, 
                                deviceInfo.voltage_mV, 
                                batteryInfo.endCheckmV,
                                averageCrate
                        );

                if(deviceInfo.voltage_mV <= batteryInfo.endCheckmV) 
                {
                        deltaV = batteryInfo.endCheckmV - deviceInfo.voltage_mV;		//check the deltaV is reasonable & acceptable

                        UG31_LOGE("[%s]:average c-rate between 2 SOV=%d\n", __func__, averageCrate);
                        UG31_LOGE("[%s]:delta V=%d,endV=%d\n", __func__, deltaV, batteryInfo.endCheckmV );

                        if((batteryInfo.SOV <= cellParameter.SOV_TABLE[SOV_NUMS-3]) &&(bFirstData == true)) {		
                                bFirstData = false;
                                batteryInfo.edvRSOC = batteryInfo.RSOC*100;		//keep the RSOC @ SOV40
                                batteryInfo.startEdvTimerFlag = 1;
                                BuildEmptyTimeTable();			//to calculate each c-rate the time period from SOV40% to EDV(3000mV)
                        }           
                        UpdateQmax(averageCrate,precentQ);         
                        bStartCheckUpdateQ = true;				//enable flag for next time check
                } 

        }
}

/*
 *refresh RSOC & NAC
 *when SOV <= 40%, use deltaTimeEDV Table & edvRSOC for calculating RSOC
*/
void UpdateRSOC_T(u8 cRateNow)
{
        //new update 08312011/Jacky
        u16 deltaRsoc;

        if(batteryInfo.deltaTimeEDV[cRateNow])
        {
                deltaRsoc = batteryInfo.edvRSOC/batteryInfo.deltaTimeEDV[cRateNow];	 //the subtraction value in each 5 seconds

                batteryInfo.deltaTimeEDV[cRateNow]--;
                if(batteryInfo.edvRSOC > deltaRsoc)
                        batteryInfo.edvRSOC -= deltaRsoc;
                else
                        batteryInfo.edvRSOC = 0;

                batteryInfo.RSOC = batteryInfo.edvRSOC/100;
                batteryInfo.NAC = (batteryInfo.LMD * batteryInfo.RSOC)/100;			//maH
        }
}

//=================================================================================================
//function to calculate average current
// Iav = Q/t
//=================================================================================================
u8 CalculateAverageCurrent(void)
{
	u8 target = 0;
	u16 averageCurrent = 0;
    
	u16 range = cellParameter.ILMD/20;				//0.5C 
	averageCurrent = abs(deviceInfo.fwCalAveCurrent_mA);
	target = (averageCurrent*10)/cellParameter.ILMD;	//get the C-rate
	if(averageCurrent - ((target * cellParameter.ILMD)/10) >=range)	//roud it
		target++;
	if(target > 20) target = 20;		//maximum to support form 0.1c to  2C
    
        UG31_LOGE("[%s]:Average Current = %d,Cal C-Rate*10 = %d\n", __func__, averageCurrent,target);
	return(target);
    
}

//
//batteryInfo.previousChargeData = deviceInfo.chargeData_mAh;	//when update RSOC, refresh previousChargeData, too.
//batteryInfo.previousAdcCounter = deviceInfo.AdcCounter;
//calculate deltaQ for RSOC
s16 CalculateRsocDeltaQ(u32 totalTime)
{
	s16 deltaQC = 0;		//coulomb counter's deltaQ
	s16 averageCurrent= 0;
	s16 deltaOfCharge = 0;
	//s16 CalCurrent = 0;
	u16 totalConvertCnt = 0;
	s16 averageAdcCode = 0;
  
	deltaOfCharge = deviceInfo.chargeRegister - batteryInfo.previousChargeRegister;		//s16
	totalConvertCnt =  deviceInfo.AdcCounter - batteryInfo.previousAdcCounter;			//u16-u16
    
	if(totalConvertCnt)
	{
		averageAdcCode = (s16) (((s32)deltaOfCharge << 12)/totalConvertCnt);	//average current from  deltaQ/adc convet cnt
			averageAdcCode = CalibrateCapacityAdcCode(averageAdcCode);            //20120927/jacky
                averageCurrent =(s16) ((((s32)averageAdcCode - cellParameter.adc1_pos_offset)*1000)/cellParameter.adc1_pgain);  //mA
                deltaQC =  (s16)div64_s64(((s64)averageCurrent * (s64)totalTime), (1000*360));	//convert to 10*maH
                UG31_LOGE("[%s] totalTime %d, deltaOfCharge %d, totalConvertCnt %d, " 
                                "averageAdcCode %d, averageCurrent %d, deltaQC %d\n",
                                __func__, totalTime, deltaOfCharge, totalConvertCnt,  
                                averageAdcCode, averageCurrent, deltaQC
                         );
                return(deltaQC);
        }

        return(0);
}
//
//=================================================================================================
// Discharging function
// batteryInfo.BatteryFullCounter: the counter of reaching TP voltage/ TP current of full charging.
// batteryInfo.previousChargeData: record the column counter(mAH) of discharging -> charging or charging -> discharging
// batteryInfo.StartChargeData: record the column counter(mAH) of the beginning of SOV voltage checking.
// batteryInfo.deltaTimeEDV: record the time between SOV 40% and EDVF(3000mV)
// batteryInfo.edvRSOC: record the RSOC value of SOV 40% as reference of calculating SOV 40% -> SOV 0%.
// offsetCharge: error value of the colomn counter of ST3100.
// percentQ: the maximum correctness of Qmax in each 5% SOV for Firmware.
//=================================================================================================
void CalculateDischarge(const int percentQ)
{
        u8 cRateNow = 0;
        s16 deltaQ;
        u16 newNAC = 0;					//2012/07/16
    
        batteryInfo.BatteryFullCounter = 0;		//the counter is for full charging, clear when discharging.
        batteryInfo.bTpConditionFlag = 0;		//clear Tp Flag
        if(batteryInfo.bChargeFlag == true) {	//charging -> discharging
                //	batteryInfo.previousChargeData = deviceInfo.chargeData_mAh;	//record the column counter 
                batteryInfo.previousChargeRegister= deviceInfo.chargeRegister;		//keep the Coulomb counter
                batteryInfo.previousAdcCounter = deviceInfo.AdcCounter;				//keep the ADc convert counter
                batteryInfo.previousTime = GetTickCount();
                batteryInfo.bChargeFlag = false;      
        } 
        cRateNow = CalculateAverageCurrent();		//Count the c-rate
        if(batteryInfo.startEdvTimerFlag == 1) {

                UG31_LOGE("[%s]:Start smooth RSOC\n", __func__);

                UpdateRSOC_T(cRateNow);
                batteryInfo.previousChargeRegister = deviceInfo.chargeRegister;
                //	batteryInfo.previousChargeData = deviceInfo.chargeData_mAh;	 //refresh previous charge data if rsoc update
                batteryInfo.previousAdcCounter = deviceInfo.AdcCounter;
                batteryInfo.previousTime = GetTickCount();				//10/13/2011/jacky
        } else{
                deltaQ = CalculateRsocDeltaQ(GetTickCount()-batteryInfo.previousTime);
                deltaQ = abs(deltaQ);		//the first digit after the decimal point.

                if(deviceInfo.voltage_mV <= cellParameter.edv1Voltage)		//2012/07/18/jacky	
                {
                        batteryInfo.RSOC = 0;
                        batteryInfo.NAC = 0;
                        batteryInfo.startChargeNAC = 0;
                        return;
                } 
                if(deltaQ)		//
                { 
                        u16 newDeltaQ = deltaQ + deltaQmaxInRsoc;
                        if(newDeltaQ >=  (batteryInfo.LMD/10)) {	//discharging of Q >= 1% * LMD
                                if(batteryInfo.RSOC)			//RSOC--
                                        batteryInfo.RSOC--;

                                batteryInfo.previousChargeRegister = deviceInfo.chargeRegister;		//08152011
                                batteryInfo.previousAdcCounter = deviceInfo.AdcCounter;
                                batteryInfo.previousTime = GetTickCount();
                                deltaQmaxInRsoc = newDeltaQ-batteryInfo.LMD/10;			//calculate the deltaQ each 1%
                        }
                        newNAC = (batteryInfo.LMD * batteryInfo.RSOC)/100; //maH
                        if(newNAC <= batteryInfo.NAC)   //for smooth NAC, 
                                batteryInfo.NAC = (batteryInfo.LMD * batteryInfo.RSOC)/100; //maH

                        UG31_LOGE("[%s]:deltaQ=%d,deltaQmaxInRsoc=%d,NAC=%d\n", __func__, deltaQ,deltaQmaxInRsoc,batteryInfo.NAC);

                        if(batteryInfo.RSOC) {
                                if(batteryInfo.bVoltageInverseFlag ==1) {	//discharging -> charging occure
                                        if(deviceInfo.voltage_mV <= batteryInfo.startChargeVoltage) {	//voltage <= the voltage of turning point
                                                bStartCheckUpdateQ = true; //set flag to start check SOV
                                                batteryInfo.bVoltageInverseFlag = 0;
                                                DoSovDeltaQCheck(cRateNow,percentQ);
                                        }
                                } else{
                                        DoSovDeltaQCheck(cRateNow,percentQ);
                                }
                        }
                }
        }
	batteryInfo.startChargeNAC = batteryInfo.NAC*10;				//08232011/Jacky update for NAC   

        UG31_LOGE("[%s]:RSOC = %d,LMD = %d\n", __func__, batteryInfo.RSOC,batteryInfo.LMD);

}

//=============================================================================
//unsigned char searchAndFindRoot(unsigned short *pTable, unsigned char MaxNums, unsigned short seed)
//Descriptions: search the tempC/c-rate/SOC
//=============================================================================
u8 CalculateTargetSOC(const short* pOcvTable, short targetVoltage)
{
	int i;
	u8 socData = 0;

	if(targetVoltage >= pOcvTable[0])	//> 100%
		return SOC_TABLE[0];
	if(targetVoltage <= pOcvTable[OCV_NUMS-1])		//< 0%
		return SOC_TABLE[OCV_NUMS-1];
	for(i=0; i<(OCV_NUMS-2); i++)		//search Table & find out the target SOC
	{
		if( (targetVoltage <= pOcvTable[i]) && (targetVoltage >= pOcvTable[i+1]) )
		{
			u8 soc1 = SOC_TABLE[i];		    //Higher
			u16 ocv1 = pOcvTable[i];
			u8 soc2 = SOC_TABLE[i+1];		//Lower
			u16 ocv2 = pOcvTable[i+1];
			socData = soc2 + ((soc1 - soc2)*(targetVoltage - ocv2))/(ocv1-ocv2);
			break;
		}
	}
	return socData;
}


//Calculate the SOC
//2012/08/29 jacky
//void FindInitSOC(const u16 targetCurrent, const u16 targetTemp, const u16 targetVoltage)
//####2012/08/29#####
void FindInitSOC(s16 targetCurrent, const u16 targetTemp, const u16 targetVoltage)
{
	u16 initSOC;
	u16 soc1,soc2;
	u8 targetCrate;
	u8 cIndex;
	//Step 1 find the Temperature t1,t2, tempIndex point to the lower or current  temperature range
	u8 tempIndex = FindIndexRangeInTable(targetTemp, TEMPERATURE_TABLE, sizeof(TEMPERATURE_TABLE));
	//Step 2, find the C rate

	targetCrate = abs(targetCurrent)*10/cellParameter.ILMD;
/*
	if(targetCurrent <= 0)		//discharge
		 cIndex = 0x81;
	else
		cIndex = 0x80;			//charge need to lookup charge table 
*/
	cIndex = 0x81;						  //20120925/jacky update 	
	if(targetCurrent > 300)   //charge status, to lookup the charge table
		cIndex = 0x80;
		
	if(tempIndex & 0x80) {	//only 1T
		tempIndex &= 0x7f;
		if(cIndex & 0x80) {  //1T 1C
			cIndex &= 0x7f;
			initSOC =  CalculateTargetSOC(cellTable.INIT_OCV[tempIndex][cIndex], targetVoltage);
		} else{	//1T 2C
			soc1 = CalculateTargetSOC(cellTable.INIT_OCV[tempIndex][cIndex], targetVoltage);		    //lower  C-rate
			soc2 = CalculateTargetSOC(cellTable.INIT_OCV[tempIndex][cIndex-1], targetVoltage);		//higher C-rate
			initSOC = Interpolation(soc1, soc2, CRATE_TABLE[cIndex-1], targetCrate, CRATE_TABLE[cIndex]);
		}
	} else { // 2T
                if(tempIndex == 0) {	//temperature > 75
                        soc1 = CalculateTargetSOC(cellTable.INIT_OCV[0][cIndex],targetVoltage);		//lower  C-rate
                        if(cIndex & 0x80) {
                                initSOC = soc1;
                        } else {
                                soc2 = CalculateTargetSOC(cellTable.INIT_OCV[0][cIndex-1], targetVoltage);		//higher C-rate
                                initSOC = Interpolation(soc1, soc2, CRATE_TABLE[cIndex-1], targetCrate, CRATE_TABLE[cIndex]);
                        }
                } else {				// 0< temp < 75,  1 <= tempIndex < TEMPERATURE_NUMS
                        if(cIndex & 0x80) { //2T 1C
                                cIndex &= 0x7f;
                                soc1 = CalculateTargetSOC(cellTable.INIT_OCV[tempIndex][cIndex], targetVoltage);		    //lower  temperature
                                soc2 = CalculateTargetSOC(cellTable.INIT_OCV[tempIndex-1][cIndex], targetVoltage);		//higher temperature
                                //lowT < T < highTi
                                //QmaxLowT < Q < QmaxHighT
                                initSOC = Interpolation(soc2, soc1, TEMPERATURE_TABLE[tempIndex], targetTemp, TEMPERATURE_TABLE[tempIndex-1]);
                        } else{ //2T 2C
                                int socLowT=0;
                                int socHighT=0;
                                //lower temperature,lower  C-rate
                                soc1 = CalculateTargetSOC(cellTable.INIT_OCV[tempIndex][cIndex], targetVoltage);		    
                                //lower temp,
                                soc2 = CalculateTargetSOC(cellTable.INIT_OCV[tempIndex][cIndex-1], targetVoltage);		
                                //Qc2 < Q < Qc1, C1<C <C2
                                socLowT = Interpolation(soc2, soc1, CRATE_TABLE[cIndex], targetCrate, CRATE_TABLE[cIndex-1]);
                                //higher temperature, lower c-rate
                                soc1 = CalculateTargetSOC(cellTable.INIT_OCV[tempIndex-1][cIndex], targetVoltage);			
                                //higher temp, higher c-rate
                                soc2 = CalculateTargetSOC(cellTable.INIT_OCV[tempIndex-1][cIndex-1], targetVoltage);		
                                socHighT = Interpolation(soc2, soc1, CRATE_TABLE[cIndex], targetCrate, CRATE_TABLE[cIndex-1]);
                                //lowT < T < highT
                                //QmaxLowT < Q < QmaxHighT
                                initSOC = Interpolation(socHighT, socLowT, TEMPERATURE_TABLE[tempIndex], targetTemp, TEMPERATURE_TABLE[tempIndex-1]);
                        }
                }
	}
	batteryInfo.RSOC = initSOC;
	batteryInfo.NAC = initSOC*batteryInfo.LMD/100;
        //   batteryInfo.NAC10 = 10* batteryInfo.NAC;					//2012/07/15
	batteryInfo.startChargeNAC = batteryInfo.NAC*10;			//08232011/Jacky
	batteryInfo.previousVoltageData = deviceInfo.voltage_mV;	//keep the last Voltage

        UG31_LOGE("[%s]:RSOC:%d,NAC:%d\n", __func__, batteryInfo.RSOC,batteryInfo.NAC);

}
        
void upiGG_CountInitQmax(void)
{
        FindInitQ(abs(deviceInfo.IT/10));
        FindInitSOC(deviceInfo.AveCurrent_mA, abs(deviceInfo.IT/10), deviceInfo.voltage_mV);

        batteryInfo.startChargeData = deviceInfo.chargeData_mAh;    //keep the coulomb counter
        //  batteryInfo.previousChargeData = deviceInfo.chargeData_mAh //to keep the charge data counter

        batteryInfo.startChargeRegister = deviceInfo.chargeRegister;//for deltaQ
        batteryInfo.previousChargeRegister = deviceInfo.chargeRegister;//

        batteryInfo.startAdcCounter = deviceInfo.AdcCounter;//for updateQmax()
        batteryInfo.previousAdcCounter = deviceInfo.AdcCounter;//to keep the adc convert counter
        batteryInfo.startTime = batteryInfo.previousTime = GetTickCount();      //

        //      deviceInfo.fwChargeData_mAH = 0;        //rest to 0
        deviceInfo.lastTime = 0;                        //initial
        bStartCheckUpdateQ = 1;                         //start to monitor deltaQ & V
        batteryInfo.bChargeFlag = false;        //default set is disCharge
}

void setAdc1Offset(void)
{
        //API_I2C_SingleRead(SECURITY,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_INTR_CTRL_C,&u8Temp);             //write Reg9d
        //      u8Temp &= 0xf3;
        //      API_I2C_SingleRead(NORMAL,isHighSpeedMode, isTenBitMode,REG_ADC1_OFFSET_LOW,(u8)cellParameter.adc1Offset)[]//write Reg58
        //      API_I2C_SingleWrite(NORMAL,isHighSpeedMode, isTenBitMode,REG_ADC1_OFFSET_HIGH,(u8)(cellParameter.adc1Offset >> 8));//write Reg59
        //
        API_I2C_SingleWrite(NORMAL,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, 
                        REG_ADC1_OFFSET_LOW,(u8)cellParameter.adc1Offset); //write Reg58
        API_I2C_SingleWrite(NORMAL,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, 
                        REG_ADC1_OFFSET_HIGH,(u8)(cellParameter.adc1Offset >> 8));//write Reg59
}

void setupAdcChopFunction(void)
{
        API_I2C_SingleWrite(SECURITY, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, 
                REG_FW_CTRL, cellParameter.chopCtrl);
}

//2012/08/29/Jacky
//reset uG31xx 
u8 ResetuG31xx(void)
{
		u8 temp;
		temp = 0x10;
		if(!API_I2C_Write(0,IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_CTRL_ADDR, 1,  &temp))
			return false;
		temp = 0x00;
		if(!API_I2C_Write(0, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_CTRL_ADDR, 1,  &temp))
			return false;
		return true;	
}

//<ASUS-Wade1_Chen20120910+>
void ConfigHwSetting(void)
{
        ug31_config.feature_flag = 0;

        if (otp2Reg.regProductType == 0x01) {
                ug31_config.feature_flag |= BIT_CALI_WITH_IT_OTP_PARA;
                ug31_config.feature_flag |= BIT_FEATURE_IT_CALIBRATION;
        } else {
                ug31_config.feature_flag = 0;

        }
}
//<ASUS-Wade1_Chen20120910->

GGSTATUS upiGG_Initial(GGBX_FILE_HEADER *pGGBXBuf)
{
        GGSTATUS status;

        // Select which GGB file we used.
        if (!pGGBXBuf) {
                return UG_READ_GGB_FAIL;
        }
        // Read GGB file contents to global variable
        if(!ReadGGBXFileToCellDataAndInitSetting(pGGBXBuf, &cellTable, &cellParameter)) {
                return UG_READ_GGB_FAIL;
        }
        // Init battery information global variable
        InitialBatteryInformation(&batteryInfo);
        bFirstData = true;
        // Init other global variable
        memset(&deviceInfo, 0x00, sizeof(GG_DEVICE_INFO));
        memset(&userReg, 0x00, sizeof(GG_USER_REG));                    //clear the register area
        memset(&user2Reg, 0x00, sizeof(GG_USER2_REG));

        // Initial I2C and Open HID
        if(!API_I2C_Init(cellParameter.clock, cellParameter.i2cAddress)) {
                return UG_INIT_I2C_FAIL;
        }
        totalCellNums = CheckIcType();          //check IC Type & get the cell Nums

		//SleepMiniSecond(2000);				////####2012/08/29#####, need wait 255 ms 
        //GetOtpSetting();
		// ResetuG31xx();				//2012/08/29/jacky
        InitGpioSetting();                      //init the GPIO1/2 functions(GPIO/ALARM/CBC_EN21/CBC_EN32)

        setupAdcChopFunction();			//11/24/2011
        setAdc1Offset();							//11/24/2011
        SetupAdc2Quene();
    //	SetupOscTuneJK();
        SetupAdc1Queue();

        // Active uG31xx by set REG_CTRL 0x02 and REG_MODE 0x10
        if(!ActiveuG31xx()) {
                return UG_ACTIVE_FAIL;
        }
//	GetOtpSetting();
        // Initial GG_REAL_DATA' deltaQ array to 0xaaa
        InitialDeltaQ(&realData);
	SleepMiniSecond(500);				////####2012/08/29#####, need wait 255 ms 
	GetOtpSetting();
        ConfigHwSetting(); //<ASUS-Wade1_Chen20120910+>
        status = upiGG_ReadDeviceInfo(&deviceInfo);
        if (status == UG_READ_DEVICE_INFO_FAIL) {
                return UG_READ_DEVICE_INFO_FAIL;
        }

	upiGG_CountInitQmax();

        //dumpInfo();

        BAT_DBG("ug31xx gauge init done.\n");

	return UG_INIT_SUCCESS;
}
/**
 * @brief Code2MeasuredValue
 *
 *  Convert calibrated code to measured value
 *
 * @para  pAdcCaliData  pointer of external GG_ADC_CALI_INFO_TYPE data
 * @return  Measured voltage
 *///####2012/08/29#####
s64 Code2MeasuredValue(CaliAdcDataType adcCaliData)
{
        s64 MeasValue;
        u16 DeltaInputVolt;
        u16 DeltaIdealCode;

        DeltaInputVolt = adcCaliData.InputVoltHigh - adcCaliData.InputVoltLow;
        DeltaIdealCode = adcCaliData.IdealCodeHigh - adcCaliData.IdealCodeLow;

        MeasValue = (s64)adcCaliData.CaliCode;
//<ASUS-WAD+>        
//      MeasValue = MeasValue*DeltaInputVolt/DeltaIdealCode + adcCaliData.InputVoltLow;
        MeasValue = div64_s64( MeasValue * (s64)DeltaInputVolt, DeltaIdealCode ) + 
                        adcCaliData.InputVoltLow;	//convert to 10*maH
//<ASUS-WAD->

        return (MeasValue);
}

/**
 * @brief upiGG_VerifyAdcCalibration
 *
 *  Verify ADC calibration algorithm
 *
 * @para  pAdcCaliData  pointer of external GG_ADC_CALI_INFO_TYPE data
 * @return  GGSTATUS
 *///####2012/08/29#####
GGSTATUS upiGG_VerifyAdcCalibration(GG_ADC_CALI_INFO_TYPE *pAdcCaliData)
{
        GGSTATUS Status = UG_READ_DEVICE_INFO_SUCCESS;

        if(upiGG_ReadAllRegister(&userReg, &user2Reg) != UG_READ_REG_SUCCESS)  //found data error
        {
                Status = UG_READ_ADC_FAIL;
                return (Status);
        }

        /// [AT] : 1. Get current internal temperature ; 07/12/2012
        pAdcCaliData->CurrITCode = userReg.regITAve >> 1;

        /// [AT] : 2. Calibrate current ; 07/12/2012
        pAdcCaliData->RawCodeCurrent = userReg.regCurrentAve;
        userReg.regCurrentAve = (s16)CalibrationAdc(CALI_ADC_1, pAdcCaliData->CurrITCode, (s32)pAdcCaliData->RawCodeCurrent, &pAdcCaliData->Current);
        pAdcCaliData->MeasCurr = (u16)Code2MeasuredValue(pAdcCaliData->Current);

        /// [AT] : 3. Calibrate VCell1 ; 07/12/2012
        pAdcCaliData->RawCodeVCell1 = userReg.regVbat1Ave;
        userReg.regVbat1Ave = (s16)CalibrationAdc(CALI_ADC_2, pAdcCaliData->CurrITCode, (s32)pAdcCaliData->RawCodeVCell1, &pAdcCaliData->VCell1);
        pAdcCaliData->MeasVCell1 = (u16)Code2MeasuredValue(pAdcCaliData->VCell1);

        /// [AT] : 4. Calibrate VCell2 ; 07/12/2012
        pAdcCaliData->RawCodeVCell2 = user2Reg.regVbat2Ave;
        user2Reg.regVbat2Ave = (s16)CalibrationAdc(CALI_ADC_2, pAdcCaliData->CurrITCode, (s32)pAdcCaliData->RawCodeVCell2, &pAdcCaliData->VCell2);
        pAdcCaliData->MeasVCell2 = (u16)Code2MeasuredValue(pAdcCaliData->VCell2);

        /// [AT] : 5. Calibrate VCell3 ; 07/12/2012
        pAdcCaliData->RawCodeVCell3 = user2Reg.regVbat3Ave;
        user2Reg.regVbat3Ave = (s16)CalibrationAdc(CALI_ADC_2, pAdcCaliData->CurrITCode, (s32)pAdcCaliData->RawCodeVCell3, &pAdcCaliData->VCell3);
        pAdcCaliData->MeasVCell3 = (u16)Code2MeasuredValue(pAdcCaliData->VCell3);

        /// [AT] : 6. Calibrate external temperature ; 07/12/2012
        pAdcCaliData->RawCodeExtTemp = userReg.regETAve;
        userReg.regETAve = (u16)CalibrationAdc(CALI_ADC_1, pAdcCaliData->CurrITCode, (s32)pAdcCaliData->RawCodeExtTemp, &pAdcCaliData->ExtTemp);
        pAdcCaliData->MeasExtTemp = (u16)Code2MeasuredValue(pAdcCaliData->ExtTemp);

        return (Status);
}

//function to handle the charge Full conditions
void  HandleChargeFull(void)
{
	if(batteryInfo.bTpConditionFlag ==1)
	{
		if(batteryInfo.tpTimer)   //smooth RSOC to 100 %
		{
			batteryInfo.RSOC++;
			if(batteryInfo.RSOC >= 100)
				batteryInfo.RSOC = 100;
			batteryInfo.tpTimer--;
			batteryInfo.NAC = (batteryInfo.LMD *batteryInfo.RSOC)/100;
		//	batteryInfo.NAC10 = 10* batteryInfo.NAC;					//2012/07/15
		}
	} else{
		batteryInfo.BatteryFullCounter++;
		if(batteryInfo.BatteryFullCounter >=cellParameter.TPTime ) {		//update from 2 to 5 to avoid current unstable,Jacky 05/17/20
			batteryInfo.BatteryFullCounter = 0;
			batteryInfo.bTpConditionFlag = 1;			//TP occurs
			batteryInfo.bVoltageInverseFlag = 0;		//charge full,
            
			if(batteryInfo.NAC >= batteryInfo.LMD) {
				batteryInfo.NAC = batteryInfo.LMD;
		//		batteryInfo.NAC10 = 10* batteryInfo.NAC;					//2012/07/15

                                UG31_LOGE("[%s]:NAC >= LMD,ROSC = %d\n", __func__, batteryInfo.RSOC);

                                if(batteryInfo.RSOC == 99)
                                {
                                        batteryInfo.RSOC = 100;
                                        batteryInfo.tpTimer = 0;   //set 0
                                } else{   //match TP condition && start smooth the RSOC to 100%
                                        batteryInfo.tpTimer = 100- batteryInfo.RSOC; //e.g 100 - 94=6%
				}
			} else { //NAC < LMD
				batteryInfo.tpTimer = 100- batteryInfo.RSOC; //e.g 100 - 94=6%

                                UG31_LOGE("[%s]:NAC < LMD\n", __func__);

			}
            
		}
	}
}

/*
Step1, Save cell table to 
Step2, write RSOC/NAC/LMD/Time Tag to uG31xx register 0x20~0x27
Step3, reset GG Coulomb counter
Register 0x01 =0x02
Step4, enter suspend
*/
//####2012/08/29#####
GGSTATUS upiGG_PreSuspend(void)
{
	GGSTATUS Status = UG_READ_DEVICE_INFO_SUCCESS;

	u8 temp;
	s32 curretTime=0;
	GG_SUSPEND_INFO  suspendData;

	suspendData.currentTime = GetTickCount();
	suspendData.LMD = batteryInfo.LMD;
	suspendData.NAC = batteryInfo.NAC;
	suspendData.RSOC = batteryInfo.RSOC;

	API_I2C_Write(0, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_RAM_AREA, sizeof(GG_SUSPEND_INFO), (u8* )&suspendData.LMD);			//write Reg9C
	temp = 0x02;			//set GG_RST = 0, reset Coulomb counter
	API_I2C_Write(0, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_CTRL_ADDR, 1,  &temp);

	UG31_LOGE("[%s]:Reset Coulumb counter\n", __func__);

	return(Status);
}

//=================================================================================================
// batteryInfo.bVoltageInverseFlag:
// batteryInfo.startChargeData:
// batteryInfo.startChargeVoltage:
// batteryInfo.startEdvTimerFlag:
//=================================================================================================
void CalculateCharge(void)
{
	s16 realCurrent;
	u32 deltaTimeMs = 0;
	u32 deltaQ;
	u16 totalConvertCnt = 0;
	s16 averageAdcCode = 0;
    
	// If charge flag is false, then record previous charge data and re-assign charge flag
	if(batteryInfo.bChargeFlag == false) {	//
		if(batteryInfo.bVoltageInverseFlag == 0)	//
		{
			batteryInfo.previousChargeRegister = deviceInfo.chargeRegister;		//08152011				//08152011
			batteryInfo.startChargeVoltage = batteryInfo.previousVoltageData;		
			batteryInfo.previousAdcCounter = deviceInfo.AdcCounter;			
			batteryInfo.previousTime = GetTickCount();
			batteryInfo.bVoltageInverseFlag = 1;

                        UG31_LOGE("Start charge voltage: %dmV\n", batteryInfo.startChargeVoltage);


		}
		batteryInfo.bChargeFlag = true; 
	}
	batteryInfo.startEdvTimerFlag = 0; //
	bStartCheckUpdateQ = 1;		       //start to monitor deltaQ & V
    
        if(batteryInfo.RSOC == 100)   //2012/05/31
                return;

        if((deviceInfo.voltage_mV >= cellParameter.TPVoltage) && (deviceInfo.AveCurrent_mA < cellParameter.TPCurrent))
        {
                HandleChargeFull();        
        } else {   //battery not full
                deltaQ = deviceInfo.chargeRegister - batteryInfo.previousChargeRegister;	//08152011
                //	totalConvertCnt = abs(batteryInfo.previousAdcCounter -deviceInfo.AdcCounter);
                totalConvertCnt =  deviceInfo.AdcCounter -batteryInfo.previousAdcCounter ;
                if((deltaQ != 0)  && (totalConvertCnt > 0) )
                {
                        averageAdcCode =  ((s32)deltaQ << 12)/totalConvertCnt;	//average current from  deltaQ/adc convet cnt
                        realCurrent = (((s32)averageAdcCode - cellParameter.adc1_pos_offset)*1000)/cellParameter.adc1_pgain;  //mA
                        deltaTimeMs = GetTickCount()-batteryInfo.previousTime;
                        deltaQ =  (u16)div64_s64((s64)(abs(realCurrent) * deltaTimeMs), (1000*360));	//convert to 10*maH
                } else{
                        deltaQ = 0;
                }
                //     deltaQ = deltaQ + deltaQmaxInCharge;
                if(deltaQ >= 10) // deltaQ > 1 maH
                {
                        batteryInfo.previousChargeRegister = deviceInfo.chargeRegister;		//update the last
                        batteryInfo.previousAdcCounter = deviceInfo.AdcCounter;
                        batteryInfo.previousTime = GetTickCount();
                        deviceInfo.fwChargeData_mAH += deltaQ;					//to keep the f/w calculated charge data(s16) maH
                        batteryInfo.startChargeNAC+= deltaQ;							//startChargeNAC = 10*NAC
                        batteryInfo.NAC = (batteryInfo.startChargeNAC/10);
                } //else {
                //	deltaQmaxInCharge = deltaQ;
                //	}

                batteryInfo.BatteryFullCounter = 0;
                if(batteryInfo.NAC >= batteryInfo.LMD) {
                        batteryInfo.NAC = batteryInfo.LMD;			//charge full, the NAC = LMD
                        batteryInfo.RSOC = 99;								//voltage still < TP voltage, set RSOC = 99%
                        batteryInfo.bVoltageInverseFlag = 0;			//charge full,
                } else {
                        batteryInfo.RSOC = (batteryInfo.NAC*100)/batteryInfo.LMD;		
                }
        }
}


//====================================================
//API Call to get the Battery Capacity
// charge full condition:
//	if((Iav <TP current) && (Voltage >= TP Voltage))
//====================================================
void upiGG_ReadCapacity(GG_CAPACITY *pExtCapacity)
{

	if(abs(deviceInfo.AveCurrent_mA) > cellParameter.standbyCurrent) {
		if(deviceInfo.AveCurrent_mA < 0) { //discharge & current > standby current
                        UG31_LOGE("***DisCharge***[%s]:RSOC =%d,Qmax = %d maH,V = %dmv, C=%dmA\n",
                                        __func__, batteryInfo.RSOC, batteryInfo.LMD, deviceInfo.voltage_mV, deviceInfo.AveCurrent_mA
                                 );

			CalculateDischarge(cellParameter.maxDeltaQ);
			batteryInfo.bTpConditionFlag = 0;
		} else {
                        UG31_LOGE("###Charge###[%s]:RSOC =%d,Qmax = %d maH,V = %dmv, C=%dmA\n",
                                        __func__, batteryInfo.RSOC,batteryInfo.LMD,deviceInfo.voltage_mV,deviceInfo.AveCurrent_mA
                                 );
			bStartCheckUpdateQ = true;				//enable flag for next time check
			CalculateCharge();
		}
	}  else {	//current < standBy current
        UG31_LOGE("@@@Under StandBy current@@@[%s]\n", __func__);
		 
		if(deviceInfo.AveCurrent_mA< 0) { //Discharge (small current discharging)
			batteryInfo.bChargeFlag = false;
		} else {  //Charge (small current)
			if(batteryInfo.bChargeFlag == false)		//First discharge to charge
			{
				batteryInfo.startChargeVoltage = batteryInfo.previousVoltageData;	// Record the last voltage before discharge to charge
				batteryInfo.bVoltageInverseFlag = 1;
                bStartCheckUpdateQ = true;	        //enable flag for next time check

               UG31_LOGE("Start charging voltage: %dmV\n", batteryInfo.startChargeVoltage);
               UG31_LOGE("Current V:%dmV\n", deviceInfo.voltage_mV);
			}
			batteryInfo.bChargeFlag = true;
		}
	}
    
	batteryInfo.previousVoltageData = deviceInfo.voltage_mV; //keep the current Voltage
	// Output result by assign value from global variable
	pExtCapacity->LMD = batteryInfo.LMD;
	pExtCapacity->NAC = batteryInfo.NAC;
	pExtCapacity->RSOC = batteryInfo.RSOC;
}

//system wakeup
// to read back the preSuspend information from uG31xx RAM area
// re-calculate the deltaQmax( the charge/discharge) during the suspend time
//####2012/08/29#####
GGSTATUS upiGG_Wakeup(void)
{
	GGSTATUS Status = UG_READ_DEVICE_INFO_SUCCESS;

	s16 deltaQC = 0;							//coulomb counter's deltaQ
	s16 averageCurrent= 0;
	s16 deltaOfCharge = 0;
	u16 totalConvertCnt = 0;
	s16 averageAdcCode = 0;
	u8 temp;
	s32 totalTime;
	GG_SUSPEND_INFO  suspendData;

	//read back from ug31xx RAM area 
	API_I2C_Read(0, IS_HIGH_SPEED_MODE, IS_TEN_BIT_MODE, REG_RAM_AREA, sizeof(GG_SUSPEND_INFO),  (u8* )&suspendData);

	totalTime = GetTickCount() - suspendData.currentTime;				//count the delta Time
	deltaOfCharge = deviceInfo.chargeRegister;		//s16
	totalConvertCnt =  deviceInfo.AdcCounter;			//u16-u16
	if(totalConvertCnt)
	{
		averageAdcCode = (s16) (((s32)deltaOfCharge << 12)/totalConvertCnt);	//average current from  deltaQ/adc convet cnt
		averageAdcCode = CalibrateCapacityAdcCode(averageAdcCode);            //20120927/jacky
		averageCurrent =(s16) ((((s32)averageAdcCode - cellParameter.adc1_pos_offset)*1000)/cellParameter.adc1_pgain);  //mA
//<ASUS-WAD+>                
//		deltaQC = (s16)((( (s64)averageCurrent * totalTime)/1000)/360);			//convert to 10*maH
                deltaQC = (s16)div64_s64( (s64)averageCurrent * (s64)totalTime, (1000*360));        
//<ASUS-WAD->
	} else {
		deltaQC = 0;
	}
	batteryInfo.NAC = suspendData.NAC + deltaQC/10;			//re-calculate the New NAC
	batteryInfo.LMD = suspendData.LMD;									//
	batteryInfo.RSOC = 	(batteryInfo.NAC * 100)/batteryInfo.LMD;		//re-calculate the new RSOC

	UG31_LOGE("[%s]:suspend time:%d ms, deltaQ=%dmAh(*10)", __func__, totalTime, deltaQC);
	UG31_LOGE("[%s]:suspend time:%d ms, deltaQ=%dmAh(*10)", __func__, totalTime, deltaQC);
	UG31_LOGE("[%s]: suspend NAC :%d suspend RSOC %d\n"
                        "wake_up NAC=%d wake_up RSOC=%d)", 
                __func__, 
                suspendData.RSOC,suspendData.NAC,
                batteryInfo.RSOC,batteryInfo.NAC
        );
	return(Status);
}

