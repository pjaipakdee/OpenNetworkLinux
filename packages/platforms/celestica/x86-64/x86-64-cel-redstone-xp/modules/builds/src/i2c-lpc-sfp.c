/*
 * Celestica LPC I2C bus driver, the LPC access to 4 cpld i2c controller
 *
 * Copyright (C)  Larry Ming  <laming at celestica.com>
 *
 * based on a (non-working) driver which was:
 *
 * Copyright (C) Celestica Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 */

#include <linux/slab.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/io.h>

#include "cel_sysfs_core.h"


#define DRIVER_NAME "cel-i2c"

#define CONFIG_CPLD_1
#define CONFIG_CPLD_2
#define CONFIG_CPLD_3
#define CONFIG_CPLD_4
#define CONFIG_CPLD_5


#ifdef CONFIG_CPLD_1
#define CPLD_IO_OFF_1 0x100
#define CPLD_1_SFP_START 0 //no QSFP
#define CPLD_1_SFP_END 0 //no QSFP
#endif

#ifdef CONFIG_CPLD_2
#define CPLD_IO_OFF_2 0x200
#define CPLD_2_SFP_START 1//no QSFP
#define CPLD_2_SFP_END 18//no QSFP
#endif

#ifdef CONFIG_CPLD_3
#define CPLD_IO_OFF_3 0x280
#define CPLD_3_SFP_START 19//no QSFP
#define CPLD_3_SFP_END 36//no QSFP
#endif

#ifdef CONFIG_CPLD_4
#define CPLD_IO_OFF_4 0x300
#define CPLD_4_SFP_START 49 
#define CPLD_4_SFP_END 54 
#endif

#ifdef CONFIG_CPLD_5
#define CPLD_IO_OFF_5 0x380
#define CPLD_5_SFP_START 37 //no QSFP
#define CPLD_5_SFP_END 48 //no QSFP
#endif

#define CPLD_SFP_RXLOS_STAT_OFFSET 0x40
#define CPLD_SFP_RXLOS_INT_OFFSET 0x43
#define CPLD_SFP_RXLOS_INTMASK_OFFSET 0x46
#define CPLD_SFP_TXDIS_OFFSET 0x50
#define CPLD_SFP_RS_OFFSET 0x53
#define CPLD_SFP_TXFAULT_OFFSET 0x56
#define CPLD_SFP_ABS_OFFSET 0x59

#define CPLD_SFP_RXLOS_STAT_OFFSET_CPLD_5 0x40
#define CPLD_SFP_RXLOS_INT_OFFSET_CPLD_5 0x42
#define CPLD_SFP_RXLOS_INTMASK_OFFSET_CPLD_5 0x44
#define CPLD_SFP_TXDIS_OFFSET_CPLD_5 0x50
#define CPLD_SFP_RS_OFFSET_CPLD_5 0x54
#define CPLD_SFP_TXFAULT_OFFSET_CPLD_5 0x56
#define CPLD_SFP_ABS_OFFSET_CPLD_5 0x56

#define CPLD_QSFP_RESET_OFFSET 0x60
#define CPLD_QSFP_LPMOD_OFFSET 0x61
#define CPLD_QSFP_ABS_OFFSET 0x62

#define SFP_COUNT 48
#define QSFP_COUNT 6

#define I2C_LPC_MAX_BUS  (54)

typedef enum {
#ifdef CONFIG_CPLD_1
	CPLD_1_I2C_LOCK_NUM,
#endif
#ifdef CONFIG_CPLD_2
	CPLD_2_I2C_LOCK_NUM,
#endif
#ifdef CONFIG_CPLD_3
	CPLD_3_I2C_LOCK_NUM,
#endif
#ifdef CONFIG_CPLD_4
	CPLD_4_I2C_LOCK_NUM,
#endif
#ifdef CONFIG_CPLD_5
	CPLD_5_I2C_LOCK_NUM,
#endif
	CPLD_I2C_BUS_LOCK_MAX,
}CPLD_LOCK_TYPE_T;

static int lpc_i2c_bus[I2C_LPC_MAX_BUS];
static int lpc_i2c_bus_num;
module_param_array(lpc_i2c_bus, int, &lpc_i2c_bus_num, 0);
MODULE_PARM_DESC(lpc_i2c_bus, "Mapping I2C bus id");

#define CLS_I2C_CLOCK_LEGACY   0
#define CLS_I2C_CLOCK_PRESERVE (~0U)

/*
 * The CEL_CPLD_I2C_XXX offsets are relative to the start of the "I2C
 * block" within each CPLD:
 *
 * CPLD2 -- 0x10
 * CPLD3 -- 0x40
 * CPLD4 -- 0x80
 *
 */
#define CEL_CPLD_I2C_PORT_ID		0x10
#define CEL_CPLD_I2C_OPCODE		0x11
#define CEL_CPLD_I2C_DEV_ADDR		0x12
#define CEL_CPLD_I2C_CMD_BYTE0		0x13
#define CEL_CPLD_I2C_CMD_BYTE1		0x14
#define CEL_CPLD_I2C_CMD_BYTE2		0x15
#define CEL_CPLD_I2C_CSR		0x16
#define CEL_CPLD_I2C_WRITE_DATA		0x20
#define WRITE_DATA(x) (CEL_CPLD_I2C_WRITE_DATA + (1*x))
#define CEL_CPLD_I2C_READ_DATA		0x30
#define READ_DATA(x) (CEL_CPLD_I2C_READ_DATA + (1*x))

#define CEL_CPLD_I2C_CLK_50KHZ		0x00
#define CEL_CPLD_I2C_CLK_100KHZ		0x40

#define CSR_MASTER_ERROR		0x80
#define CSR_BUSY			0x40
#define CSR_MASTER_RESET_L		0x01

#define OPCODE_DATA_LENGTH_SHIFT	4
#define OPCODE_CMD_LENGTH		1

#define DEV_ADDR_READ_OP		0x1
#define DEV_ADDR_WRITE_OP		(~(0x1))

//#define kernel_debug

struct cel_cpld_i2c {
	struct device     *m_dev;
	int	sff_port_id;		/* bus mux for sfp port */
	int sff_port_start;
	int sff_port_end;
	//void __iomem      *m_base;
	phys_addr_t m_base;
	struct i2c_adapter m_adap;
	u8                 m_clk_freq;
};


static struct cel_cpld_i2c	cel_i2c[I2C_LPC_MAX_BUS];
static struct i2c_adapter 		i2c_lpc_adap[I2C_LPC_MAX_BUS];
static struct mutex i2c_xfer_lock[CPLD_I2C_BUS_LOCK_MAX];

static struct cel_sysfs_obj *sff_obj;
static struct cel_sysfs_obj *cpld_obj;


#define cel_i2c_clk_50Khz    outb(63, i2c->base + CLS_I2C_FDR)
#define cel_i2c_clk_100Khz    outb(31, i2c->base + CLS_I2C_FDR)
#define cel_i2c_clk_200Khz    outb(15, i2c->base + CLS_I2C_FDR)
#define cel_i2c_clk_400Khz    outb(7, i2c->base + CLS_I2C_FDR)

typedef enum{
	CLK50KHZ = 0,
	CLK100KHZ,
	CLK200KHZ,
	CLK400KHZ,
}I2C_CLK;


static char readme[] = "Usage:\n"
                       "\techo address > read           # Read the value at address\n"
                       "\techo address:value > write    # Write value to address\n";



/*
 * Wait up to 1 second for the controller to be come non-busy.
 *
 * Returns:
 *   - success:  0
 *   - failure:  negative status code
 */
static int cel_cpld_wait(struct cel_cpld_i2c *i2c)
{
	unsigned long orig_jiffies = jiffies;
	int rc = 0;
	u8 csr;

	/* Allow bus up to 1s to become not busy */
	while ((csr = inb(i2c->m_base + CEL_CPLD_I2C_CSR)) & CSR_BUSY) {
		//CEL_DEBUG("addr: 0x%08x, status: 0x%02x\n", i2c->m_base + CEL_CPLD_I2C_CSR, csr);
		if (signal_pending(current)) {
			return -EINTR;
		}
		if (time_after(jiffies, orig_jiffies + HZ)) {
			dev_warn(i2c->m_dev, "Bus busy timeout\n");
			rc = -ETIMEDOUT;
			break;
		}
		schedule();
	}

	if (csr & CSR_MASTER_ERROR) {
		/* Typically this means the SFP+ device is not present. */
		/* Clear master error with the master reset. */
		outb(~CSR_MASTER_RESET_L,
		       i2c->m_base + CEL_CPLD_I2C_CSR);
		udelay(3000);
		outb(CSR_MASTER_RESET_L,
		       i2c->m_base + CEL_CPLD_I2C_CSR);
		rc = rc ? rc : -EIO;
	}

	return rc;
}

static int cel_cpld_i2c_write(struct cel_cpld_i2c *i2c, int target,
			      u8 offset, const u8 *data, int length)
{
	u8 tmp, xfer_len, i;
	int ret, total_xfer = 0;

	CEL_DEBUG("target=0x%02x, offset=0x%02x, length=%d\n", target, offset, length);
	/* The CEL-CPLD I2C master writes in units of 8 bytes */
	while (length > 0) {

		/* Configure byte offset within device */
		outb(offset + total_xfer,
		       i2c->m_base + CEL_CPLD_I2C_CMD_BYTE0);

		/* Configure transfer length - max of 8 bytes */
		xfer_len = (length > 8) ? 8 : length;
		tmp = (xfer_len << OPCODE_DATA_LENGTH_SHIFT);
		tmp |= OPCODE_CMD_LENGTH;
		outb(tmp, i2c->m_base + CEL_CPLD_I2C_OPCODE);

		/* Load the transmit data into the send buffer */
		CEL_DEBUG("data: ");
		for (i = 0; i < xfer_len; i++) {
			outb(data[total_xfer + i], i2c->m_base + WRITE_DATA(i));
			CEL_DEBUG("0x%02x ", data[total_xfer + i]);
		}
		CEL_DEBUG("\n");

		/* Initiate write transaction */
		tmp = (target << 1) & DEV_ADDR_WRITE_OP;
		outb(tmp, i2c->m_base + CEL_CPLD_I2C_DEV_ADDR);
		CEL_DEBUG("opcode=0x%02x, cmd=0x%02x, dev_addr=0x%02x\n", inb(i2c->m_base + CEL_CPLD_I2C_OPCODE),
			inb(i2c->m_base + CEL_CPLD_I2C_CMD_BYTE0), inb(i2c->m_base + CEL_CPLD_I2C_DEV_ADDR));

		/* Wait for transfer completion */
		ret = cel_cpld_wait(i2c);
		if (ret)
			return ret;

		total_xfer += xfer_len;
		length -= xfer_len;
	}

	return 0;
}

static int cel_cpld_i2c_read(struct cel_cpld_i2c *i2c, int target,
			     u8 offset, u8 *data, int length)
{
	u8 tmp, xfer_len, i;
	int ret, total_xfer = 0;

	CEL_DEBUG("target=0x%02x, offset=0x%02x, length=%d\n", target, offset, length);
	/* The CEL-CPLD I2C master reads in units of 8 bytes */
	while (length > 0) {

		/* Configure byte offset within device */
		outb(offset + total_xfer,
		       i2c->m_base + CEL_CPLD_I2C_CMD_BYTE0);

		/* Configure transfer length - max of 8 bytes */
		xfer_len = (length > 8) ? 8 : length;
		tmp = (xfer_len << OPCODE_DATA_LENGTH_SHIFT);
		tmp |= OPCODE_CMD_LENGTH;
		outb(tmp, i2c->m_base + CEL_CPLD_I2C_OPCODE);

		/* Initiate read transaction */
		tmp = (target << 1) | DEV_ADDR_READ_OP;
		outb(tmp, i2c->m_base + CEL_CPLD_I2C_DEV_ADDR);

		CEL_DEBUG("opcode=0x%02x, cmd=0x%02x, dev_addr=0x%02x\n", inb(i2c->m_base + CEL_CPLD_I2C_OPCODE),
			inb(i2c->m_base + CEL_CPLD_I2C_CMD_BYTE0), inb(i2c->m_base + CEL_CPLD_I2C_DEV_ADDR));
		/* Wait for transfer completion */
		ret = cel_cpld_wait(i2c);
		if (ret)
			return ret;

		/* Gather up the results */
		CEL_DEBUG("data: ");
		for (i = 0; i < xfer_len; i++) {
			data[total_xfer + i] = inb(i2c->m_base + READ_DATA(i));
			CEL_DEBUG("0x%02x ", data[total_xfer + i]);
		}
		CEL_DEBUG("\n");

		total_xfer += xfer_len;
		length -= xfer_len;
	}

	return 0;
}
static int cel_lock(struct cel_cpld_i2c *i2c, int id){
#ifdef CONFIG_CPLD_1
	if(id >= CPLD_1_SFP_START && id <= CPLD_1_SFP_END) {
		mutex_lock(&i2c_xfer_lock[CPLD_1_I2C_LOCK_NUM]);
		return 0;
	}
#endif

#ifdef CONFIG_CPLD_2
	if(id >= CPLD_2_SFP_START && id <= CPLD_2_SFP_END) {
		mutex_lock(&i2c_xfer_lock[CPLD_2_I2C_LOCK_NUM]);
		return 0;
	}
#endif

#ifdef CONFIG_CPLD_3
	if(id >= CPLD_3_SFP_START && id <= CPLD_3_SFP_END) {
		mutex_lock(&i2c_xfer_lock[CPLD_3_I2C_LOCK_NUM]);
		return 0;
	}
#endif

#ifdef CONFIG_CPLD_4
	if(id >= CPLD_4_SFP_START && id <= CPLD_4_SFP_END) {
		mutex_lock(&i2c_xfer_lock[CPLD_4_I2C_LOCK_NUM]);
		return 0;
	}
#endif

#ifdef CONFIG_CPLD_5
	if(id >= CPLD_5_SFP_START && id <= CPLD_5_SFP_END) {
		mutex_lock(&i2c_xfer_lock[CPLD_5_I2C_LOCK_NUM]);
		return 0;
	}
#endif

	return -1;
}

static int cel_unlock(struct cel_cpld_i2c *i2c, int id){
#ifdef CONFIG_CPLD_1
		if(id >= CPLD_1_SFP_START && id <= CPLD_1_SFP_END) {
			mutex_unlock(&i2c_xfer_lock[CPLD_1_I2C_LOCK_NUM]);
			return 0;
		}
#endif
	
#ifdef CONFIG_CPLD_2
		if(id >= CPLD_2_SFP_START && id <= CPLD_2_SFP_END) {
			mutex_unlock(&i2c_xfer_lock[CPLD_2_I2C_LOCK_NUM]);
			return 0;
		}
#endif
	
#ifdef CONFIG_CPLD_3
		if(id >= CPLD_3_SFP_START && id <= CPLD_3_SFP_END) {
			mutex_unlock(&i2c_xfer_lock[CPLD_3_I2C_LOCK_NUM]);
			return 0;
		}
#endif
	
#ifdef CONFIG_CPLD_4
		if(id >= CPLD_4_SFP_START && id <= CPLD_4_SFP_END) {
			mutex_unlock(&i2c_xfer_lock[CPLD_4_I2C_LOCK_NUM]);
			return 0;
		}
#endif
	
#ifdef CONFIG_CPLD_5
		if(id >= CPLD_5_SFP_START && id <= CPLD_5_SFP_END) {
			mutex_unlock(&i2c_xfer_lock[CPLD_5_I2C_LOCK_NUM]);
			return 0;
		}
#endif

	return -1;
}

static int cel_cpld_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct i2c_msg *pmsg;
	int ret = 0;
	struct cel_cpld_i2c *i2c = i2c_get_adapdata(adap);
	u8 offset;

	CEL_DEBUG("sff_port_id=%d, m_base=0x%08x\n", i2c->sff_port_id, i2c->m_base);
	int id = i2c->sff_port_id;
	cel_lock(i2c, id);
	/*select port, id 1~48*/
	outb(CEL_CPLD_I2C_CLK_100KHZ+id, i2c->m_base + CEL_CPLD_I2C_PORT_ID);
	

	/* Allow bus to become not busy */
	ret = cel_cpld_wait(i2c);
	if (ret)
	{
		cel_unlock(i2c, id);
		return ret;
	}

	/*
	 * This is somewhat gimpy.
	 *
	 * The CEL-CPLD I2C master is special built to read/write SFP+
	 * EEPROMs only.  It is *not* a general purpose I2C master.
	 * The clients of this master are *always* expected to be
	 * "at,24c04" or "sff-8436 based qsfp" compatible EEPROMs.
	 *
	 * As such we have the following expectations for READ operation:
	 *
	 *    - number of messages is "2"
	 *    - msg[0] contains info about the offset within the 512-byte EEPROM
	 *      - msg[0].len = 1
	 *      - msg[0].buf[0] contains the offset
	 *    - msg[1] contains info about the read payload
	 *
	 * As such we have the following expectations for WRITE operation:
	 *
	 *    - number of messages is "1"
	 *    - msg[0] contains info about the offset within the 512-byte EEPROM
	 *    - msg[0].buf[0] contains the offset
	 *    - msg[0] also contains info about the write payload
	 */

	/*
	 * The offset within the EEPROM is stored in msg[0].buf[0].
	 */
	offset = msgs[0].buf[0];

	if (num == 1) {
		pmsg = &msgs[0];
	} else {
		pmsg = &msgs[1];
	}

	if ((offset + pmsg->len) > 0x200)
	{
		cel_unlock(i2c, id);
		return -EINVAL;
	}
		

	if (pmsg->flags & I2C_M_RD) {
/*
		if (num != 2) {
			dev_warn(i2c->m_dev, "Expecting 2 i2c messages. Got %d\n", num);
			cel_unlock(i2c, id);
			return -EINVAL;
		}
*/
		if (msgs[0].len != 1) {
			dev_warn(i2c->m_dev, "Expecting mgs[0].len == 1. Got %d\n",
                msgs[0].len);
			cel_unlock(i2c, id);
			return -EINVAL;
		}

		ret = cel_cpld_i2c_read(i2c, pmsg->addr, offset, pmsg->buf, pmsg->len);
	} else {
		ret = cel_cpld_i2c_write(i2c, pmsg->addr, offset, &(pmsg->buf[1]), (pmsg->len - 1));
	}

	cel_unlock(i2c, id);
	return (ret < 0) ? ret : num;
}

static u32 cel_cpld_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm cel_cpld_algo = {
	.master_xfer = cel_cpld_xfer,
	.functionality = cel_cpld_functionality,
};

static int __init i2c_lpc_add_bus (struct i2c_adapter *adap)
{
	int ret = 0;
	/* Register new adapter */
	adap->algo = &cel_cpld_algo;
	//ret = i2c_add_numbered_adapter(adap);
	ret = i2c_add_adapter(adap);
	return ret;
}

static void cel_cpld_i2c_bus_setup(struct cel_cpld_i2c *i2c,
					 u32 clock)
{
	u8 clk_freq = CEL_CPLD_I2C_CLK_50KHZ;

	if (clock == 100000)
		clk_freq = CEL_CPLD_I2C_CLK_100KHZ;

	i2c->m_clk_freq = clk_freq;
	outb(clk_freq, i2c->m_base + CEL_CPLD_I2C_PORT_ID);

	/* Reset the I2C master logic */
	outb(~CSR_MASTER_RESET_L, i2c->m_base + CEL_CPLD_I2C_CSR);
	udelay(3000);
	outb(CSR_MASTER_RESET_L, i2c->m_base + CEL_CPLD_I2C_CSR);

}

//static int i2c_init_internal_data(struct device *dev)
static int i2c_init_internal_data(void)
{
	int i;

	for( i = 0; i < I2C_LPC_MAX_BUS; i++ ){
		cel_i2c[i].m_clk_freq = 100000;//100khz
		cel_i2c[i].sff_port_id = i+1;
	}

	/*sfp port, portid 1~48, +10 = /dev/i2c-11 ~ /dev/i2c-58*/

#ifdef CONFIG_CPLD_1
	for( i = CPLD_1_SFP_START - 1; i >= 0 && i < CPLD_1_SFP_END; i++ ) {
		cel_i2c[i].m_base = CPLD_IO_OFF_1;
		cel_i2c[i].sff_port_start = CPLD_1_SFP_START;
		cel_i2c[i].sff_port_end = CPLD_1_SFP_END;
		cel_cpld_i2c_bus_setup(&cel_i2c[i], 100000);
	}
#endif

#ifdef CONFIG_CPLD_2
	for( i = CPLD_2_SFP_START - 1; i >= 0 && i < CPLD_2_SFP_END; i++ ) {
		cel_i2c[i].m_base = CPLD_IO_OFF_2;
		cel_i2c[i].sff_port_start = CPLD_2_SFP_START;
		cel_i2c[i].sff_port_end = CPLD_2_SFP_END;
		cel_cpld_i2c_bus_setup(&cel_i2c[i], 100000);
	}
#endif

#ifdef CONFIG_CPLD_3
	for( i = CPLD_3_SFP_START - 1; i >= 0 && i < CPLD_3_SFP_END; i++ ) {
		cel_i2c[i].m_base = CPLD_IO_OFF_3;
		cel_i2c[i].sff_port_start = CPLD_3_SFP_START;
		cel_i2c[i].sff_port_end = CPLD_3_SFP_END;
		cel_cpld_i2c_bus_setup(&cel_i2c[i], 100000);
	}
#endif

#ifdef CONFIG_CPLD_4
	for( i = CPLD_4_SFP_START - 1; i >= 0 && i < CPLD_4_SFP_END; i++ ) {
		cel_i2c[i].m_base = CPLD_IO_OFF_4;
		cel_i2c[i].sff_port_start = CPLD_4_SFP_START;
		cel_i2c[i].sff_port_end = CPLD_4_SFP_END;
		cel_cpld_i2c_bus_setup(&cel_i2c[i], 100000);
	}
#endif

#ifdef CONFIG_CPLD_5
	for( i = CPLD_5_SFP_START - 1; i >= 0 && i < CPLD_5_SFP_END; i++ ) {
		cel_i2c[i].m_base = CPLD_IO_OFF_5;
		cel_i2c[i].sff_port_start = CPLD_5_SFP_START;
		cel_i2c[i].sff_port_end = CPLD_5_SFP_END;
		cel_cpld_i2c_bus_setup(&cel_i2c[i], 100000);
	}
#endif
    
	return 0;
}


static ssize_t get_lpmode(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    int lpmod = 0;
	int offset;
	struct i2c_adapter *adap = container_of(dev, struct i2c_adapter, dev);
	struct cel_cpld_i2c *i2c = i2c_get_adapdata(adap);

	offset = i2c->sff_port_id - i2c->sff_port_start;
	
	if(i2c->sff_port_id <= SFP_COUNT){
		// if(i2c->sff_port_id >= CPLD_5_SFP_START){
		// 	ret = inb(i2c->m_base + CPLD_SFP_ABS_OFFSET_CPLD_5 + (offset / 8));
		// }else
		// {
		// 	ret = inb(i2c->m_base + CPLD_SFP_ABS_OFFSET + (offset / 8));
		// }
		ret = 0; //SFP don't have LP_MOD
	}else{
		ret = inb(i2c->m_base + CPLD_QSFP_LPMOD_OFFSET + (offset / 8));
	}


	lpmod = ret & (0x1 << (offset % 8));
	

    return sprintf(buf, "%d\n", !!lpmod);

}

static ssize_t set_lpmode(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    unsigned char data;
	int lpmod;
	int offset;
	struct i2c_adapter *adap = container_of(dev, struct i2c_adapter, dev);
	struct cel_cpld_i2c *i2c = i2c_get_adapdata(adap);
	if(i2c->sff_port_id <= SFP_COUNT){
		return 0; //SFP don't have LP_MOD
	}else{
		offset = i2c->sff_port_id - i2c->sff_port_start;
		if (sscanf(buf, "%d", &lpmod) <= 0) {
			return -EINVAL;
		}
		data = inb(i2c->m_base + CPLD_QSFP_LPMOD_OFFSET + (offset / 8));
		if(lpmod) {
			data |= (0x1 << (offset % 8));
		} else {
			data &= ~(0x1 << (offset % 8));
		}
		outb(data, i2c->m_base + CPLD_QSFP_LPMOD_OFFSET + (offset / 8));
	}
	
	
    return count;;
}

static ssize_t get_txdis(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    int txdis = 0;
	int offset;
	struct i2c_adapter *adap = container_of(dev, struct i2c_adapter, dev);
	struct cel_cpld_i2c *i2c = i2c_get_adapdata(adap);

	offset = i2c->sff_port_id - i2c->sff_port_start;
	
	if(i2c->sff_port_id <= SFP_COUNT){
		if(i2c->sff_port_id >= CPLD_5_SFP_START){
			ret = inb(i2c->m_base + CPLD_SFP_TXDIS_OFFSET_CPLD_5 + (offset / 8));
		}else
		{
			ret = inb(i2c->m_base + CPLD_SFP_TXDIS_OFFSET + (offset / 8));
		}
	}else{
		ret = 1;
	}


	txdis = ret & (0x1 << (offset % 8));
	

    return sprintf(buf, "%d\n", !!txdis);

}

static ssize_t set_txdis(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    unsigned char data;
	int txdis;
	int offset;
	int raw_offset;
	struct i2c_adapter *adap = container_of(dev, struct i2c_adapter, dev);
	struct cel_cpld_i2c *i2c = i2c_get_adapdata(adap);
	if(i2c->sff_port_id <= SFP_COUNT){
		offset = i2c->sff_port_id - i2c->sff_port_start;

		if(i2c->sff_port_id >= CPLD_5_SFP_START){
			raw_offset = CPLD_SFP_TXDIS_OFFSET;
		}else
		{
			raw_offset = CPLD_SFP_TXDIS_OFFSET_CPLD_5;
		}

		if (sscanf(buf, "%d", &txdis) <= 0) {
			return -EINVAL;
		}
		data = inb(i2c->m_base + raw_offset + (offset / 8));
		if(txdis) {
			data |= (0x1 << (offset % 8));
		} else {
			data &= ~(0x1 << (offset % 8));
		}
		outb(data, i2c->m_base + raw_offset + (offset / 8));
	}else{
		return 0;
	}
	
	
    return count;
}

static ssize_t get_rscon(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    int rscon = 0;
	int offset;
	struct i2c_adapter *adap = container_of(dev, struct i2c_adapter, dev);
	struct cel_cpld_i2c *i2c = i2c_get_adapdata(adap);

	offset = i2c->sff_port_id - i2c->sff_port_start;
	
	if(i2c->sff_port_id <= SFP_COUNT){
		if(i2c->sff_port_id >= CPLD_5_SFP_START){
			ret = inb(i2c->m_base + CPLD_SFP_RS_OFFSET_CPLD_5 + (offset / 8));
		}else
		{
			ret = inb(i2c->m_base + CPLD_SFP_RS_OFFSET + (offset / 8));
		}
	}else{
		ret = 1;
	}


	rscon = ret & (0x1 << (offset % 8));
	

    return sprintf(buf, "%d\n", !!rscon);

}

static ssize_t set_rscon(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    unsigned char data;
	int rscon;
	int offset;
	int raw_offset;
	struct i2c_adapter *adap = container_of(dev, struct i2c_adapter, dev);
	struct cel_cpld_i2c *i2c = i2c_get_adapdata(adap);
	if(i2c->sff_port_id <= SFP_COUNT){
		offset = i2c->sff_port_id - i2c->sff_port_start;

		if(i2c->sff_port_id >= CPLD_5_SFP_START){
			raw_offset = CPLD_SFP_RS_OFFSET;
		}else
		{
			raw_offset = CPLD_SFP_RS_OFFSET_CPLD_5;
		}

		if (sscanf(buf, "%d", &rscon) <= 0) {
			return -EINVAL;
		}
		data = inb(i2c->m_base + raw_offset + (offset / 8));
		if(rscon) {
			data |= (0x1 << (offset % 8));
		} else {
			data &= ~(0x1 << (offset % 8));
		}
		outb(data, i2c->m_base + raw_offset + (offset / 8));
	}else{
		return 0;
	}
	
	
    return count;
}

static ssize_t get_reset(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    int reset = 0;
	int offset;
	struct i2c_adapter *adap = container_of(dev, struct i2c_adapter, dev);
	struct cel_cpld_i2c *i2c = i2c_get_adapdata(adap);

	offset = i2c->sff_port_id - i2c->sff_port_start;

	if(i2c->sff_port_id <= SFP_COUNT){
		return 0; //SFP don't have reset
	}else{
		ret = inb(i2c->m_base + CPLD_QSFP_RESET_OFFSET + (offset / 8));
	}
	reset = !(ret & (0x1 << (offset % 8)));
	

    return sprintf(buf, "%d\n", reset);
}

static ssize_t set_reset(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    unsigned char data;
	int reset;
	int offset;
	struct i2c_adapter *adap = container_of(dev, struct i2c_adapter, dev);
	struct cel_cpld_i2c *i2c = i2c_get_adapdata(adap);


	if(i2c->sff_port_id <= SFP_COUNT){
		return 0; //SFP don't have reset
	}else{
		offset = i2c->sff_port_id - i2c->sff_port_start;
		if (sscanf(buf, "%d", &reset) <= 0) {
			return -EINVAL;
		}
		data = inb(i2c->m_base + CPLD_QSFP_RESET_OFFSET + (offset / 8));
		if(reset) {
			data &= ~(0x1 << (offset % 8));
		} else {
			data |= (0x1 << (offset % 8));
		}
		outb(data, i2c->m_base + CPLD_QSFP_RESET_OFFSET + (offset / 8));
	}
	
    return count;
}

static ssize_t get_present(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    int present = 0;
	int offset;
	struct i2c_adapter *adap = container_of(dev, struct i2c_adapter, dev);
	struct cel_cpld_i2c *i2c = i2c_get_adapdata(adap);

	offset = i2c->sff_port_id - i2c->sff_port_start;
	if(i2c->sff_port_id <= SFP_COUNT){
		if(i2c->sff_port_id >= CPLD_5_SFP_START){
			ret = inb(i2c->m_base + CPLD_SFP_ABS_OFFSET_CPLD_5 + (offset / 8));
		}else
		{
			ret = inb(i2c->m_base + CPLD_SFP_ABS_OFFSET + (offset / 8));
		}
	}else{
		ret = inb(i2c->m_base + CPLD_QSFP_ABS_OFFSET + (offset / 8));
	}
	
	present = !(ret & (0x1 << (offset % 8)));
	CEL_DEBUG("port_id=%d, offset=%d, present=0x%02x\n", i2c->sff_port_id, offset, present);

    return sprintf(buf, "%d\n", present);
}

static ssize_t get_rxlos(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    int rxlos = 0;
	int offset;
	struct i2c_adapter *adap = container_of(dev, struct i2c_adapter, dev);
	struct cel_cpld_i2c *i2c = i2c_get_adapdata(adap);

	offset = i2c->sff_port_id - i2c->sff_port_start;
	if(i2c->sff_port_id <= SFP_COUNT){
		if(i2c->sff_port_id >= CPLD_5_SFP_START){
			ret = inb(i2c->m_base + CPLD_SFP_RXLOS_STAT_OFFSET_CPLD_5 + (offset / 8));
		}else
		{
			ret = inb(i2c->m_base + CPLD_SFP_RXLOS_STAT_OFFSET + (offset / 8));
		}
	}else{
		ret = 1;
	}
	
	rxlos = !(ret & (0x1 << (offset % 8)));
	CEL_DEBUG("port_id=%d, offset=%d, rxlos=0x%02x\n", i2c->sff_port_id, offset, rxlos);

    return sprintf(buf, "%d\n", rxlos);
}

static ssize_t get_txfault(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    int txfault = 0;
	int offset;
	struct i2c_adapter *adap = container_of(dev, struct i2c_adapter, dev);
	struct cel_cpld_i2c *i2c = i2c_get_adapdata(adap);

	offset = i2c->sff_port_id - i2c->sff_port_start;
	if(i2c->sff_port_id <= SFP_COUNT){
		if(i2c->sff_port_id >= CPLD_5_SFP_START){
			ret = inb(i2c->m_base + CPLD_SFP_TXFAULT_OFFSET_CPLD_5 + (offset / 8));
		}else
		{
			ret = inb(i2c->m_base + CPLD_SFP_TXFAULT_OFFSET + (offset / 8));
		}
	}else{
		ret = 1;
	}
	
	txfault = !(ret & (0x1 << (offset % 8)));
	CEL_DEBUG("port_id=%d, offset=%d, txfault=0x%02x\n", i2c->sff_port_id, offset, txfault);

    return sprintf(buf, "%d\n", txfault);
}

static ssize_t cpld_rw_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, readme);
}

static ssize_t cpld_read(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long addr;
	int err, len;

	len = strlen(buf);


	err = strict_strtoul(buf, 16, &addr);
	if (err) {
		printk("Invalid value\n");
		return -EINVAL;
	}

	printk(KERN_ALERT "0x%02x\n", inb(addr));

	return count;
}

static ssize_t cpld_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long addr, value;
	int err, len;
	const char *s = NULL;
	char addrstr[16];

	s = strchr(buf, ':');
	if( s == NULL) {
		printk("Wrong format\n");
		return -EINVAL;
	}

	len = s - buf;
	CEL_DEBUG("buf=%s, len=%d\n", buf, len);

	strncpy(addrstr, buf, len);
	addrstr[len] = '\0';
	CEL_DEBUG("addrstr=%s\n", addrstr);

	err = strict_strtoul(addrstr, 16, &addr);
	if(err) {
		printk("Invalid address\n");
		return -EINVAL;
	}

	err = strict_strtoul(s+1, 16, &value);
	CEL_DEBUG("value=0x%02x\n", value);
	if(err) {
		printk("Invalid value\n");
		return -EINVAL;
	}

	outb(value, addr);

	return count;
}

#ifdef CONFIG_CPLD_1
static ssize_t cpld1_ver_show(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{

	return sprintf(buf, "0x%02x\n", inb(CPLD_IO_OFF_1));;
}
#endif

#ifdef CONFIG_CPLD_2
static ssize_t cpld2_ver_show(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{

	return sprintf(buf, "0x%02x\n", inb(CPLD_IO_OFF_2));;
}
#endif

#ifdef CONFIG_CPLD_3
static ssize_t cpld3_ver_show(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{

	return sprintf(buf, "0x%02x\n", inb(CPLD_IO_OFF_3));;
}
#endif

#ifdef CONFIG_CPLD_4
static ssize_t cpld4_ver_show(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{

	return sprintf(buf, "0x%02x\n", inb(CPLD_IO_OFF_4));;
}
#endif

#ifdef CONFIG_CPLD_5
static ssize_t cpld5_ver_show(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{

	return sprintf(buf, "0x%02x\n", inb(CPLD_IO_OFF_5));;
}
#endif

static DEVICE_ATTR(present, S_IRUGO, get_present, NULL);
static DEVICE_ATTR(rxlos, S_IRUGO, get_rxlos, NULL);
static DEVICE_ATTR(txdis, S_IRUGO | S_IWUSR, get_txdis, set_txdis);
static DEVICE_ATTR(rscon, S_IRUGO | S_IWUSR, get_rscon, set_rscon);
static DEVICE_ATTR(txfault, S_IRUGO, get_txfault, NULL);
static DEVICE_ATTR(lpmode, S_IRUGO | S_IWUSR, get_lpmode, set_lpmode);
static DEVICE_ATTR(reset,  S_IRUGO | S_IWUSR, get_reset, set_reset);
static DEVICE_ATTR(read,  S_IRUGO | S_IWUSR, cpld_rw_show, cpld_read);
static DEVICE_ATTR(write,  S_IRUGO | S_IWUSR, cpld_rw_show, cpld_write);
#ifdef CONFIG_CPLD_1
static DEVICE_ATTR(cpld1_ver,  S_IRUGO | S_IWUSR, cpld1_ver_show, NULL);
#endif
#ifdef CONFIG_CPLD_2
static DEVICE_ATTR(cpld2_ver,  S_IRUGO | S_IWUSR, cpld2_ver_show, NULL);
#endif
#ifdef CONFIG_CPLD_3
static DEVICE_ATTR(cpld3_ver,  S_IRUGO | S_IWUSR, cpld3_ver_show, NULL);
#endif
#ifdef CONFIG_CPLD_4
static DEVICE_ATTR(cpld4_ver,  S_IRUGO | S_IWUSR, cpld4_ver_show, NULL);
#endif
#ifdef CONFIG_CPLD_5
static DEVICE_ATTR(cpld5_ver,  S_IRUGO | S_IWUSR, cpld5_ver_show, NULL);
#endif


static struct attribute *cel_sff_attrs[] = {
	&dev_attr_lpmode.attr,
    &dev_attr_reset.attr,
    &dev_attr_present.attr,
	&dev_attr_rxlos.attr,
    &dev_attr_txdis.attr,
    &dev_attr_rscon.attr,
	&dev_attr_txfault.attr,
    NULL,
};

static struct attribute_group cel_sff_attr_grp = {
    .attrs = cel_sff_attrs,
};

static struct attribute *cel_cpld_attrs[] = {
#ifdef CONFIG_CPLD_1
	&dev_attr_cpld1_ver.attr,
#endif
#ifdef CONFIG_CPLD_2
	&dev_attr_cpld2_ver.attr,
#endif
#ifdef CONFIG_CPLD_3
	&dev_attr_cpld3_ver.attr,
#endif
#ifdef CONFIG_CPLD_4
	&dev_attr_cpld4_ver.attr,
#endif
#ifdef CONFIG_CPLD_5
	&dev_attr_cpld5_ver.attr,
#endif
    &dev_attr_read.attr,
	&dev_attr_write.attr,
    NULL,
};

static struct attribute_group cel_cpld_attr_grp = {
    .attrs = cel_cpld_attrs,
};


//static int __init i2c_lpc_init (struct device *dev)
static int i2c_lpc_init (void)
{
	int i, k, m;
	int ret;
	char port_name[CEL_NAME_SIZE] = "\0";

	CEL_DEBUG("start\n");
	memset (&i2c_lpc_adap,      0, sizeof(i2c_lpc_adap));
	memset (&cel_i2c, 0, sizeof(cel_i2c));

	for(i=0; i < CPLD_I2C_BUS_LOCK_MAX; i++)
		mutex_init(&i2c_xfer_lock[i]);

	/* Initialize driver's itnernal data structures */
	i2c_init_internal_data();

	for (i = 0 ; i < I2C_LPC_MAX_BUS ; i++) {
		cel_i2c[i].m_dev = kzalloc(sizeof(struct device), GFP_KERNEL);
		if(!cel_i2c[i].m_dev) {
			k = i;
			goto err1;
		}
		i2c_lpc_adap[i].owner = THIS_MODULE;
		i2c_lpc_adap[i].class = I2C_CLASS_HWMON | I2C_CLASS_SPD;

		//i2c_lpc_adap[i].algo      = &cel_algo;
		i2c_lpc_adap[i].algo_data = &cel_i2c[i];
		i2c_lpc_adap[i].nr = i+11;/* /dev/i2c-11 ~ /dev/i2c-58  sfp port1 ~ 48*/
		sprintf( i2c_lpc_adap[ i ].name, "i2c-lpc-%d", i);
		i2c_set_adapdata(&i2c_lpc_adap[i], &cel_i2c[i]);

		/* Add the bus via the algorithm code */
		if( i2c_lpc_add_bus( &i2c_lpc_adap[ i ] ) != 0)
		{
			printk("Cannot add bus %d to algorithm layer\n", i);
			k = i;
			goto err2;
		}
#ifdef kernel_debug
		printk( "Registered bus id: %s\n", kobject_name(&i2c_lpc_adap[ i ].dev.kobj));
#endif
		ret = sysfs_create_group(&i2c_lpc_adap[ i ].dev.kobj, &cel_sff_attr_grp);
		if (ret) {
			k = i;
			goto err3;
		}
	}

	sff_obj = cel_sysfs_kobject_create(PORT_DIR_NAME, NULL);
	if(!sff_obj) {
		printk("create kobj %s error", PORT_DIR_NAME);
		k = i - 1;
		goto err3;
	}
	for(i = 0; i < I2C_LPC_MAX_BUS; i++) {
		memset(port_name, 0, CEL_NAME_SIZE);
		sprintf(port_name, "port%d", i + 1);
		ret = sysfs_create_link(&sff_obj->kobj, &i2c_lpc_adap[i].dev.kobj, port_name);
		if (ret) {
			printk("Cannot create the link: %s, err=%d\n", port_name, ret);
			k = I2C_LPC_MAX_BUS - 1;
			m = i;
			goto err4;
		}
	}

	cpld_obj = cel_sysfs_kobject_create(CPLD_NAME, NULL);
	if(!cpld_obj) {
		printk("create cpld_obj %s error", CPLD_NAME);
		k = I2C_LPC_MAX_BUS - 1;
		m = I2C_LPC_MAX_BUS;
		goto err4;
	}
	if (sysfs_create_group(cpld_obj, &cel_cpld_attr_grp) != 0) {
        printk("sysfs_create_group board error. \n");
        goto err5;
    }

	CEL_DEBUG("OK\n");

	return 0;

err5:
	kobject_put(cpld_obj);
err4:
	for(i = 0; i < m; i++){
		memset(port_name, 0, CEL_NAME_SIZE);
		sprintf(port_name, "port%d", i + 1);
		sysfs_remove_link(&sff_obj->kobj, port_name);
	}
	kobject_put(sff_obj);
err3:
	i2c_del_adapter(&i2c_lpc_adap[k]);
err2:
	kfree(cel_i2c[k].m_dev);
	for(i = 0; i < k; i++){
		i2c_del_adapter(&i2c_lpc_adap[i]);
	}
err1:
	for(i = 0; i < k; i++){
		kfree(cel_i2c[i].m_dev);
	}
	return -ENODEV;
}

/*
 * Module init/exit methods begin here.  The init and exit methods
 * register/de-register two devices:
 *
 * - The i2c bus adapter device
 * - The i2c mux device
 *
 */
static int __init cel_i2c_init(void)
{
	/* First register the i2c adapter */
	i2c_lpc_init();

	return 0;
}


static void __exit cel_i2c_exit(void)
{
	int i;
	char port_name[CEL_NAME_SIZE] = "\0";

	for(i = 0; i < I2C_LPC_MAX_BUS; i++){
		memset(port_name, 0, CEL_NAME_SIZE);
		sprintf(port_name, "port%d", i + 1);
		sysfs_remove_link(&sff_obj->kobj, port_name);
	}
	kobject_put(cpld_obj);
	kobject_put(sff_obj);

	for( i = 0; i < I2C_LPC_MAX_BUS; i++ ){
		i2c_del_adapter(&i2c_lpc_adap[i]);
		kfree(cel_i2c[i].m_dev);
	}

}


module_init(cel_i2c_init);
module_exit(cel_i2c_exit);

MODULE_AUTHOR("Peerapong Jaipakdee <pjaipakdee@celestica.com>");
MODULE_AUTHOR("Larry Ming <laming@celestica.com>");
MODULE_DESCRIPTION("Celestica CPLD Virtual I2C Bus Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRIVER_NAME);
