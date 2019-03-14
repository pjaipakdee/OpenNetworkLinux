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
 * Fan Platform Implementation Defaults.
 *
 ***********************************************************/
#include <onlp/platformi/fani.h>
#include <onlplib/mmap.h>
#include <fcntl.h>
#include "platform_lib.h"


typedef struct fan_path_S
{
    char status[LEN_FILE_NAME];
    char speed[LEN_FILE_NAME];
    char ctrl_speed[LEN_FILE_NAME];
    char r_speed[LEN_FILE_NAME];
}fan_path_T;

#define _MAKE_FAN_PATH_ON_MAIN_BOARD(prj,id) \
    { #prj"fan"#id"_fault", #prj"fan"#id"_input", \
      #prj"pwm"#id, #prj"fan"#id"_input" }

#define MAKE_FAN_PATH_ON_MAIN_BOARD(prj,id) _MAKE_FAN_PATH_ON_MAIN_BOARD(prj,id)

#define MAKE_FAN_PATH_ON_PSU(folder) \
    {#folder"/psu_fan1_fault",  #folder"/psu_fan1_speed_rpm", \
     #folder"/psu_fan1_duty_cycle_percentage", ""  }

static fan_path_T fan_path[] =  /* must map with onlp_fan_id */
{
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, 0),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, 1),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, 2),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, 3),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, 4),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, 5),
    // MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, 6),
    // MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, 7),
    // MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, 8),
    MAKE_FAN_PATH_ON_PSU(psu1),
    MAKE_FAN_PATH_ON_PSU(psu2)
};

#define MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(id) \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##id##_ON_MAIN_BOARD), "Chassis Fan "#id, 0 }, \
        0x0, \
        (ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE), \
        0, \
        0, \
        ONLP_FAN_MODE_INVALID, \
    }

#define MAKE_FAN_INFO_NODE_ON_PSU(psu_id, fan_id) \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fan_id##_ON_PSU##psu_id), "Chassis PSU-"#psu_id " Fan "#fan_id, 0 }, \
        0x0, \
        (ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE), \
        0, \
        0, \
        ONLP_FAN_MODE_INVALID, \
    }

/* Static fan information */
onlp_fan_info_t linfo[] = {
    { }, /* Not used */
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(1),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(2),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(3),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(4),
    // MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(5),
    // MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(6),
    // MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(7),
    // MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(8),
    MAKE_FAN_INFO_NODE_ON_PSU(1,1),
    MAKE_FAN_INFO_NODE_ON_PSU(2,1),
};

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_FAN(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define OPEN_READ_FILE(fd,fullpath,data,nbytes,len) \
    DEBUG_PRINT("[Debug][%s][%d][openfile: %s]\n", __FUNCTION__, __LINE__, fullpath); \
    if ((fd = open(fullpath, O_RDONLY)) == -1)  \
       return ONLP_STATUS_E_INTERNAL;           \
    if ((len = read(fd, r_data, nbytes)) <= 0){ \
        close(fd);                              \
        return ONLP_STATUS_E_INTERNAL;}         \
    DEBUG_PRINT("[Debug][%s][%d][read data: %s]\n", __FUNCTION__, __LINE__, r_data); \
    if (close(fd) == -1)                        \
        return ONLP_STATUS_E_INTERNAL

// static int fan_order[CHASSIS_FAN_COUNT] = {2, 1, 5, 3, 4};
// static int ecm2305_1_ch[EMC2305_CHAN_NUM] = {2, 4, 5, 3, 1};
// static int ecm2305_2_ch[EMC2305_CHAN_NUM] = {2, 1, 4, 5, 3};

//static int fan_order[CHASSIS_FAN_COUNT] = {2, 1, 5, 3};
static int fan_order[CHASSIS_FAN_COUNT] = {1, 0, 4, 2};
static int ecm2305_1_ch[EMC2305_CHAN_NUM] = {2, 4, 3, 1};
static int ecm2305_2_ch[EMC2305_CHAN_NUM] = {2, 1, 5, 3};

int
onlp_fani_info_get_fan_direction(int id)
{
	int i = 0, direct = ONLP_FAN_STATUS_FAILED;
	int   nbytes = 0;
    char  r_data[10]   = {0};
    char  fullpath[PREFIX_PATH_LEN] = {0};

	if(id > CHASSIS_FAN_COUNT || id <= 0) {
		id = 1;
	}
	for(i = 0; i < CHASSIS_FAN_COUNT; i++) {
		if(id == fan_order[i])
			break;
	}
	sprintf(fullpath, "%sgpio%d/value", PREFIX_PATH_ON_GPIO, FAN_DIRECTOR_GPIO_BASE + fan_order[id-1]);	
    //printf("[fn %s] id=%d fan_order=%d GPIO path %s\n",__FUNCTION__,id,fan_order[id-1],fullpath);
	if(deviceNodeReadString(fullpath, r_data, sizeof(r_data), nbytes) < 0) {
		printf("%s: read %s error\n", __FUNCTION__, fullpath);
	}
	direct = (atoi(r_data) == 1) ? ONLP_FAN_STATUS_F2B : ONLP_FAN_STATUS_B2F;

    
    return direct;
}

static int
onlp_fani_info_get_fan_present(int id)
{
	int i = 0, present = ONLP_FAN_STATUS_FAILED;
	int   nbytes = 0;
    char  r_data[10]   = {0};
    char  fullpath[PREFIX_PATH_LEN] = {0};

	if(id > CHASSIS_FAN_COUNT || id <= 0) {
		id = 1;
	}
	for(i = 0; i < CHASSIS_FAN_COUNT; i++) {
		if(id == fan_order[i])
			break;
	}
	sprintf(fullpath, "%sgpio%d/value", PREFIX_PATH_ON_GPIO, FAN_ABS_GPIO_BASE + fan_order[id-1]);	
    //printf("[fn %s] id=%d fan_order=%d GPIO path %s\n",__FUNCTION__,id,fan_order[id-1],fullpath);
	if(deviceNodeReadString(fullpath, r_data, sizeof(r_data), nbytes) < 0) {
		printf("%s: read %s error\n", __FUNCTION__, fullpath);
	}
	present = (atoi(r_data) == 0) ? ONLP_FAN_STATUS_PRESENT : ONLP_FAN_STATUS_FAILED;

    
    return present;
}

    
static int
_onlp_fani_info_get_fan(int local_id, onlp_fan_info_t* info)
{
    int   fd, len, nbytes = 10;
	int emc2305_idx, rear_index;
	int id1, id2;
    char  r_data[10]   = {0};
    char  fullpath[PREFIX_PATH_LEN] = {0};

	/* get fan direction */
    info->status = onlp_fani_info_get_fan_direction(local_id);
	if(info->status & ONLP_FAN_STATUS_F2B) {
		emc2305_idx = 2;
		rear_index = 1;
	} else {
		emc2305_idx = 1;
		rear_index = 2;
	}
	id1 = ecm2305_1_ch[local_id - 1];
	id2 = ecm2305_2_ch[local_id - 1];
	//printf("local id=%d  id1=%d id2=%d\n",local_id,id1,id2);
    /* get fan fault status (turn on when any one fails)*/
	memset(fullpath, 0, PREFIX_PATH_LEN);
    sprintf(fullpath, "%s/%d/%s", PREFIX_PATH_ON_MAIN_BOARD, emc2305_idx, fan_path[id1].status);	
    OPEN_READ_FILE(fd,fullpath,r_data,nbytes,len);
    if (atoi(r_data) > 0) {
        info->status |= ONLP_FAN_STATUS_FAILED;
        return ONLP_STATUS_OK;
    }


    /* get fan speed (take the min from two speeds)*/
	memset(fullpath, 0, PREFIX_PATH_LEN);
    sprintf(fullpath, "%s/%d/%s", PREFIX_PATH_ON_MAIN_BOARD, emc2305_idx, fan_path[id1].speed);	
    OPEN_READ_FILE(fd,fullpath,r_data,nbytes,len);
    info->rpm = atoi(r_data);

	memset(fullpath, 0, PREFIX_PATH_LEN);
    sprintf(fullpath, "%s/%d/%s", PREFIX_PATH_ON_MAIN_BOARD, rear_index, fan_path[id2].r_speed);	
    OPEN_READ_FILE(fd,fullpath,r_data,nbytes,len);
	memset(fullpath, 0, PREFIX_PATH_LEN);
    if (info->rpm > atoi(r_data)) {
        info->rpm = atoi(r_data);
		sprintf(fullpath, "%s/%d/%s", PREFIX_PATH_ON_MAIN_BOARD, rear_index, fan_path[id2].ctrl_speed);
    } else {
		sprintf(fullpath, "%s/%d/%s", PREFIX_PATH_ON_MAIN_BOARD, emc2305_idx, fan_path[id1].ctrl_speed);
    }
    /* get speed percentage*/	
    OPEN_READ_FILE(fd,fullpath,r_data,nbytes,len);
    info->percentage = (atoi(r_data) * 100) / 255;      

	/* check present */
	info->status |= onlp_fani_info_get_fan_present(local_id);

    return ONLP_STATUS_OK;
}

static int
_onlp_fani_info_get_fan_on_psu(int local_id, onlp_fan_info_t* info)
{
    int   fd, len, nbytes = 10;
    char  r_data[10]   = {0};
    char  fullpath[PREFIX_PATH_LEN] = {0};

    //printf("local id=%d\n",local_id);
    /* get fan direction */
    info->status = onlp_fani_info_get_fan_direction(local_id);

    /* get fan fault status */
	memset(fullpath, 0, PREFIX_PATH_LEN);
    sprintf(fullpath, "%s%s", PREFIX_PATH_ON_PSU, fan_path[local_id].status);	
    OPEN_READ_FILE(fd,fullpath,r_data,nbytes,len);
    info->status |= (atoi(r_data) > 0) ? ONLP_FAN_STATUS_FAILED : 0;
    
    /* get fan speed*/
	memset(fullpath, 0, PREFIX_PATH_LEN);
    sprintf(fullpath, "%s%s", PREFIX_PATH_ON_PSU, fan_path[local_id].speed);	
    OPEN_READ_FILE(fd,fullpath,r_data,nbytes,len);    
    info->rpm = atoi(r_data); 

    /* get speed percentage*/
	memset(fullpath, 0, PREFIX_PATH_LEN);
    sprintf(fullpath, "%s%s", PREFIX_PATH_ON_PSU, fan_path[local_id].ctrl_speed);	
    OPEN_READ_FILE(fd,fullpath,r_data,nbytes,len);
    info->percentage = atoi(r_data);
	if (info->rpm > 0) {
    	info->status |= ONLP_FAN_STATUS_PRESENT;
	}

    return ONLP_STATUS_OK;
}

/*
 * This function will be called prior to all of onlp_fani_* functions.
 */
int
onlp_fani_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* info)
{
    int rc = 0;
    int local_id;
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    *info = linfo[local_id];
    //printf("local_id in %s = %d\n",__FUNCTION__,local_id);
    switch (local_id)
    {
	    case FAN_1_ON_PSU1:
        case FAN_1_ON_PSU2:
            
            rc = _onlp_fani_info_get_fan_on_psu(local_id+1, info);						
            break;
        case FAN_1_ON_MAIN_BOARD:
        case FAN_2_ON_MAIN_BOARD:
        case FAN_3_ON_MAIN_BOARD:
        case FAN_4_ON_MAIN_BOARD:
        // case FAN_5_ON_MAIN_BOARD:
        // case FAN_6_ON_MAIN_BOARD:
        // case FAN_7_ON_MAIN_BOARD:
        // case FAN_8_ON_MAIN_BOARD:
            rc =_onlp_fani_info_get_fan(local_id, info);						
            break;
        default:
            rc = ONLP_STATUS_E_INVALID;
            break;
    }	
    
    return rc;
}

/*
 * This function sets the speed of the given fan in RPM.
 *
 * This function will only be called if the fan supprots the RPM_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int
onlp_fani_rpm_set(onlp_oid_t id, int rpm)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function sets the fan speed of the given OID as a percentage.
 *
 * This will only be called if the OID has the PERCENTAGE_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int
onlp_fani_percentage_set(onlp_oid_t id, int p)
{
    int local_id;
	int pwm;
    char fullpath[PREFIX_PATH_LEN] = {0};

    VALIDATE(id);
	
    local_id = ONLP_OID_ID_GET(id);

    /* reject p=0 (p=0, stop fan) */
    if (p == 0){
        return ONLP_STATUS_E_INVALID;
    }

	memset(fullpath, 0, PREFIX_PATH_LEN);
    /* get fullpath */
    switch (local_id)
	{
        case FAN_1_ON_PSU1:
        case FAN_1_ON_PSU2:
            sprintf(fullpath, "%s%s", PREFIX_PATH_ON_PSU, fan_path[local_id].ctrl_speed);
			deviceNodeWriteInt(fullpath, p, sizeof(p));
			DEBUG_PRINT("[Debug][%s][%d][openfile: %s][data=%d]\n", __FUNCTION__, __LINE__, fullpath, p);
            break;
        case FAN_1_ON_MAIN_BOARD:
        case FAN_2_ON_MAIN_BOARD:
        case FAN_3_ON_MAIN_BOARD:
        case FAN_4_ON_MAIN_BOARD:
        // case FAN_5_ON_MAIN_BOARD:
			pwm = (p * 255) / 100;
            sprintf(fullpath, "%s/%d/%s", PREFIX_PATH_ON_MAIN_BOARD, 1, fan_path[local_id].ctrl_speed);
			deviceNodeWriteInt(fullpath, pwm, sizeof(pwm));
			DEBUG_PRINT("[Debug][%s][%d][openfile: %s][data=%d]\n", __FUNCTION__, __LINE__, fullpath, pwm);
			memset(fullpath, 0, PREFIX_PATH_LEN);
			sprintf(fullpath, "%s/%d/%s", PREFIX_PATH_ON_MAIN_BOARD, 2, fan_path[local_id].ctrl_speed);
			deviceNodeWriteInt(fullpath, pwm, sizeof(pwm));
			DEBUG_PRINT("[Debug][%s][%d][openfile: %s][data=%d]\n", __FUNCTION__, __LINE__, fullpath, pwm);
            break;
        default:
            return ONLP_STATUS_E_INVALID;
    }
    

    return ONLP_STATUS_OK;
}


/*
 * This function sets the fan speed of the given OID as per
 * the predefined ONLP fan speed modes: off, slow, normal, fast, max.
 *
 * Interpretation of these modes is up to the platform.
 *
 */
int
onlp_fani_mode_set(onlp_oid_t id, onlp_fan_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function sets the fan direction of the given OID.
 *
 * This function is only relevant if the fan OID supports both direction
 * capabilities.
 *
 * This function is optional unless the functionality is available.
 */
int
onlp_fani_dir_set(onlp_oid_t id, onlp_fan_dir_t dir)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * Generic fan ioctl. Optional.
 */
int
onlp_fani_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

