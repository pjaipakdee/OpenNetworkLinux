/*
 * Watchdog driver for the Seastone cel
 *
 * Copyright (C) 2017 Celestica Corp.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/delay.h>

#include "cel_sysfs_core.h"


#define DRIVER_NAME "cel_wdt"

#define RESET_CTRL		0x102
#define WDT_MASK		0x04
#define WDI_GPIO_DIR	0x504
#define WDI_GPIO		0x508

static bool nowayout = WATCHDOG_NOWAYOUT;

static struct cel_sysfs_obj *wdt_obj;


struct cel_wdt_drvdata {
	struct watchdog_device wdt;
	struct mutex lock;
};

static struct resource cel_wdt_resources[] = {
    {
        .flags  = IORESOURCE_IO,
    },
};

static void cel_wdt_dev_release( struct device * dev)
{
    return;
}

static struct platform_device cel_wdt_dev = {
    .name           = DRIVER_NAME,
    .id             = -1,
    .num_resources  = ARRAY_SIZE(cel_wdt_resources),
    .resource       = cel_wdt_resources,
    .dev = {
        .release = cel_wdt_dev_release,
    }
};

static int cel_wdt_start(struct watchdog_device *wdt_dev)
{
    struct cel_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);
    unsigned char reset_ctrl = 0x00;
    unsigned long gpio ,dir;

    mutex_lock(&drvdata->lock);

    gpio = inl(WDI_GPIO);
    gpio |= 1 << 15;
    outl(gpio, WDI_GPIO);

    outl((inl(WDI_GPIO_DIR) & (~(1 << 15))), WDI_GPIO_DIR);

    reset_ctrl = inb(RESET_CTRL);

    gpio = inl(WDI_GPIO);
    gpio &= ~(1 << 15);
    outl_p( gpio, WDI_GPIO );

    mdelay(10);

    gpio = inl(WDI_GPIO);
    gpio |= (1 << 15);
    outl_p( gpio, WDI_GPIO );

    reset_ctrl |= WDT_MASK;
    outb(reset_ctrl, RESET_CTRL);

    mutex_unlock(&drvdata->lock);

    return 0;
}

static int cel_wdt_stop(struct watchdog_device *wdt_dev)
{
    struct cel_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);
    unsigned long reset_ctrl;

    mutex_lock(&drvdata->lock);

    reset_ctrl = inb(RESET_CTRL);
    reset_ctrl &= ~WDT_MASK;
    outb(reset_ctrl, RESET_CTRL);

    mutex_unlock(&drvdata->lock);

    return 0;
}

static int cel_wdt_ping(struct watchdog_device *wdt_dev)
{
    struct cel_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);
    unsigned long gpio;

    mutex_lock(&drvdata->lock);

    gpio = inl(WDI_GPIO);
    gpio &= ~(1 << 15);
    outl_p( gpio, WDI_GPIO );

    mdelay(10);

    gpio = inl(WDI_GPIO);
    gpio |= (1 << 15);
    outl_p( gpio, WDI_GPIO );

    mutex_unlock(&drvdata->lock);

    return 0;
}

static const struct watchdog_info cel_wdt_info = {
    .options = WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
    .identity = "cel Watchdog",
};

static const struct watchdog_ops cel_wdt_ops = {
    .owner = THIS_MODULE,
    .start = cel_wdt_start,
    .stop = cel_wdt_stop,
    .ping = cel_wdt_ping,
};

static int cel_wdt_probe(struct platform_device *pdev)
{
    struct cel_wdt_drvdata *drvdata;
    int ret;

    drvdata = devm_kzalloc(&pdev->dev, sizeof(*drvdata),
					           GFP_KERNEL);
    if (!drvdata) {
        ret = -ENOMEM;
        goto err;
    }

    mutex_init(&drvdata->lock);

    drvdata->wdt.info = &cel_wdt_info;
    drvdata->wdt.ops = &cel_wdt_ops;

    watchdog_set_nowayout(&drvdata->wdt, nowayout);
    watchdog_set_drvdata(&drvdata->wdt, drvdata);

    ret = watchdog_register_device(&drvdata->wdt);
    if (ret != 0) {
        dev_err(&pdev->dev, "watchdog_register_device() failed: %d\n",
                ret);
        goto err;
    }

    platform_set_drvdata(pdev, drvdata);

	wdt_obj = cel_sysfs_kobject_init();
	if(!wdt_obj) {
		printk("create kobj %s error", WDT_DIR_NAME);
		return -ENODEV;
	}
	ret = sysfs_create_link(&wdt_obj->kobj, &pdev->dev.kobj, WDT_DIR_NAME);
	if (ret) {
		printk("Cannot create the link: err=%d\n", ret);
		kobject_put(wdt_obj);
		return -ENODEV;
	}

err:
        return ret;
}

static int cel_wdt_remove(struct platform_device *pdev)
{
    struct cel_wdt_drvdata *drvdata = platform_get_drvdata(pdev);

	sysfs_remove_link(&wdt_obj->kobj, WDT_DIR_NAME);
	kobject_put(wdt_obj);
    watchdog_unregister_device(&drvdata->wdt);

    return 0;
}

static struct platform_driver cel_wdt_drv = {
    .probe = cel_wdt_probe,
    .remove = cel_wdt_remove,
    .driver = {
        .name = DRIVER_NAME,
    },
};

int cel_wdt_init(void)
{
    platform_device_register(&cel_wdt_dev);
    platform_driver_register(&cel_wdt_drv);

    return 0;
}

void cel_wdt_exit(void)
{
    platform_driver_unregister(&cel_wdt_drv);
    platform_device_unregister(&cel_wdt_dev);
}

module_init(cel_wdt_init);
module_exit(cel_wdt_exit);

MODULE_AUTHOR("Larry Ming <laming@celestica.com>");
MODULE_DESCRIPTION("Celestica Watchdog");
MODULE_LICENSE("GPL");
