#include "virtual_tty_controller.h"

//#define VIRTUAL_TTY_DEBUG
#ifdef VIRTUAL_TTY_DEBUG
#define virtual_tty_debug(fmt, argv...) \
    do \
    {\
        printk(fmt,##argv); \
    }while(0)
#else
#define virtual_tty_debug(fmt, argv...)
#endif


static struct tty_driver *virtual_tty_driver;
static struct virtual_tty_port *virtual_tty_port_table[MAX_VIRTUAL_TTYS];
static DEFINE_SPINLOCK(virtual_tty_port_table_lock);

static struct virtual_tty_port *virtual_tty_port_get(unsigned index)
{
    struct virtual_tty_port *port = NULL;

    if (index >= MAX_VIRTUAL_TTYS)
        return NULL;

    spin_lock(&virtual_tty_port_table_lock);
    port = virtual_tty_port_table[index];
    if (port)
        tty_port_get(&port->port);
    spin_unlock(&virtual_tty_port_table_lock);

    return port;
}

static void virtual_tty_port_put(struct virtual_tty_port *port)
{
    tty_port_put(&port->port);
}


static int virtual_tty_install(struct tty_driver *driver, struct tty_struct *tty)
{
    int idx = tty->index;
    struct virtual_tty_port *port = virtual_tty_port_get(idx);
    int ret = 0;
    printk("%s:%d\n", __FUNCTION__, __LINE__);


    if(port == NULL)
    {
        printk("%s:%d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    
    printk("%s:%d\n", __FUNCTION__, __LINE__);
    ret = tty_standard_install(driver, tty);
    printk("%s:%d\n", __FUNCTION__, __LINE__);

    if (ret == 0)
    {
        tty->driver_data = port;
    }
    else
    {
        printk("%s:%d\n", __FUNCTION__, __LINE__);
        virtual_tty_port_put(port);
    }
    
    printk("%s:%d\n", __FUNCTION__, __LINE__);
    return ret;
}

static void virtual_tty_remove(struct tty_driver *driver, struct tty_struct *tty)
{
    printk("%s:%d\n", __FUNCTION__, __LINE__);
    /*此处即将driver->ttys，其实可以不实现该remove函数，因tty_driver_remove_tty默认也是将driver->ttys[tty->index]置为null，
    但是当我们自己定义了该变量之后tty_driver_remove_tty则只调用我们注册的remove接口，而不会执行将driver->ttys[tty->index]置为null*/
    driver->ttys[tty->index] = NULL;
}

static void virtual_tty_port_remove(struct virtual_tty_port * port)
{
    struct virtual_tty_port *virtual_port = NULL;

    spin_lock(&virtual_tty_port_table_lock);
    virtual_port = virtual_tty_port_table[port->index];
    if (virtual_port)
    {   
        virtual_tty_port_table[port->index] = NULL;
    }
    else
    {
        printk("%s:%d\n", __FUNCTION__, __LINE__);
    }
    spin_unlock(&virtual_tty_port_table_lock);
    
    virtual_tty_port_put(virtual_port);
}

static void virtual_tty_cleanup(struct tty_struct *tty)
{
    struct virtual_tty_port *virtual_port = NULL;
    int idx = tty->index;

    printk("%s:%d\n", __FUNCTION__, __LINE__);
    if(idx >= MAX_VIRTUAL_TTYS)
        return;

    virtual_port = tty->driver_data;
    
    tty->driver_data = NULL;
    virtual_tty_port_put(virtual_port);
}

static int virtual_tty_port_add(struct virtual_tty_port * port)
{
    int ret = 0;

    if(port->index >= MAX_VIRTUAL_TTYS)
        return -1;

    spin_lock(&virtual_tty_port_table_lock);

    if (virtual_tty_port_table[port->index] == NULL)
    {   
        virtual_tty_port_table[port->index] = port;
        ret = 0;
    }
    else
    {
        printk("%s:%d\n", __FUNCTION__, __LINE__);
        ret = -1;
    }
    spin_unlock(&virtual_tty_port_table_lock);

    return ret;
}


static int virtual_tty_controller_open(struct tty_struct * tty, struct file * filp)
{
    struct virtual_tty_port *virtual_tty_port = tty->driver_data;


    return tty_port_open(&virtual_tty_port->port, tty, filp);
}

static void virtual_tty_controller_close(struct tty_struct * tty, struct file * filp)
{
    struct virtual_tty_port *virtual_tty_port = tty->driver_data;

    flush_work(&virtual_tty_port->work);
    tty_port_close(&virtual_tty_port->port, tty, filp);

    printk("%s:%d\n", __FUNCTION__, __LINE__);
}
static void  virtual_tty_controller_hangup(struct tty_struct *tty)
{
    struct virtual_tty_port *virtual_tty_port = tty->driver_data;
    tty_port_hangup(&virtual_tty_port->port);
}


static int virtual_tty_controller_write(struct tty_struct * tty,const unsigned char *buf, int count)
{
    struct virtual_tty_port *tty_port = tty->driver_data;
    int ret = 0;

    ret = kfifo_in_locked(&tty_port->xmit_fifo, buf, count, &tty_port->write_lock);

    schedule_work(&tty_port->work);
    return ret;
}

static void virtual_tty_flush_to_port(struct work_struct *workp)
{
    unsigned char buff[VIRTUAL_XMIT_SIZE];
    struct virtual_tty_port *tty_port = container_of(workp, struct virtual_tty_port, work);
    int len = 0;
    int count = 0;
    struct tty_struct *tty;

    virtual_tty_debug("%s:%d\n", __FUNCTION__, __LINE__);
    
    tty = tty_port_tty_get(&tty_port->port);
    len = kfifo_out_locked(&tty_port->xmit_fifo, buff, sizeof(buff), &tty_port->write_lock);

    for(count = 0; count < len; count++)
    {
        if(tty_port->loopback_flag)
        {
            
            tty_insert_flip_char(&tty_port->port, buff[count], TTY_NORMAL);
        }
        virtual_tty_debug("0x%02hhx ", buff[count]);
    }
    virtual_tty_debug("\n");
    
    if(tty_port->loopback_flag)
    {
        virtual_tty_debug("%s:%d push receive data to ldisc...\n", __FUNCTION__, __LINE__);
        tty_flip_buffer_push(&tty_port->port);
    }

    /*当前fifo中已存储数据大小小于WAKEUP_CHARS，可继续接收数据，因此调用tty_wakeup，唤醒该tty_struct->write_wait队列头上等待的队列
    ，用于唤醒sleep的写操作，进行数据的写入操作*/
    len = kfifo_len(&tty_port->xmit_fifo);
    if (len < WAKEUP_CHARS)
    {
        tty_wakeup(tty);
    }
    tty_kref_put(tty);
}



static int virtual_tty_write_room(struct tty_struct *tty)
{
    struct virtual_tty_port *tty_port = tty->driver_data;

    return VIRTUAL_XMIT_SIZE - kfifo_len(&tty_port->xmit_fifo);
}

/*该接口用于设置串口的termios信息，包括字节宽度、波特率等，此处仅仅作为显示而已，针对具体设备的设置，则解析
相应的参数，并设置到具体的寄存器中即可*/
static void virtual_tty_set_termios(struct tty_struct *tty, struct ktermios *old)
{
    struct ktermios *termios = &tty->termios;
    unsigned char cval = 0;
    unsigned int baud = 0;

    switch (termios->c_cflag & CSIZE)
    {
        case CS5:
            cval = UART_LCR_WLEN5;
            break;
        case CS6:
            cval = UART_LCR_WLEN6;
            break;
        case CS7:
            cval = UART_LCR_WLEN7;
            break;
        default:
        case CS8:
            cval = UART_LCR_WLEN8;
            break;
    }

    if (termios->c_cflag & CSTOPB)
        cval |= UART_LCR_STOP;
    if (termios->c_cflag & PARENB)
        cval |= UART_LCR_PARITY;
    if (!(termios->c_cflag & PARODD))
        cval |= UART_LCR_EPAR;




    baud = tty_termios_baud_rate(termios);
    if(baud < VIRTUAL_TTY_MIN_SPEED || baud > VIRTUAL_TTY_MAX_SPEED)
    {
        termios->c_cflag &= ~CBAUD;
        if (old)
        {
            baud = tty_termios_baud_rate(old);
        }
    }

    printk("baud = %d cval = %d\n", baud, cval);
}

/*这两个接口是modem 
控制信息的设置与获取，因本模块仅仅模拟串口驱动，此处就没有做进一步实现，若是
具体的串口设备，则可以根据芯片的datasheet进行实现即可*/
static int virtual_tty_tiocmget(struct tty_struct *tty)
{

    printk("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}

static int virtual_tty_tiocmset(struct tty_struct *tty,unsigned int set, unsigned int clear)
{

    printk("%s:%d\n", __FUNCTION__, __LINE__);

    return 0;
}


static int virtual_tty_port_activate(struct tty_port *tport, struct tty_struct *tty)
{
    struct virtual_tty_port *tty_port = tty->driver_data;
    
    tty_port->loopback_flag = true;

    printk("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}
static void virtual_tty_port_shutdown(struct tty_port *port)
{
    struct virtual_tty_port *tty_port = container_of(port, struct virtual_tty_port, port);
    
    tty_port->loopback_flag = false;
    
    printk("%s:%d\n", __FUNCTION__, __LINE__);
}

static void virtual_tty_port_destruct(struct tty_port *port)
{
    struct virtual_tty_port *tty_port = container_of(port, struct virtual_tty_port, port);

    if(tty_port != NULL)
    {
        flush_work(&tty_port->work);
        kfifo_free(&tty_port->xmit_fifo);
        kfree(tty_port);
    }

    printk("%s:%d\n", __FUNCTION__, __LINE__);
}


static const struct tty_port_operations virtual_tty_port_ops =
{
    .activate = virtual_tty_port_activate,
    .shutdown = virtual_tty_port_shutdown,
    .destruct = virtual_tty_port_destruct,
};



/*

S_IRUSR | S_IWUSR

*/
static ssize_t vtty_receive_buff_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    struct virtual_tty_port *tty_port = dev_get_drvdata(dev);
    struct tty_struct *tty;
    int i = 0;

    if(count <= 0 || tty_port == NULL)
        return -EINVAL;

    tty = tty_port_tty_get(&tty_port->port);

    for(i = 0; i < count; i++)
    {
        tty_insert_flip_char(&tty_port->port, buf[i], TTY_NORMAL);
    }
    
    virtual_tty_debug("%s:%d push receive data to ldisc...\n", __FUNCTION__, __LINE__);
    tty_flip_buffer_push(&tty_port->port);

    tty_kref_put(tty);

    return count;
}
static ssize_t vtty_loopback_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct virtual_tty_port *tty_port = dev_get_drvdata(dev);

    if(tty_port == NULL)
    {
        printk("tty_Port=NULL\n");
        return -1;
    }
    
    return  sprintf(buf, "%d\n", tty_port->loopback_flag);
}


static ssize_t vtty_loopback_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    struct virtual_tty_port *tty_port = dev_get_drvdata(dev);
    u32 loopback_flag = simple_strtoul(buf, NULL, 10);

    if(tty_port == NULL)
    {
        printk("tty_Port=NULL\n");
        return -1;
    }

    tty_port->loopback_flag = loopback_flag > 0?1:0;

    return count;
}

static DEVICE_ATTR(vtty_receive_buff, S_IWUSR, NULL, vtty_receive_buff_store);
static DEVICE_ATTR(vtty_loopback, S_IWUSR|S_IRUSR, vtty_loopback_show, vtty_loopback_store);


static struct attribute *virtual_tty_attrs[] =
{
    &dev_attr_vtty_receive_buff.attr,
    &dev_attr_vtty_loopback.attr,
    NULL
};
static const struct attribute_group virtual_tty_attr_group = 
{
    .attrs = virtual_tty_attrs,
};


static int virtual_tty_port_platform_probe(struct platform_device *platform_dev)
{
    struct virtual_tty_port *port;
    struct virtual_tty_port_platform_config *port_config_datap = NULL;
    int ret;

    port_config_datap = (struct virtual_tty_port_platform_config *)platform_dev->dev.platform_data;
    port = kzalloc(sizeof(struct virtual_tty_port), GFP_KERNEL);
    if (!port)
        return -ENOMEM;

    
    printk("virtual tty port=%d register\n", port_config_datap->port_index);
    tty_port_init(&port->port);
    port->port.ops = &virtual_tty_port_ops;
    port->index = port_config_datap->port_index;
    
    if (kfifo_alloc(&port->xmit_fifo, VIRTUAL_XMIT_SIZE, GFP_KERNEL))
    {
        kfree(port);
        return -ENOMEM;
    }
    
    spin_lock_init(&port->write_lock);
    INIT_WORK(&port->work, virtual_tty_flush_to_port);
    
    ret = virtual_tty_port_add(port);
    if (ret)
    {
        kfifo_free(&port->xmit_fifo);
        kfree(port);
    }
    else
    {
        port->tty_dev = tty_port_register_device(&port->port, virtual_tty_driver, port->index, NULL);
        if (IS_ERR(port->tty_dev)) 
        {
            printk("%s:%d err....\n", __FUNCTION__, __LINE__);
            virtual_tty_port_remove(port);
            ret = PTR_ERR(port->tty_dev);

        }
        else
        {
            sysfs_create_group(&(port->tty_dev->kobj), &virtual_tty_attr_group);
            dev_set_drvdata(port->tty_dev, port);
        }

    }

    if(ret == 0)
    {
        platform_set_drvdata(platform_dev, port);
    }
    
    printk("%s:%d\n", __FUNCTION__, __LINE__);
    return ret;
} 

static int virtual_tty_port_platform_remove(struct platform_device *platform_dev)
{
    struct virtual_tty_port *port = platform_get_drvdata(platform_dev);

    platform_set_drvdata(platform_dev, NULL);
    
    dev_set_drvdata(port->tty_dev, NULL);
    tty_unregister_device(virtual_tty_driver, port->index);
    virtual_tty_port_remove(port);

    printk("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}


static struct platform_driver virtual_tty_port_platform_driver = {
    .driver = {
        .name = "virtual_tty_port_dev",
        .owner = THIS_MODULE,
    },
    .probe = virtual_tty_port_platform_probe,
    .remove = virtual_tty_port_platform_remove,
};

static const struct tty_operations virtual_tty_ops = 
{
    .open = virtual_tty_controller_open,
    .close = virtual_tty_controller_close,
    .write = virtual_tty_controller_write,
    .hangup = virtual_tty_controller_hangup,
    .write_room = virtual_tty_write_room,
    .install = virtual_tty_install,
    .cleanup = virtual_tty_cleanup,
    .remove = virtual_tty_remove,
    .set_termios = virtual_tty_set_termios,
    .tiocmget = virtual_tty_tiocmget,
    .tiocmset = virtual_tty_tiocmset
};

static int __init virtual_tty_init(void)
{
    int ret = 0;

    virtual_tty_driver = alloc_tty_driver(MAX_VIRTUAL_TTYS);
    if (virtual_tty_driver == 0)
        return -ENOMEM;

    virtual_tty_driver->driver_name = "virtual_tty_driver";
    virtual_tty_driver->name = "vttyM";
    virtual_tty_driver->major = 0;
    virtual_tty_driver->minor_start = 0;
    virtual_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
    virtual_tty_driver->subtype = SERIAL_TYPE_NORMAL;
    virtual_tty_driver->init_termios = tty_std_termios;
    virtual_tty_driver->init_termios.c_iflag = 0;
    virtual_tty_driver->init_termios.c_oflag = 0;
    virtual_tty_driver->init_termios.c_cflag = B38400 | CS8 | CREAD;
    virtual_tty_driver->init_termios.c_lflag = 0;
    virtual_tty_driver->flags = TTY_DRIVER_RESET_TERMIOS |
        TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
    tty_set_operations(virtual_tty_driver, &virtual_tty_ops);

    ret = tty_register_driver(virtual_tty_driver);

    if(ret)
        return ret;

    ret = platform_driver_register(&virtual_tty_port_platform_driver);

    return ret;
}

static void __exit virtual_tty_exit(void)
{

    platform_driver_unregister(&virtual_tty_port_platform_driver);
    tty_unregister_driver(virtual_tty_driver);
    put_tty_driver(virtual_tty_driver);
}


module_init(virtual_tty_init);
module_exit(virtual_tty_exit);

MODULE_DESCRIPTION("Virtual tty Drivers");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("jerry_chg");
