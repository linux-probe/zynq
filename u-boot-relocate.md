# u-boot relocate code研究

[TOC]
## 常规编译

C文件如下：

```c
int a = 20; 
void test(void)
{
        a = 30; 
}

int main()
{
        test();
        return 0;
}
```

使用如下命令编译成汇编语言：

arm-linux-gnueabihf-gcc -O0  -S main.c -o main.S 

-O0表示不做任何优化

-S表示编译成汇编语言，不链接

编译后的汇编如下（有删减）：

```assembly
a:
        .word   20  
        .text
        .align  2
        .global test
        .syntax divided
        .arm
        .type   test, %function
test:
        @ args = 0, pretend = 0, frame = 0 
        @ frame_needed = 1, uses_anonymous_args = 0 
        @ link register save eliminated.
        str     fp, [sp, #-4]!
        add     fp, sp, #0
        movw    r3, #:lower16:a
        movt    r3, #:upper16:a
        mov     r2, #30 
        str     r2, [r3]
        mov     r0, r0  @ nop 
        sub     sp, fp, #0
        @ sp needed
        ldr     fp, [sp], #4
        bx      lr  
        .size   test, .-test
```

a为一个lable，该symbals地址处定义了一个.word(四字节)变量，初始值为20，起始就是全局变量a。

test lable，就是test函数的入口，其中a = 30，的汇编代码如下：

movw    r3, #:lower16:a

movt    r3, #:upper16:a

第一条表示将a lable地址的低16 bit加载到寄存器r3，高16bit清零

第二条表示将a label地址的高16 bit加载到寄存器r3，低16bit不变

这样R3存储了一个32bit的地址，该地址就是label a的地址

  mov     r2, #30

将30 赋值到r2 寄存器

 str     r2, [r3]

将r2寄存的值存储到r3寄存器值对应的地址处，既给a变量赋值。



## u-boot编译

u-boot编译的时候会加上如下选项

-mword-relocations

编译命令如下：

arm-linux-gnueabihf-gcc -mword-relocations -O0  -S main.c -o main.S

编译成的汇编代码如下：

```
a:
        .word   20  
        .text
        .align  2
        .global test
        .syntax divided
        .arm
        .type   test, %function
test:
        @ args = 0, pretend = 0, frame = 0 
        @ frame_needed = 1, uses_anonymous_args = 0 
        @ link register save eliminated.
        str     fp, [sp, #-4]!
        add     fp, sp, #0
        ldr     r3, .L2 
        mov     r2, #30 
        str     r2, [r3]
        mov     r0, r0  @ nop 
        sub     sp, fp, #0
        @ sp needed
        ldr     fp, [sp], #4
        bx      lr  
.L3:
        .align  2
.L2:
        .word   a   
        .size   test, .-test
```

可以看到代码和正常编译的已经不一样了。

赋值部分代码如下：

 ldr     r3, .L2    将.L2 label的地址处的值加载到r3寄存器中。

可以看下.L2部分，定义了一个word 变量，该变量的值是a，我们知道a是一个label，起始表示的就是一个地址。

所以.L2处定义了一个变量，**该变量的值是a label的地址，起始也就是变量a的地址**

 str     r2, [r3]  将r2的值存入r3寄存器值对应的地址处，r3的值就是a的地址，就是表示将r2的值存入a lable地址处。

从上面可以看出，.L2起始是紧挨着函数的，而且寻找也是用的是PC + offset的方式，是位置无关的。我们只要修改.L2 地址处的值就可以改变a变量的地址了。



## relocate code准备

### setup_mon_len

```c
static int setup_mon_len(void)
{
	gd->mon_len = (ulong)&__bss_end - (ulong)_start;
	return 0;
}
```



### setup_dest_addr

gd->relocaddr = gd->ram_top;

主要是设置gd->relocaddr



### reserve_uboot

```c
static int reserve_uboot(void)
{
	/*
	 * reserve memory for U-Boot code, data & bss
	 * round down to next 4 kB limit
	 */
  	/*gd->mon_len为code, data & bss等段的长度，减去该部分用于留给u-boot*/
	gd->relocaddr -= gd->mon_len;
	gd->relocaddr &= ~(4096 - 1);/*4K对齐*/
	gd->start_addr_sp = gd->relocaddr;
	return 0;
}
```



###  setup_reloc

```c
static int setup_reloc(void)
{
  	/**
  	 *gd->relocaddr是uboot要被搬运到的地址起始地址
  	 *CONFIG_SYS_TEXT_BASE是编译时指定的起始地址
  	 */
	gd->reloc_off = gd->relocaddr - CONFIG_SYS_TEXT_BASE;
	memcpy(gd->new_gd, (char *)gd, sizeof(gd_t));
	debug("Relocation Offset is: %08lx\n", gd->reloc_off);
	debug("Relocating to %08lx, new gd at %08lx, sp at %08lx\n",
	      gd->relocaddr, (ulong)map_to_sysmem(gd->new_gd),
	      gd->start_addr_sp);
	return 0;
}
```

执行完了上面之后，就返回汇编部分



## relocate_code

```assembly
	adr	lr, here
	ldr	r0, [r9, #GD_RELOC_OFF]		/* r0 = gd->reloc_off */
	add	lr, lr, r0

	ldr	r0, [r9, #GD_RELOCADDR]		/* r0 = gd->relocaddr */
	b	relocate_code
here:
```

前面三条语句分别解释如下：

1.获得here label的地址，该地址是运行时地址。

> adr其实是伪指令，不是真正的汇编指令，在编译的时候，该指令会被替换为add或sub 然后加上> pc在加上offset的形式。`adr lr, here`的汇编结果为`add lr, pc, #12`所以无论在什么>.情况下都可以真确获取here地址，只不过代码运行的起始地址不一样，here的地址也就不一样

2.r9为gd变量的指针，表示将gd->reloc_off加载到r9寄存器中。

3.gd->reloc_off + here = 搬运后here lable的地址。lr为链接寄存器，在执行返回时候用，因为relocate_code之后要跳转到搬运后的代码，该步骤是计算返回后跳转地址



### relocate_code

代码如下：

```assembly
/*
 * void relocate_code(addr_moni)
 *
 * This function relocates the monitor code.
 *
 * NOTE:
 * To prevent the code below from containing references with an R_ARM_ABS32
 * relocation record type, we never refer to linker-defined symbols directly.
 * Instead, we declare literals which contain their relative location with
 * respect to relocate_code, and at run time, add relocate_code back to them.
 */

ENTRY(relocate_code)
	/* r1 <- SRC &__image_copy_start, __image_copy_start等于CONFIG_SYS_TEXT_BASE*/
	ldr	r1, =__image_copy_start	
	/* r4 <- relocation offset, r0 = gd->relocaddr,如果r0等于r1，跳过relocate*/
	subs	r4, r0, r1 
	beq	relocate_done		/* skip relocation */
	ldr	r2, =__image_copy_end	/* r2 <- SRC &__image_copy_end */

copy_loop:
	/*将r1地址处的数据加载8个字节到r10，r11寄存器，然后 r1 = r1 + 4*2*/
	ldmia	r1!, {r10-r11}		/* copy from source address [r1]    */
	/*将r10，r11寄存器数据存入r0地址处，然后 r0 = r0 + 4*2*/
	stmia	r0!, {r10-r11}		/* copy to   target address [r0]    */
	cmp	r1, r2			/* until source end address [r2]    */
	blo	copy_loop

	/*
	 * fix .rel.dyn relocations
	 */
	ldr	r2, =__rel_dyn_start	/* r2 <- SRC &__rel_dyn_start */
	ldr	r3, =__rel_dyn_end	/* r3 <- SRC &__rel_dyn_end */
fixloop:
	ldmia	r2!, {r0-r1}		/* (r0,r1) <- (SRC location,fixup) */
	and	r1, r1, #0xff
	/*#define R_ARM_RELATIVE	23(0x17)Adjust by program base*/
	cmp	r1, #R_ARM_RELATIVE		/*如果等于R_ARM_RELATIVE需要重定位*/
	bne	fixnext
	/*.rel.dyn格式,参考.rel.dyn格式的章节*/
	
	/* relative fix: increase location by offset */
	/**
	 *r0是紧跟在函数后面，访问这个变量的间接地址，
	 *r4=gd->relocaddr - CONFIG_SYS_TEXT_BASE
	 *r0 + r4 = r0地址对应的拷贝后的地址，因为镜像已经搬运玩。
	 */
	add	r0, r0, r4	
	ldr	r1, [r0]	/*r0地址处的数据为真正变量的地址。取出变量真正的地址*/
	add	r1, r1, r4	/*r1 + r4 = 变量对应的搬用后的地址。*/
	str	r1, [r0]	/*将变量对应搬运后的地址写入搬运后的间接地址处*/
	/**
	 *经过上面的操作，当访问变量时，由于简介变量都是通过基于PC + offset的方式
	 *进行寻找，是位置无关的，通过修改间接变量处的值，从而修改了真是变量的地址，
	 *达到了relocate的目的。这样应该比经过got表的方法快捷，简单，size也会小吧。
	 */
fixnext:
	cmp	r2, r3
	blo	fixloop

relocate_done:
	bx	lr /*跳转到relocate之后的here*/
ENDPROC(relocate_code)

```



## adr指令

adr其实是伪指令，不是真正的汇编指令，在编译的时候，该指令会被替换为add或sub 然后加上pc在加上offset的形式，如u-boot中一下指令，其中here为一个lable：

```assembly
adr	lr, here
```

最终被编译成如下指令：

```assembly
add     lr, pc, #12
```

adr表示将一个标号的地址存入一个寄存器，但是该地址是相对地址，和代码运行情况有关，比如：`add r0, pc, #0`假如这段代码在 0x30000000 运行，那么 adr r0, _start 得到 r0 = 0x3000000c；如果在地址 0 运行，就是 0x0000000c 了。

adr可以获取该标号在运行时的地址，使用adr指令可以实现位置无关代码，无论在什么地址运行都可以正确得到标号的值。



## .rel.dyn格式

反汇编后看到的.rel.dyn的数据如下：

4069d5c:       040021c4        streq   r2, [r0], #-452 ; 0xfffffe3c
4069d60:       00000017        andeq   r0, r0, r7, lsl r0

第一列表示的是反汇编后的地址，第二轮是真是的数据，后面的是反汇编后的指令，其实根部不是指令，这里的单纯就是数据。

040021c4 ：是一个变量的间接地址，相当于.L2的地址



## u-boot摘录

```
ifneq ($(CONFIG_SPL_BUILD),y)
# Check that only R_ARM_RELATIVE relocations are generated.
ALL-y += checkarmreloc
# The movt / movw can hardcode 16 bit parts of the addresses in the
# instruction. Relocation is not supported for that case, so disable
# such usage by requiring word relocations.
PLATFORM_CPPFLAGS += $(call cc-option, -mword-relocations)
PLATFORM_CPPFLAGS += $(call cc-option, -fno-pic)
endif
```

加-fno-pic是为了不生成 .got section and R_ARM_GOT_BREL，参考参考https://www.mail-archive.com/u-boot@lists.denx.de/msg194919.html