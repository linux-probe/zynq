#zynq u-boot

## 链接脚本

编译zynq_zed使用的链接脚本定义在：u-boot-xlnx/arch/arm/mach-zynq/u-boot.lds。最终生成的链接脚本存放在u-boot根目录,名称为u-boot.lds，内如如下（有删减）：

```assembly
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
 . = 0x00000000;
 . = ALIGN(4);
 .text :
 {
  *(.__image_copy_start)
  *(.vectors)
  arch/arm/cpu/armv7/start.o (.text*)
  *(.text*)
 }
 . = ALIGN(4);
 .rodata : { *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*))) }
 . = ALIGN(4);
 .data : {
  *(.data*)
 }
 . = ALIGN(4);
 . = .;
 . = ALIGN(4);
 .u_boot_list : {
  KEEP(*(SORT(.u_boot_list*)));
 }
 . = ALIGN(4);
 .image_copy_end :
 {
  *(.__image_copy_end)
 }
 .rel_dyn_start :
 {
  *(.__rel_dyn_start)
 }
 .rel.dyn : {
  *(.rel*)
 }
 .rel_dyn_end :
 {
  *(.__rel_dyn_end)
 }
 .end :
 {
  *(.__end)
 }
 _image_binary_end = .;
 .bss_start __rel_dyn_start (OVERLAY) : {
  KEEP(*(.__bss_start));
  __bss_base = .;
 }
 .bss __bss_base (OVERLAY) : {
  *(.bss*)
   . = ALIGN(4);
   __bss_limit = .;
 }
 .bss_end __bss_limit (OVERLAY) : {
  KEEP(*(.__bss_end));
 }
```

### 入口地址

从上面可以看出程序的入口符号为_start,定义在arch\arm\lib\vectors.S，内容如下：

```
.globl _start
	.section ".vectors", "ax"

_start:

#ifdef CONFIG_SYS_DV_NOR_BOOT_CFG
	.word	CONFIG_SYS_DV_NOR_BOOT_CFG
#endif
	b	reset
	ldr	pc, _undefined_instruction
	ldr	pc, _software_interrupt
	ldr	pc, _prefetch_abort
	ldr	pc, _data_abort
	ldr	pc, _not_used
	ldr	pc, _irq
	ldr	pc, _fiq
```

该文件的代码定义在段.vectors段中。

虽然可以看到在链接脚本中 . = 0x00000000;但是起始代码是从0x400 0000开始的，这是在链接的时候传递给ld的参数，链接的命令如下（有删减）：

```c++
arm-linux-gnueabihf-ld.bfd   -pie  --gc-sections -Bstatic -Ttext 0x4000000 -o u-boot -T u-boot.lds arch/arm/cpu/armv7/start.o --start-group  arch/arm/cpu/built-in.o  arch/arm/cpu/armv7/built-in.o   --end-group arch/arm/lib/eabi_compat.o  arch/arm/lib/lib.a -Map u-boot.map
```

其中-pie表示位置无关。-Ttext 0x4000000指定了text段的入口地址。

### .text 输出段

```
 .text :
 {
  *(.__image_copy_start)
  *(.vectors)
  arch/arm/cpu/armv7/start.o (.text*)
  *(.text*)
 }
```

从上面的链接脚本可以看到.text输出段，主要包含下面的输入段：

1. ``*(.__image_copy_start)``所有的名为``.__image_copy_start``段
2. 所有的``.vectors``段
3. ``arch/arm/cpu/armv7/start.o``的``.text``段
4. 所有的``.text``段

上面的链接是有顺序的，顺序就按照各个输入段出现的顺序。

#### ``.__image_copy_start``和``.__image_copy_end``

``__image_copy_start``和``__image_copy_end``是配合在一块使用的，定义文件	arch\arm\lib\sections.c，如下：

```c
char __image_copy_start[0] __attribute__((section(".__image_copy_start")));
char __image_copy_end[0] __attribute__((section(".__image_copy_end")));
```

看下__image_copy_end：

```assembly
 .image_copy_end :
 {
  *(.__image_copy_end)
 }
```

其实，``.image_copy_end``输出段的大小为0，因为只有``char __image_copy_end[0]``被定义在该段，而且``char __image_copy_end[0]``的大小为0。同样在.text输出段中``  *(.__image_copy_start)``的大小也为0。

至于为什么需要这样来，可能和使用的参数-pie有关系，需要对arm的位置无关有详细了解。

这个文章对pie有一点说明：https://www.ibm.com/developerworks/cn/linux/l-cn-sdlstatic/

在sections.c文件中，作者做了一个简单的说明：

```c++
/**
 * These two symbols are declared in a C file so that the linker
 * uses R_ARM_RELATIVE relocation, rather than the R_ARM_ABS32 one
 * it would use if the symbols were defined in the linker file.
 * Using only R_ARM_RELATIVE relocation ensures that references to
 * the symbols are correct after as well as before relocation.
 *
 * We need a 0-byte-size type for these symbols, and the compiler
 * does not allow defining objects of C type 'void'. Using an empty
 * struct is allowed by the compiler, but causes gcc versions 4.4 and
 * below to complain about aliasing. Therefore we use the next best
 * thing: zero-sized arrays, which are both 0-byte-size and exempt from
 * aliasing warnings.
 */
```

定义在C文件中，linker使用R_ARM_RELATIVE重定位，而不是使用R_ARM_ABS32，如果symbols定义在linker file（链接脚本）那么就会使用R_ARM_ABS32重定位。

我么需要一个0字节大小的类型来定义这个symbols，编译器不允许定义C语言类型为void的变量。使用一个空的结构体编译是允许的，但是gcc 4.4和一下的编译器会把抱怨，因此，使用了更好的：零大小的数组。

####  ``__image_copy_start`` 和 `` __image_copy_end``赋值

这两个变量在编译后的值是多少，u-boot.map中查到

```
 *(.__image_copy_start)
 .__image_copy_start
                0x0000000004000000        0x0 arch/arm/lib/built-in.o
                0x0000000004000000                __image_copy_start

```

.__image_copy_start的值为0x0000000004000000

```
.image_copy_end
                0x0000000004069894        0x0   
 *(.__image_copy_end)
 .__image_copy_end
                0x0000000004069894        0x0 arch/arm/lib/built-in.o
```

__image_copy_end值为0x0000000004069894（会动态变化）

从定义

```c
char __image_copy_start[0] __attribute__((section(".__image_copy_start")));
char __image_copy_end[0] __attribute__((section(".__image_copy_end")));
```

``char __image_copy_start[0]``被放到了段``.__image_copy_start``，那么则表示数组``__image_copy_start``是从段``.__image_copy_start``地址处开始的。段``.__image_copy_start``的地址就是代码的起始地址，所以``__image_copy_start``的值是0x0000000004000000（``__image_copy_start``是数组名，表示一个常量指针）。而且在gcc中零元素的数组恰好大小为0，既不占用空间。

如果将``char __image_copy_start[0] __attribute__((section(".__image_copy_start")));``换为``char __image_copy_start[1] __attribute__((section(".__image_copy_start")));``,``__image_copy_start``的值还是0x0000000004000000，只不过需要占用一个字节内存而已。这样程序的入口就变成__image_copy_start变量的地址了，如下图，反汇编如下：

```assembly
04000000 <__image_copy_start>:
04000020 <_start>:
 4000020:       ea0000be        b       4000320 <reset>
 4000024:       e59ff014        ldr     pc, [pc, #20]   ; 4000040 <_undefined_instruction>
 4000028:       e59ff014        ldr     pc, [pc, #20]   ; 4000044 <_software_interrupt>
 400002c:       e59ff014        ldr     pc, [pc, #20]   ; 4000048 <_prefetch_abort>
 4000030:       e59ff014        ldr     pc, [pc, #20]   ; 400004c <_data_abort>
 4000034:       e59ff014        ldr     pc, [pc, #20]   ; 4000050 <_not_used>
 4000038:       e59ff014        ldr     pc, [pc, #20]   ; 4000054 <_irq>
 400003c:       e59ff014        ldr     pc, [pc, #20]   ; 4000058 <_fiq>
```

所以需要采用那么怪异的定义方式。

### reset

reset symbal定义在arch\arm\cpu\armv7\start.S,如下：

```assembly
	.globl	reset
	.globl	save_boot_params_ret
#ifdef CONFIG_ARMV7_LPAE
	.global	switch_to_hypervisor_ret
#endif

reset:
	/* Allow the board to save important registers */
	b	save_boot_params
save_boot_params_ret:
```

#### save_boot_params

通过r0寄存器判断是否从spl加载的u-boot，并把结果存放在from_spl变量中，然后返回到save_boot_params_ret。

####  _main

_main定义在arch\arm\lib\crt0.S

主要是准备C语言执行环境，调用board_init_f函数
##### 设置堆栈指针

```assembly
ldr	sp, =(CONFIG_SYS_INIT_SP_ADDR)
```

sp为堆栈指针寄存器，在C程序函数调用，参数入栈和出栈时会用到，是运行C代码必须要配置的。

##### gd定义

```c
#ifdef CONFIG_ARM64
#define DECLARE_GLOBAL_DATA_PTR		register volatile gd_t *gd asm ("x18")
#else
#define DECLARE_GLOBAL_DATA_PTR		register volatile gd_t *gd asm ("r9")
#endif
#endif
```


##### board_init_f

board_init_f定义在common\board_f.c中

```c
void board_init_f(ulong boot_flags)
{
	gd->flags = boot_flags;
	gd->have_console = 0;

	if (initcall_run_list(init_sequence_f))
		hang();
}

```

###### init_sequence_f

```c
static init_fnc_t init_sequence_f[] = {
	setup_mon_len,
	fdtdec_setup,
	initf_malloc,
	initf_console_record,
	arch_cpu_init,		/* basic arch cpu dependent setup */
	mach_cpu_init,		/* SoC/machine dependent CPU setup */
	initf_dm,
	arch_cpu_init_dm,
	timer_init,		/* initialize timer */
	mark_bootstage,
	env_init,		/* initialize environment */
	init_baud_rate,		/* initialze baudrate settings */
	serial_init,		/* serial communications setup */
	console_init_f,		/* stage 1 init of console */
	display_options,	/* say that we are here */
	display_text_info,	/* show debugging info if required */
	print_cpuinfo,		/* display cpu info (and speed) */
	show_board_info,
	INIT_FUNC_WATCHDOG_INIT
	INIT_FUNC_WATCHDOG_RESET
	announce_dram_init,
	/* TODO: unify all these dram functions? */
	dram_init,		/* configure available RAM banks */
	INIT_FUNC_WATCHDOG_RESET
	INIT_FUNC_WATCHDOG_RESET
	INIT_FUNC_WATCHDOG_RESET
	/*
	 * Now that we have DRAM mapped and working, we can
	 * relocate the code and continue running from DRAM.
	 *
	 * Reserve memory at end of RAM for (top down in that order):
	 *  - area that won't get touched by U-Boot and Linux (optional)
	 *  - kernel log buffer
	 *  - protected RAM
	 *  - LCD framebuffer
	 *  - monitor code
	 *  - board info struct
	 */
	setup_dest_addr,
	reserve_round_4k,/*gd->relocaddr &= ~(4096 - 1);4K对齐*/
	reserve_mmu,
	reserve_trace,
	reserve_malloc,
	reserve_board,
	setup_machine,
	reserve_global_data,
	reserve_fdt,
	reserve_arch,
	reserve_stacks,
	setup_dram_config,
	show_dram_config,
	display_new_sp,
	INIT_FUNC_WATCHDOG_RESET
	reloc_fdt,
	setup_reloc,
	NULL,
};
```

###### setup_dest_addr

```c
static int setup_dest_addr(void)
{
	debug("Ram size: %08lX\n", (ulong)gd->ram_size);
	/*获取ram大小*/
	gd->ram_size = board_reserve_ram_top(gd->ram_size);
	/*下面两句都是获取ram_top地址*/
	gd->ram_top += get_effective_memsize();
	gd->ram_top = board_get_usable_ram_top(gd->mon_len);
	gd->relocaddr = gd->ram_top;
	debug("Ram top: %08lX\n", (ulong)gd->ram_top);
	return 0;
}
```

###### setup_reloc

```c
static int setup_reloc(void)
{
#ifdef CONFIG_SYS_TEXT_BASE
  	/**
  	 *gd->relocaddr = gd->ram_top;
  	 *CONFIG_SYS_TEXT_BASE:0x4000000
  	 *relocate地址和编译起始地址的差值。需要后期继续分析
  	 */
	gd->reloc_off = gd->relocaddr - CONFIG_SYS_TEXT_BASE;
#endif
	memcpy(gd->new_gd, (char *)gd, sizeof(gd_t));
	return 0;
}
```

##### relocate_code

##### relocate_vectors

参考u-boot-relocate.md

##### c_runtime_cpu_setup

无效icache

##### claen bss

##### board_init_r

```c
void board_init_r(gd_t *new_gd, ulong dest_addr)
{
	if (initcall_run_list(init_sequence_r))
		hang();
	/* NOTREACHED - run_main_loop() does not return */
	hang();
}
```

```c
int initcall_run_list(const init_fnc_t init_sequence[])
{
	const init_fnc_t *init_fnc_ptr;

	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		unsigned long reloc_ofs = 0;
		int ret;

		if (gd->flags & GD_FLG_RELOC)
			reloc_ofs = gd->reloc_off;

		debug("initcall: %p", (char *)*init_fnc_ptr - reloc_ofs);
		if (gd->flags & GD_FLG_RELOC)
			debug(" (relocated to %p)\n", (char *)*init_fnc_ptr);
		else
			debug("\n");
		ret = (*init_fnc_ptr)();
		if (ret) {
			printf("initcall sequence %p failed at call %p (err=%d)\n",
			       init_sequence,
			       (char *)*init_fnc_ptr - reloc_ofs, ret);
			return -1;
		}
	}
	return 0;
}

```

```c
init_fnc_t init_sequence_r[] = {
	initr_trace,
	initr_reloc,
	initr_caches,
	initr_reloc_global_data,
	initr_barrier,
	initr_malloc,
	initr_console_record,
#ifdef CONFIG_SYS_NONCACHED_MEMORY
	initr_noncached,
#endif
	bootstage_relocate,
#ifdef CONFIG_DM
	initr_dm,
#endif
	initr_bootstage,
#if defined(CONFIG_ARM) || defined(CONFIG_NDS32)
	board_init,	/* Setup chipselects */
#endif
	/*
	 * TODO: printing of the clock inforamtion of the board is now
	 * implemented as part of bdinfo command. Currently only support for
	 * davinci SOC's is added. Remove this check once all the board
	 * implement this.
	 */
#ifdef CONFIG_CLOCKS
	set_cpu_clk_info, /* Setup clock information */
#endif
#ifdef CONFIG_EFI_LOADER
	efi_memory_init,
#endif
	stdio_init_tables,
	initr_serial,
	initr_announce,
	INIT_FUNC_WATCHDOG_RESET
#ifdef CONFIG_NEEDS_MANUAL_RELOC
	initr_manual_reloc_cmdtable,
#endif
#if defined(CONFIG_PPC) || defined(CONFIG_M68K) || defined(CONFIG_MIPS)
	initr_trap,
#endif
#ifdef CONFIG_ADDR_MAP
	initr_addr_map,
#endif
#if defined(CONFIG_BOARD_EARLY_INIT_R)
	board_early_init_r,
#endif
	INIT_FUNC_WATCHDOG_RESET
#ifdef CONFIG_LOGBUFFER
	initr_logbuffer,
#endif
#ifdef CONFIG_POST
	initr_post_backlog,
#endif
	INIT_FUNC_WATCHDOG_RESET
#ifdef CONFIG_SYS_DELAYED_ICACHE
	initr_icache_enable,
#endif
#if defined(CONFIG_PCI) && defined(CONFIG_SYS_EARLY_PCI_INIT)
	/*
	 * Do early PCI configuration _before_ the flash gets initialised,
	 * because PCU ressources are crucial for flash access on some boards.
	 */
	initr_pci,
#endif
#ifdef CONFIG_WINBOND_83C553
	initr_w83c553f,
#endif
#ifdef CONFIG_ARCH_EARLY_INIT_R
	arch_early_init_r,
#endif
	power_init_board,
#ifndef CONFIG_SYS_NO_FLASH
	initr_flash,
#endif
	INIT_FUNC_WATCHDOG_RESET
#if defined(CONFIG_PPC) || defined(CONFIG_M68K) || defined(CONFIG_X86) || \
	defined(CONFIG_SPARC)
	/* initialize higher level parts of CPU like time base and timers */
	cpu_init_r,
#endif
#ifdef CONFIG_PPC
	initr_spi,
#endif
#ifdef CONFIG_CMD_NAND
	initr_nand,
#endif
#ifdef CONFIG_CMD_ONENAND
	initr_onenand,
#endif
#ifdef CONFIG_GENERIC_MMC
	initr_mmc,
#endif
#ifdef CONFIG_HAS_DATAFLASH
	initr_dataflash,
#endif
	initr_env,
#ifdef CONFIG_SYS_BOOTPARAMS_LEN
	initr_malloc_bootparams,
#endif
	INIT_FUNC_WATCHDOG_RESET
	initr_secondary_cpu,
#if defined(CONFIG_ID_EEPROM) || defined(CONFIG_SYS_I2C_MAC_OFFSET)
	mac_read_from_eeprom,
#endif
	INIT_FUNC_WATCHDOG_RESET
#if defined(CONFIG_PCI) && !defined(CONFIG_SYS_EARLY_PCI_INIT)
	/*
	 * Do pci configuration
	 */
	initr_pci,
#endif
	stdio_add_devices,
	initr_jumptable,
#ifdef CONFIG_API
	initr_api,
#endif
	console_init_r,		/* fully init console as a device */
#ifdef CONFIG_DISPLAY_BOARDINFO_LATE
	show_board_info,
#endif
#ifdef CONFIG_ARCH_MISC_INIT
	arch_misc_init,		/* miscellaneous arch-dependent init */
#endif
#ifdef CONFIG_MISC_INIT_R
	misc_init_r,		/* miscellaneous platform-dependent init */
#endif
	INIT_FUNC_WATCHDOG_RESET
#ifdef CONFIG_CMD_KGDB
	initr_kgdb,
#endif
	interrupt_init,
#if defined(CONFIG_ARM) || defined(CONFIG_AVR32)
	initr_enable_interrupts,
#endif
#if defined(CONFIG_MICROBLAZE) || defined(CONFIG_AVR32) || defined(CONFIG_M68K)
	timer_init,		/* initialize timer */
#endif
#if defined(CONFIG_STATUS_LED)
	initr_status_led,
#endif
	/* PPC has a udelay(20) here dating from 2002. Why? */
#ifdef CONFIG_CMD_NET
	initr_ethaddr,
#endif
#ifdef CONFIG_BOARD_LATE_INIT
	board_late_init,
#endif
#if defined(CONFIG_CMD_AMBAPP)
	ambapp_init_reloc,
#if defined(CONFIG_SYS_AMBAPP_PRINT_ON_STARTUP)
	initr_ambapp_print,
#endif
#endif
#if defined(CONFIG_SCSI) && !defined(CONFIG_DM_SCSI)
	INIT_FUNC_WATCHDOG_RESET
	initr_scsi,
#endif
#ifdef CONFIG_CMD_DOC
	INIT_FUNC_WATCHDOG_RESET
	initr_doc,
#endif
#ifdef CONFIG_BITBANGMII
	initr_bbmii,
#endif
#ifdef CONFIG_CMD_NET
	INIT_FUNC_WATCHDOG_RESET
	initr_net,
#endif
#ifdef CONFIG_POST
	initr_post,
#endif
#if defined(CONFIG_CMD_PCMCIA) && !defined(CONFIG_CMD_IDE)
	initr_pcmcia,
#endif
#if defined(CONFIG_CMD_IDE)
	initr_ide,
#endif
#ifdef CONFIG_LAST_STAGE_INIT
	INIT_FUNC_WATCHDOG_RESET
	/*
	 * Some parts can be only initialized if all others (like
	 * Interrupts) are up and running (i.e. the PC-style ISA
	 * keyboard).
	 */
	last_stage_init,
#endif
#ifdef CONFIG_CMD_BEDBUG
	INIT_FUNC_WATCHDOG_RESET
	initr_bedbug,
#endif
#if defined(CONFIG_PRAM) || defined(CONFIG_LOGBUFFER)
	initr_mem,
#endif
#ifdef CONFIG_PS2KBD
	initr_kbd,
#endif
#if defined(CONFIG_SPARC)
	prom_init,
#endif
	run_main_loop,
};
```

###### initr_reloc_global_data

```c
static int initr_reloc_global_data(void)
{
	monitor_flash_len = _end - __image_copy_start;
	/*
	* The fdt_blob needs to be moved to new relocation address
	* incase of FDT blob is embedded with in image
	*/
	gd->fdt_blob += gd->reloc_off;/*重定位fdt_blob地址*/
	return 0;
}
```

