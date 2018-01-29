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

### 对已有文件系统挂载

由于已经进行了格式化，所以以后系统启动只需要执行步骤2和步骤4就可以。

