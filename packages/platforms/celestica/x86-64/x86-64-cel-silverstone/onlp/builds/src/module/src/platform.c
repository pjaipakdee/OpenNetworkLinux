//////////////////////////////////////////////////////////////
//   PLATFORM FUNCTION TO INTERACT WITH SYS_CPLD AND BMC    //
//////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/io.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>

#include "platform.h"

char command[256];
FILE *fp;
static char *sdr_value = NULL;

static struct device_info fan_information[FAN_COUNT + 1] = {
    {"unknown", "unknown"}, //check
    {}, //Fan 1
    {}, //Fan 2
    {}, //Fan 3
    {}, //Fan 4
    {},
    {},
    {},
};

static struct device_info psu_information[PSU_COUNT + 1] = {
    {"unknown", "unknown"}, //check
    {}, //PSU 1
    {}, //PSU 2
};

static const struct fan_config_p fan_sys_reg[FAN_COUNT + 1] = {
    {},
    {0x22, 0x26, 0x21, 0x20}, //FAN_1
    {0x32, 0x36, 0x31, 0x30}, //FAN_2
    {0x42, 0x46, 0x41, 0x40}, //FAN_3
    {0x52, 0x56, 0x51, 0x50}, //FAN_4
    {0x62, 0x66, 0x61, 0x60}, //FAN_5
    {0x72, 0x76, 0x71, 0x70}, //FAN_6
    {0x82, 0x86, 0x81, 0x80}, //FAN_7

};

static const struct led_reg_mapper led_mapper[LED_COUNT + 1] = {
    {},
    {"LED_SYSTEM", LED_SYSTEM_H, LED_SYSTEM_REGISTER},
    {"LED_FAN_1", LED_FAN_1_H, 0x24},
    {"LED_FAN_2", LED_FAN_2_H, 0x34},
    {"LED_FAN_3", LED_FAN_3_H, 0x44},
    {"LED_FAN_4", LED_FAN_4_H, 0x54},
    {"LED_FAN_5", LED_FAN_5_H, 0x64},
    {"LED_FAN_6", LED_FAN_6_H, 0x74},
    {"LED_FAN_7", LED_FAN_7_H, 0x84},
    {"LED_ALARM", LED_ALARM_H, ALARM_REGISTER},
    {"LED_PSU_LEFT", LED_PSU_L_H, PSU_LED_REGISTER},
    {"LED_PSU_RIGHT", LED_PSU_R_H, PSU_LED_REGISTER},
};

static const struct psu_reg_bit_mapper psu_mapper [PSU_COUNT + 1] = {
    {},
    {0xa160, 3, 7, 1},
    {0xa160, 2, 6, 0},
};

static const struct search_psu_sdr_info_mapper search_psu_sdr_info[12] = {
    {"PSU1_VIn", 'V'},
    {"PSU1_CIn", 'A'},
    {"PSU1_PIn", 'W'},
    {"PSU1_VOut", 'V'},
    {"PSU1_COut", 'A'},
    {"PSU1_POut", 'W'},
    {"PSU2_VIn", 'V'},
    {"PSU2_CIn", 'A'},
    {"PSU2_PIn", 'W'},
    {"PSU2_VOut", 'V'},
    {"PSU2_COut", 'A'},
    {"PSU2_POut", 'W'},
};

void update_shm_mem(void)
{
    (void)fill_shared_memory(ONLP_SENSOR_CACHE_SHARED, ONLP_SENSOR_CACHE_SEM, ONLP_SENSOR_CACHE_FILE);
    (void)fill_shared_memory(ONLP_FRU_CACHE_SHARED, ONLP_FRU_CACHE_SEM, ONLP_FRU_CACHE_FILE);
    (void)fill_shared_memory(ONLP_SENSOR_LIST_CACHE_SHARED, ONLP_SENSOR_LIST_SEM, ONLP_SENSOR_LIST_FILE);
}

int is_cache_exist(){
    const char *path="/tmp/onlp-sensor-cache.txt";
    const char *time_setting_path="/var/opt/interval_time.txt";
    time_t current_time;
    int interval_time = 30; //set default to 30 sec
    double diff_time;
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

int is_shm_mem_ready(){

    const char *sdr_cache_path="/run/shm/onlp-sensor-list-cache-shared";
    const char *fru_cache_path="/run/shm/onlp-fru-cache-shared";

    if(access(fru_cache_path, F_OK) == -1 || access(sdr_cache_path, F_OK) == -1 ){ //Shared cache files not exist
        return 0;
    }

    return 1;
}

int create_cache(){
    (void)system("ipmitool sdr > /tmp/onlp-sensor-cache.txt");
    (void)system("ipmitool fru > /tmp/onlp-fru-cache.txt");
    (void)system("ipmitool sensor list > /tmp/onlp-sensor-list-cache.txt");
    update_shm_mem();
    return 1;
}

uint8_t read_register(uint16_t dev_reg)
{
    int status;

    sprintf(command, "echo 0x%x >  %sgetreg", dev_reg, SYS_CPLD_PATH);

    fp = popen(command, "r");
    if (!fp)
    {
        printf("Failed : Can't specify CPLD register\n");
        return -1;
    }
    pclose(fp);
    fp = popen("cat " SYS_CPLD_PATH "getreg", "r");
    if (!fp)
    {
        printf("Failed : Can't open sysfs\n");
        return -1;
    }
    fscanf(fp, "%x", &status);
    pclose(fp);

    return status;
}

int exec_ipmitool_cmd(char *cmd, char *retd)
{
    int ret = 0;
    int i = 0;
    char c;
    FILE *pFd = NULL;

    pFd = popen(cmd, "r");
    if (pFd != NULL)
    {
        c = fgetc(pFd);
        while (c != EOF)
        {
            //printf ("%c", c);
            retd[i] = c;
            i++;
            c = fgetc(pFd);
        }
        pclose(pFd);
    }

    return ret;
}

int fan_cpld_read_reg(uint8_t reg, uint8_t *result)
{
    int ret = 0;
    char command[256];
    char buffer[128];

    sprintf(command, "ipmitool raw 0x3a 0x03 0x01 0x01 0x%02x", reg);
    exec_ipmitool_cmd(command, buffer);
    *result = strtol(buffer, NULL, 16);
    
    return ret;
}

uint8_t getFanPresent(int id)
{
    int ret = -1;
    uint16_t fan_stat_reg;
    uint8_t result;

    if (id <= (FAN_COUNT))
    {
        fan_stat_reg = fan_sys_reg[id].ctrl_sta_reg;
        fan_cpld_read_reg(fan_stat_reg, &result);
        ret = result;
    }

    return ret;
}

uint8_t getFanSpeed(int id)
{
    uint8_t value;

    if (id <= (FAN_COUNT))
    {
        uint16_t fan_stat_reg;
        fan_stat_reg = fan_sys_reg[id].pwm_reg;
        fan_cpld_read_reg(fan_stat_reg, &value);
    }

    return value;
}

uint8_t getLEDStatus(int id)
{
    uint8_t ret = 0xFF;

    if (id >= (LED_COUNT + 2) || id < 0)
        return 0xFF;

    if (id <= (LED_COUNT))
    {
        uint8_t result = 0;
        uint16_t led_stat_reg;
        led_stat_reg = led_mapper[id].dev_reg;
        if(id >=2 && id <= 8){
            fan_cpld_read_reg(led_stat_reg,&result);
        }else{
            result = read_register(led_stat_reg);
        }        
        ret = result;
    }

    return ret;
}

char *read_tmp_cache(char *cmd)
{
    FILE *pFd = NULL;
    char c; 
    char *str = (char *)malloc(sizeof(char) * 16384);
    memset (str, 0, sizeof (char) * 16384);
    int i = 0;

    pFd = popen(cmd, "r");
    if (pFd != NULL)
    {
        c = fgetc(pFd);
        while (c != EOF)
        {
            str[i] = c;
            i++;
            c = fgetc(pFd);
        }

        pclose(pFd);
    }
    else
    {
        printf("execute command %s failed\n", command);
    }

    return str;
}

uint8_t getPsuStatus_sysfs_cpld(int id)
{
    uint8_t ret = 0xFF;
    uint16_t psu_stat_reg;

    if (id <= (PSU_COUNT))
    {
        uint8_t result = 0;
        psu_stat_reg = psu_mapper[id].sta_reg;
        result = read_register(psu_stat_reg);
        ret = result;
    }

    return ret;
}

char *read_psu_sdr(int id)
{
    FILE *pFd = NULL;
    char c;
    char *str = (char *)malloc(sizeof(char) * 5000);
    int i = 0;

    sprintf(command,"cat /tmp/onlp-sensor-cache.txt | grep PSU");
    pFd = popen(command, "r");
    if (pFd != NULL)
    {

        c = fgetc(pFd);
        while (c != EOF)
        {
            str[i] = c;
            i++;
            c = fgetc(pFd);
        }

        pclose(pFd);
    }
    else
    {
        printf("execute command %s failed\n", command);
    }

    return str;
}

int psu_get_info(int id, int *mvin, int *mvout, int *mpin, int *mpout, int *miin, int *miout)
{
    int ret = 0;
    int search_length = 39;
    int value_length = 19;
    int position = 0;
    int val_pos = 0;
    int str_index = 0;
    int val[12]={0,0,0,0,0,0,0,0,0,0,0,0};
    char ctemp[18] = "\0";
    float ftemp = 0.0;

    if(sdr_value == NULL)
        sdr_value = read_psu_sdr(id);

    if(id == 2)
        str_index = 6;

    for (; str_index < NELEMS(search_psu_sdr_info); str_index++)
    {
        position = keyword_match(sdr_value, search_psu_sdr_info[str_index].keyword);

        if (position != -1)
        {
            int status_pos = position + search_length;
            if (sdr_value[status_pos] != 'n')
            {
                int v_pos = position + value_length;
                int a = 0;
                for (a = 0; a <= value_length-1; a++)
                {
                    if (sdr_value[v_pos + a] != search_psu_sdr_info[str_index].unit)
                    {
                        append(ctemp, sdr_value[v_pos + a]);
                    }
                    else
                    {
                        ftemp = atof(ctemp);
                        val[val_pos] = ftemp * 1000;
                        memset(ctemp, 0, sizeof(ctemp));
                        val_pos++;
                        break;
                    }
                }
            }
        }
    }

    //free malloc after complete use sdr_value variable
    if(id >= PSU_COUNT)
    {
        free(sdr_value);
        sdr_value=NULL;
    }

    *mvin = val[0];
    *miin = val[1];
    *mpin = val[2];
    *mvout = val[3];
    *miout = val[4];
    *mpout = val[5];

    return ret;
}

int psu_get_model_sn(int id, char *model, char *serial_number)
{
    int index;
    char *token;
    char *tmp = (char *)NULL;
    int len = 0;
    int ret = -1;
    int search_psu_id = 1;

    if (0 == strcasecmp(psu_information[0].model, "unknown")) {
        
        index = 0;
        if(is_shm_mem_ready()){
            ret = open_file(ONLP_FRU_CACHE_SHARED,ONLP_FRU_CACHE_SEM, &tmp, &len);
            if(ret < 0 || !tmp){
                printf("Failed - Failed to obtain system information\n");
                (void)free(tmp);
                tmp = (char *)NULL;
                return ret;
            }
        }else{
            // use unsafe method to read the cache file.
            sprintf(command, "cat %s",ONLP_FRU_CACHE_FILE);
            tmp = read_tmp_cache(command);
        }
        
        char *content, *temp_pointer;
        int flag = 0;

        content = strtok_r(tmp, "\n", &temp_pointer);

        while(content != NULL){
            if (strstr(content, "FRU Device Description : FRU_PSU")) {
                flag = 1;
                index++;
            }
            if(flag == 1){
                if (strstr(content, "Board Product")) {
                    token = strtok(content, ":");
                    token = strtok(NULL, ":");
                    char* trim_token = trim(token);
                    sprintf(psu_information[index].model,"%s",trim_token);
                }
                else if (strstr(content, "Board Serial")) {
                    token = strtok(content, ":");
                    token = strtok(NULL, ":");
                    char* trim_token = trim(token);
                    sprintf(psu_information[index].serial_number,"%s",trim_token);
                    flag = 0;
                    search_psu_id++;
                }
            }
            if(search_psu_id > PSU_COUNT){
                content = NULL;
            }else{
                content = strtok_r(NULL, "\n", &temp_pointer);
            }
        }

        sprintf(psu_information[0].model,"pass"); //Mark as complete

        if(temp_pointer){
            temp_pointer = (char *)NULL;
        }
        if(tmp){
    	    (void)free(tmp);
	        tmp = (char *)NULL;
        }
    }

    strcpy(model, psu_information[id].model);
    strcpy(serial_number, psu_information[id].serial_number);

    return 1;
}

void append(char *s, char c)
{
    int len = strlen(s);
    s[len] = c;
    s[len + 1] = '\0';
}

int keyword_match(char *a, char *b)
{
    int position = 0;
    char *x, *y;

    x = a;
    y = b;

    while (*a)
    {
        while (*x == *y)
        {
            x++;
            y++;
            if (*x == '\0' || *y == '\0')
                break;
        }
        if (*y == '\0')
            break;

        a++;
        position++;
        x = a;
        y = b;
    }
    if (*a)
        return position;
    else
        return -1;
}

int getFaninfo(int id, char *model, char *serial)
{
    int index;
    char *token;
    char *tmp = (char *)NULL;
    char search_header[35];
    int len = 0;
    int ret = -1;
    int search_fan_id = 1;

    if (0 == strcasecmp(fan_information[0].model, "unknown")) {
        index = 0;
        if(is_shm_mem_ready()){
            ret = open_file(ONLP_FRU_CACHE_SHARED,ONLP_FRU_CACHE_SEM, &tmp, &len);
            if(ret < 0 || !tmp){
                printf("Failed - Failed to obtain system information\n");
                (void)free(tmp);
                tmp = (char *)NULL;
                return ret;
            }
        }else{
            sprintf(command, "cat %s",ONLP_FRU_CACHE_FILE);
            tmp = read_tmp_cache(command);
        }
        char *content, *temp_pointer;
        int flag = 0;
        content = strtok_r(tmp, "\n", &temp_pointer);

        while(content != NULL){
            sprintf(search_header,"FRU Device Description : FRU_FAN%d",search_fan_id);
            if (strstr(content, search_header)) {
                flag = 1;
                index++;
            }
            if(flag == 1){
                if (strstr(content, "Board Serial")) {
                    token = strtok(content, ":");
                    token = strtok(NULL, ":");
                    char* trim_token = trim(token);
                    sprintf(fan_information[index].serial_number,"%s",trim_token);
                }
                else if (strstr(content, "Board Part Number")) {
                    token = strtok(content, ":");
                    token = strtok(NULL, ":");
                    char* trim_token = trim(token);
                    sprintf(fan_information[index].model,"%s",trim_token);
                    flag = 0;
                    search_fan_id++;
                }
            }
            if(search_fan_id > FAN_COUNT){
                content = NULL;
            }else{
                content = strtok_r(NULL, "\n", &temp_pointer);
            }
        }

        sprintf(fan_information[0].model,"pass"); //Mark as complete

        if(temp_pointer){
            temp_pointer = (char *)NULL;
        }
        if(tmp){
    	    (void)free(tmp);
	        tmp = (char *)NULL;
        }
    }
    strcpy(model, fan_information[id].model);
    strcpy(serial, fan_information[id].serial_number);

    
    return 1;
}

void array_trim(char *strIn, char *strOut)
{
    int i, j;

    i = 0;
    j = strlen(strIn) - 1;

    while(strIn[i] == ' ') ++i;
    while(strIn[j] == ' ') --j;

    strncpy(strOut, strIn + i , j - i + 1);
    strOut[j - i + 1] = '\0';
}

int getSensorInfo(int id, int *temp, int *warn, int *error, int *shutdown)
{
    char *tmp = (char *)NULL;
    int len = 0;
    int index  = 0;

	int i = 0;
	int ret = 0;
    char strTmp[10][128] = {{0}, {0}};
    char *token = NULL;
    char *Thermal_sensor_name[13] = {
        "TEMP_CPU", "TEMP_BB", "TEMP_SW_U16", "TEMP_SW_U52",
        "TEMP_FAN_U17", "TEMP_FAN_U52", "PSU1_Temp1", "PSU1_Temp2",
        "PSU2_Temp1", "PSU2_Temp2","SW_U04_Temp","SW_U14_Temp","SW_U4403_Temp"};

	if((NULL == temp) || (NULL == warn) || (NULL == error) || (NULL == shutdown))
	{
		printf("%s null pointer!\n", __FUNCTION__);
		return -1;
	}

    /*
        String example:			  
        ipmitool sensor list | grep TEMP_FAN_U52
        TEMP_CPU     | 1.000      | degrees C  | ok  | 5.000  | 9.000  | 16.000  | 65.000  | 73.000  | 75.606
        TEMP_FAN_U52 | 32.000	  | degrees C  | ok  | na  |  na  | na  | na  | 70.000  | 75.000
        PSUR_Temp1   | na         | degrees C  | na  | na  | na   | na   | na  | na  | na
    */
    if(is_shm_mem_ready()){
        ret = open_file(ONLP_SENSOR_LIST_CACHE_SHARED,ONLP_SENSOR_LIST_SEM, &tmp, &len);
        if(ret < 0 || !tmp){
            printf("Failed - Failed to obtain system information\n");
            (void)free(tmp);
            tmp = (char *)NULL;
            return ret;
        }
    }else{
        // use unsafe method to read the cache file.
        sprintf(command, "cat %s",ONLP_SENSOR_LIST_FILE);
        tmp = read_tmp_cache(command);
    }
    
    char *content, *temp_pointer;
    int flag = 0;
    content = strtok_r(tmp, "\n", &temp_pointer);
    while(content != NULL){
        if (strstr(content, Thermal_sensor_name[id - 1])) {
            flag = 1;
            index++;
        }
        if(flag == 1){

            i = 0;
            token = strtok(content, "|");
            while( token != NULL ) 
            {
                array_trim(token, &strTmp[i][0]);
                i++;
                if(i > 10) break;
                token = strtok(NULL, "|");
            }
            
            flag = 3;
        }
        
        if(flag == 3){
            content = NULL;
        }else{
            content = strtok_r(NULL, "\n", &temp_pointer);
        }
    }

    if(0 == strcmp(&strTmp[1][0], "na")) return -1;
    else *temp = atof(&strTmp[1][0]) * 1000.0;

    if(0 == strcmp(&strTmp[7][0], "na")) *warn = 0;	
    else *warn = atof(&strTmp[7][0]) * 1000.0;

    if(0 == strcmp(&strTmp[8][0], "na")) *error = 0;	
    else *error = atof(&strTmp[8][0]) * 1000.0;

    if(0 == strcmp(&strTmp[9][0], "na")) *shutdown = 0;	
    else *shutdown = atof(&strTmp[9][0]) * 1000.0;

    if(content){
        content = (char *)NULL;
    }
    if(temp_pointer){
        temp_pointer = (char *)NULL;
    }
    if(tmp){
    	(void)free(tmp);
	    tmp = (char *)NULL;
    }

    return 0;
}

int deviceNodeReadBinary(char *filename, char *buffer, int buf_size, int data_len)
{
    int fd;
    int len;

    if ((buffer == NULL) || (buf_size < 0))
    {
        return -1;
    }

    if ((fd = open(filename, O_RDONLY)) == -1)
    {
        return -1;
    }

    if ((len = read(fd, buffer, buf_size)) < 0)
    {
        close(fd);
        return -1;
    }

    if ((close(fd) == -1))
    {
        return -1;
    }

    if ((len > buf_size) || (data_len != 0 && len != data_len))
    {
        return -1;
    }

    return 0;
}

int deviceNodeReadString(char *filename, char *buffer, int buf_size, int data_len)
{
    int ret;

    if (data_len >= buf_size)
    {
        return -1;
    }

    ret = deviceNodeReadBinary(filename, buffer, buf_size - 1, data_len);
    if (ret == 0)
    {
        buffer[buf_size - 1] = '\0';
    }

    return ret;
}

char* trim (char *s)
{
    int i;

    while (isspace (*s)) s++;   // skip left side white spaces
    for (i = strlen (s) - 1; (isspace (s[i])); i--) ;   // skip right side white spaces
    s[i + 1] = '\0';
    
    return s;
}

int fill_shared_memory(const char *shm_path, const char *sem_path, const char *cache_path)
{
    int seg_size = 0;    
    int shm_fd = -1;   
    struct shm_map_data * shm_map_ptr = (struct shm_map_data *)NULL;

    if(!shm_path || !sem_path || !cache_path){
	return -1;
    }

    seg_size = sizeof(struct shm_map_data);

    shm_fd = shm_open(shm_path, O_CREAT|O_RDWR, S_IRWXU|S_IRWXG);
    if(shm_fd < 0){
        
	printf("\nshm_path:%s. errno:%d\n", shm_path, errno);
        return -1;
    }   
 
    ftruncate(shm_fd, seg_size);

    shm_map_ptr = (struct shm_map_data *)mmap(NULL, seg_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0); 
    if(shm_map_ptr == MAP_FAILED){
	    printf("\nMAP_FAILED. errno:%d.\n", errno);
    	close(shm_fd);
        return -1;
    }

    if(access(cache_path, F_OK) == -1)
    {
        munmap(shm_map_ptr, seg_size);
        close(shm_fd);
        return -1;
    }
 
    struct stat sta;
    stat(cache_path, &sta);
    int st_size = sta.st_size;
    if(st_size == 0){
        munmap(shm_map_ptr, seg_size);
	    close(shm_fd);
	    return -1;
    }

    char *cache_buffer = (char *)malloc(st_size); 
    if(!cache_buffer){ 
        munmap(shm_map_ptr, seg_size);
	    close(shm_fd);
        return -1;
    }

    memset(cache_buffer, 0, st_size);
 
    FILE *cache_fp = fopen(cache_path, "r");
    if(!cache_fp)
    {
        free(cache_buffer);   
        munmap(shm_map_ptr, seg_size);
	    close(shm_fd);
        return -1;
    }

    int cache_len = fread(cache_buffer, 1, st_size, cache_fp);
    if(st_size != cache_len)
    {
        munmap(shm_map_ptr, seg_size);
        close(shm_fd);
        free(cache_buffer);
        fclose(cache_fp);
        return -1;
    }

    sem_t * sem_id = sem_open(sem_path, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if(sem_id == SEM_FAILED){
        munmap(shm_map_ptr, seg_size);
	    close(shm_fd);
        free(cache_buffer);
        fclose(cache_fp);
        return -1;
    }    

    sem_wait(sem_id);

    memcpy(shm_map_ptr->data, cache_buffer, st_size); 
    
    shm_map_ptr->size = st_size;
 
    sem_post(sem_id);

    (void)free(cache_buffer);
    
    sem_close(sem_id);

    munmap(shm_map_ptr, seg_size);
   
    close(shm_fd);

    return 0; 
}

int dump_shared_memory(const char *shm_path, const char *sem_path, struct shm_map_data *shared_mem)
{
    sem_t *sem_id = (sem_t *)NULL;
    struct shm_map_data *map_ptr = (struct shm_map_data *)NULL;
    int seg_size = 0;
    int shm_fd = -1;

    if(!shm_path || !sem_path || !shared_mem){
	    return -1;
    }

    seg_size = sizeof(struct shm_map_data);


    shm_fd = shm_open(shm_path, O_RDONLY, 0666);
    if(shm_fd < 0){
        return -1; 
    }

    map_ptr = (struct shm_map_data *)mmap(NULL, seg_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if(map_ptr == MAP_FAILED){
        close(shm_fd);
        return -1;
    }   
 
    sem_id = sem_open(sem_path, 0);
    if(SEM_FAILED == sem_id){
        munmap(map_ptr, seg_size);
        close(shm_fd);
        return -1;
    }

    sem_wait(sem_id);
    
    memcpy(shared_mem, map_ptr, sizeof(struct shm_map_data));
   
    sem_post(sem_id);

    sem_close(sem_id);
    
    munmap(map_ptr, seg_size);
    close(shm_fd);

    return 0;
}

int open_file(const char *shm_path, const char *sem_path, char **cache_data, int *cache_size)
{
    int res = -1;
    char *tmp_ptr = (char *)NULL;
    struct shm_map_data shm_map_tmp;

    memset(&shm_map_tmp, 0, sizeof(struct shm_map_data));

    res = dump_shared_memory(shm_path, sem_path, &shm_map_tmp);
    if(!res){
	tmp_ptr = malloc(shm_map_tmp.size);
        if(!tmp_ptr){
	    res = -1;
	    return res;
	}	

	memset(tmp_ptr, 0, shm_map_tmp.size);

        memcpy(tmp_ptr, shm_map_tmp.data, shm_map_tmp.size);

        *cache_data = tmp_ptr;

        *cache_size = shm_map_tmp.size;
    }

    return res; 
}