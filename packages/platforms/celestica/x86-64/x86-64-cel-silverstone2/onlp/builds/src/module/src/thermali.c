#include <unistd.h>
#include <onlplib/mmap.h>
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include <fcntl.h>
#include "platform.h"

static onlp_thermal_info_t thermal_info[] = {
    { },
    { { ONLP_THERMAL_ID_CREATE(1), "CPU internal Temp",    0}, //Temp_CPU
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(2), "Base board Middle inlet Temp", 0}, // TEMP_BB_U3
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(3), "ASIC Internal Temp",   0},// TEMP_SW_Internal
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(4), "Switch Board right inlet Temp",   0},// TEMP_SW_U16
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(5), "Switch board left inlet Temp",   0},// TEMP_SW_U52
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(6), "Fan board right inlet Temp",   0},// TEMP_FAN_U17
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(7), "Fan board middle inlet Temp",   0},// TEMP_FAN_U52
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(8), "Power IC IR3215 internal Temp (U71_1)",   0}, // SW_U71_Temp1
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(9), "Power IC IR3215 internal Temp (U71_2)",   0}, // SW_U71_temp2
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(10), "Power IC IR3215 internal Temp (U78)",   0},// SW_U78_Temp
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(11), "PSUL_Temp1",   ONLP_PSU_ID_CREATE(1)},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(12), "PSUL_Temp2",   ONLP_PSU_ID_CREATE(1)},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(13), "PSUR_Temp1",   ONLP_PSU_ID_CREATE(2)},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(14), "PSUR_Temp2",   ONLP_PSU_ID_CREATE(2)},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
};

int onlp_thermali_init(void)
{
    return ONLP_STATUS_OK;
}

int onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t *info_p)
{
    int thermal_id;
    int thermal_status = 0;
    int temp, warn, err, shutdown;

    thermal_id = ONLP_OID_ID_GET(id);
    memcpy(info_p, &thermal_info[thermal_id], sizeof(onlp_thermal_info_t));

    /* Get thermal temperature. */
    thermal_status = get_sensor_info(thermal_id, &temp, &warn, &err, &shutdown);
    if (-1 == thermal_status)
    {
        info_p->status = ONLP_THERMAL_STATUS_FAILED;
    }
    else
    {
        info_p->status = ONLP_THERMAL_STATUS_PRESENT;
        info_p->mcelsius = temp;
        info_p->thresholds.warning = warn;
        info_p->thresholds.error = err;
        info_p->thresholds.shutdown = shutdown;
    }

    return ONLP_STATUS_OK;
}
