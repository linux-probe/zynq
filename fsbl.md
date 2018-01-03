FSBL简单分析

#### 链接脚本

链接脚本的名称为lscript.ld，路径在项目的sdk的fsbl\src。有删减。

```c
MEMORY
{
   ps7_ram_0_S_AXI_BASEADDR : ORIGIN = 0x00000000, LENGTH = 0x00030000
   ps7_ram_1_S_AXI_BASEADDR : ORIGIN = 0xFFFF0000, LENGTH = 0x0000FE00
}
/* Specify the default entry point to the program */
ENTRY(_vector_table)

SECTIONS
{
.text : {
   *(.vectors)
   *(.boot)
   *(.text)
   *(.text.*)
} > ps7_ram_0_S_AXI_BASEADDR

.init : {
   KEEP (*(.init))
} > ps7_ram_0_S_AXI_BASEADDR

.fini : {
   KEEP (*(.fini))
} > ps7_ram_0_S_AXI_BASEADDR

.rodata : {
   __rodata_start = .;
   *(.rodata)
   *(.rodata.*)
   *(.gnu.linkonce.r.*)
   __rodata_end = .;
} > ps7_ram_0_S_AXI_BASEADDR

_end = .;
}
```

从连接脚本看定义了两memory空间，第一个起始地址为0x00000000长度192K，第二个起始地址为0xFFFF0000长度64K。这两个空间都是OCM空间，在系统启动的时候，三个64KB的sections被映射到低地址空间，另一个section被影射到高地址空间。

从连接脚本可以看到程序的入口地址为0，既符号_vector_table的地址，该符号定义在fsbl_bsp\ps7_cortexa9_0_

\libsrc\standalone_v6_1\src\asm_vectors.S

```assembly
.org 0
.text
.globl _vector_table
.section .vectors
_vector_table:
	B	_boot
	B	Undefined
	B	SVCHandler
	B	PrefetchAbortHandler
	B	DataAbortHandler
	NOP	/* Placeholder for address exception vector*/
	B	IRQHandler
	B	FIQHandler
```

这一点从反汇编的代码中也能看出来：

_boot符号定义在fsbl_bsp\ps7_cortexa9_0\libsrc\standalone_v6_1\src\boot.S	