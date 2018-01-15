# u-boot relocate code研究

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

## relocate code

