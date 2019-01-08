#include <unistd.h>
#include <onlplib/mmap.h>
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include <fcntl.h>

// #include "i2c_chips.h"
#include "platform.h"


static onlp_thermal_info_t thermal_info[] = {
    { },
    { { ONLP_THERMAL_ID_CREATE(1), "Temp_CPU",    0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(2), "TEMP_BB", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(3), "TEMP_SW_U16",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(4), "TEMP_SW_U52",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(5), "TEMP_FAN_U17",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(6), "TEMP_FAN_U52",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(7), "PSUL_Temp1",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(8), "PSUL_Temp2",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(9), "PSUR_Temp1",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(10), "PSUR_Temp2",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
};

int
onlp_thermali_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info_p)
{
    int thermal_id;
    thermal_id = ONLP_OID_ID_GET(id);
    *info_p = thermal_info[thermal_id];
    int thermal_status = getThermalStatus_Ipmi(thermal_id,&(info_p->mcelsius));
    info_p->thresholds.warning = 0;
    info_p->thresholds.error = 0;
    info_p->thresholds.shutdown = 0;
    if(!(thermal_status))
        info_p->status = ONLP_THERMAL_STATUS_FAILED;

    return ONLP_STATUS_OK;
}
