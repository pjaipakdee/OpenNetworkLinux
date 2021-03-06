/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2017 (C) Delta Networks, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 * </bsn.cl>
 ************************************************************
 *
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/

#include "platform_lib.h"
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include "x86_64_delta_agc7646v1_log.h"


#define VALIDATE(_id)                           \
    do {                                        \
        if (!ONLP_OID_IS_THERMAL(_id)) {         \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

static char *thermal_path[] =  /* must map with onlp_thermal_id */
{
    "reserved",
    "8-004d/hwmon/hwmon1/temp1_input"
};

#define dni_onlp_thermal_threshold(WARNING_DEFAULT, ERROR_DEFAULT, SHUTDOWN_DEFAULT){ \
    WARNING_DEFAULT,                                                                  \
    ERROR_DEFAULT,                                                                    \
    SHUTDOWN_DEFAULT,                                                                 \
}

/* Static values */
static onlp_thermal_info_t thermal_info[] = {
	{ }, /* Not used */
    { { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_CPU_BOARD), "Board sensor near CPU (U36)", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_FAN_BOARD), "Board sensor on Fan_BD (UT15)", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_MAIN_BOARD_TEMP_1), "Board sensor near MAC (U24)", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_4_ON_MAIN_BOARD_TEMP_2), "Board sensor near MAC (U25)", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_5_ON_MAIN_BOARD_TEMP_3_1), "Board sensor near MAC (U3 REMOTE)", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_6_ON_MAIN_BOARD_TEMP_3_2), "Board sensor near MAC (U3 LOCAL)", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_7_ON_PSU1), "PSU-1 internal sensor", ONLP_PSU_ID_CREATE(PSU1_ID)},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_8_ON_PSU2), "PSU-2 internal Sensor", ONLP_PSU_ID_CREATE(PSU2_ID)},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    }
};

/*
 * This will be called to intiialize the thermali subsystem.
 */
int
onlp_thermali_init(void)
{
    lockinit();
    return ONLP_STATUS_OK;
}

/*
 * Retrieve the information structure for the given thermal OID.
 *
 * If the OID is invalid, return ONLP_E_STATUS_INVALID.
 * If an unexpected error occurs, return ONLP_E_STATUS_INTERNAL.
 * Otherwise, return ONLP_STATUS_OK with the OID's information.
 *
 * Note -- it is expected that you fill out the information
 * structure even if the sensor described by the OID is not present.
 */
int
onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info)
{
    uint8_t local_id = 0;
    int rv = ONLP_STATUS_OK;
    UINT4 multiplier = 1000;
    UINT4 u4Data = 0;
    char fullpath[VENDOR_MAX_PATH_SIZE] = {0};
    int r_data = 0;

    VALIDATE(id);
    local_id = ONLP_OID_ID_GET(id);
    *info = thermal_info[local_id];

    if (strcmp(thermal_dev_list[local_id].dev_name, "") == 0 &&
                thermal_dev_list[local_id].id != 0)
    {
        sprintf(fullpath, "%s%s", PREFIX_PATH, thermal_path[local_id]);
        r_data = dni_i2c_lock_read_attribute(fullpath, ATTRIBUTE_BASE_DEC);
        info->mcelsius = r_data;
    }
    else if (thermal_dev_list[local_id].dev_name != NULL &&
             local_id <= NUM_OF_THERMAL_ON_MAIN_BROAD + NUM_OF_PSU_ON_MAIN_BROAD)
    {
        rv = dni_bmc_sensor_read(thermal_dev_list[local_id].dev_name,
                                 &u4Data,
                                 multiplier,
                                 thermal_dev_list[local_id].dev_type);
        if (u4Data == 0 || rv == ONLP_STATUS_E_GENERIC)
            rv = ONLP_STATUS_E_INTERNAL;
        else
            info->mcelsius = u4Data;
    }
    else
    {
        AIM_LOG_ERROR("Invalid Thermal ID!\n");
        rv = ONLP_STATUS_E_PARAM;
    }

    return rv;
}
