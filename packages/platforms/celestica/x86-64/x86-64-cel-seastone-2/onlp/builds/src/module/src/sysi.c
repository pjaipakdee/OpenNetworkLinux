/*
 * Copyright
 */
#include <unistd.h>
#include <fcntl.h>

#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/fani.h>
#include <onlp/platformi/sysi.h>
#include <sys/stat.h>
#include <time.h>
#include <x86_64_cel_seastone_2/x86_64_cel_seastone_2_config.h>

#include "x86_64_cel_seastone_2_int.h"
#include "x86_64_cel_seastone_2_log.h"
#include "platform.h"

static int is_cache_exist(){
    const char *path="/tmp/onlp-sensor-cache.txt";
    time_t current_time;
    double diff_time;
    struct stat fst;
    bzero(&fst,sizeof(fst));

    if (access(path, F_OK) == -1){ //Cache not exist
        return -1;
    }else{ //Cache exist
        current_time = time(NULL);
        if (stat(path,&fst) != 0) { printf("stat() failed"); exit(-1); }

        diff_time = difftime(current_time,fst.st_mtime);

        if(diff_time > 60){
            return -1;
        }
        return 1;
    }
}

static int create_cache(){
    //const char *path="/tmp/onlp-sensor-cache.txt";
    
    //  if (access(path, F_OK) == -1){ //Cache not exist
    //     printf("rm and create new file at %s\n",path);
    //     system("rm /tmp/onlp-sensor-cache.txt");
    // }else{ //Cache exist
    //     printf("just create new file at %s\n",path);
    //     system("ipmitool sdr > /tmp/onlp-sensor-cache.txt");
    //     system("ipmitool fru > /tmp/onlp-sensor-cache.txt");
    // }
    system("ipmitool sdr > /tmp/onlp-sensor-cache.txt");
    system("ipmitool fru > /tmp/onlp-fru-cache.txt");
    return 1;
}

const char*
onlp_sysi_platform_get(void)
{
    return "x86-64-cel-seastone-2-r0";
}

int
onlp_sysi_init(void)
{
    const char *path="/dev/ipmi0";
    char kernel_version[128],command[256];

    if (access(path, F_OK) == -1){
        FILE *fp = NULL;
        fp = popen("uname -r", "r");
        if (!fp) {
            printf("Error: ONLP can't get kernel version.\n");
            return -1;
        }
        fscanf(fp, "%[^\n]\n", kernel_version);

        pclose(fp);
        //printf("kernel version = %s",kernel_version);
        printf("Probing require driver\n");
        sprintf(command,"insmod /lib/modules/%s/kernel/drivers/char/ipmi/ipmi_devintf.ko",kernel_version);
        system(command);
        sprintf(command,"insmod /lib/modules/%s/kernel/drivers/char/ipmi/ipmi_si.ko",kernel_version);
        system(command);
        sprintf(command,"insmod /lib/modules/%s/kernel/drivers/char/ipmi/ipmi_ssif.ko",kernel_version);
        system(command);
    }
    return ONLP_STATUS_OK;
}

int
onlp_sysi_onie_data_get(uint8_t** data, int* size)
{
    //clock_t t;
    uint8_t* rdata = aim_zmalloc(256);
    //t = clock();
    if(onlp_file_read(rdata, 256, size, PREFIX_PATH_ON_SYS_EEPROM) == ONLP_STATUS_OK) {
        if(*size == 256) {
            *data = rdata;
            //t = clock() - t;
            //double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds 
        
            //DEBUG_PRINT("[time_debug] [%s] took %f seconds to execute \n",__FUNCTION__, time_taken);
            return ONLP_STATUS_OK;
        }
    }
    aim_free(rdata);
    rdata = NULL;
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
    //clock_t t;
    //double time_taken;
    memset(table, 0, max*sizeof(onlp_oid_t));
    //t = clock();
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

    //t = clock() - t;
    //time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds 
    //DEBUG_PRINT("[time_debug] [%s] took %f seconds to execute \n",__FUNCTION__, time_taken);
    return ONLP_STATUS_OK;
}
