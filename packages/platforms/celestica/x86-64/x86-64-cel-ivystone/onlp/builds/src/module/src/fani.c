#include <onlp/platformi/fani.h>

//#include "i2c_chips.h"
#include "platform.h"

onlp_fan_info_t f_info[FAN_COUNT + 1] = {
    {},
    {
        {ONLP_FAN_ID_CREATE(CHASSIS_FAN_1), "Chassis Fan 1", 0},
        0x0,
    },
    {
        {ONLP_FAN_ID_CREATE(CHASSIS_FAN_2), "Chassis Fan 2", 0},
        0x0,
    },
    {
        {ONLP_FAN_ID_CREATE(CHASSIS_FAN_3), "Chassis Fan 3", 0},
        0x0,
    },
    {
        {ONLP_FAN_ID_CREATE(CHASSIS_FAN_4), "Chassis Fan 4", 0},
        0x0,
    },
    {
        {ONLP_FAN_ID_CREATE(CHASSIS_FAN_5), "Chassis Fan 5", 0},
        0x0,
    },
    {
        {ONLP_FAN_ID_CREATE(PSU_FAN_1_1), "PSU-1-1 Fan", 0},
        0x0,
    },
    {
        {ONLP_FAN_ID_CREATE(PSU_FAN_1_2), "PSU-1-2 Fan", 0},
        0x0,
    },
    {
        {ONLP_FAN_ID_CREATE(PSU_FAN_2_1), "PSU-2-1 Fan", 0},
        0x0,
    },
    {
        {ONLP_FAN_ID_CREATE(PSU_FAN_2_2), "PSU-2-2 Fan", 0},
        0x0,
    }};

int onlp_fani_init(void)
{
    //fanInit();
    return ONLP_STATUS_OK;
}

int onlp_fani_rpm_set(onlp_oid_t id, int rpm)
{
    int fan_id;
    unsigned short p;

    /* Max speed 13800 RPM. 1 % is 138 RPM */
    p = (unsigned short)(rpm / 138);
    if (p > 100)
        p = 100;

    fan_id = ONLP_OID_ID_GET(id) - 1;

    fanSpeedSet(fan_id, p);
    return ONLP_STATUS_OK;
}

int onlp_fani_percentage_set(onlp_oid_t id, int p)
{
    int fan_id;

    fan_id = ONLP_OID_ID_GET(id) - 1;

    fanSpeedSet(fan_id, (unsigned short)p);
    return ONLP_STATUS_OK;
}

int onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t *info_p)
{
    int fan_id;
    uint8_t fan_status;

    fan_id = ONLP_OID_ID_GET(id);

    // if (fan_id > FAN_COUNT)
    //     return ONLP_STATUS_E_INTERNAL;

    *info_p = f_info[fan_id];

    switch (fan_id)
    {
    case CHASSIS_FAN_1:
    case CHASSIS_FAN_2:
    case CHASSIS_FAN_3:
    case CHASSIS_FAN_4:
    case CHASSIS_FAN_5:

        fan_status = get_fan_present_status(fan_id);

        if (!(fan_status & 0x01))
            info_p->status |= ONLP_FAN_STATUS_PRESENT;
        else
            return ONLP_STATUS_E_MISSING;

        if ((fan_status >> 1) & 0x01)
            info_p->status |= ONLP_FAN_STATUS_B2F;
        else
            info_p->status |= ONLP_FAN_STATUS_F2B;

        //getFaninfo(fan_id,info_p->model,info_p->serial);

        get_rear_fan_pwm(fan_id, &(info_p->rpm));
        get_rear_fan_per(fan_id, &(info_p->percentage));
        
        break;
    case PSU_FAN_1_1:
    case PSU_FAN_1_2:
    case PSU_FAN_2_1:
    case PSU_FAN_2_2:
        /* code */
        break;

    default:
        break;
    }

    return ONLP_STATUS_OK;
}
