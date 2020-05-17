# virt_uart_with_tty_if

#### 介绍
使用tty提供的接口，完成虚拟uart的创建

#### 软件架构
软件架构说明
	本次实现的虚拟串口主要是借助tty_register_driver、tty_port_register_device实现，而不是借助
        uart_register_driver、uart_add_one_port，借助uart_register_driver、uart_add_one_port
        实现一次虚拟串口驱动请移步https://gitee.com/jerry_chg/virt_uart_with_tty_if.git


本次虚拟串口实现的功能
	1. 虚拟串口的名称为vttyM（如串口0，则为/dev/vttyM0）;
	2. 应用程序可通过打开设备文件/dev/vttyM0，实现对串口vttyM0的读写操作；
	3. /dev/vttyM0支持loopback模式，即应用程序通过/dev/vttyM0打开串口后，向串口写入数据后，vttyM0则会将写入的数据作为接收
            数据，再刷新到串口中;
	4. 可通过sysfs，通过设置/sys/class/tty/vttyM0/vtty_loopback属性，进行loopback模式的开启与关闭（默认开启loopback模式）；
	5. 可通过sysfs文件系统，通过向/sys/class/tty/vttyM0/vtty_receive_buff中写入数据，来模拟vttM0串口的数据接收，
           如echo "vtty test" >vtty_receive_buff，则表示串口接收到数据“vtty test”，此时若读取/dev/vttyM0，则可以获取到数据
           “vtty test”
	
本次虚拟串口实现涉及的知识点
	1. 借助platform device接口，实现虚拟串口端口对应platfomr的创建（如实现vttyM0、vttyM1两个串口，则创建对应的两
            个platform device）；
	2. 借助platform driver接口，实现platform device对应的driver，在platform driver的probe接口中，调用
           tty_port_register_device完成tty端口及其对应字符设备的注册
	3. 借助sysfs_create_group，创建tty端口对应的sysfs属性文件，实现loopback的控制以及模拟串口接收数据的模拟；
	4. 借助tty_register_driver，完成虚拟tty 控制器驱动的注册，并实现对应的接口


#### 安装教程

1.  make 
2.  make install
3.  完成以上命令后，则在./images下即可看到编译成功的驱动

#### 使用说明

1.  insmod virtual_tty_platform_dev.ko
2.  insmod virtual_tty_controller.ko
完成上面两步后，即生成/dev/vttyM0的设备，然后就可以像打开平常串口一样，打开该串口。目前仅支持回环模式。
