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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static int is_cache_exist(){
    const char *path="/tmp/onlp-sensor-cache.txt";
    const char *time_setting_path="/var/opt/interval_time.txt";
    time_t current_time;
    double diff_time;
    int interval_time = 30; //set default to 30 sec
    struct stat fst;
    bzero(&fst,sizeof(fst));

    //Read setting
    if(access(time_setting_path, F_OK) == -1){ //Setting not exist
        return -1;
    }else{
        FILE *fp;
        
        fp = fopen(time_setting_path, "r"); // read setting
        
        if (fp == NULL)
        {
            perror("Error while opening the file.\n");
            exit(EXIT_FAILURE);
        }

        fscanf(fp,"%d", &interval_time);

        fclose(fp);
    }

    if (access(path, F_OK) == -1){ //Cache not exist
        return -1;
    }else{ //Cache exist
        current_time = time(NULL);
        if (stat(path,&fst) != 0) { printf("stat() failed"); exit(-1); }

        diff_time = difftime(current_time,fst.st_mtime);

        if(diff_time > interval_time){
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

int
onlp_sysi_platform_info_get(onlp_platform_info_t* pi)
{

    off_t offset = 0xFDC504C4;
    size_t len = 4;

    size_t pagesize = sysconf(_SC_PAGE_SIZE);
    off_t page_base = (offset / pagesize) * pagesize;
    off_t page_offset = offset - page_base;

    int fd = open("/dev/mem", O_RDWR);
    unsigned char *mem = mmap(NULL, page_offset + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
    if (mem == MAP_FAILED) {
        perror("Can't map memory");
        return -1;
    }

    close(fd);

    pi->other_versions = aim_fstrdup("PAD_CFG_DW1_UART0_TXD = %08x", *(uint32_t *)&mem[page_offset]);

    return ONLP_STATUS_OK;
}

void
onlp_sysi_platform_info_free(onlp_platform_info_t* pi)
{
    aim_free(pi->other_versions);
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
