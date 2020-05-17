#ifndef VIRTUAL_TTY_CONTROLLER_H_
#define VIRTUAL_TTY_CONTROLLER_H_

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
#include <linux/serial_reg.h>

#define MAX_VIRTUAL_TTYS 6
#define VIRTUAL_XMIT_SIZE   512
#define WAKEUP_CHARS    16

#define VIRTUAL_TTY_MIN_SPEED 57600
#define VIRTUAL_TTY_MAX_SPEED 1152000

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
    struct device *tty_dev;
};


#endif
