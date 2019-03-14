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
#include <onlp/platformi/sfpi.h>

#include <fcntl.h> /* For O_RDWR && open */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "platform_lib.h"

static char sfp_node_path[PREFIX_PATH_LEN] = {0};

static int 
cel_redstone_xp_sfp_node_read_int(char *node_path, int *value, int data_len)
{
    int ret = 0;
    char buf[8];    
    *value = 0;

    ret = deviceNodeReadString(node_path, buf, sizeof(buf), data_len);

    if (ret == 0) {
        *value = atoi(buf);
    }

    return ret;
}

static char* 
cel_redstone_xp_sfp_get_port_path(int port, char *node_name)
{
    sprintf(sfp_node_path, "%s/port%d/%s", PREFIX_PATH_ON_SFP, port, node_name);

    return sfp_node_path;
}

static uint64_t
cel_sfp_get_all_ports_present(void)
{
	int i, ret;
	uint64_t present = 0;
    char* path;

	for(i = 0; i < CHASSIS_SFP_NUM_MAX; i++) {
		path = cel_redstone_xp_sfp_get_port_path(i + 1, "present");
	    if (cel_redstone_xp_sfp_node_read_int(path, &ret, 0) != 0) {
	        ret = 0;
	    }
		present |= ((uint64_t)ret << i);
	}

    return present;
}


/************************************************************
 *
 * SFPI Entry Points
 *
 ***********************************************************/
int
onlp_sfpi_init(void)
{
    /* Called at initialization time */    
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    /*
     * Ports {0, 32}
     */
    int p;
    AIM_BITMAP_CLR_ALL(bmap);
    
    for(p = 0; p < CHASSIS_SFP_NUM_MAX; p++) {
        AIM_BITMAP_SET(bmap, p);
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_is_present(int port)
{
    /*
     * Return 1 if present.
     * Return 0 if not present.
     * Return < 0 if error.
     */
    int present;
    char* path = cel_redstone_xp_sfp_get_port_path(port + 1, "present");

    if (cel_redstone_xp_sfp_node_read_int(path, &present, 0) != 0) {
        AIM_LOG_ERROR("Unable to read present status from port(%d)\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }
    
    return present;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
	int i = 0;
    uint64_t presence_all = 0;

	presence_all = cel_sfp_get_all_ports_present();

    /* Populate bitmap */
    for(i = 0; presence_all; i++) {
        AIM_BITMAP_MOD(dst, i, (presence_all & 1));
        presence_all >>= 1;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
	char sub_path[10];
    char* path;

	sprintf(sub_path, "/%d-0050/eeprom", CHASSIS_SFP_I2C_BUS_BASE + port);
	path= cel_redstone_xp_sfp_get_port_path(port + 1, sub_path);

    /*
     * Read the SFP eeprom into data[]
     *
     * Return MISSING if SFP is missing.
     * Return OK if eeprom is read
     */
    memset(data, 0, 256);
    
    if (deviceNodeReadBinary(path, (char*)data, 256, 256) != 0) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}

