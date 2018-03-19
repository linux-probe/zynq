# linux kernel

## uImage 编译

默认情况下linux 中没有类中编译uImage使用的LOADD_ADDR，所以编译的时候需要指定，命令如下：

make ARCH=arm uImage CROSS_COMPILE=arm-linux-gnueabihf- LOADADDR=0x2080000

## dtb编译

make ARCH=arm dtbs CROSS_COMPILE=arm-linux-gnueabihf- 

## rootfs

使用busybox制作文件系统

编译完成后，busybox生成的东西比较简单，只有下面几项：

bin  linuxrc  sbin usr

还需要我们添加，添加完之后如下：

bin  dev  etc  linuxrc  proc  sbin  sys  tmp  usr

dev目录下需要创建console设备节点，命令如下：

mknod  console -c 5  1

etc目录如下：

.
├── fstab
├── init.d
│   └── rcS
├── inittab
└── rpc

该etc目录可以中examples/bootfloppy目录下复制过去

cpio文件系统制作：

find . | cpio --quiet -H newc -o >../imagefile.img

去上层目录执行（要不然制作的文件系统有时有问题）：

 gzip -9 -n  imagefile.img  会生成 imagefile.img.gz

mkimage -A arm -T ramdisk -C none -O linux -a 0x4000000 -e 0x4000000 -d  imagefile.img.gz  initramfs.img

可以参考网站：

http://blog.163.com/yu_hongchang/blog/static/3989413820118885447983/

## C库移植

### 全部移植

1.将xilinx-sdk/aarch32/lin/gcc-arm-linux-gnueabi/arm-linux-gnueabihf/libc目录下对应文件夹中的文件都拷贝到根文件系统的对应目录下。**cp拷贝的时候加上-d参数，这样拷贝的软链接还是软链接**

但是这样有一个缺点，就是完整libc目录很大，有154M，这样做成的文件系统是不能烧到qspi flash中，因为MIZ 701X-FUN的qspi-flash只有32M，做出来的根文件系统大于32M了。解决方法有两个：

1.移植适用于嵌入式的C库，例如culibc，newlib，但是之前没有做过，而且移植新的libc库，需要使用该c库制作工具链，然后使用新的工具链编译u-boot，kernel，busybox，很麻烦。

2.只吧需要的C库移植到根文件系统中，下面进行描述

### 部分移植

本来可以使用ldd查看可执行文件依赖什么库，但只zyqn提供的工具链中没有ldd命令，可以使用下面的命令查看：

arm-linux-gnueabihf-readelf -d busybox

输出如下：

```
Dynamic section at offset 0xe700c contains 25 entries:
  Tag        Type                         Name/Value
 0x00000001 (NEEDED)                     Shared library: [libm.so.6]
 0x00000001 (NEEDED)                     Shared library: [libc.so.6]
```
表示依赖libm.so.6和libc.so.6两个库，起始这两个库只是软链接，拷贝的时候，把两个软链接也连接到的库都拷贝到跟文件系统/lib下。

通过测试发现启动的时候，执行linuxrc出错。经过测试还需ld-linux-armhf.so.3。该库适合动态库加载有关，如果没有该库，动态库加载是不成功的。该库的名称可以通过下面的命令查看。

rm-linux-gnueabihf-readelf -l busybox

输出如下：
```
Program Headers:
  Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
  EXIDX          0x0e6f14 0x000f6f14 0x000f6f14 0x00008 0x00008 R   0x4
  PHDR           0x000034 0x00010034 0x00010034 0x00100 0x00100 R E 0x4
  INTERP         0x000134 0x00010134 0x00010134 0x00019 0x00019 R   0x1

[Requesting program interpreter: /lib/ld-linux-armhf.so.3]
```
给出了用于动态库加载的库名称和该库存放路径。

将该软链接和对链接的文件拷贝到根文件系统的/lib下

将xilinx-sdk/aarch32/lin/gcc-arm-linux-gnueabi/arm-linux-gnueabihf/libc/sbin下的可执行文件拷贝到根文件系统的对应路径下下。

经过以上步骤，执行busybox，出现权限错误（将busybox复制到静态编译的跟文件系统中测试），这时候需要给上述步骤所有的库和可执行文件增加可执行权限。之后busybox可以正常执行，

重新制作根文件系统，可以正常启动。

## boot linux

通过tftpboot命令启动linux

```
tftpboot ${kernel_load_address} ${kernel_image} 
tftpboot ${devicetree_load_address} ${devicetree_image}
tftpboot ${ramdisk_load_address} ${ramdisk_image}
bootm ${kernel_load_address} ${ramdisk_load_address} ${devicetree_load_address}
```
## ubifs文件系统

### 系统第一次启动

这时候分区的还没有格式化，执行的命令如下：

1.格式化要格式的分区

通过下面的命令查看mtd分区信息

```
/mnt # cat /proc/mtd 
dev:    size   erasesize  name
mtd0: 00100000 00010000 "qspi-fsbl-uboot"
mtd1: 00500000 00010000 "qspi-linux"
mtd2: 00020000 00010000 "qspi-device-tree"
mtd3: 005e0000 00010000 "qspi-rootfs"
mtd4: 00400000 00010000 "qspi-bitstream"
```

以格式化mt3为例

 flash_eraseall /dev/mtd3

2.ubiattach /dev/ubi_ctrl -m 3

​	3为mtd的序号，执行该步骤之后，会在/dev/下创建ubi0设备文件

3.ubimkvol /dev/ubi0 -N rootfs 

​	rootfs为卷的名称，执行该步骤后会在/dev/下创建ubi0_0设备文件

4.挂载

​	mount -t ubifs /dev/ubi0_0  /mnt
可以参考http://blog.csdn.net/mirkerson/article/details/8314018

### 对已有文件系统挂载

由于已经进行了格式化，所以以后系统启动只需要执行步骤2和步骤4就可以。

