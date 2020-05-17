#include "virtual_tty_platform_dev.h"


static void virtual_tty_port_dev_release(struct device *dev)
{

}
static struct virtual_tty_port_platform_config virtual_tty_port0_config =
{

    .port_index = 0,
};
static struct virtual_tty_port_platform_config virtual_tty_port1_config =
{

    .port_index = 1,
};
static struct platform_device virtual_tty_port_platform_device = {
    .name = "virtual_tty_port_dev",
    .id = 0,
    .dev =
    {
        .platform_data = &virtual_tty_port0_config,
        .release = virtual_tty_port_dev_release,
    }
};

static struct platform_device virtual_tty_port2_platform_device = {
    .name = "virtual_tty_port_dev",
    .id = 1,
    .dev =
    {
        .platform_data = &virtual_tty_port1_config,
        .release = virtual_tty_port_dev_release,
    }
};
static int __init virtual_tty_dev_init(void)
{
    platform_device_register(&virtual_tty_port_platform_device);
    platform_device_register(&virtual_tty_port2_platform_device);

    return 0;
}

static void __exit virtual_tty_dev_exit(void)
{
    platform_device_unregister(&virtual_tty_port_platform_device);
    platform_device_unregister(&virtual_tty_port2_platform_device);
}


module_init(virtual_tty_dev_init);
module_exit(virtual_tty_dev_exit);

MODULE_DESCRIPTION("Virtual tty Devs");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("jerry_chg");
