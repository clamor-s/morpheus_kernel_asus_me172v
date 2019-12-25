#
#  Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
#  Written by wade chen Wade1_Chen@asus.com
#

$(obj)/ug31xx.ggbx:  $(obj)/ug31xx.ggb $(obj)/ug31xx_translate.py
	@echo "Generate $@ .... "
	$(obj)/ug31xx_translate.py -m GGB2GGBX -o $@ $< 

$(obj)/ug31xx_factory_ggb_data.c: $(obj)/ug31xx.ggbx
	@echo "Generate $@ .... "
	$(obj)/ug31xx_translate.py -m GGBX2BUF -o $@ $< 

$(obj)/bq27520_factory_data.c: $(obj)/bq27xx_translate.py $(obj)/bq27xx_CosLight.dffs $(obj)/bq27xx_LG.dffs
	@echo "Generate $@ .... "
	$(obj)/bq27xx_translate.py -o $@ $(obj)/bq27xx_CosLight.dffs  $(obj)/bq27xx_LG.dffs


ug31xx_battery-y += ug31xx_api.o
ug31xx_battery-y += ug31xx_i2c.o
ug31xx_battery-y += ug31xx_battery_core.o
ug31xx_battery-y += ug31xx_factory_ggb_data.o
ug31xx_battery-y += ug31xx_output.o
ug31xx_battery-$(CONFIG_PROC_FS)  += ug31xx_proc_fs.o
ug31xx_battery-$(CONFIG_ASUS_ENGINEER_MODE)	+= ug31xx_proc_fs_asustag.o

asus_battery-y += asus_battery_power.o
asus_battery-$(CONFIG_PROC_FS) += asus_battery_proc_fs.o
asus_battery-$(CONFIG_ASUS_ENGINEER_MODE) += asus_battery_proc_fs_asustag.o

bq27520_battery-y += bq27520_battery_i2c.o
bq27520_battery-y += bq27520_factory_data.o
bq27520_battery-$(CONFIG_PROC_FS) += bq27520_proc_fs.o
bq27520_battery-$(CONFIG_ASUS_ENGINEER_MODE) += bq27520_proc_fs_asustag.o
bq27520_battery-$(CONFIG_ASUS_ENGINEER_MODE) += bq27520_battery_upt_i2c.o

fake_battery-y += fake_battery_dev.o

obj-$(CONFIG_ASUS_BATTERY) += asus_battery.o
obj-$(CONFIG_ASUS_BQ27520_BATTERY) += bq27520_battery.o
obj-$(CONFIG_ASUS_UG31XX_BATTERY) += ug31xx_battery.o
obj-$(CONFIG_ASUS_FAKE_BATTERY) += fake_battery.o

