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
#ifndef __PLATFORM_LIB_H__
#define __PLATFORM_LIB_H__

#include "x86_64_cel_redstone_xp_log.h"

#define NUM_OF_CPLD           5


#define CHASSIS_FAN_COUNT     4
#define CHASSIS_THERMAL_COUNT 4
#define CHASSIS_SFP_NUM_MAX   54
#define CHASSIS_SFP_I2C_BUS_BASE 2 //lpc to i2c bus start num is 2
#define NUM_OF_PSU_ON_MAIN_BROAD      2
#define NUM_OF_LED_ON_MAIN_BROAD      3
#define PSU1_ID 1
#define PSU2_ID 2

#define PSU1_AC_PMBUS_PREFIX "/sys/celestica/psu/psu1/"
#define PSU2_AC_PMBUS_PREFIX "/sys/celestica/psu/psu2/"

#define PSU1_AC_PMBUS_NODE(node) PSU1_AC_PMBUS_PREFIX#node
#define PSU2_AC_PMBUS_NODE(node) PSU2_AC_PMBUS_PREFIX#node

//#define PSU1_AC_HWMON_PREFIX "/sys/bus/i2c/devices/10-0050/"
//#define PSU2_AC_HWMON_PREFIX "/sys/bus/i2c/devices/11-0053/"

//#define PSU1_AC_HWMON_NODE(node) PSU1_AC_HWMON_PREFIX#node
//#define PSU2_AC_HWMON_NODE(node) PSU2_AC_HWMON_PREFIX#node

#define PREFIX_PATH_ON_SYS_EEPROM "/sys/celestica/tlv_eeprom/eeprom"
#define PREFIX_PATH_ON_MAIN_BOARD   "/sys/celestica/fantray/"
#define PREFIX_PATH_ON_PSU          "/sys/celestica/psu/"
#define PREFIX_PATH_ON_GPIO         "/sys/class/gpio/"
#define PREFIX_PATH_ON_SFP          "/sys/celestica/sff/"
#define PREFIX_PATH_ON_LED          "/sys/celestica/leds/leds/"
#define PREFIX_PATH_ON_LED_BLINK_NODE   "/sys/celestica/leds/blink"

#define PREFIX_PATH_ON_CPLD       "/sys/celestica/cpld/"


#define PREFIX_PATH_LEN        100
#define FAN_ABS_GPIO_BASE (472 + 10)
#define FAN_DIRECTOR_GPIO_BASE (472 + 15) //487

#define MAX_FAN_SPEED     18000
#define MAX_PSU_FAN_SPEED 25500

#define PROJECT_NAME
#define LEN_FILE_NAME 80

#define EMC2305_NUM 2
#define EMC2305_CHAN_NUM 4

typedef enum{
	FAN_RESERVED = 0,
	FAN_1_ON_MAIN_BOARD,
	FAN_2_ON_MAIN_BOARD,
	FAN_3_ON_MAIN_BOARD,
	FAN_4_ON_MAIN_BOARD,
	// FAN_5_ON_MAIN_BOARD,
	// FAN_6_ON_MAIN_BOARD,
	// FAN_7_ON_MAIN_BOARD,
	// FAN_8_ON_MAIN_BOARD,
	FAN_1_ON_PSU1,
	FAN_1_ON_PSU2,
}FAN_T;


int deviceNodeWriteInt(char *filename, int value, int data_len);
int deviceNodeReadBinary(char *filename, char *buffer, int buf_size, int data_len);
int deviceNodeReadString(char *filename, char *buffer, int buf_size, int data_len);
int onlp_fani_info_get_fan_direction(int id);

typedef enum psu_type {
    PSU_TYPE_UNKNOWN,
    PSU_TYPE_AC_F2B,
    PSU_TYPE_AC_B2F
} psu_type_t;


#define DEBUG_MODE 1

#if (DEBUG_MODE == 1)
    #define DEBUG_PRINT(format, ...)   printf(format, __VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

#endif  /* __PLATFORM_LIB_H__ */

