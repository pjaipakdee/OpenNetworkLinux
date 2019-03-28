#include <onlp/platformi/ledi.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

// #include "i2c_chips.h"
#include "platform.h"

#ifndef BMC_RESTFUL_API_SUPPORT
enum onlp_led_id
{
    LED_RESERVED = 0,
    LED_SYSTEM,
    LED_FAN_1,
    LED_FAN_2,
    LED_FAN_3,
    LED_FAN_4,
    LED_FAN_5,
    LED_ALARM,
    LED_PSU

};
#else
enum onlp_led_id
{
    LED_RESERVED = 0,
    LED_SYSTEM,
    LED_PSU,
    LED_FAN
};
#endif

/*
 * Get the information for the given LED OID.
 */
#ifndef BMC_RESTFUL_API_SUPPORT
static onlp_led_info_t led_info[] =
    {
        {},
        {
            {ONLP_LED_ID_CREATE(LED_SYSTEM), "Chassis System LED", 0},
            ONLP_LED_STATUS_PRESENT,
            ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN |
                ONLP_LED_CAPS_GREEN_BLINKING | ONLP_LED_CAPS_AUTO,
        },
        {
            {ONLP_LED_ID_CREATE(LED_FAN_1), "Chassis FAN(1) LED", 0},
            ONLP_LED_STATUS_PRESENT,
            ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_RED | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
        },
        {
            {ONLP_LED_ID_CREATE(LED_FAN_2), "Chassis FAN(2) LED", 0},
            ONLP_LED_STATUS_PRESENT,
            ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_RED | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
        },
        {
            {ONLP_LED_ID_CREATE(LED_FAN_3), "Chassis FAN(3) LED", 0},
            ONLP_LED_STATUS_PRESENT,
            ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_RED | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
        },
        {
            {ONLP_LED_ID_CREATE(LED_FAN_4), "Chassis FAN(4) LED", 0},
            ONLP_LED_STATUS_PRESENT,
            ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_RED | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
        },
        {
            {ONLP_LED_ID_CREATE(LED_FAN_5), "Chassis FAN(5) LED", 0},
            ONLP_LED_STATUS_PRESENT,
            ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_RED | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
        },
        {
            {ONLP_LED_ID_CREATE(LED_ALARM), "Alert LED", 0},
            ONLP_LED_STATUS_PRESENT,
            ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN |
                ONLP_LED_CAPS_GREEN_BLINKING,
        },
        {
            {ONLP_LED_ID_CREATE(LED_PSU), "PSU LED", 0},
            ONLP_LED_STATUS_PRESENT,
            ONLP_LED_CAPS_AUTO | ONLP_LED_CAPS_YELLOW,
        }
};
#else
static onlp_led_info_t led_info[] =
{
        {},
        {
            {ONLP_LED_ID_CREATE(LED_SYSTEM), "Chassis System LED", 0},
            ONLP_LED_STATUS_PRESENT,
            ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_GREEN, 
        },

        {
            {ONLP_LED_ID_CREATE(LED_PSU), "Chassis Power LED", 0},
            ONLP_LED_STATUS_PRESENT,
            ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_GREEN, 
        },

	{
            {ONLP_LED_ID_CREATE(LED_FAN), "Chassis Fan LED", 0},
            ONLP_LED_STATUS_PRESENT,
            ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_GREEN, 
        }
};
#endif
int onlp_ledi_init(void)
{
    //printf("onlp call onlp_ledi_init\n");
    return ONLP_STATUS_OK;
}

int onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t *info_p)
{
    int led_id;
    uint8_t led_color = 0;
    //uint8_t blink_status = 0;
    //uint8_t result[LED_COUNT];
    uint8_t result = 0;
    
    //(void)system("curl -d \'{\"data\":\"cat /sys/bus/i2c/devices/i2c-0/0-000d/psu_led_ctrl_en 2>/dev/null | head -n 1\"}\' http://240.1.1.1:8080/api/sys/raw | python -m json.tool > /tmp/onlp-psu-led-cache.txt");
    //(void)system("curl -d \'{\"data\":\"cat /sys/bus/i2c/devices/i2c-0/0-000d/fan_led_ctrl_en 2>/dev/null | head -n 1\"}\' http://240.1.1.1:8080/api/sys/raw | python -m json.tool > /tmp/onlp-fan-led-cache.txt");

    led_id = ONLP_OID_ID_GET(id);
    // *info = linfo[ONLP_OID_ID_GET(id)];
    *info_p = led_info[led_id];

    //printf("led id = %d  status = %d\n",led_id,result);
    //printf("before assign info_p->mode = %d\n",info_p->mode);
    switch (led_id)
    {
    case LED_SYSTEM:
        result = get_led_status(led_id);

        if (result != 0xFF)
            info_p->status |= ONLP_LED_STATUS_ON;
        led_color = (result >> 4) & 0x3;
        if (led_color == 0)
        {
            if (led_id == LED_SYSTEM)
                info_p->mode |= ONLP_LED_MODE_AUTO;
            // if (led_id == LED_ALARM)
            //     info_p->mode |= ONLP_LED_MODE_OFF;
        }
        if (led_color == 1)
        {
            info_p->mode = ONLP_LED_MODE_GREEN;
        }
        if (led_color == 2)
        {
            info_p->mode = ONLP_LED_MODE_YELLOW;
        }
        if (led_color == 3)
        {
            info_p->mode = ONLP_LED_MODE_OFF;
            break;
        }

        /*blink_status = result & 0x3;
        if (blink_status == 1 || blink_status == 2)
        {
            int current_mode = info_p->mode;
            info_p->mode = current_mode + 1;
        }*/

        break;

/*    case LED_FAN_1:
    case LED_FAN_2:
    case LED_FAN_3:
    case LED_FAN_4:
    case LED_FAN_5:
        result = get_fan_led_status(led_id - LED_SYSTEM);

        if (result != 0xFF)
            info_p->status |= ONLP_LED_STATUS_ON;
        led_color = result & 0x3;

        if (led_color == 0)
        {
            info_p->mode |= ONLP_LED_MODE_OFF;
        }
        if (led_color == 1)
        {
            info_p->mode |= ONLP_LED_MODE_GREEN;
        }
        if (led_color == 2)
        {
            info_p->mode |= ONLP_LED_MODE_RED;
        }

        break;
*/
    case LED_PSU:
        result = get_led_status(led_id);

        if (result != 0xFF)
            info_p->status |= ONLP_LED_STATUS_ON;
        led_color = result & 0x3;
        if (led_color == 1)
        {
            info_p->mode = ONLP_LED_MODE_GREEN;
        }
        else
        {
            info_p->mode = ONLP_LED_MODE_YELLOW;
        }
        break;
    case LED_FAN:
         result = get_led_status(led_id);

        if (result != 0xFF)
            info_p->status |= ONLP_LED_STATUS_ON;
        led_color = result & 0x3;
        if (led_color == 1)
        {
            info_p->mode = ONLP_LED_MODE_GREEN;
        }
        else
        {
            info_p->mode = ONLP_LED_MODE_YELLOW;
        }
        break;
    default: 
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return ONLP_STATUS_OK;
}
