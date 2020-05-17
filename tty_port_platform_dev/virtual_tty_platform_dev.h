#ifndef VIRTUAL_TTY_PLATFORM_DEV_H_
#define VIRTUAL_TTY_PLATFORM_DEV_H_ 
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/mdio.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/kdev_t.h>
#include <linux/interrupt.h>
#include <linux/syscalls.h>
#include <linux/mount.h>
#include <linux/device.h>
#include <linux/genhd.h>
#include <linux/namei.h>
#include <linux/shmem_fs.h>
#include <linux/ramfs.h>
#include <linux/sched.h>
#include <linux/vfs.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/dcache.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/kfifo.h>

#define MAX_VIRTUAL_TTYS 6
#define VIRTUAL_XMIT_SIZE   512
struct virtual_tty_port_platform_config
{
    int port_index;
};
struct virtual_tty_port {
    struct tty_port port;
    unsigned int index;
    bool loopback_flag;
    struct work_struct work;
    struct kfifo xmit_fifo;
    spinlock_t write_lock;
};

#if 0
#define VIRTUAL_I2C_DEV_REGS_NUM    16

typedef struct virtual_i2c_dev_info_s
{
    struct list_head node;
    int addr;
    u16 regs[VIRTUAL_I2C_DEV_REGS_NUM];
}virtual_i2c_dev_info_t;

typedef struct virtual_i2c_adapter_info_s
{
    struct list_head list;
    struct mutex virtual_i2c_mutex;
}virtual_i2c_adapter_info_t;


typedef struct virtual_i2c_bus_s
{
    virtual_i2c_adapter_info_t virtual_dev_info;
    struct i2c_adapter adapter;
}virtual_i2c_bus_t;

#endif
#endif
