工作环境：
	此次CFM module是工作在openSUSE环境下
	内核version:2.6.22.5-31
	内核完整路径：/usr/src/linux-2.6.22.5-31
			需在该路径下有内核源码
	gcc version:4.2.1
	GNU Make version:3.81
使用步骤：
	1.编译内核源码
		进入/usr/src/linux-2.6.22.5-31
			执行：make menuconfig
			执行：make
		注：不必全部编译
	2.编译CFM源码(cfm-release-v1.1.tar.gz)
		把CFM源码copy到/usr/src目录下
		进入/usr/src
			执行：tar zxvf cfm-release-v1.1.tar.gz
		进入/usr/src/cfm-release-v1.1
			执行：make
	3.创建设备
			执行：/bin/mknod /dev/cfmemulator   c 10  130
	4.加载模块到内核中
		进入/usr/src/cfm-release-v1.1
			执行：insmod cfmemulator.ko
	5.通过用户态程序测试
		进入/usr/src/cfm-release-v1.1/app
			通过修改example.c中的内容，可以向内核传递不用的ioctl cmd
			执行：make
			执行：./example
	6.卸载CFM模块
		首先关闭用户态程序
		然后
			执行：rmmod cfmemulator
补充：
	1.以上所执行命令都需要root用户权限
	2.用户态采用双网卡来模拟两个Port的情况。
	  在example.c中有TWONIC选项，1表示使用双网卡，0表示1个网卡，根据实际情况进行设置
	3.FlowID和VlanID的对应关系为：FlowID=VlanID/10
	4.因recvfrom会把自己发送的包抓取回来，而且对于其中源地址不是自己的数据包无法丢弃,
	  为此加入了MD5机制，对发送的数据包计算MD5值并存储该MD5值（采用循环队列），
	  对接收到的数据包同样计算MD5值并与发送时存储的MD5队列进行比较，若MD5值已存在，证明是自己发出的数据包，进而丢弃。
	5.对management API的测试代码，在example.c中都有，在TestCase文档中同样也有，
	  具体测试时，要根据需要选取。
虚拟机双网卡设置：
	初始情况下，一般为单网卡。按照下列步骤设置为双网卡：
	1.关闭虚拟机，在Edit菜单下，选择Virtual Network Editor，选择Host Virtual Network Mapping
	  中设置VMnet与Windows网卡的映射关系。将VMnet0设置为“Bridged to 具体的network adapter”，
	  将VMnet9 设置为“Bridge to 另一个network adapter”，并记录下来他们的对应关系，应用退出。
	2.单击Edit virtual machine setting，并单击界面上的Add 按钮，选择Network Adapter，进入下一步，
	  Network connection为Custom，选择VMnet9，单击Finished。退出，即完成了另一张网卡的添加。
	  单击Edit virtual machine setting，并选择原来的Network Adapter，设置它的Network connection为
	  Custom，选择VMnet0，单击OK。
	3.开启虚拟机SUSE，并在系统中设置网络，添加一个网络连接，并设置IP地址。
	4.ifconfig，查看网络情况，eth号小的网卡，对应着WMnet0，eth号大的网卡，对应着WMnet9，可得知
	  eth与Windows网卡的对应情况。

	注：目前的example.c中使用的是eth0和eth1，eth0对应Port1，eth1对应Port2，要根据实际情况进行设定