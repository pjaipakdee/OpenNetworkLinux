#include <linux/module.h>                                                       
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>

#include "cel_sysfs_core.h"


#define KOBJ_NAME "celestica"
/*
 * This module create a kset in sysfs called /sys/celestica
*/

static struct kset *cel_sysfs_kset;

#define to_cel_sysfs_obj(x) container_of(x, struct cel_sysfs_obj, kobj)

/* a custom attribute that works just for a struct cel_sysfs_obj. */
struct cel_sysfs_attribute {
	struct attribute attr;
	ssize_t (*show)(struct cel_sysfs_obj *foo, struct cel_sysfs_attribute *attr, char *buf);
	ssize_t (*store)(struct cel_sysfs_obj *foo, struct cel_sysfs_attribute *attr, const char *buf, size_t count);
};
#define to_cel_sysfs_attr(x) container_of(x, struct cel_sysfs_attribute, attr)

static ssize_t cel_sysfs_attr_show(struct kobject *kobj,
			     struct attribute *attr,
			     char *buf)
{
	struct cel_sysfs_attribute *attribute;
	struct cel_sysfs_obj *device;

	attribute = to_cel_sysfs_attr(attr);
	device = to_cel_sysfs_obj(kobj);

	if (!attribute->show)
		return -EIO;

	return attribute->show(device, attribute, buf);
}

static ssize_t cel_sysfs_attr_store(struct kobject *kobj,
			      struct attribute *attr,
			      const char *buf, size_t len)
{
	struct cel_sysfs_attribute *attribute;
	struct cel_sysfs_obj *obj;

	attribute = to_cel_sysfs_attr(attr);
	obj = to_cel_sysfs_obj(kobj);

	if (!attribute->store)
		return -EIO;

	return attribute->store(obj, attribute, buf, len);
}

static const struct sysfs_ops cel_sysfs_sysfs_ops = {
	.show = cel_sysfs_attr_show,
	.store = cel_sysfs_attr_store,
};

static void cel_sysfs_obj_release(struct kobject *kobj)
{
	struct cel_sysfs_obj *obj;

	obj = to_cel_sysfs_obj(kobj);
	kfree(obj);
}

static struct kobj_type cel_sysfs_ktype = {
	.sysfs_ops = &cel_sysfs_sysfs_ops,
	.release = cel_sysfs_obj_release,
	.default_attrs = NULL,
};


struct cel_sysfs_obj *cel_sysfs_kobject_create(const char *name, struct kobject *parent)
{
	struct cel_sysfs_obj *obj;
	int retval;

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
        CEL_DEBUG("cel_sysfs_kobject_create %s kzalloc error", name);
		return NULL;
   }

	obj->kobj.kset = cel_sysfs_kset;

	retval = kobject_init_and_add(&obj->kobj, &cel_sysfs_ktype, parent, "%s", name);
	if (retval) {
		kobject_put(&obj->kobj);
        CEL_DEBUG("kobject_init_and_add %s error", name);
		return NULL;
	}

    kobject_uevent(&obj->kobj, KOBJ_ADD);
	return obj;
}
EXPORT_SYMBOL(cel_sysfs_kobject_create);

struct cel_sysfs_obj *cel_sysfs_kobject_init(void)
{
	struct cel_sysfs_obj *obj;

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
        CEL_DEBUG("cel_sysfs_kobject_create %s kzalloc error");
		return NULL;
	}

	obj->kobj.kset = cel_sysfs_kset;
	obj->kobj.sd = cel_sysfs_kset->kobj.sd;

	kobject_init(&obj->kobj, &cel_sysfs_ktype);

    kobject_uevent(&obj->kobj, KOBJ_ADD);
	return obj;
}
EXPORT_SYMBOL(cel_sysfs_kobject_init);


void cel_sysfs_kobject_delete(struct cel_sysfs_obj **obj)
{
	kobject_put(&((*obj)->kobj));
    *obj = NULL;
}
EXPORT_SYMBOL(cel_sysfs_kobject_delete);

void cel_sysfs_kobject_change(struct cel_sysfs_obj *obj)
{
    kobject_uevent(&obj->kobj, KOBJ_CHANGE);
}
EXPORT_SYMBOL(cel_sysfs_kobject_change);

static int __init cel_sysfs_init(void)
{

	cel_sysfs_kset = kset_create_and_add(KOBJ_NAME, NULL, NULL);
	if (!cel_sysfs_kset) {
		printk("%s kset_create_and_add error", __func__);
		return -ENOMEM;
	}

	return 0;
}

static void __exit cel_sysfs_exit(void)
{
	kset_unregister(cel_sysfs_kset);

}

module_init(cel_sysfs_init);
module_exit(cel_sysfs_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Larry Ming <laming@celestica.com>");
MODULE_DESCRIPTION("Celestica sysfs core driver");  
