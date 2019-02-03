#include <unistd.h>
#include <onlplib/mmap.h>
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include <fcntl.h>

#include "i2c_chips.h"
#include "platform.h"

static onlp_thermal_info_t thermal_info[] = {
    { { ONLP_THERMAL_ID_CREATE(THERMAL_MAIN_BOARD_REAR), "Chassis Thermal (Rear)",    0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_BCM), "BCM SOC Thermal sensor", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_CPU), "CPU Core",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_MAIN_BOARD_FRONT), "Chassis Thermal Sensor (Front)",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_PSU1), "PSU-1 Thermal Sensor", ONLP_PSU_ID_CREATE(1)},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_PSU2), "PSU-2 Thermal Sensor", ONLP_PSU_ID_CREATE(2)},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            }
};

int
onlp_thermali_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info)
{
    int sensor_id,ret;
    struct psuInfo psu;
    short temp;

    sensor_id = ONLP_OID_ID_GET(id) - 1;

    *info = thermal_info[sensor_id];

    switch (sensor_id) {
      case THERMAL_MAIN_BOARD_REAR:
      case THERMAL_MAIN_BOARD_FRONT:
      case THERMAL_BCM:
      case THERMAL_CPU:
        tsTempGet(sensor_id, &temp);
        info->mcelsius = temp;
        break;
      case THERMAL_PSU1:
        ret = getPsuInfo(0, &psu);
        if(ret == 0){
            info->mcelsius = psu.temp;
            break;
        }else{
            return ONLP_STATUS_E_INTERNAL;
        }
        
      case THERMAL_PSU2:
        ret = getPsuInfo(1, &psu);
        if(ret == 0){
            info->mcelsius = psu.temp;
            break;
        }else{
            return ONLP_STATUS_E_INTERNAL;
        }
        
    }
    return ONLP_STATUS_OK;
}

