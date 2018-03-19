# mtd-utils移植

mtd-utils移植依赖三个库，下来分别移植依赖的库

## libuuid库

1.下载libuuid-1.0.3.tar.gz

2.解压libuuid-1.0.3.tar.gz

​	tar -xvf libuuid-1.0.3.tar.gz

3.进入libuuid-1.0.3目录，然后创建install目录用于编译成功后的安装目录。

4.配置
```
./configure --host=arm-linux-gnueabihf  --prefix=/home/assin/zynx/rootfs/rootfs-xlnx/ubifs/libs/libuuid-1.0.3/install/
```

5.编译

make -j8

6.安装

make install

## zlib库

1.下载zlib-1.2.11.tar.gz

2.解压zlib-1.2.11.tar.gz

3.进入zlib-1.2.11，然后创建install目录用于编译成功后的安装目录

4.配置

```
./configure --prefix=/home/assin/zynx/rootfs/rootfs-xlnx/ubifs/libs/zlib-1.2.11/install
```

4.zlib不支持交叉编译配置，需要配置之后修改Makefile

将下面对应的gcc，ar都替换成交叉编译工具链的对应软件。

```makefile
CC=arm-linux-gnueabihf-gcc
LDSHARED=arm-linux-gnueabihf-gcc -shared -Wl,-soname,libz.so.1,--version-script,zlib.map
CPP=arm-linux-gnueabihf-gcc -E
AR=arm-linux-gnueabihf-ar
```

5.编译

make -j8

6.安装

make install

## lzo库

1.下载lzo-2.10.tar.gz

2.解压lzo-2.10.tar.gz

3.进入lzo-2.10目录，然后创建install目录用于编译完成之后进行安装

4.配置

```makefile
./configure --host=arm-linux-gnueabihf --prefix=/home/assin/zynx/rootfs/rootfs-xlnx/ubifs/libs/lzo-2.10/install/
```

5.编译

make -j8

6.安装

## mtd-utils编译

使用git先下载，然后进行配置

1.配置

```
./configure --host=arm-linux-gnueabihf --prefix=/home/assin/zynx/rootfs/rootfs-xlnx/ubifs/mtd-utils/install/  CPPFLAGS="-I/home/assin/zynx/rootfs/rootfs-xlnx/ubifs/libs/libuuid-1.0.3/install/include -I/home/assin/zynx/rootfs/rootfs-xlnx/ubifs/libs/zlib-1.2.11/install/include -I/home/assin/zynx/rootfs/rootfs-xlnx/ubifs/libs/lzo-2.10/install/include" LDFLAGS="-L/home/assin/zynx/rootfs/rootfs-xlnx/ubifs/libs/libuuid-1.0.3/install/lib -L/home/assin/zynx/rootfs/rootfs-xlnx/ubifs/libs/zlib-1.2.11/install/lib -L/home/assin/zynx/rootfs/rootfs-xlnx/ubifs/libs/lzo-2.10/install/lib"
```

CPPFLAGS指定头文件的路径，LDFLAGS指定连接库的路径。

2.编译

make -j8

3.make install

编译出来的可执行文件都是静态编译的，因此不需要将编译时依赖的库复制到板子上去。

