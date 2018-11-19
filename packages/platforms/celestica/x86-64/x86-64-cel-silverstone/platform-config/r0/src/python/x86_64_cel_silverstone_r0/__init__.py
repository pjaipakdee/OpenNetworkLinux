from onl.platform.base import *
from onl.platform.celestica import *

class OnlPlatform_x86_64_cel_silverstone_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_2x10_32x100):
    PLATFORM='x86-64-cel-silverstone-r0'
    MODEL="silverstone"
    SYS_OBJECT_ID=".2060.1"

    def baseconfig(self):
        print "Initialize Silverstone driver"

        self.insmod("optoe.ko")
        self.insmod("switchboard.ko")
        self.insmod("baseboard.ko")
        self.insmod("mc24lc64t.ko")

        #eeprom driver
        self.new_i2c_device('24lc64t', 0x56, 0)
        
        return True
