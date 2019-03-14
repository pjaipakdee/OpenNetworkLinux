/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2017 Celestica Corporation.
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
 *
 *
 ***********************************************************/
#include <unistd.h>
#include <fcntl.h>

#include <onlplib/file.h>
#include <onlp/platformi/sysi.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/fani.h>
#include <onlp/platformi/psui.h>

#include "x86_64_cel_redstone_xp_int.h"
#include "x86_64_cel_redstone_xp_log.h"

#include "platform_lib.h"

#define THERMAL_SIMULATE 1

/*fan control definition*/
#define F2B_TEMP_LOW   35
#define F2B_TEMP_HIGH  46
#define F2B_PWM_MIN    122
#define F2B_PWM_MAX    255

#define B2F_TEMP_LOW   30
#define B2F_TEMP_HIGH  43
#define B2F_PWM_MIN    121
#define B2F_PWM_MAX    255

#define TEMP_HYSTERESIS 2
#define TEMP_READ_ERRORS_MAX 5

typedef enum {
	TEMP_RESERVED = 0,
	CPU,
	ROA,
	BCM,
	FOA,
	LM75_TEMP_COUNT,
} lm75_chip;

enum {
	NO_CHANGE = 1,
	DO_CHANGE
};

enum {
	T_NOCHANGE = 0,
	T_RISE,
	T_DROP,
};
static int fan_flow_trend = T_NOCHANGE;
static int fan_direct = ONLP_FAN_STATUS_FAILED;
static int thermal_temp[LM75_TEMP_COUNT] = {0};
static int old_pwm = 0;
static int sensor_num = TEMP_RESERVED;
static int fan_fail = -1;
/*fan control end*/

#define NUM_OF_THERMAL_ON_MAIN_BROAD  CHASSIS_THERMAL_COUNT
#define NUM_OF_FAN_ON_MAIN_BROAD      CHASSIS_FAN_COUNT

static char arr_cplddev_name[NUM_OF_CPLD][10] =
{
    "cpld1_ver",
    "cpld2_ver",
    "cpld3_ver",
    "cpld4_ver",
    "cpld5_ver",
};

const char*
onlp_sysi_platform_get(void)
{
    return "x86-64-cel-redstone-xp-r0";
}

int
onlp_sysi_onie_data_get(uint8_t** data, int* size)
{
    uint8_t* rdata = aim_zmalloc(256);
    if(onlp_file_read(rdata, 256, size, PREFIX_PATH_ON_SYS_EEPROM) == ONLP_STATUS_OK) {
        if(*size == 256) {
            *data = rdata;
            return ONLP_STATUS_OK;
        }
    }

    aim_free(rdata);
    *size = 0;
    return ONLP_STATUS_E_INTERNAL;
}

int
onlp_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int i, v[NUM_OF_CPLD]={0};
	char  r_data[10]   = {0};
	char  fullpath[PREFIX_PATH_LEN] = {0};

    for (i=0; i < NUM_OF_CPLD; i++) {
		memset(fullpath, 0, PREFIX_PATH_LEN);
	    sprintf(fullpath, "%s%s", PREFIX_PATH_ON_CPLD, arr_cplddev_name[i]);
		if (deviceNodeReadString(fullpath, r_data, sizeof(r_data), 0) != 0) {
	        DEBUG_PRINT("%s(%d): read %s error\n", __FUNCTION__, __LINE__, fullpath);
	        return ONLP_STATUS_E_INTERNAL;
    	}
		v[i] = strtol(r_data, NULL, 0);
    }
    pi->cpld_versions = aim_fstrdup("CPLD1=0x%02x CPLD2=0x%02x CPLD3=0x%02x CPLD4=0x%02x CPLD5=0x%02x", v[0], v[1], v[2], v[3], v[4]);
    return 0;
}

void
onlp_sysi_platform_info_free(onlp_platform_info_t* pi)
{
    aim_free(pi->cpld_versions);
}


int
onlp_sysi_oids_get(onlp_oid_t* table, int max)
{
    int i;
    onlp_oid_t* e = table;
    memset(table, 0, max*sizeof(onlp_oid_t));

    /* 4 Thermal sensors on the chassis */
    for (i = 1; i <= NUM_OF_THERMAL_ON_MAIN_BROAD; i++)
    {
        *e++ = ONLP_THERMAL_ID_CREATE(i);
    }

    /* 5 LEDs on the chassis */
    for (i = 1; i <= NUM_OF_LED_ON_MAIN_BROAD; i++)
    {
        *e++ = ONLP_LED_ID_CREATE(i);
    }

    /* 2 PSUs on the chassis */
    for (i = 1; i <= NUM_OF_PSU_ON_MAIN_BROAD; i++)
    {
        *e++ = ONLP_PSU_ID_CREATE(i);
    }

    /* 4 Fans on the chassis */
    for (i = 1; i <= NUM_OF_FAN_ON_MAIN_BROAD; i++)
    {
        *e++ = ONLP_FAN_ID_CREATE(i);
    }

    return 0;
}

static int cel_set_fan_speed(int pwm)
{
	int pec;
	int i;
	int ret = 0;

	pec = (pwm * 100) / 255;
	for(i = 0; i < (CHASSIS_FAN_COUNT + NUM_OF_PSU_ON_MAIN_BROAD); i++) {
		ret += onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(i + 1), pec);
	}

	return ret;
}

static int find_index(int a[], int num_elements, int value)
{
   int i;
   for (i=0; i<num_elements; i++)
   {
	 if (a[i] == value)
	 {
	    return i;
	 }
   }
   return(-1); 
}


static int cel_fans_check(void)
{
	int i = 0;
	onlp_fan_info_t fan_info;
	onlp_psu_info_t psu_info;
    static onlp_oid_t fan_oid_table[ONLP_OID_TABLE_SIZE] = {0};
	static onlp_oid_t psu_oid_table[ONLP_OID_TABLE_SIZE] = {0};
	int psu_present_status[AIM_ARRAYSIZE(psu_oid_table)] ={0};

	if(fan_oid_table[0] == 0 || psu_oid_table[0] == 0) {
        /* We haven't retreived the system FAN oids yet. */
        onlp_sys_info_t si;
        onlp_oid_t* oidp;

        if(onlp_sys_info_get(&si) < 0) {
            return -1;
        }
        ONLP_OID_TABLE_ITER_TYPE(si.hdr.coids, oidp, FAN) {
            fan_oid_table[i++] = *oidp;
        }
		i = 0;
		ONLP_OID_TABLE_ITER_TYPE(si.hdr.coids, oidp, PSU) {
            psu_oid_table[i++] = *oidp;
        }
    }

	for(i = 0; i < AIM_ARRAYSIZE(fan_oid_table); i++) {
		if(fan_oid_table[i] == 0) {
            break;
        }
		if(onlp_fani_info_get(fan_oid_table[i], &fan_info) < 0) {
			DEBUG_PRINT("[Debug][%s][%d][get fan(%d) info error!]\n", __FUNCTION__, __LINE__, i + 1);
		}
		if(((fan_info.status & ONLP_FAN_STATUS_PRESENT) == 0) || (fan_info.status & ONLP_FAN_STATUS_FAILED)) {
			DEBUG_PRINT("[Debug][%s][%d][fan(%d) is not present or failed!]\n", __FUNCTION__, __LINE__, i + 1);
			cel_set_fan_speed(F2B_PWM_MAX);
			fan_fail = 1;
			return ONLP_STATUS_E_INVALID;
		}
	}

	//Retrive PSU status for validate
	for(i = 0; i < AIM_ARRAYSIZE(psu_oid_table); i++) {
		if(psu_oid_table[i] == 0) {
            break;
        }
		if(onlp_psu_info_get(psu_oid_table[i], &psu_info) < 0) {
			DEBUG_PRINT("[Debug][%s][%d][get psu(%d) info error!]\n", __FUNCTION__, __LINE__, i + 1);
		}
		//Add by Peerapong : Append the psu status to array
		psu_present_status[i] = psu_info.status;
		// if(((psu_info.status & ONLP_FAN_STATUS_PRESENT) == 0) || (psu_info.status & ONLP_FAN_STATUS_FAILED)) {
		// 	//Add by Peerapong to make Fan run with 1 Plug PSU.
		// 	// if((psu_info.status & ONLP_FAN_STATUS_PRESENT) && (psu_info.status & ONLP_FAN_STATUS_FAILED)){
		// 	// 	DEBUG_PRINT("[Debug][%s][%d][psu(%d) is present but failed!]\n", __FUNCTION__, __LINE__, i + 1);
		// 	// 	break;
		// 	// }
		// 	printf("Fail\n");
		// 	DEBUG_PRINT("[Debug][%s][%d][psu(%d) is not present or failed!]\n", __FUNCTION__, __LINE__, i + 1);
		// 	//cel_set_fan_speed(F2B_PWM_MAX);
		// 	cel_set_fan_speed(100);
		// 	fan_fail = 1;
		// 	return ONLP_STATUS_E_INVALID;
		// }
	}

	//Add by Peerapong to make FSC run with 1 Insert PSU.

	int not_present_index = find_index(psu_present_status,AIM_ARRAYSIZE(psu_present_status),0);
	if(not_present_index != -1){
		DEBUG_PRINT("[Debug][%s][%d][psu(%d) is not present]\n", __FUNCTION__, __LINE__, not_present_index + 1);
		//printf("psu(%d) : psu_info.status = %d,ONLP_PSU_STATUS_PRESENT = %d,ONLP_PSU_STATUS_FAILED = %d,ONLP_PSU_STATUS_UNPLUGGED = %d\n",fail_index + 1,psu_info.status,ONLP_PSU_STATUS_PRESENT,ONLP_PSU_STATUS_FAILED,ONLP_PSU_STATUS_UNPLUGGED);
	}

	int fail_index = find_index(psu_present_status,AIM_ARRAYSIZE(psu_present_status),ONLP_PSU_STATUS_FAILED);
	if(fail_index != -1){
		DEBUG_PRINT("[Debug][%s][%d][psu(%d) is failed!]\n", __FUNCTION__, __LINE__, i + 1);
		cel_set_fan_speed(F2B_PWM_MAX);
		fan_fail = 1;
		return ONLP_STATUS_E_INVALID;
	}


	if(fan_fail == 1) {
		fan_fail = 0;
	} else {
		fan_fail = -1;
	}

	return 0;
}

static int cel_calculate_pwm(int dir, int temp)
{
	int curve_slope;
	int pwm;
	if(dir == ONLP_FAN_STATUS_F2B) {
		curve_slope = (((F2B_PWM_MAX - F2B_PWM_MIN) * 10) / (F2B_TEMP_HIGH - F2B_TEMP_LOW) + 5) / 10;
		pwm = curve_slope * (temp - F2B_TEMP_LOW) + F2B_PWM_MIN;
		pwm = (pwm >= F2B_PWM_MAX) ? F2B_PWM_MAX : pwm;
		pwm = (pwm <= F2B_PWM_MIN) ? F2B_PWM_MIN : pwm;
	} else {
		curve_slope = (((B2F_PWM_MAX - B2F_PWM_MIN) * 10) / (B2F_TEMP_HIGH - B2F_TEMP_LOW) + 5) / 10;
		pwm = curve_slope * (temp - B2F_TEMP_LOW) + B2F_PWM_MIN;
		pwm = (pwm >= B2F_PWM_MAX) ? B2F_PWM_MAX : pwm;
		pwm = (pwm <= B2F_PWM_MIN) ? B2F_PWM_MIN : pwm;
	}

	return pwm;
}

static int cel_fan_init(void)
{
	int i;
	int pwm;
	int ret;
	onlp_thermal_info_t thermal_info;

	for(i = 0; i < CHASSIS_FAN_COUNT; i++) {
		fan_direct = onlp_fani_info_get_fan_direction(i + 1);
		if(fan_direct != ONLP_FAN_STATUS_FAILED)
			break;
	}
	if(fan_direct == ONLP_FAN_STATUS_F2B) {
		sensor_num = FOA;
	} else {
		sensor_num = ROA;
	}
	DEBUG_PRINT("[Debug][%s][%d][fan_direct=%d]\n", __FUNCTION__, __LINE__, fan_direct);
	for(i = 0; i < CHASSIS_THERMAL_COUNT; i++) {
		onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(i + 1), &thermal_info);
		thermal_temp[i + 1] = thermal_info.mcelsius / 1000;
		DEBUG_PRINT("[Debug][%s][%d][temp[%d]:%d]\n", __FUNCTION__, __LINE__, i + 1, thermal_temp[i + 1]);
	}
	/*assue flow trend is raise*/
	fan_flow_trend = T_RISE;
	pwm = cel_calculate_pwm(fan_direct, thermal_temp[sensor_num]);
	old_pwm = pwm;
	ret = cel_set_fan_speed(pwm);
	if(ret < 0) {
		printf("set fan speed fail!\n");
		return ONLP_STATUS_E_INVALID;
	}

	return 0;
}

int
onlp_sysi_platform_manage_fans(void)
{
	int temp, old_temp, bcm_temp;
	int pwm, bcm_pwm;
	static int temp_errors = 0;
#ifdef THERMAL_SIMULATE
	static int count = 27, mod=1;
#endif
	int ret;
	onlp_thermal_info_t thermal_info;

	if(fan_flow_trend == T_NOCHANGE) {
		cel_fan_init();
	}
	if(onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(sensor_num), &thermal_info) < 0) {
		if(++temp_errors > TEMP_READ_ERRORS_MAX) {
			cel_set_fan_speed(F2B_PWM_MAX);
			printf("Thermal: read temperature error more than %d times, set fan speed to max\n", TEMP_READ_ERRORS_MAX);
		}
		return ONLP_STATUS_E_INVALID;
	} else {
		temp_errors = 0;
	}
	if(cel_fans_check() < 0) {
		return ONLP_STATUS_E_INVALID;
	}
	temp = thermal_info.mcelsius / 1000;
#ifdef THERMAL_SIMULATE
	count += mod;
	temp = count;
	if(temp >= 50)
		mod = -1;
	else if(temp <= 25) {
		mod = 1;
	}
#endif	
	old_temp = thermal_temp[sensor_num];
	printf("[Debug][%s][%d][temp=%d, old_temp=%d, old_pwm=%d]\n", __FUNCTION__, __LINE__, temp, old_temp, old_pwm);
	DEBUG_PRINT("[Debug][%s][%d][temp=%d, old_temp=%d, old_pwm=%d]\n", __FUNCTION__, __LINE__, temp, old_temp, old_pwm);
	if(abs(temp - old_temp) >= TEMP_HYSTERESIS || fan_fail == 0) {
		if(temp >= old_temp) {
			fan_flow_trend = T_RISE;
		} else {
			fan_flow_trend = T_DROP;
		}
		pwm = cel_calculate_pwm(fan_direct, temp);
		if(onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(BCM), &thermal_info) >= 0) {
			bcm_temp = thermal_info.mcelsius / 1000;
		} else {
			bcm_temp = thermal_temp[BCM];
		}
		bcm_pwm = old_pwm + (765 * (bcm_temp - thermal_temp[BCM])) / 100 + 102 * (bcm_temp - 63) / 100;
		printf("[Debug][%s][%d][pwm=%d, bcm_pwm=%d]\n", __FUNCTION__, __LINE__, pwm, bcm_pwm);
		DEBUG_PRINT("[Debug][%s][%d][pwm=%d, bcm_pwm=%d]\n", __FUNCTION__, __LINE__, pwm, bcm_pwm);
		if(pwm < bcm_pwm) {
			pwm = bcm_pwm;
		}
		ret = cel_set_fan_speed(pwm);
		if(ret < 0) {
			printf("set fail\n");
			DEBUG_PRINT("[Debug][%s][%d][set pwm fail: pwm=%d]\n", __FUNCTION__, __LINE__, pwm);
			return ONLP_STATUS_E_INVALID;
		}
		thermal_temp[BCM] = bcm_temp;
		thermal_temp[sensor_num] = temp;
	}

    return ONLP_STATUS_OK;
}

int
onlp_sysi_platform_manage_leds(void)
{

	onlp_psu_info_t psu_info;
	int i;
	/* Turn orn STAT LED when system up*/
	if(onlp_ledi_set(ONLP_LED_ID_CREATE(1),1) != ONLP_STATUS_OK){
		return ONLP_STATUS_E_INVALID;
	}

	/* Validate PSU good status from mvout and decide to turn on of off*/
	for(i=1; i<=NUM_OF_PSU_ON_MAIN_BROAD;i++){
		if(onlp_psui_info_get(ONLP_PSU_ID_CREATE(i),&psu_info) == ONLP_STATUS_OK){

			/* Check power good status */
			if(psu_info.mvout>0){
				//Turn LED Orn
				if(onlp_ledi_set(ONLP_LED_ID_CREATE(i+1),1) != ONLP_STATUS_OK){
					return ONLP_STATUS_E_INVALID;    
				}

			}else{
				if(onlp_ledi_set(ONLP_LED_ID_CREATE(i+1),0) != ONLP_STATUS_OK){
					return ONLP_STATUS_E_INVALID;
				}
			}
		}else{
			DEBUG_PRINT("[Debug][%s][%d][Can't retrive PSU mvout info : pwm=%d]\n", __FUNCTION__, __LINE__, psu_info.mvout);
			return ONLP_STATUS_E_INVALID;
		}
	}

    return ONLP_STATUS_OK;
}

