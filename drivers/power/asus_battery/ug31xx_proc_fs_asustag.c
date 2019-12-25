/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>

#include "asus_battery.h"
#include "ug31xx_reg_def.h"
#include "ug31xx_i2c.h"
#include "ug31xx_typeDefine.h"
#include "ug31xx_api.h"
#include "ug31xx_proc_fs.h"
#include "ug31xx_battery_core.h"

extern struct battery_dev_info ug31_dev_info;
extern struct mutex ug31_dev_info_mutex;

extern  CELL_TABLE      cellTable;
extern  CELL_PARAMETER  cellParameter;
extern  GG_BATTERY_INFO         batteryInfo;
extern  GG_DEVICE_INFO          deviceInfo;
extern struct mutex  ug31_gauge_mutex;
extern const u8  TEMPERATURE_TABLE[TEMPERATURE_NUMS];
extern const u8  CRATE_TABLE[C_RATE_NUMS]; 
extern u8  SOC_TABLE[OCV_NUMS] ;
extern const u8  OCV_CRATE_TABLE[2] ;
extern GG_OTP1_REG             otp1Reg;             //otp block 1 0xe0~0xe3
extern GG_OTP2_REG             otp2Reg;             //otp block 2 0xf0~0xff
extern GG_OTP3_REG             otp3Reg;             //otp block 3 0x70~0x73

static char  proc_buf[8*1024];
static u8 ggb_file_buf[8*1024]; //save input buffer
static int return_val;

int ug31xx_proc_protocol_read(char *page, char **start, off_t off, int count, int *eof, void *date)
{
        int len; 
        len = sprintf(page, "0x%08X\n", return_val);

        return len;
}

int ug31xx_proc_protocol_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
        int reg_off=0, value=0, ret=0;
        char mode, len;
        int byte_len=ONE_BYTE;
        struct i2c_client *client=NULL;

        mutex_lock(&ug31_dev_info_mutex);    
        client = ug31_dev_info.i2c;
        mutex_unlock(&ug31_dev_info_mutex);

        dev_info(&client->dev, "%s\n", __func__);

        if (count > sizeof(proc_buf)) {
                dev_err(&client->dev, "data error\n");
                return_val = -EINVAL;
                return -EINVAL;
        }

        if (copy_from_user(proc_buf, buffer, count)) {
                dev_err(&client->dev, "read data from user space error\n");
                return_val = -EFAULT;
                return -EFAULT;
        }

        sscanf(proc_buf, "%c%c %02X %02X\n", &mode, &len, &reg_off, &value);
        if (len == 'b') {
                byte_len = 1;
        }else if (len == 'w') {
                byte_len = 2;
        }else {
                BAT_DBG_E("Error operation in length.\n");
                return_val = -EFAULT;
                return -EFAULT;
        }

        if (mode == 'w') {
                dev_info(&client->dev, "%s write reg:%02X, value: %04X, len: %d\n", 
                                __func__, reg_off, value, byte_len);
                ret = _API_I2C_Write(reg_off, byte_len, (u8 *)&value);

        } else if (mode == 'r') {
                ret = _API_I2C_Read(reg_off, byte_len, (u8 *)&value);
                dev_info(&client->dev, "%s read reg:%02X, value: %04X, len: %d\n", 
                                __func__, reg_off, value, byte_len);
                return_val = value;
        }else {
                dev_info(&client->dev, "mode fail\n");
        }

        if (!ret) {
                dev_err(&client->dev, "i2c operation  fail. %d\n", ret);
                return_val = -EIO;
                return -EIO;
        }

        memset(proc_buf, 0x00, sizeof(proc_buf));

        return count;
}

struct reg_entry {
        u32 reg_off;
        u32 type;
        u32 value;
};

typedef enum _dump_type {
        TYPE_BYTE_HEX=1,
        TYPE_WORD_HEX,
        TYPE_BYTE_INT,
        TYPE_WORD_INT,
}dump_type;
struct reg_entry dump_reg[] = {
        {0x02, TYPE_BYTE_HEX, -1},
        {0x03, TYPE_BYTE_HEX, -1},
        {0x04, TYPE_BYTE_HEX, -1},
        {0x05, TYPE_BYTE_HEX, -1},
        {0x0c, TYPE_BYTE_HEX, -1},
        {0x0d, TYPE_BYTE_HEX, -1},
        {0x44, TYPE_BYTE_HEX, -1},
        {0x45, TYPE_BYTE_HEX, -1},
        {0x8f, TYPE_BYTE_HEX, -1},
        {0x90, TYPE_BYTE_HEX, -1},
        {0x91, TYPE_BYTE_HEX, -1},
        {0x9a, TYPE_BYTE_HEX, -1},
        {0x9b, TYPE_BYTE_HEX, -1},
        {0x9c, TYPE_BYTE_HEX, -1},
        {0xc1, TYPE_BYTE_HEX, -1},
        {0xc2, TYPE_BYTE_HEX, -1},
        {0xc3, TYPE_BYTE_HEX, -1},
        {0xc4, TYPE_BYTE_HEX, -1},
        {0xc5, TYPE_BYTE_HEX, -1},
        {0xc6, TYPE_BYTE_HEX, -1},
        {0xc7, TYPE_BYTE_HEX, -1},
        {0xc8, TYPE_BYTE_HEX, -1},
        {0xc9, TYPE_BYTE_HEX, -1},
        {0xca, TYPE_BYTE_HEX, -1},
        {0xcb, TYPE_BYTE_HEX, -1},
};
int ug31xx_proc_reg_dump_read(char *page, char **start, off_t off, int count, int *eof, void *date)
{
        int len=0, local_len; 
        int i=0;

        for (i=0; i<sizeof(dump_reg)/sizeof(struct reg_entry); i++) {
                dump_reg[i].value = 0;
                switch(dump_reg[i].type) {
                        case TYPE_BYTE_INT:                                
                                if (!_API_I2C_Read(dump_reg[i].reg_off, 1, (u8 *)&dump_reg[i].value)) {
                                        dump_reg[i].value = -1;
                                        continue;
                                }
                                local_len = sprintf(page, "0x%02X %d\n", dump_reg[i].reg_off, dump_reg[i].value);
                                page += local_len;
                                len += local_len;
                                break;

                        case TYPE_BYTE_HEX:
                                if (!_API_I2C_Read(dump_reg[i].reg_off, 1, (u8 *)&dump_reg[i].value)) {
                                        dump_reg[i].value = -1;
                                        continue;
                                }
                                local_len = sprintf(page, "0x%02X 0x%02X\n", dump_reg[i].reg_off, dump_reg[i].value);
                                page += local_len;
                                len += local_len;
                                break;
                        case TYPE_WORD_INT:
                                if (!_API_I2C_Read(dump_reg[i].reg_off, 2, (u8 *)&dump_reg[i].value)) {
                                        dump_reg[i].value = -1;
                                        continue;
                                }
                                local_len = sprintf(page, "0x%02X 0x%d\n", dump_reg[i].reg_off, dump_reg[i].value);
                                page += local_len;
                                len += local_len;
                                break;
                        case TYPE_WORD_HEX:
                                if (!_API_I2C_Read(dump_reg[i].reg_off, 2, (u8 *)&dump_reg[i].value)) {
                                        dump_reg[i].value = -1;
                                        continue;
                                }
                                local_len = sprintf(page, "0x%02X 0x%04X\n", dump_reg[i].reg_off, dump_reg[i].value);
                                page += local_len;
                                len += local_len;
                                break;
                        default:
                                BAT_DBG_E("%s, unsupport type.\n", __func__);
                                return 0;
                }
        }

        return len;
}

int ug31xx_proc_reg_dump_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
        return count;
}

#define UG_DUMP(...) \
do { \
        local_len = sprintf(page, __VA_ARGS__); \
        len += local_len; \
        page += local_len; \
}while(0);



ssize_t ug31_proc_ggb_write(struct file *file, const char *buffer, size_t count, loff_t *pos)
{
        struct i2c_client *client=NULL;

        mutex_lock(&ug31_dev_info_mutex);    
        client = ug31_dev_info.i2c;
        mutex_unlock(&ug31_dev_info_mutex);

        if (*pos + count >= sizeof(proc_buf)) {
                dev_err(&client->dev, "write op error!! %llu, %d\n", *pos, count);
                return -EINVAL;
        }

        if (copy_from_user(&proc_buf[*pos], buffer, count)) {
                dev_err(&client->dev, "read data from user space error\n");
                return -EFAULT;
        }

        //printk("copy %llu, %d", *pos, count);
        //for (i=0; i<0x20; i++) {
        //        printk("0x%02X, ", proc_buf[i]);
        //}
        //printk("\n");
        *pos += count;

        return count;
}

int ug31_proc_ggb_release(struct inode *ino, struct file *fp)
{
        int ret=0;
        struct i2c_client *client=NULL;
        int i;

        mutex_lock(&ug31_dev_info_mutex);    
        client = ug31_dev_info.i2c;
        mutex_unlock(&ug31_dev_info_mutex);

        for (i=0; i<16; i++) 
                ret += proc_buf[i];

        if (!ret) {
                //No data need to write.
                return 0;
        }

        mutex_lock(&ug31_gauge_mutex);    
        memcpy(ggb_file_buf, proc_buf, sizeof(ggb_file_buf));
        ret = upiGG_Initial((struct GGBX_FILE_HEADER *)ggb_file_buf);
        mutex_unlock(&ug31_gauge_mutex);    

        memset(proc_buf, 0x00, sizeof(proc_buf));

        if (!ret) {
                dev_err(&client->dev, "Re-inital upi driver error.\n");
                return -EINVAL;
        }

        return seq_release(ino, fp);

}

int ug31xx_proc_info_dump_read(char *page, char **start, off_t off, int count, int *eof, void *date)
{

        int len=0, local_len=0;

        mutex_lock(&ug31_gauge_mutex);    

        UG_DUMP("LMD: %d\n", batteryInfo.LMD);
        UG_DUMP("NAC: %d\n", batteryInfo.NAC);
        UG_DUMP("RSOC: %d\n", batteryInfo.RSOC);

        UG_DUMP("voltage(mV): %d\n", deviceInfo.voltage_mV);
        UG_DUMP("average_current(mA): %d\n", deviceInfo.AveCurrent_mA);
        UG_DUMP("IT: %d\n", deviceInfo.IT);
        UG_DUMP("ET: %d\n", deviceInfo.ET);

        UG_DUMP("av_energy(mWh): %d\n", batteryInfo.LMD * deviceInfo.voltage_mV);

        mutex_unlock(&ug31_gauge_mutex);    

        return len;
}

int ug31xx_proc_info_dump_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
        return count;
}

void dump_INIT_OCV(struct seq_file *seq, void *v)
{
        int i=0, j=0, k=0;

        mutex_lock(&ug31_gauge_mutex);    

        for (i=0; i<TEMPERATURE_NUMS; i++) {
                for (j=0; j<OCV_C_RATE_NUMS; j++) {
                        for (k=0; k<OCV_NUMS; k++) {
                                seq_printf(seq, "INIT_OCV %02doC 0.%dC %d%% %d\n", 
                                                TEMPERATURE_TABLE[i], OCV_CRATE_TABLE[j], 
                                                SOC_TABLE[k], cellTable.INIT_OCV[i][j][k]);
                        }// for k
                }// for j
                seq_printf(seq, "\n"); 
        } //for i

        mutex_unlock(&ug31_gauge_mutex);    
}

void dump_CELL_VOLTAGE_TABLE(struct seq_file *seq, void *v)
{
        int i=0, j=0, k=0;

        mutex_lock(&ug31_gauge_mutex);    

        for (i=0; i<TEMPERATURE_NUMS; i++) {
                for (j=0; j<C_RATE_NUMS; j++) {
                        for (k=0; k<SOV_NUMS; k++) {
                                seq_printf(seq, "CELL_VOLTAGE_TABLE %02doC 0.%dC %d%% %d\n", 
                                                TEMPERATURE_TABLE[i], CRATE_TABLE[j], 
                                                cellParameter.SOV_TABLE[k]/10, cellTable.CELL_VOLTAGE_TABLE[i][j][k]);
                        }// for k
                }// for j
                seq_printf(seq, "\n"); 
        } //for i

        mutex_unlock(&ug31_gauge_mutex);    
}

void dump_CELL_NAC_TABLE(struct seq_file *seq, void *v)
{
        int i=0, j=0, k=0;

        mutex_lock(&ug31_gauge_mutex);    

        for (i=0; i<TEMPERATURE_NUMS; i++) {
                for (j=0; j<C_RATE_NUMS; j++) {
                        for (k=0; k<SOV_NUMS; k++) {
                                seq_printf(seq, "CELL_NAC_TABLE %02doC 0.%dC %d%% %d\n", 
                                                TEMPERATURE_TABLE[i], CRATE_TABLE[j], 
                                                cellParameter.SOV_TABLE[k]/10, cellTable.CELL_NAC_TABLE[i][j][k]);
                        }// for k
                }// for j
                seq_printf(seq, "\n"); 
        } //for i

        mutex_unlock(&ug31_gauge_mutex);    
}

void dump_CELL_PARAMETER(struct seq_file *seq, void *v)
{
        int i;

        mutex_lock(&ug31_gauge_mutex);    

        seq_printf(seq, "Total struct size: %d\n", cellParameter.totalSize);
        seq_printf(seq, "Firmware version: %d\n", cellParameter.fw_ver);
        seq_printf(seq, "customer: %s\n", cellParameter.customer);
        seq_printf(seq, "project: %s\n", cellParameter.project);
        seq_printf(seq, "ggb_version: %04X\n", cellParameter.ggb_version);
        seq_printf(seq, "customer self-defined: %s\n", cellParameter.customerSelfDef);
        seq_printf(seq, "project self-defined: %s\n", cellParameter.projectSelfDef);
        seq_printf(seq, "cell type: 0x%04X\n", cellParameter.cell_type_code);
        seq_printf(seq, "ICType: 0x%02X\n", cellParameter.ICType);
        seq_printf(seq, "gpio1: 0x%02X\n", cellParameter.gpio1);
        seq_printf(seq, "gpio2: 0x%02X\n", cellParameter.gpio2);
        seq_printf(seq, "gpio34: 0x%02X\n", cellParameter.gpio34);
        seq_printf(seq, "Chop control ?? : 0x%02X\n", cellParameter.chopCtrl);
        seq_printf(seq, "ADC1 offset ?? : %d\n", cellParameter.adc1Offset);
        seq_printf(seq, "Cell number ?? : %d\n", cellParameter.cellNumber);
        seq_printf(seq, "Assign cell one to: %d\n", cellParameter.assignCellOneTo);
        seq_printf(seq, "Assign cell two to: %d\n", cellParameter.assignCellTwoTo);
        seq_printf(seq, "Assign cell three to: %d\n", cellParameter.assignCellThreeTo);
        seq_printf(seq, "I2C Address: : 0x%02X\n", cellParameter.i2cAddress);
        seq_printf(seq, "I2C 10bit address: : 0x%02X\n", cellParameter.tenBitAddressMode); 
        seq_printf(seq, "I2C high speed: 0x%02X\n", cellParameter.highSpeedMode);
        seq_printf(seq, "clock(kHz): %d\n", cellParameter.clock);
        seq_printf(seq, "RSense(m ohm): %d\n", cellParameter.rSense);
        seq_printf(seq, "ILMD(mAH) ?? : %d\n", cellParameter.ILMD);
        seq_printf(seq, "EDV1 Voltage(mV): %d\n", cellParameter.edv1Voltage);
        seq_printf(seq, "Standby current ?? : %d\n", cellParameter.standbyCurrent);
        seq_printf(seq, "TP Current(mA)?? : %d\n", cellParameter.TPCurrent);
        seq_printf(seq, "TP Voltage(mV)?? : %d\n", cellParameter.TPVoltage);
        seq_printf(seq, "TP Time ?? : %d\n", cellParameter.TPTime);
        seq_printf(seq, "Offset R ?? : %d\n", cellParameter.offsetR);
        seq_printf(seq, "Delta R ?? : %d\n", cellParameter.deltaR);
        seq_printf(seq, "max delta Q(%%)  ?? : %d\n", cellParameter.maxDeltaQ);
        seq_printf(seq, "Offset mAH  ?? : %d\n", cellParameter.offsetmaH);
        seq_printf(seq, "time interval (s) : %d\n", cellParameter.timeInterval);
        seq_printf(seq, "COC ?? : %d\n", cellParameter.COC);
        seq_printf(seq, "DOC ?? : %d\n", cellParameter.DOC);
        seq_printf(seq, "UC ?? : %d\n", cellParameter.UC);
        seq_printf(seq, "OV1 (mV) ?? : %d\n", cellParameter.OV1);
        seq_printf(seq, "UV1 (mV) ?? : %d\n", cellParameter.UV1);
        seq_printf(seq, "OV2 (mV) ?? : %d\n", cellParameter.UV2);
        seq_printf(seq, "UV2 (mV) ?? : %d\n", cellParameter.UV2);
        seq_printf(seq, "OV3 (mV) ?? : %d\n", cellParameter.UV3);
        seq_printf(seq, "UV3 (mV) ?? : %d\n", cellParameter.UV3);
        seq_printf(seq, "OVP (mV) ?? : %d\n", cellParameter.OVP);
        seq_printf(seq, "UVP (mV) ?? : %d\n", cellParameter.UVP);
        seq_printf(seq, "OIT (C) ?? : %d\n", cellParameter.OIT);
        seq_printf(seq, "UIT (C) ?? : %d\n", cellParameter.UIT);
        seq_printf(seq, "OET (C) ?? : %d\n", cellParameter.OET);
        seq_printf(seq, "UET (C) ?? : %d\n", cellParameter.UET);
        seq_printf(seq, "CBC_21 ?? : %d\n", cellParameter.CBC21);
        seq_printf(seq, "CBC_32 ?? : %d\n", cellParameter.CBC32);
        seq_printf(seq, "OSC tune J?? : %d\n", cellParameter.oscTuneJ);
        seq_printf(seq, "OSC tune K?? : %d\n", cellParameter.oscTuneK);
        seq_printf(seq, "Alarm timer ?? : %d\n", cellParameter.alarm_timer);
        seq_printf(seq, "Clock div. A?? : %d\n", cellParameter.clkDivA);
        seq_printf(seq, "Clock div. B?? : %d\n", cellParameter.clkDivB);
        seq_printf(seq, "PWM timer ?? : %d\n", cellParameter.pwm_timer);
        seq_printf(seq, "Alarm enable?? : 0x%02X\n", cellParameter.alarmEnable);
        seq_printf(seq, "CBC enable?? : 0x%02X\n", cellParameter.cbcEnable);
        seq_printf(seq, "OTP1 scale?? : %d\n", cellParameter.otp1Scale);
        seq_printf(seq, "BAT2 8V ideal ADC code?? : %d\n", cellParameter.vBat2_8V_IdealAdcCode);
        seq_printf(seq, "BAT2 6V ideal ADC code?? : %d\n", cellParameter.vBat2_6V_IdealAdcCode);
        seq_printf(seq, "BAT3 12V ideal ADC code?? : %d\n", cellParameter.vBat3_12V_IdealAdcCode);
        seq_printf(seq, "BAT3 9V ideal ADC code?? : %d\n", cellParameter.vBat3_9V_IdealAdcCode);
        //seq_printf("BAT2 Ra : %d\n", cellParameter.vBat2Ra);
        //seq_printf("BAT2 Rb : %d\n", cellParameter.vBat2Rb);
        //seq_printf("BAT3 Ra : %d\n", cellParameter.vBat3Ra);
        //seq_printf("BAT3 Rb : %d\n", cellParameter.vBat3Rb);
        seq_printf(seq, "ADC1 pgain: %d\n", cellParameter.adc1_pgain);
        seq_printf(seq, "ADC1 ngain: %d\n", cellParameter.adc1_ngain);
        seq_printf(seq, "ADC1 pos. offset: %d\n", cellParameter.adc1_pos_offset);
        seq_printf(seq, "ADC2 gain: %d\n", cellParameter.adc2_gain);
        seq_printf(seq, "ADC2 offset: %d\n", cellParameter.adc2_offset);
        seq_printf(seq, "R ?? : %d\n", cellParameter.R);
        for (i=0; i<sizeof(cellParameter.rtTable)/sizeof(u16); i++) {
                seq_printf(seq, "RTTable[%02d]: %d\n", i, cellParameter.rtTable[i]);
        }
        for (i=0; i<sizeof(cellParameter.SOV_TABLE)/sizeof(u16); i++) {
                seq_printf(seq, "SOV Table[%02d]: %d\n", i, cellParameter.SOV_TABLE[i]/10);
        }
        seq_printf(seq, "ADC d1: %d\n", cellParameter.adc_d1);
        seq_printf(seq, "ADC d2: %d\n", cellParameter.adc_d2);
        seq_printf(seq, "ADC d3: %d\n", cellParameter.adc_d3);
        seq_printf(seq, "ADC d4: %d\n", cellParameter.adc_d4);
        seq_printf(seq, "ADC d5: %d\n", cellParameter.adc_d5);        

        mutex_unlock(&ug31_gauge_mutex);    
}

void dump_OTP(struct seq_file *seq, void *v)
{
        int i;

        u32 *pOTP1 = (u32 *)&otp1Reg;
        u32 *pOTP2 = (u32 *)&otp2Reg;
        u32 *pOTP3 = (u32 *)&otp3Reg;

        mutex_lock(&ug31_gauge_mutex);    

        seq_printf(seq, "OTP1: 0x%08X\n", *pOTP1);
        seq_printf(seq, "OTP2: 0x%08X\n", pOTP2[0]);
        seq_printf(seq, "OTP3: 0x%08X\n", pOTP2[1]);
        seq_printf(seq, "OTP4: 0x%08X\n", pOTP2[2]);
        seq_printf(seq, "OTP5: 0x%08X\n", pOTP2[3]);
        seq_printf(seq, "OTP6: 0x%08X\n", *pOTP3);


        mutex_unlock(&ug31_gauge_mutex);    
}
void (*seq_dump_func_tbl[])(struct seq_file *seq, void *v) = {
        dump_INIT_OCV,
        dump_CELL_VOLTAGE_TABLE,
        dump_CELL_NAC_TABLE,
        dump_CELL_PARAMETER,
        dump_OTP,
};
#define NUM_SEQ_FUNC_TBL  (sizeof(seq_dump_func_tbl)/sizeof(seq_dump_func_tbl[0]))


static void *ug31_ggb_seq_start(struct seq_file *seq, loff_t *pos)
{
        if (*pos >= NUM_SEQ_FUNC_TBL) return NULL;

	return seq_dump_func_tbl[*pos];
}

static void *ug31_ggb_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
        (*pos)++;
        if (*pos >= NUM_SEQ_FUNC_TBL) return NULL;

	return seq_dump_func_tbl[*pos];
}

static void ug31_ggb_seq_stop(struct seq_file *seq, void *v)
{
}

static int ug31_ggb_seq_show(struct seq_file *seq, void *v)
{
        void (*p_seq_dump_func)(struct seq_file *, void *) = v;

        p_seq_dump_func(seq, v);

	return 0;
}
static const struct seq_operations ug31_ggb_seq_ops = {
	.start = ug31_ggb_seq_start,
	.next  = ug31_ggb_seq_next,
	.stop  = ug31_ggb_seq_stop,
	.show  = ug31_ggb_seq_show,
};

static int ug31_ggb_seq_open(struct inode *inode, struct file *file)
{
        memset(proc_buf, 0x00, sizeof(proc_buf));

	return seq_open(file, &ug31_ggb_seq_ops);
}

static const struct file_operations ggb_file_proc_fops = {
	.owner	 = THIS_MODULE,
	.open    = ug31_ggb_seq_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release,
        .write = ug31_proc_ggb_write,
        .release = ug31_proc_ggb_release,
};

int ug31xx_register_proc_fs_test(void)
{
        struct proc_dir_entry *entry=NULL;
        struct battery_dev_info tmp_dev_info;

        mutex_lock(&ug31_dev_info_mutex);    
        tmp_dev_info = ug31_dev_info;
        mutex_unlock(&ug31_dev_info_mutex);

        if (tmp_dev_info.test_flag & TEST_UG31_PROC_INFO_DUMP) {
                entry = create_proc_entry("ug31_test_info_dump", 0666, NULL);
                if (!entry) {
                        BAT_DBG_E("Unable to create ug31_test_info_dump\n");
                        return -EINVAL;
                }
                entry->read_proc = ug31xx_proc_info_dump_read;
                entry->write_proc = ug31xx_proc_info_dump_write;
        }


        if (tmp_dev_info.test_flag & TEST_UG31_PROC_REG_DUMP) {
                entry = create_proc_entry("ug31_test_reg_dump", 0666, NULL);
                if (!entry) {
                        BAT_DBG_E("Unable to create ug31_test_reg_dump\n");
                        return -EINVAL;
                }
                entry->read_proc = ug31xx_proc_reg_dump_read;
                entry->write_proc = ug31xx_proc_reg_dump_write;
        }

        if (tmp_dev_info.test_flag & TEST_UG31_PROC_GGB_INFO_DUMP) {
                if (proc_create("ug31_test_ggb_dump", 0666, NULL, &ggb_file_proc_fops) == NULL) {
                        BAT_DBG_E("Unable to create ug31_test_ggb_dump\n");
                        return -EINVAL;
                }
        }

        if (tmp_dev_info.test_flag & TEST_UG31_PROC_I2C_PROTOCOL) {
                entry = create_proc_entry("ug31_test_protocol", 0666, NULL);
                if (!entry) {
                        BAT_DBG_E("Unable to create ug31_test_protocol\n");
                        return -EINVAL;
                }
                entry->read_proc = ug31xx_proc_protocol_read;
                entry->write_proc = ug31xx_proc_protocol_write;
        }

        return 0;
}
