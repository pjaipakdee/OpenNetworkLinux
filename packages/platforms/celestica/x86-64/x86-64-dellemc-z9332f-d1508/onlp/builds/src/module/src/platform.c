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
    //{pwm_reg,ctrl_sta_reg,rear_spd_reg,front_spd_reg}
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
    "Temp_CPU", "TEMP_BB", "TEMP_SW_U16", "TEMP_SW_U52",
    "TEMP_FAN_U17", "TEMP_FAN_U52", "PSUL_Temp1", "PSUL_Temp2",
    "PSUR_Temp1", "PSUR_Temp2"};

int write_to_dump(uint8_t dev_reg)
{
    uint8_t ret;

    sprintf(command, "echo 0x%x> " SYS_CPLD_PATH "getreg", dev_reg);
    ret = system(command);
    return ret;
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

int cpld_b_read_reg(uint16_t reg, uint8_t *result)
{
    int ret = 0;
    char command[256];
    char buffer[128];

    reg = reg - 0xa100;
    sprintf(command, "ipmitool raw 0x3a 0x03 0x00 0x01 0x%02x", reg);
    exec_ipmitool_cmd(command, buffer);
    *result = strtol(buffer, NULL, 16);

    return ret;
}

int cpld_b_write_reg(uint16_t reg, uint8_t value)
{
    int ret = 0;
    char command[256];
    char buffer[128];

    sprintf(command, "ipmitool raw 0x3a 0x03 0x00 0x02 0x%02x 0x%02x", reg, value);
    exec_ipmitool_cmd(command, buffer);

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

int fan_cpld_write_reg(uint8_t reg, uint8_t value)
{
    int ret = 0;
    char command[256];
    char buffer[128];

    sprintf(command, "ipmitool raw 0x3a 0x03 0x01 0x02 0x%02x 0x%02x", reg, value);
    exec_ipmitool_cmd(command, buffer);

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

int getFanPresent_tmp(int id)
{
    int ret = -1;
    uint16_t fan_stat_reg;
    uint8_t result;

    if (id <= (FAN_COUNT))
    {
        fan_stat_reg = fan_sys_reg[id].ctrl_sta_reg;
        fan_cpld_read_reg(fan_stat_reg, &result);
        ret = result & 0x01;
    }

    return ret;
}

int getFanAirflow_tmp(int id)
{
    int ret = -1;
    uint16_t fan_stat_reg;
    uint8_t result;

    if (id <= (FAN_COUNT))
    {
        fan_stat_reg = fan_sys_reg[id].ctrl_sta_reg;
        fan_cpld_read_reg(fan_stat_reg, &result);
        ret = (result >> 1) & 0x1;
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


int getFanSpeed_PWM(int id, int *speed)
{
    int ret = -1;
    uint8_t value;
    uint8_t max_speed = 0xFF;
    int max_rpm_speed = 13800;
    int lowest_rpm_speed = 1200;
    float speed_percentage;

    if (id <= (FAN_COUNT))
    {
        uint16_t fan_stat_reg;
        fan_stat_reg = fan_sys_reg[id].pwm_reg;
        fan_cpld_read_reg(fan_stat_reg, &value);

        speed_percentage = (value * 100) / max_speed;
        *speed = (speed_percentage * ((max_rpm_speed - lowest_rpm_speed) / 100) + lowest_rpm_speed);
    }

    return ret;
}

int getFanSpeed_Percentage(int id, int *speed)
{
    int ret = -1;
    uint8_t value;
    uint8_t max_speed = 0xFF;

    if (id <= (FAN_COUNT))
    {
        uint16_t fan_stat_reg;
        fan_stat_reg = fan_sys_reg[id].pwm_reg;
        fan_cpld_read_reg(fan_stat_reg, &value);

        *speed = (value * 100) / max_speed;
    }

    return ret;
}

uint8_t getLEDStatus(int id)
{

// static const struct led_reg_mapper led_mapper[LED_COUNT + 1] = {
//     {},
//     {"LED_SYSTEM", LED_SYSTEM_H, LED_SYSTEM_REGISTER},
//     {"LED_FAN_1", LED_FAN_1_H, 0x24},
//     {"LED_FAN_2", LED_FAN_2_H, 0x34},
//     {"LED_FAN_3", LED_FAN_3_H, 0x44},
//     {"LED_FAN_4", LED_FAN_4_H, 0x54},
//     {"LED_FAN_5", LED_FAN_5_H, 0x64},
//     {"LED_FAN_6", LED_FAN_6_H, 0x74},
//     {"LED_FAN_7", LED_FAN_7_H, 0x84},
//     {"LED_ALARM", LED_ALARM_H, ALARM_REGISTER},
//     {"LED_PSU_LEFT", LED_PSU_L_H, PSU_LED_REGISTER},
//     {"LED_PSU_RIGHT", LED_PSU_R_H, PSU_LED_REGISTER},
// };
    uint8_t ret = 0xFF;

    if (id >= (LED_COUNT + 2) || id < 0)
        return 0xFF;

    if (id <= (LED_COUNT))
    {
        //uint16_t fan_stat_reg;
        uint8_t result = 0;
        //fan_stat_reg  = fan_sys_reg[id].ctrl_sta_reg;
        uint16_t led_stat_reg;
        // uint8_t result = 0;
        led_stat_reg = led_mapper[id].dev_reg;
        if(id >=2 && id <= 8){
            fan_cpld_read_reg(led_stat_reg,&result);
        }else{
            //cpld_b_read_reg(led_stat_reg, &result);
            result = read_register(led_stat_reg);
        }        
        ret = result;
        //printf("result led id = %d result=%x ret=%d\n",id,result,ret);
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
        cpld_b_read_reg(THERMAL_REGISTER, &result);
        ret = result;
        //printf("result = %x , %d\n",ret,ret);
    }

    return ret;
}

char *read_ipmi(char *cmd)
{
    FILE *pFd = NULL;
    char c; //,s_id[4]; //stat[5000],
    char *str = (char *)malloc(sizeof(char) * 5000);
    memset (str, 0, sizeof (char) * 5000);
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

uint8_t getPsuStatus(int id)
{
    //int ret = -1;
    uint16_t psu_stat_reg;
    uint8_t value;

    if (id <= (PSU_COUNT))
    {
        psu_stat_reg = psu_mapper[id].sta_reg;
        cpld_b_read_reg(psu_stat_reg, &value);
        // value = (value >> psu_mapper[id].bit_present) & 0x01;
        // ret = value;
    }

    return value;
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
        //ret = (result >> 2) & 0x1;
        ret = result;
        //printf("result result=%x ret=%d\n",result,ret);
    }

    return ret;
}

int getPsuPresent(int id)
{
    int ret = -1;
    uint16_t psu_stat_reg;
    uint8_t value;

    if (id <= (PSU_COUNT))
    {
        psu_stat_reg = psu_mapper[id].sta_reg;
        cpld_b_read_reg(psu_stat_reg, &value);
        value = (value >> psu_mapper[id].bit_present) & 0x01;
        ret = value;
    }

    return ret;
}

int getPsuAcStatus(int id)
{
    int ret = -1;
    uint16_t psu_stat_reg;
    uint8_t value;

    if (id <= (PSU_COUNT))
    {
        psu_stat_reg = psu_mapper[id].sta_reg;
        cpld_b_read_reg(psu_stat_reg, &value);
        value = (value >> psu_mapper[id].bit_ac_sta) & 0x01;
        ret = value;
    }

    return ret;
}

int getPsuPowStatus(int id)
{
    int ret = -1;
    uint16_t psu_stat_reg;
    uint8_t value;

    if (id <= (PSU_COUNT))
    {
        psu_stat_reg = psu_mapper[id].sta_reg;
        cpld_b_read_reg(psu_stat_reg, &value);
        value = (value >> psu_mapper[id].bit_pow_sta) & 0x01;
        ret = value;
    }

    return ret;
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

int psu_get_info(int id, int *mvin, int *mvout, int *mpin, int *mpout, int *miin, int *miout)
{
    int ret = 0;
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
            int status_pos = position + 39;
            //printf("Status = 0%c1%c2%c\n",sdr_value[status_pos],sdr_value[status_pos+1],sdr_value[status_pos+2]);
            if (sdr_value[status_pos] != 'n')
            {
                int v_pos = position + 19;
                //printf("Map the %s with %c\n",search_psu_sdr_info[str_index].keyword,search_psu_sdr_info[str_index].unit);
                for (int a = 0; a <= 18; a++)
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
    char buf[256];
    int index;
    char *token;

    if (0 == strcasecmp(psu_information[0].model, "unknown")) {
        
        index = 0;
        sprintf(command, "ipmitool fru print | grep -A 10 FRU_PSU");
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
            if (strstr(buf, "Product Serial")) {
                token = strtok(buf, ":");
                token = strtok(NULL, ":");
                char* trim_token = trim(token);
                sprintf(psu_information[index].serial_number,"%s",trim_token);
            }
            else if (strstr(buf, "Product Name")) {
                token = strtok(buf, ":");
                token = strtok(NULL, ":");
                char* trim_token = trim(token);
                sprintf(psu_information[index].model,"%s",trim_token);
            
            }
        }
        sprintf(psu_information[0].model,"pass"); //Mark as complete
        pclose(fp);
        
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

int getThermalStatus_Ipmi(int id, int *tempc)
{
    char data_temp[500] = "\0";
    int position;
    char ctemp[18] = "\0";
    float ftemp = 0.0;

    if(temp_sdr_value == NULL)
    {
        sprintf(command, "ipmitool sdr list | grep 'Temp\\|TEMP'");
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
        if( id >= THERMAL_COUNT)
        {
            free(temp_sdr_value);
            temp_sdr_value=NULL;
        }
        return 0;
    }
    else
    {
        int response_len = strlen(data_temp);
        for (int i = 18; i < response_len; i++)
        {
            if (data_temp[i] == 'c')
                break;
            append(ctemp, data_temp[i]);
        }
        ftemp = atof(ctemp);
        *tempc = ftemp * 1000;
    }
    
    //Free malloc temp_sdr_value after use.
    if( id >= THERMAL_COUNT)
    {
        free(temp_sdr_value);
        temp_sdr_value=NULL;
    }
    
    return 1;
}

void __trim(char *strIn, char *strOut)
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
	int i = 0;
	int ret = 0;
	char ipmi_ret[1024] = {'\0'};
	char ipmi_cmd[512] = {'\0'};
    char strTmp[10][128] = {{0}, {0}};
    char *token = NULL;
	
	if((NULL == temp) || (NULL == warn) || (NULL == error) || (NULL == shutdown))
	{
		printf("%s null pointer!\n", __FUNCTION__);
		return -1;
	}

    /*
        String example:			  
        ipmitool sensor list | grep TEMP_FAN_U52
        Temp_CPU     | 1.000      | degrees C  | ok  | 5.000  | 9.000  | 16.000  | 65.000  | 73.000  | 75.606
        TEMP_FAN_U52 | 32.000	  | degrees C  | ok  | na  |  na  | na  | na  | 70.000  | 75.000
        PSUR_Temp1   | na         | degrees C  | na  | na  | na   | na   | na  | na  | na
    */
    sprintf(
        ipmi_cmd, 
        "ipmitool sensor list | grep %s",  
        Thermal_sensor_name[id - 1]
        ); 

    ret = exec_ipmitool_cmd(ipmi_cmd, ipmi_ret);
    if(ret != 0)
    {
        printf("exec_ipmitool_cmd: %s failed!\n", ipmi_cmd);
        return -1;
    }   

    i = 0;
    token = strtok(ipmi_ret, "|");
    while( token != NULL ) 
    {
        __trim(token, &strTmp[i][0]);
        i++;
        if(i > 10) break;
        token = strtok(NULL, "|");
    }

    if(0 == strcmp(&strTmp[1][0], "na")) return -1;
    else *temp = atof(&strTmp[1][0]) * 1000.0;

    if(0 == strcmp(&strTmp[7][0], "na")) *warn = 0;	
    else *warn = atof(&strTmp[7][0]) * 1000.0;

    if(0 == strcmp(&strTmp[8][0], "na")) *error = 0;	
    else *error = atof(&strTmp[8][0]) * 1000.0;

    if(0 == strcmp(&strTmp[9][0], "na")) *shutdown = 0;	
    else *shutdown = atof(&strTmp[9][0]) * 1000.0;

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
