from onl.platform.base import *
from onl.platform.celestica import *

class OnlPlatform_x86_64_cel_ivystone_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_128x400):
    PLATFORM='x86-64-cel-ivystone-r0'
    MODEL="Ivystone"
    SYS_OBJECT_ID=".2060.1"

    def baseconfig(self):
        onlp_interval_time = 30 #second
        file_path = "/var/opt/interval_time.txt"
        
        print("Initialize Ivystone Platform driver")

        self.insmod("dimm-bus.ko")
        #self.insmod("i2c-imc.ko")
        os.system("insmod /lib/modules/`uname -r`/onl/celestica/x86-64-cel-ivystone/i2c-imc.ko allow_unsafe_access=1")
        self.insmod("baseboard_cpld.ko")
        self.insmod("switchboard_fpga.ko")
        self.insmod("mc24lc64t.ko")
        
        ###### new configuration for SDK support ########
        # os.system("insmod /lib/modules/`uname -r`/kernel/net/core/pktgen.ko")
        # os.system("insmod /lib/modules/`uname -r`/kernel/net/core/drop_monitor.ko")
        # os.system("insmod /lib/modules/`uname -r`/kernel/net/ipv4/tcp_probe.ko")

        ##### Config ma1 vlan interface to interact with OpenBMC #######
        os.system("ip link add link ma1 name ma1.4088 type vlan id 4088")
        os.system("ip addr add 240.1.1.2/30 dev ma1.4088")
        os.system("ip link set ma1.4088 up")

        #eeprom driver
        self.new_i2c_device('24lc64t', 0x56, 0)
        

        if os.path.exists(file_path):
            pass
        else:
            with open(file_path, 'w') as f:  
                f.write("{0}\r\n".format(onlp_interval_time))
            f.close()
        return True
