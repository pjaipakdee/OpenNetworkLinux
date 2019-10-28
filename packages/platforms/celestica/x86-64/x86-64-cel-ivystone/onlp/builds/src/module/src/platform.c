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
#include <cjson/cJSON.h>
#include <cjson_util/cjson_util.h>

#include "platform.h"

char command[256];
FILE *fp;
static char *sdr_value = NULL;
static char *temp_sdr_value = NULL;

static struct device_info fan_information[FAN_COUNT + 1] = {
    {"unknown", "unknown"}, //check
    {}, //Fan 1
    {}, //Fan 2
    {}, //Fan 3
    {}, //Fan 4
};

static struct device_info psu_information[PSU_COUNT + 1] = {
    {"unknown", "unknown"}, //check
    {}, //PSU 1
    {}, //PSU 2
};

static const struct fan_config_p fan_sys_reg[FAN_COUNT + 1] = {
    /*	uint16_t front_spd_reg;
	uint16_t rear_spd_reg;
    uint16_t pwm_reg;
	uint16_t led_ctrl_reg;
	uint16_t ctrl_sta_reg;*/
    {},
    {0x20, 0x21, 0x22, 0x24,0x26}, //Fan 1
    {0x40, 0x41, 0x42, 0x44,0x46}, //Fan 2
    {0x60, 0x60, 0x62, 0x64,0x66}, //Fan 3
    {0x80, 0x81, 0x82, 0x84,0x86}, //Fan 4
    {0xA0, 0xA1, 0xA2, 0xA4,0xA6}  //Fan 5
};

static const struct led_reg_mapper led_mapper[LED_COUNT + 1] = {
    /*char *name;
    uint16_t device;
    uint16_t dev_reg;*/
    {},
    {"LED_SYSTEM", 1, LED_SYSTEM_REGISTER},
    {"LED_FAN_1", 2, 0x24},
    {"LED_FAN_2", 3, 0x44},
    {"LED_FAN_3", 4, 0x64},
    {"LED_FAN_4", 5, 0x84},
    {"LED_FAN_5", 6, 0xA4},
    {"LED_ALARM", 7, ALARM_REGISTER},
    {"LED_PSU", 8, PSU_LED_REGISTER}
};

static const struct search_psu_sdr_info_mapper search_psu_sdr_info[12] = {
    {"PSUL_VIn", 'V'},
    {"PSUL_CIn", 'A'},
    {"PSUL_PIn", 'W'},
    {"PSUL_VOut", 'V'},
    {"PSUL_COut", 'A'},
    {"PSUL_POut", 'W'},
    {"PSUR_VIn", 'V'},
    {"PSUR_CIn", 'A'},
    {"PSUR_PIn", 'W'},
    {"PSUR_VOut", 'V'},
    {"PSUR_COut", 'A'},
    {"PSUR_POut", 'W'},
};

char *Thermal_sensor_name[THERMAL_COUNT] = {
    "Base_Temp_U5", "Base_Temp_U7", "CPU_Temp", "Switch_Temp_U1",
    "Switch_Temp_U18", "Switch_Temp_U28", "Switch_Temp_U29", "PSUL_Temp1",
    "PSUL_Temp2", "PSUR_Temp1", "PSUR_Temp2","Switch_U21_Temp","Switch_U33_Temp"};

int write_to_dump(uint8_t dev_reg)
{
    uint8_t ret;

    sprintf(command, "echo 0x%x> " SYS_CPLD_PATH "getreg", dev_reg);
    ret = system(command);
    return ret;
}

/* Parse text to JSON, then render back to text, and print! */
void doit(char *text)
{
    char *out = NULL;
    cJSON *json = NULL;

    json = cJSON_Parse(text);
    if (!json)
    {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
    }
    else
    {
        out = cJSON_Print(json);
        cJSON_Delete(json);
        printf("%s\n", out);
        free(out);
    }
}

uint8_t read_register(uint16_t dev_reg)
{
    int status;
    //printf("Input of read_register = %u\n",dev_reg);
    //sprintf(command,"echo %x",dev_reg);// > "SYSCPLD_PATH"dump"
    sprintf(command, "echo 0x%x >  %sgetreg", dev_reg, SYS_CPLD_PATH);
    //printf("Command %s\n",command);
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

uint8_t read_fan_register(uint16_t dev_reg)
{
    int status;
    //printf("Input of read_register = %u\n",dev_reg);
    //sprintf(command,"echo %x",dev_reg);// > "SYSCPLD_PATH"dump"
    sprintf(command, "echo 0x%x >  %sgetreg", dev_reg, FAN_CPLD_PATH);
    //printf("Command %s\n",command);
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

uint8_t write_register(uint16_t dev_reg, uint16_t write_data)
{

    int status;
    //printf("Input of read_register = %u",dev_reg);
    //sprintf(command,"echo %x",dev_reg);// > "SYSCPLD_PATH"dump"
    //sprintf(command,"echo %x >  %sgetreg",dev_reg,SYS_CPLD_PATH);
    //printf("Command %s\n",command);
    sprintf(command, "echo 0x%x %x>  %ssetreg", dev_reg, write_data, SYS_CPLD_PATH);
    fp = popen(command, "r");
    if (!fp)
    {
        printf("Failed : Can't specify CPLD register\n");
        return -1;
    }
    fscanf(fp, "%x", &status);
    pclose(fp);

    return status;
}

int get_fan_present_status(int id)
{
    int ret = -1;

    // if(id >= (FAN_COUNT+2)|| id<0)
    //     return -1;

    if (id <= (FAN_COUNT))
    {
        uint16_t fan_stat_reg;
        uint8_t result = 0;
        fan_stat_reg = fan_sys_reg[id].ctrl_sta_reg;
        result = read_fan_register(fan_stat_reg);
        ret = result;
        //ret = result;
        //printf("result result=%x ret=%d\n",result,ret);
    }

    return ret;
}

int get_rear_fan_pwm(int id, int *speed)
{
    //unsigned char data;
    unsigned short value;
    int ret = -1;
    uint8_t max_speed = 0xFF;
    //int max_rpm_speed = 13800;
    //int lowest_rpm_speed = 1200;
    int max_rpm_speed = 18000;
    int lowest_rpm_speed = 1000;
    float speed_percentage;

    if (id <= (FAN_COUNT))
    {
        uint16_t fan_stat_reg;
        // uint8_t result = 0;
        fan_stat_reg = fan_sys_reg[id].rear_spd_reg;
        value = read_register(fan_stat_reg);
        //printf("value=%x maxspeed=%x",value,max_rpm_speed);
        speed_percentage = (value * 100) / max_speed;
        *speed = (speed_percentage * ((max_rpm_speed - lowest_rpm_speed) / 100) + lowest_rpm_speed);
    }

    return ret;
}

int get_rear_fan_per(int id, int *speed)
{

    //unsigned char data;
    unsigned short value;
    int ret = -1;
    uint8_t max_speed = 0xFF;

    if (id <= (FAN_COUNT))
    {
        uint16_t fan_stat_reg;
        // uint8_t result = 0;
        fan_stat_reg = fan_sys_reg[id].rear_spd_reg;
        value = read_register(fan_stat_reg);
        //printf("value=%x maxspeed=%x",value,max_speed);
        *speed = (value * 100) / max_speed;
    }

    return ret;
}

int fanSpeedSet(int id, unsigned short speed)
{

    int result;

    if (id <= (FAN_COUNT - 1))
    {
        uint16_t fan_reg = fan_sys_reg[id].pwm_reg;
        result = write_register(fan_reg, speed);
    }

    return result;
}

uint8_t get_psu_status(int id)
{

    uint8_t ret = 0xFF;

    if (id >= (PSU_COUNT + 2) || id < 0)
        return 0xFF;

    if (id <= (PSU_COUNT))
    {
        //uint16_t fan_stat_reg;
        uint8_t result = 0;
        //fan_stat_reg  = fan_sys_reg[id].ctrl_sta_reg;
        result = read_register(PSU_REGISTER);
        //ret = (result >> 2) & 0x1;
        ret = result;
        //printf("result result=%x ret=%d\n",result,ret);
    }

    return ret;
}

uint8_t get_led_status(int id)
{

    //     {"LED_SYSTEM",LED_SYSTEM,LED_SYSTEM_REGISTER},
    // {"LED_FAN_1",LED_FAN_1,0xA141},
    // {"LED_FAN_2",LED_FAN_2,0xA145},
    // {"LED_FAN_4",LED_FAN_4,0xA14D},
    // {"LED_FAN_5",LED_FAN_5,0xA151},
    // {"LED_ALARM",LED_ALARM,ALARM_REGISTER},
    // {"LED_PSU_1",LED_PSU_1,PSU_LED_REGISTER},
    // {"LED_PSU_2",LED_PSU_2,PSU_LED_REGISTER},
    uint8_t ret = 0xFF;

    if (id >= (LED_COUNT + 2) || id < 0)
        return 0xFF;

    if (id <= (LED_COUNT))
    {
        uint8_t result = 0;
        uint16_t led_stat_reg;

        led_stat_reg = led_mapper[id].dev_reg;
        result = read_register(led_stat_reg);
        
        ret = result;
        //printf("result result=%x ret=%d\n",result,ret);
    }

    return ret;
}

uint8_t get_fan_led_status(int id)
{

    //     {"LED_SYSTEM",LED_SYSTEM,LED_SYSTEM_REGISTER},
    // {"LED_FAN_1",LED_FAN_1,0xA141},
    // {"LED_FAN_2",LED_FAN_2,0xA145},
    // {"LED_FAN_4",LED_FAN_4,0xA14D},
    // {"LED_FAN_5",LED_FAN_5,0xA151},
    // {"LED_ALARM",LED_ALARM,ALARM_REGISTER},
    // {"LED_PSU_1",LED_PSU_1,PSU_LED_REGISTER},
    // {"LED_PSU_2",LED_PSU_2,PSU_LED_REGISTER},
    uint8_t ret = 0xFF;

    if (id >= (LED_COUNT + 2) || id < 0)
        return 0xFF;

    if (id <= (LED_COUNT))
    {
        uint8_t result = 0;
        uint16_t led_stat_reg;

        led_stat_reg = led_mapper[id].dev_reg;
        result = read_fan_register(led_stat_reg);
        
        ret = result;
        //printf("result result=%x ret=%d\n",result,ret);
    }

    return ret;
}

uint8_t getThermalStatus(int id)
{

    uint8_t ret = 0;
    // if(id >= (THERMAL_COUNT)|| id<0)
    //     return 0;

    if (id <= (THERMAL_COUNT - 1))
    {
        uint8_t result = 0;
        result = read_register(THERMAL_REGISTER);
        ret = result;
        //printf("result = %x , %d\n",ret,ret);
    }

    return ret;
}

int led_mask(int start, uint8_t value)
{

    int twobits_mask = 3;

    int cal = ((value >> start) & twobits_mask);

    return cal;
}
char *read_fru(int fru_id)
{
    FILE *pFd = NULL;
    char c; //,s_id[4]; //stat[5000],
    char *str = (char *)malloc(sizeof(char) * 5000);
    int i = 0;
    //float marks;
    //sprintf(command,"cat /home/tdcadmin/Work/Git/seastone2-diag/onl-sysinfo/fru.txt");
    //sprintf(s_id,"%d",id);

    sprintf(command, "ipmitool fru print %d", fru_id);
    pFd = popen(command, "r");
    if (pFd != NULL)
    {

        c = fgetc(pFd);
        while (c != EOF)
        {
            //printf ("%c", c);
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

char *read_psu_sdr(int id)
{
    FILE *pFd = NULL;
    char c; //,s_id[4]; //stat[5000],
    char *str = (char *)malloc(sizeof(char) * 5000);
    int i = 0;

    sprintf(command,"ipmitool  sdr | grep PSU");
    pFd = popen(command, "r");
    if (pFd != NULL)
    {

        c = fgetc(pFd);
        while (c != EOF)
        {
            //printf ("%c", c);
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

char *read_ipmi(char *cmd)
{
    FILE *pFd = NULL;
    char c; //,s_id[4]; //stat[5000],
    char *str = (char *)malloc(sizeof(char) * 5000);
    int i = 0;
    //float marks;
    //sprintf(command,"cat /home/tdcadmin/Work/Git/seastone2-diag/onl-sysinfo/fru.txt");
    //sprintf(s_id,"%d",id);
    //sprintf(command);
    pFd = popen(cmd, "r");
    if (pFd != NULL)
    {

        c = fgetc(pFd);
        while (c != EOF)
        {
            //printf ("%c", c);
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

    //char *sdr_value = NULL;
    int position = 0, val_pos = 0;
    if(sdr_value == NULL){
        sdr_value = read_psu_sdr(id);
    }
        
    
    char ctemp[18] = "\0";
    float ftemp = 0.0;
    int str_index = 0;
    if(id == 2)
        str_index = 6;

    int val[12]={0,0,0,0,0,0,0,0,0,0,0,0};

    for (; str_index < NELEMS(search_psu_sdr_info); str_index++)
    {

        position = keyword_match(sdr_value, search_psu_sdr_info[str_index].keyword);

        if (position != -1)
        {
            int status_pos = position + 39;
            //printf("Status = 0%c1%c2%c\n",sdr_value[status_pos],sdr_value[status_pos+1],sdr_value[status_pos+2]);
            if (sdr_value[status_pos] != 'n')
            {
                int v_pos = position + 19;
                int a = 0;
                //printf("Map the %s with %c\n",search_psu_sdr_info[str_index].keyword,search_psu_sdr_info[str_index].unit);
                for (; a <= 18; a++)
                {
                    //printf("%c",sdr_value[v_pos+a]);
                    // 			//if(search_psu_sdr_info[str_index].unit!=NULL){
                    //                 printf("%c",sdr_value[v_pos+a]);
                    if (sdr_value[v_pos + a] != search_psu_sdr_info[str_index].unit)
                    {
                        // ctemp[a]=sdr_value[v_pos+a];
                        append(ctemp, sdr_value[v_pos + a]);
                        //printf("%c",sdr_value[v_pos+a]);
                    }
                    else
                    {
                        //printf("ctemp = %s\n",ctemp);
                        ftemp = atof(ctemp);
                        //printf("ftemp %f\n",ftemp);
                        val[val_pos] = ftemp * 1000;
                        //printf("val[%d] = %d %s\n",val_pos,val[val_pos],search_psu_sdr_info[str_index].keyword);
                        memset(ctemp, 0, sizeof(ctemp));
                        val_pos++;
                        break;
                    }
                }
            }
        }
    }

    *mvin = val[0];
    *miin = val[1];
    *mpin = val[2];
    *mvout = val[3];
    *miout = val[4];
    *mpout = val[5];

    return 1;
}

int psu_get_model_sn(int id, char *model, char *serial_number)
{
    //char buf[256];
    //int index;
    char *token = NULL;
    cJSON *json = NULL;
    cJSON *items = NULL;

    if (0 == strcasecmp(psu_information[0].model, "unknown")) {
        
        long len = 0;
        char *data = NULL;
        fp = fopen(PSU_CACHE_PATH, "rb");

        /* get the length */
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        data = (char*)malloc(len + 1);

        fread(data, 1, len, fp);
        data[len] = '\0';
        fclose(fp);

        json = cJSON_Parse(data);
        
        if (!json)
        {
            printf("Error before: [%s]\n", cJSON_GetErrorPtr());
        }
        else
        {
            
            items = cJSON_GetObjectItem(json, "informations");
            token = cJSON_Print(items);
            int i = 0;
            for (; i < PSU_COUNT; i++) {  // Presumably "max" can be derived from "items" somehow
                // char *psu_name = NULL;
                // cJSON *model = NULL;
                // cJSON *sn = NULL;
                // cJSON* item = cJSON_GetArrayItem(items, i);
                // sprintf(psu_name,"PSU%d FRU",i+1);
                // //psu_name = cJSON_GetObjectItem(item, psu_name);
                // model = cJSON_GetObjectItem(item, "Product Name");
                // sn = cJSON_GetObjectItem(item, "Serial Number");
                // char* rendered1 = cJSON_Print(model);
                // char* rendered2 = cJSON_Print(sn);
                // printf("%s %s %s",psu_name,rendered1,rendered2);
                // free(psu_name);
                // cJSON_Delete(model);
                // cJSON_Delete(sn);
            }
            printf("%s\n", token);
            free(token);

            cJSON_Delete(json);
            cJSON_Delete(items);
        }

        free(data);
        //pclose(fp);
        
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

char* read_fans_fru(){

    FILE *pFd = NULL;
    char c; //,s_id[4]; //stat[5000],
    char *str = (char *)malloc(sizeof(char) * 5000);
    int i = 0;

    sprintf(command, "ipmitool fru print | grep -A 4 FRU_FAN");
    pFd = popen(command, "r");
    if (pFd != NULL)
    {

        c = fgetc(pFd);
        while (c != EOF)
        {
            //printf ("%c", c);
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

int getFaninfo(int id, char *model, char *serial)
{
    char buf[256];
    int index;
    char *token;

    if (0 == strcasecmp(fan_information[0].model, "unknown")) {
        
        index = 0;
        sprintf(command, "ipmitool fru print | grep -A 10 FRU_FAN");
        fp = popen(command, "r");
        if (fp == 0)
        {
            printf("Failed - Failed to obtain system information\n");
            return -1;
        }
        while (EOF != fscanf(fp, "%[^\n]\n", buf))
        {
            if (strstr(buf, "FRU Device Description")) {
                index++;
            }
            if (strstr(buf, "Board Serial")) {
                token = strtok(buf, ":");
                token = strtok(NULL, ":");
                char* trim_token = trim(token);
                sprintf(fan_information[index].serial_number,"%s",trim_token);
            }
            else if (strstr(buf, "Board Part Number")) {
                token = strtok(buf, ":");
                token = strtok(NULL, ":");
                char* trim_token = trim(token);
                sprintf(fan_information[index].model,"%s",trim_token);
            
            }
        }
        sprintf(fan_information[0].model,"pass"); //Mark as complete
        pclose(fp);
        
    }

    strcpy(model, fan_information[id].model);
    strcpy(serial, fan_information[id].serial_number);

    return 1;
}

char *read_temp(char *name)
{
    FILE *pFd = NULL;
    char c; //,s_id[4]; //stat[5000],
    char *str = (char *)malloc(sizeof(char) * 5000);
    int i = 0;
    //float marks;
    //sprintf(command,"cat /home/tdcadmin/Work/Git/seastone2-diag/onl-sysinfo/fru.txt");
    //sprintf(s_id,"%d",id);

    sprintf(command, "ipmitool sensor | grep %s ", name);
    pFd = popen(command, "r");
    if (pFd != NULL)
    {

        c = fgetc(pFd);
        while (c != EOF)
        {
            //printf ("%c", c);
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

int getThermalStatus_Ipmi(int id, int *tempc)
{
    //printf("desc = %s\n",desc);
    char data_temp[500] = "\0";
    int position;
    char ctemp[18] = "\0";
    float ftemp = 0.0;
    if(temp_sdr_value == NULL){
        sprintf(command, "ipmitool sdr list | grep Temp");
        temp_sdr_value = read_ipmi(command);
    }
    position = keyword_match(temp_sdr_value, Thermal_sensor_name[id - 1]);
    if (position != -1){
        //Grep word untill newline
        //printf("Position = %d | text = %c%c",position,temp_sdr_value[position],temp_sdr_value[position+1]);
        int search_pos = 0;
        while(temp_sdr_value[position+search_pos] != '\n'){
            append(data_temp,temp_sdr_value[position+search_pos]);
            search_pos++;
        }
    }

    if (keyword_match(data_temp, "ns") > 0)
    {
        if( id >= THERMAL_COUNT){
            free(temp_sdr_value);
            temp_sdr_value=NULL;
        }
        return 0;
    }
    else
    {
        int response_len = strlen(data_temp);
        int i = 18;
        for (; i < response_len; i++)
        {
            if (data_temp[i] == 'c')
                break;
            append(ctemp, data_temp[i]);
        }
        ftemp = atof(ctemp);
        *tempc = ftemp * 1000;
    }
    
    //Free malloc temp_sdr_value after use.
    if( id >= THERMAL_COUNT){
        free(temp_sdr_value);
        temp_sdr_value=NULL;
    }

    return 1;
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
    //printf("deviceNodeReadBinary = ret %d buffer = %s\n",ret,buffer);
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
    //printf ("%s\n", s);
    return s;
}
