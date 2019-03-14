#ifndef _CEL_SYSFS_CORE_H_
#define _CEL_SYSFS_CORE_H_


#define CEL_INFO(fmt, args...) printk( KERN_INFO "cel_sysfs: " fmt, ##args)
#define CEL_ERR(fmt, args...)  printk( KERN_ERR "cel_sysfs: " fmt, ##args)

#define DEBUG 1
#define DEBUG_CEL 1

#ifdef DEBUG
#ifdef DEBUG_CEL
#define CEL_DEBUG(fmt, args...) printk( KERN_DEBUG "cel_sysfs: %s:%d " fmt, __FUNCTION__, __LINE__, ##args)
#endif /*DEBUG_CEL*/
#else
#define CEL_DEBUG(fmt, args...)
#endif /*DEBUG*/

#define CEL_NAME_SIZE 10

#define PORT_DIR_NAME "sff"
#define CPLD_NAME "cpld"
#define PSU_DIR_NAME "psu"
#define EMC2305_DIR_NAME "fantray"
#define LEDS_DIR_NAME "leds"
#define EEPROM_DIR_NAME "tlv_eeprom"
#define WDT_DIR_NAME "watchdog"
#define LM75_DIR_NAME "sensors"


struct cel_sysfs_obj {
	struct kobject kobj;
};

enum{
    HW_UNPRESENT    = 0,
    HW_PRESENT      = 1,
};

extern struct cel_sysfs_obj *cel_sysfs_kobject_create(const char *name, struct kobject *parent);
extern struct cel_sysfs_obj *cel_sysfs_kobject_init(void);
extern void cel_sysfs_kobject_delete(struct cel_sysfs_obj **obj);
extern void cel_sysfs_kobject_change(struct cel_sysfs_obj *obj);

#endif /* _CEL_SYSFS_CORE_H_ */

