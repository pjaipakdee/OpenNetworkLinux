/*
 * Copyright (C) 2017-2018 CELESTICA, INC. ALL RIGHTS RESERVED.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
 
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/acpi.h>
#include <linux/io.h>
#include <linux/dmi.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <uapi/linux/stat.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#include "cloverstone_switchboard.h"

#define MOD_VERSION             "1.0.0"
#define CMM_FPGA_NAME   "cloverstone_cmmfpga"
#define LC0_CLASS_NAME  "cloverstone_lc0fpga"
#define LC1_CLASS_NAME  "cloverstone_lc1fpga"
#define DRIVER_NAME     "cloverstone"
#define FPGA_PCI_NAME   "cloverstone_fpga_pci"

#define CMM_FPGA_DEV    "cmm_fpga"
#define LC0_FPGA_DEV    "lc0_fpga"
#define LC1_FPGA_DEV    "lc1_fpga"

/* MISC           */
#define FPGA_VERSION            0x0000
#define FPGA_VERSION_MJ_MSK     0xff00
#define FPGA_VERSION_MN_MSK     0x00ff
#define FPGA_SCRATCH            0x0004
#define FPGA_BROAD_TYPE         0x0008
#define FPGA_BROAD_REV_MSK      0x0038
#define FPGA_BROAD_ID_MSK       0x0007
#define FPGA_PLL_STATUS         0x0014
#define BMC_I2C_SCRATCH         0x0020
#define FPGA_SLAVE_CPLD_REST    0x0030
#define FPGA_PERIPH_RESET_CTRL  0x0034
#define FPGA_INT_STATUS         0x0040
#define FPGA_INT_SRC_STATUS     0x0044
#define FPGA_INT_FLAG           0x0048
#define FPGA_INT_MASK           0x004c
#define FPGA_MISC_CTRL          0x0050
#define FPGA_MISC_STATUS        0x0054
#define FPGA_AVS_VID_STATUS     0x0068
#define FPGA_FEATURE_CARD_GPIO  0x0070
#define FPGA_PORT_XCVR_READY    0x000c

#ifndef PCI_VENDOR_ID_XILINXCLS
#define PCI_VENDOR_ID_XILINXCLS 0x10EE
#endif
#define FPGA_PCIE_DEVICE_ID 0x7011
#define FPGA_PCI_BAR_NUM 0

#define CMM_FPGA_BTYPE 0
#define LC0_FPGA_BTYPE 1
#define LC1_FPGA_BTYPE 2

struct fpga_reg_data {
	uint32_t addr;
	uint32_t value;
};

enum {
	READREG,
	WRITEREG
};

static int max_devices = 3;
module_param(max_devices, int, 0644);
MODULE_PARM_DESC(max_devices, "max number of switchtec device instances");

static dev_t fpga_devt;
static DEFINE_IDA(fpga_minor_ida);

struct class *fpga_class;
EXPORT_SYMBOL_GPL(fpga_class);

static struct fpgatec_priv *fpgadev_p_create(struct pci_dev *pdev)
{
    struct fpgatec_priv *fpga_dev_data;

    fpga_dev_data = kzalloc_node(sizeof(*fpga_dev_data), GFP_KERNEL, dev_to_node(&pdev->dev));
    if (!fpga_dev_data) {
             printk(KERN_INFO "fpga_dev_data = NULL\n");
            return ERR_PTR(-ENOMEM);
    }

    return fpga_dev_data;
}

static void fpga_dev_release(struct device *dev)
{
    struct fpgatec_priv *fpga_dev_data = container_of(dev, struct fpgatec_priv, ddev);

    kfree(fpga_dev_data);
}

static void fpga_dev_kill(struct fpgatec_priv *fpga_dev_data)
{
	pci_clear_master(fpga_dev_data->pdev);
}

static int fpgatec_dev_open(struct inode *inode, struct file *filp)
{
	struct fpgatec_priv *fpga_dev_data;

	fpga_dev_data = container_of(inode->i_cdev, struct fpgatec_priv, chrdev);
	if (!fpga_dev_data) {
		printk(KERN_INFO "open faild stadev = NULL\n");
	}
	filp->private_data = fpga_dev_data;
	nonseekable_open(inode, filp);

	//dev_dbg(&stdev->pdev, "%s: %p\n", __func__, stdev);
	return 0;
}

static long fpgafw_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long reg_data)
{
	int ret = 0;
	struct fpgatec_priv *fpga_dev_data = file->private_data;
	struct fpga_reg_data data;
	void __iomem * data_base_addr;

	if (copy_from_user(&data, (void __user*)reg_data, sizeof(data)) != 0) {
		return -EFAULT;
	}

	data_base_addr = fpga_dev_data->base_addr;
	//printk(KERN_INFO "base address:0x%p\n", data_base_addr);
	mutex_lock(&fpga_dev_data->fpga_lock);

    // Switch function to read and write.
    switch (cmd){
        case READREG:
            data.value = ioread32(data_base_addr+data.addr);
            if (copy_to_user((void __user*)reg_data ,&data, sizeof(data)) != 0){
                mutex_unlock(&fpga_dev_data->fpga_lock);
                return -EFAULT;
            }
            break;
        case WRITEREG:
            iowrite32(data.value, data_base_addr+data.addr);
            break;
        default:
            mutex_unlock(&fpga_dev_data->fpga_lock);
            return -EINVAL;
    }

    mutex_unlock(&fpga_dev_data->fpga_lock);
    return ret;
}

const struct file_operations fpgafw_fops = {
    .owner      = THIS_MODULE,
    .open           = fpgatec_dev_open,
    .unlocked_ioctl = fpgafw_unlocked_ioctl,
};

static int cmm_fpga_fw_init(struct fpgatec_priv *f_priv, struct pci_dev *pdev)
{
#if 0
        int rc;
        int major_num, minor;;
        char *dev_name, *class_name;
        struct class *dev_class;
        static struct device *dev = NULL;
        struct cdev *cdev;

        printk(KERN_INFO "Device registered correctly with major number %d\n", major_num);

        // Register the device class
        class_name = f_priv->class_name;
        dev_class = class_create(THIS_MODULE, class_name);/*_check*/
        if (IS_ERR(dev_class)) {                // Check for error and clean up if there is
                unregister_chrdev(major_num, dev_name);
                printk(KERN_ALERT "Failed to register device class\n");
                return PTR_ERR(dev_class);          // Correct way to return an error on a pointer
        }
        printk(KERN_INFO "Device class registered correctly\n");

        minor = ida_simple_get(&fpga_minor_ida, 0, 0,
                               GFP_KERNEL);
        if (minor < 0) {
                rc = minor;
                goto err_put;
        }

        dev->devt = MKDEV(MAJOR(fpga_devt), minor);
        dev_set_name(dev, "cloverstone_fpga%d", minor);

        cdev = &f_priv->cdev;
        cdev_init(cdev, &fpgafw_fops);
        cdev->owner = THIS_MODULE;

        rc = cdev_device_add(&f_priv->chrdev, &f_priv->ddev);
        if (rc)
                goto err_devadd;

        /f_priv->ddev = dev;
        /f_priv->fpgafw_class = dev_class;
        printk(KERN_INFO "FPGA fw upgrade device node created correctly\n"); // Made it! device was initialized
        return 0;

err_put:
        put_device(&f_priv->dev);
                return ERR_PTR(rc);
err_devadd:
        fpga_dev_kill(f_priv);
        ida_simple_remove(&fpga_minor_ida, MINOR(f_priv->dev.devt));
        put_device(&f_priv->dev);
        return rc;
#endif
        return 0;
}

static int lc_fpga_fw_init(struct fpgatec_priv *f_priv, struct pci_dev *pdev)
{
    int rc;
    int minor;
    static struct device *dev = NULL;
    struct cdev *cdev;

    printk(KERN_INFO "Initializing the fpga driver\n");

    dev = &f_priv->ddev;
    device_initialize(dev);
    dev->class = fpga_class;
    dev->parent = &pdev->dev;
    //dev->groups = switchtec_device_groups;
    dev->release = fpga_dev_release;

    f_priv->fpgafw_class = fpga_class;

    minor = ida_simple_get(&fpga_minor_ida, 0, 0, GFP_KERNEL);
    if (minor < 0) {
        rc = minor;
        goto err_put;
    }

    dev->devt = MKDEV(MAJOR(fpga_devt), minor);
    dev_set_name(dev, "fpga%d", minor);

    cdev = &f_priv->chrdev;
    cdev_init(cdev, &fpgafw_fops);
    cdev->owner = THIS_MODULE;

    rc = cdev_device_add(&f_priv->chrdev, &f_priv->ddev);
    if (rc)
		goto err_devadd;

    printk(KERN_INFO "FPGA fw upgrade device node created correctly\n");
    //cloverstone_dev.dev.p = f_priv;
    //platform_device_register(&cloverstone_dev);
    //platform_driver_register(&cloverstone_drv);
    return 0;

err_devadd:
        fpga_dev_kill(f_priv);
        ida_simple_remove(&fpga_minor_ida, MINOR(f_priv->ddev.devt));
err_put:
        put_device(&f_priv->ddev); 
}

static int fpgadev_init_pci(struct fpgatec_priv *f_priv, struct pci_dev *pdev)
{
	int err, i;
	uint32_t buff;
	void __iomem *base_addr;
	struct device *dev = &pdev->dev;

	if ((err = pci_enable_device(pdev))) {
		dev_err(dev, "pci_enable_device probe error %d for device %s\n",
			err, pci_name(pdev));
		return err;
	}

    if ((err = pci_request_regions(pdev, FPGA_PCI_NAME)) < 0) {
        dev_err(dev, "pci_request_regions error %d\n", err);
		goto pci_disable;
    }

    /* bar0: data mmio region */
    base_addr = pci_iomap(pdev, FPGA_PCI_BAR_NUM, 0);
    if (!base_addr) {
        dev_err(dev, "cannot iomap region of size %lu\n",(unsigned long)base_addr);
        goto pci_release;
    }
	
	f_priv->base_addr = base_addr;
	f_priv->mmio_start = pci_resource_start(pdev, FPGA_PCI_BAR_NUM);
	f_priv->mmio_len = pci_resource_len(pdev, FPGA_PCI_BAR_NUM);

	dev_info(dev, "data_mmio iomap base = 0x%lx \n",
	                                        (unsigned long)f_priv->base_addr);
	dev_info(dev, "data_mmio_start = 0x%lx data_mmio_len = %lu\n",
	                                        (unsigned long)f_priv->mmio_start,
	                                        (unsigned long)f_priv->mmio_len);

	printk(KERN_INFO "FPGA PCIe driver probe OK.\n");
    printk(KERN_INFO "FPGA ioremap registers of size %lu\n",(unsigned long)f_priv->mmio_len);
    printk(KERN_INFO "");
#if 1
    iowrite32(0x11223344, f_priv->base_addr+4);
    buff = ioread32(f_priv->base_addr);
    printk(KERN_INFO "FPGA VERSION : 0x0   %8.8x\n", buff);
    buff = ioread32(f_priv->base_addr + 4);
    printk(KERN_INFO "FPGA Scratch : 0x04   %8.8x\n", buff);
#endif
	f_priv->pdev = pdev;
	pci_set_drvdata(pdev, f_priv);

	return 0;
pci_release:
	pci_release_regions(pdev);
pci_disable:
	pci_disable_device(pdev);

	return -EBUSY;
}

static int i = 0;
static int fpga_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int rc;
	struct fpgatec_priv *fpga_dev_data;
	uint32_t fpga_board_type;

	printk(KERN_INFO "fpga pci probe start\n");

	fpga_dev_data = fpgadev_p_create(pdev);
	if (IS_ERR(fpga_dev_data))
		return PTR_ERR(fpga_dev_data);

	rc = fpgadev_init_pci(fpga_dev_data, pdev);
	if(rc != 0) {
		printk(KERN_ERR "fpga pci probe error\n");
		return rc;
	}

    //fpga_board_type = ioread32(fpga_dev_data->base_addr + FPGA_BROAD_TYPE);
    fpga_board_type = 1;

    switch (fpga_board_type)
    {
        case CMM_FPGA_BTYPE:
            //stdev->dev_name = CMM_FPGA_DEV;
            //stdev->class_name = CMM_FPGA_NAME;
            //cmm_fpga_fw_init(stdev, pdev);
            break;
        case LC0_FPGA_BTYPE:
            fpga_dev_data->dev_name = LC0_FPGA_DEV;
            fpga_dev_data->class_name = LC0_CLASS_NAME;
            lc_fpga_fw_init(fpga_dev_data, pdev);
            break;
        case LC1_FPGA_BTYPE:
            fpga_dev_data->dev_name = LC1_FPGA_DEV;
            fpga_dev_data->class_name = LC1_CLASS_NAME;
            lc_fpga_fw_init(fpga_dev_data, pdev);
            break;
    	default:
			break;
    }
    return 0;
}

static void fpga_pci_remove(struct pci_dev *pdev)
{
	struct fpgatec_priv *f_pdev = pci_get_drvdata(pdev);

    pci_set_drvdata(pdev, NULL);
    cdev_device_del(&f_pdev->chrdev, &f_pdev->ddev);
    ida_simple_remove(&fpga_minor_ida, MINOR(f_pdev->ddev.devt));
    dev_info(&f_pdev->ddev, "unregistered.\n");
    printk(KERN_INFO "Goodbye!\n");

    pci_iounmap(pdev, f_pdev->base_addr);
    pci_release_regions(pdev);
    pci_disable_device(pdev);
    printk(KERN_INFO "FPGA PCIe driver remove OK.\n");
};

static const struct pci_device_id fpga_id_table[] = {
    {  PCI_VDEVICE(XILINXCLS, FPGA_PCIE_DEVICE_ID) },
    {0},
};
MODULE_DEVICE_TABLE(pci, fpga_id_table);

static struct pci_driver pci_dev_ops = {
    .name       = FPGA_PCI_NAME,
    .probe      = fpga_pci_probe,
    .remove     = fpga_pci_remove,
    .id_table   = fpga_id_table,
};

static int __init cloverstone_init(void)
{
    int rc;

	printk(KERN_INFO "cloverstone init\n");
	rc = alloc_chrdev_region(&fpga_devt, 0, max_devices, "fpga");
	if (rc) {
		printk(KERN_INFO "alloc_chrdev_region error\n");
		return rc;
	}

	fpga_class = class_create(THIS_MODULE, "fpga");
	if (IS_ERR(fpga_class)) {
		rc = PTR_ERR(fpga_class);
		printk(KERN_INFO "class_create error\n");
		goto err_create_class;
	}

    rc = pci_register_driver(&pci_dev_ops);
    if (rc) {
		printk(KERN_INFO "pci_register_driver error\n");
		goto err_pci_register;
	}

    return 0;

err_pci_register:
	class_destroy(fpga_class);
err_create_class:
    unregister_chrdev_region(fpga_devt, max_devices);
    return rc;
}

static void __exit cloverstone_exit(void)
{
    //platform_driver_unregister(&cloverstone_drv);
    //platform_device_unregister(&cloverstone_dev);
	pci_unregister_driver(&pci_dev_ops);
    class_destroy(fpga_class);
    unregister_chrdev_region(fpga_devt, max_devices);
    ida_destroy(&fpga_minor_ida);
}

module_init(cloverstone_init);
module_exit(cloverstone_exit);

MODULE_AUTHOR("Carl Wang. carlwang@celestica.com");
MODULE_DESCRIPTION("Celestica cloverstone platform driver");
MODULE_VERSION(MOD_VERSION);
MODULE_LICENSE("GPL");

