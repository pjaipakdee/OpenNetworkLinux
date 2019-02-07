from onl.platform.base import *
from onl.platform.celestica import *

class OnlPlatform_x86_64_cel_silverstone_dp_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_24x100_5x400):
    PLATFORM='x86-64-cel-silverstone-dp-r0'
    MODEL="Silverstone DP"
    SYS_OBJECT_ID=".2060.1"

    def baseconfig(self):
        print("Initialize Silverstone-DP driver")

        self.insmod("optoe.ko")
        self.insmod("switchboard.ko")
        self.insmod("baseboard.ko")
        self.insmod("mc24lc64t.ko")

        ###### new configuration for SDK support ########
        os.system("insmod /lib/modules/`uname -r`/kernel/net/core/pktgen.ko")
        os.system("insmod /lib/modules/`uname -r`/kernel/net/core/drop_monitor.ko")
        os.system("insmod /lib/modules/`uname -r`/kernel/net/ipv4/tcp_probe.ko")


        #eeprom driver
        self.new_i2c_device('24lc64t', 0x56, 0)
        
        return True
