include/env_default.h

```c
#ifdef  CONFIG_BOOTCOMMAND
        "bootcmd="      CONFIG_BOOTCOMMAND              "\0"
#endif
```

include\configs\zynq-common.h

```c
#define CONFIG_BOOTCOMMAND		"run $modeboot"
```

board\xilinx\zynq\board.c	

```c
int board_late_init(void)
{
	switch ((zynq_slcr_get_boot_mode()) & ZYNQ_BM_MASK) {
	case ZYNQ_BM_QSPI:
		setenv("modeboot", "qspiboot");
		break;
	case ZYNQ_BM_NAND:
		setenv("modeboot", "nandboot");
		break;
	case ZYNQ_BM_NOR:
		setenv("modeboot", "norboot");
		break;
	case ZYNQ_BM_SD:
		setenv("modeboot", "sdboot");
		break;
	case ZYNQ_BM_JTAG:
		setenv("modeboot", "jtagboot");
		break;
	default:
		setenv("modeboot", "");
		break;
	}

	return 0;
}
```

include\configs\zynq-common.h

```
"qspiboot=echo Copying Linux from QSPI flash to RAM... && " \
		"sf probe 0 0 0 && " \
		"sf read ${kernel_load_address} 0x100000 ${kernel_size} && " \
		"sf read ${devicetree_load_address} 0x600000 ${devicetree_size} && " \
		"echo Copying ramdisk... && " \
		"sf read ${ramdisk_load_address} 0x620000 ${ramdisk_size} && " \
		"bootm ${kernel_load_address} ${ramdisk_load_address} ${devicetree_load_address}\0" 

"jtagboot=echo TFTPing Linux to RAM... && " \
		"tftpboot ${kernel_load_address} ${kernel_image} && " \
		"tftpboot ${devicetree_load_address} ${devicetree_image} && " \
		"tftpboot ${ramdisk_load_address} ${ramdisk_image} && " \
		"bootm ${kernel_load_address} ${ramdisk_load_address} ${devicetree_load_address}\0" 
```

