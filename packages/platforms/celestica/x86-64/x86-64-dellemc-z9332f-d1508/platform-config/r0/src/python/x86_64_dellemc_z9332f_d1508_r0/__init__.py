from onl.platform.base import *
from onl.platform.celestica import *
import subprocess,string

class OnlPlatform_x86_64_dellemc_z9332f_d1508_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_2x10_32x100):
    PLATFORM='x86-64-dellemc-z9332f-d1508-r0'
    MODEL="Dell EMC"
    SYS_OBJECT_ID=".2060.1"

    def baseconfig(self):
        onlp_interval_time = 30 #second
        file_path = "/var/opt/interval_time.txt"

        print("Initialize Dell EMC Z9332F D1508 driver")

        qsfp_qty = 32
        sfp_qty = 2
        qsfp_offset = 9

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
            self.new_i2c_device('optoe2',0x50,x+1)
        
        for y in range(qsfp_qty):
            self.new_i2c_device('optoe1',0x50,qsfp_offset+y+1)

        #init baseboard
        #self.new_i2c_device('baseboard',0x0d,5)

        #Create new admin:admin user if not exist
        command = ['cat','/etc/sudoers']
        validater = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        result = validater.stdout.read().find('admin')
        if(result == -1):
            os.system("mkdir /home/admin")
            os.system("useradd -g root -d /home/admin admin")
            os.system("echo 'admin:admin' | chpasswd")
            os.system("echo 'admin    ALL=(ALL:ALL) ALL' >> /etc/sudoers")
            os.system("usermod -s /bin/bash admin")

        os.system("echo '3' > /proc/sys/kernel/printk")

        if os.path.exists(file_path):
            pass
        else:
            with open(file_path, 'w') as f:  
                f.write("{0}\r\n".format(onlp_interval_time))
            f.close()
        
        #initialize onlp cache files
        print("Initialize ONLP Cache files")
        os.system("ipmitool fru > /tmp/onlp-fru-cache.tmp; sync; rm -f /tmp/onlp-fru-cache.txt; mv /tmp/onlp-fru-cache.tmp /tmp/onlp-fru-cache.txt")
        os.system("ipmitool sensor list > /tmp/onlp-sensor-list-cache.tmp; sync; rm -f /tmp/onlp-sensor-list-cache.txt; mv /tmp/onlp-sensor-list-cache.tmp /tmp/onlp-sensor-list-cache.txt")

        return True
