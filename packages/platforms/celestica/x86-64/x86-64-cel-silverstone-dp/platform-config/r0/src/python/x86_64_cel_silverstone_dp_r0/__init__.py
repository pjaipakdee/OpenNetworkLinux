from onl.platform.base import *
from onl.platform.celestica import *

class OnlPlatform_x86_64_cel_silverstone_dp_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_24x100_5x400):
    PLATFORM='x86-64-cel-silverstone-dp-r0'
    MODEL="Silverstone DP"
    SYS_OBJECT_ID=".2060.1"

    def baseconfig(self):
        onlp_interval_time = 30 #second
        file_path = "/var/opt/interval_time.txt"
        print("Initialize Silverstone-DP driver")


        qsfp_qty = 24
        sfp_qty = 2
        qsfp_dd_qty = 6
        qsfp_offset = 9
        actual_port_num = 1

        self.insmod("i2c-ocores.ko")
        self.insmod("i2c-cls.ko")
        self.insmod("cls-switchboard.ko")
        #self.insmod("baseboard.ko")
        self.insmod("cpld_b.ko")
        #self.insmod("switchboard.ko")
        self.insmod("switchboard-diag.ko")
        self.insmod("mc24lc64t.ko")
        self.insmod("optoe.ko")
        self.insmod("xcvr-cls.ko")

        ###### new configuration for SDK support ########
        os.system("insmod /lib/modules/`uname -r`/kernel/net/core/pktgen.ko")
        os.system("insmod /lib/modules/`uname -r`/kernel/net/core/drop_monitor.ko")
        os.system("insmod /lib/modules/`uname -r`/kernel/net/ipv4/tcp_probe.ko")


        #tlv eeprom device
        self.new_i2c_device('24lc64t', 0x56, 0)

        #transceiver device
        for x in range(sfp_qty):
            self.new_i2c_device('optoe2',0x50,x+1) #1-2
            os.system("echo 'SFP{1}' > /sys/bus/i2c/devices/{0}-0050/port_name".format(x+1,actual_port_num))
            actual_port_num += 1

        actual_port_num = 1

        for y in range(qsfp_qty):
            self.new_i2c_device('optoe1',0x50,qsfp_offset+y+1) #10-33
            os.system("echo 'QSFP{1}' > /sys/bus/i2c/devices/{0}-0050/port_name".format(qsfp_offset+y+1,actual_port_num))
            actual_port_num += 1

        actual_port_num = 1

        for z in range(qsfp_dd_qty):
            self.new_i2c_device('optoe3',0x50,qsfp_offset+qsfp_qty+z+1) #34-39
            os.system("echo 'QSFPDD{1}' > /sys/bus/i2c/devices/{0}-0050/port_name".format(qsfp_offset+qsfp_qty+z+1,actual_port_num))
            actual_port_num += 1
            
        os.system("echo '3' > /proc/sys/kernel/printk")

        if os.path.exists(file_path):
            pass
        else:
            with open(file_path, 'w') as f:  
                f.write("{0}\r\n".format(onlp_interval_time))
            f.close()
        
        #initialize onlp cache files
        print("Initialize ONLP Cache files")
        os.system("ipmitool fru > /tmp/onlp-fru-cache.txt")
        os.system("ipmitool sensor list > /tmp/onlp-sensor-list-cache.txt")

        return True
