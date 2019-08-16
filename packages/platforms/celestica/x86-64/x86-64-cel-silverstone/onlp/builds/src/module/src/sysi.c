/*
 * Copyright
 */
#include <unistd.h>
#include <fcntl.h>

#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/fani.h>
#include <onlp/platformi/sysi.h>
#include <x86_64_cel_silverstone/x86_64_cel_silverstone_config.h>

#include "x86_64_cel_silverstone_int.h"
#include "x86_64_cel_silverstone_log.h"
#include "platform.h"
//Below include add for support Cache system
#include <sys/stat.h>
#include <time.h>
#include <sys/mman.h>
#include <semaphore.h>

static char arr_cplddev_name[NUM_OF_CPLD][10] =
{
    "version"
};

const char*
onlp_sysi_platform_get(void)
{
    return "x86-64-cel-silverstone-r0";
}

// static int is_cache_exist(){
//     const char *path="/tmp/onlp-sensor-cache.txt";
//     const char *time_setting_path="/var/opt/interval_time.txt";
//     time_t current_time;
//     double diff_time;
//     int interval_time = 30; //set default to 30 sec
//     struct stat fst;
//     bzero(&fst,sizeof(fst));

//     //Read setting
//     if(access(time_setting_path, F_OK) == -1){ //Setting not exist
//         return -1;
//     }else{
//         FILE *fp;
        
//         fp = fopen(time_setting_path, "r"); // read setting
        
//         if (fp == NULL)
//         {
//             perror("Error while opening the file.\n");
//             exit(EXIT_FAILURE);
//         }

//         fscanf(fp,"%d", &interval_time);

//         fclose(fp);
//     }

//     if (access(path, F_OK) == -1){ //Cache not exist
//         return -1;
//     }else{ //Cache exist
//         current_time = time(NULL);
//         if (stat(path,&fst) != 0) { printf("stat() failed"); exit(-1); }

//         diff_time = difftime(current_time,fst.st_mtime);

//         if(diff_time > interval_time){
//             return -1;
//         }
//         return 1;
//     }
// }

int
onlp_sysi_init(void)
{
    return ONLP_STATUS_OK;
}

// static void update_shm_mem(void)
// {
//     (void)fill_shared_memory(ONLP_SENSOR_CACHE_SHARED, ONLP_SENSOR_CACHE_SEM, ONLP_SENSOR_CACHE_FILE);
//     (void)fill_shared_memory(ONLP_FRU_CACHE_SHARED, ONLP_FRU_CACHE_SEM, ONLP_FRU_CACHE_FILE);
//     (void)fill_shared_memory(ONLP_SENSOR_LIST_CACHE_SHARED, ONLP_SENSOR_LIST_SEM, ONLP_SENSOR_LIST_FILE);
// }

// static int create_cache(){
//     (void)system("ipmitool sdr > /tmp/onlp-sensor-cache.txt");
//     (void)system("ipmitool fru > /tmp/onlp-fru-cache.txt");
//     (void)system("ipmitool sensor list > /tmp/onlp-sensor-list-cache.txt");
//     update_shm_mem();
//     return 1;
// }


int
onlp_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int i, v[NUM_OF_CPLD]={0};
	char  r_data[10]   = {0};
	char  fullpath[PREFIX_PATH_LEN] = {0};

    for (i=0; i < NUM_OF_CPLD; i++) {
		memset(fullpath, 0, PREFIX_PATH_LEN);
	    sprintf(fullpath, "%s%s", SYS_CPLD_PATH, arr_cplddev_name[i]);
		if (deviceNodeReadString(fullpath, r_data, sizeof(r_data), 0) != 0) {
	        DEBUG_PRINT("%s(%d): read %s error\n", __FUNCTION__, __LINE__, fullpath);
	        return ONLP_STATUS_E_INTERNAL;
    	}
		v[i] = strtol(r_data, NULL, 0);
    }
    pi->cpld_versions = aim_fstrdup("CPLD_B=0x%02x", v[0]);
    return 0;
}


void
onlp_sysi_platform_info_free(onlp_platform_info_t* pi)
{
    aim_free(pi->cpld_versions);
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
    rdata = NULL;
    *size = 0;
    DEBUG_PRINT("[Debug][%s][%d][Can't get onie data]\n", __FUNCTION__, __LINE__);
    return ONLP_STATUS_E_INTERNAL;
}

void
onlp_sysi_onie_data_free(uint8_t* data)
{
    aim_free(data);
}

int onlp_sysi_platform_manage_init(void)
{
    //printf("Check the sequence from onlp_sysi_platform_manage_init\n");
    if(is_cache_exist()<1){
        create_cache();
    }
    return ONLP_STATUS_OK;
}

int onlp_sysi_platform_manage_fans(void){
    //printf("Check the sequence from onlp_sysi_platform_manage_fans\n");
    if(is_cache_exist()<1){
        create_cache();
    }
    return ONLP_STATUS_OK;
}

int onlp_sysi_platform_manage_leds(void){
    //printf("Check the sequence from onlp_sysi_platform_manage_leds\n");
    if(is_cache_exist()<1){
        create_cache();
    }
    return ONLP_STATUS_OK;
}

int
onlp_sysi_oids_get(onlp_oid_t* table, int max)
{
    int i;
    onlp_oid_t* e = table;

    memset(table, 0, max*sizeof(onlp_oid_t));

    /* 2 PSUs */
    *e++ = ONLP_PSU_ID_CREATE(1);
    *e++ = ONLP_PSU_ID_CREATE(2);

    // // /* LEDs Item */
    for (i=1; i<=LED_COUNT; i++)
        *e++ = ONLP_LED_ID_CREATE(i);

    // // /* THERMALs Item */
    for (i=1; i<=THERMAL_COUNT; i++)
        *e++ = ONLP_THERMAL_ID_CREATE(i);

    // /* Fans Item */
    for (i=1; i<=FAN_COUNT; i++)
        *e++ = ONLP_FAN_ID_CREATE(i);

    return ONLP_STATUS_OK;
}
