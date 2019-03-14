from onl.platform.base import *
from onl.platform.celestica import *

class OnlPlatform_x86_64_cel_redstone_xp_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_48x10_6x40):
    PLATFORM='x86-64-cel-redstone-xp-r0'
    MODEL="Redstone-XP"
    SYS_OBJECT_ID=".2060.1"

    def baseconfig(self):
        cmd = "i2cget -f -y 1 0x70 1"
        out = subprocess.check_output(cmd, shell=True)
        data = int(out, 16)
        if data == 9 or data == 12 or data == 13:
            cmd = "i2cset -f -y 1 0x70 0x1 0x0"
        elif data == 10 or data == 14 or data == 15:
            cmd = "i2cset -f -y 1 0x70 0x1 0x1"
        elif data == 0 or data == 1 or data == 5:
            cmd = "i2cset -f -y 1 0x70 0x1 0x4"
        elif data == 2 or data == 2 or data == 6:
            cmd = "i2cset -f -y 1 0x70 0x1 0x5"
        out = subprocess.check_output(cmd, shell=True)
        #cel_sysfs_core modulize
        self.insmod('cel_sysfs_core')

        #self.new_i2c_device('24lc64t', 0x50, 58)
        # def new_i2c_device(self, driver, addr, bus_number):
        #     bus = '/sys/bus/i2c/devices/i2c-%d' % bus_number    
        #     devdir = "%d-%4.4x" % (bus_number, addr) 
        #     return self.new_device(driver, addr, bus, devdir)

        # def new_device(self, driver, addr, bus, devdir):
        # if not os.path.exists(os.path.join(bus, devdir)):
        #     try:
        #         with open("%s/new_device" % bus, "w") as f:
        #             f.write("%s 0x%x\n" % (driver, addr))
        #     except Exception, e:
        #         print "Unexpected error initialize device %s:0x%x:%s: %s" % (driver, addr, bus, e)
        # else:
        #     print("Device %s:%x:%s already exists." % (driver, addr, bus))

        #CPLD, SFF modulize
        self.insmod('i2c-lpc-sfp')
        for i in range(54):
        #for i in range(32):
            self.new_i2c_device('sff8436', 0x50, i + 2)

        #PCA9548 modulize
        self.new_i2c_device('pca9548', 0x73, 1)
        self.new_i2c_device('pca9548', 0x71, 1)


        #LED modulize
        self.insmod('leds')

        #watchdog modulize
        self.insmod('watchdog')

        #DPS800ab modulize
        self.insmod('dps800_psu')
        self.new_i2c_device('dps_800ab_16_d', 0x58, 56)#PSU_L
        self.new_i2c_device('dps_800ab_16_d', 0x59, 57)#PSU_R


        #TLV EEPROM modulize
        self.insmod('mc24lc64t')
        self.new_i2c_device('24lc64t', 0x50, 58)


        #emc2305 modulize
        self.insmod('emc2305')
        self.new_i2c_device('emc2305', 0x4d, 59)
        self.new_i2c_device('emc2305', 0x2e, 59)

        #LM75B modulize
        self.insmod('lm75')
        # self.new_i2c_device('cel_lm75b', 0x48, 60)#lm75_cpu
        # self.new_i2c_device('cel_lm75b', 0x4e, 61)#lm75_out
        # self.new_i2c_device('cel_lm75b', 0x49, 68)#lm75_in
        # self.new_i2c_device('cel_lm75b', 0x4a, 69)#lm75_sw
        
        # self.new_i2c_device('cel_lm75b', 0x48, 60)#lm75_cpu
        # self.new_i2c_device('cel_lm75b', 0x4e, 61)#lm75_out
        # self.new_i2c_device('cel_lm75b', 0x49, 68)#lm75_in
        # self.new_i2c_device('cel_lm75b', 0x4a, 69)#lm75_sw
        self.new_i2c_device('cel_lm75b', 0x48, 60)
        self.new_i2c_device('cel_lm75b', 0x4e, 61)
        self.new_i2c_device('cel_lm75b', 0x49, 68)
        self.new_i2c_device('cel_lm75b', 0x4a, 69)
        

        #PCA9506 modulize
        self.new_i2c_device('pca9505', 0x20, 63)
        for i in range(472, 512):
            cmd = "echo " + str(i) + "> /sys/class/gpio/export"
            os.system(cmd)

        return True


