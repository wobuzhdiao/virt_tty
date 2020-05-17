1.在/sys/class/tty/vttyMX目录下，存在两个属性文件
a.vtty_loopback用于表示是否进行loopback（即要写入vtty中的数据，再返回给应用层）
	关闭loopback
	echo 0 >vtty_loopback
	开启loopback
	echo 1 > vtty_loopback

b.当需要表示vtty接收的数据时，可以通过向vtty_receive_buff中写入数据来模拟vtty串口接收数据的流程，
	如下所示，则表示vtty串口接收到数据“vtty test”
	echo "vtty test" >vtty_receive_buff
