# Arch Lab

本实验主要依托于CSAPP的第四章，为了完成实验，实验者需要对于书中所描述的Y86-84指令集具有较为清晰的理解。实验具体分为三个子部分：

* 利用书中所构造的Y86-64指令集编写汇编语言，以实现等效的三个C语言函数；
* 在书中SEQ（Y86-84处理器的顺序实现）上增加一条额外的指令；
* 优化书中的PIPE（Y86-84处理器的流水线实现）及用其来执行的汇编程序，使得执行该汇编程序的CPE（每元素周期数）最小。

在实验开始前，有一个需要注意的地方。在实验过程中，如果你的系统没有配置Tcl/Tk的话，需要将实验中各目录下的 `Makefile` 中的 `TKLIBS` 和 `TKINC` 注释掉，以取消Tcl/Tk模式，防止报错。

## Part A

在本部分中，需要利用Y86-64汇编语言实现三个C语言函数，并利用 `sim/misc` 中针对该指令集设计的编译器和模拟器中运行代码：

### sum_list

```
1 /* sum_list - Sum the elements of a linked list */ 8 long sum_list(list_ptr ls)
2 {
3 	 long val = 0;
4 	 while (ls) {
5 		val += ls->val;
6 		ls = ls->next;
7 	 }
8	 return val;
9 }
```

该函数的功能是对链表元素的值进行求和，需要编写一个循环汇编语句：

```
  1 # Execution begins at address 0
  2         .pos 0
  3         irmovq stack,%rsp
  4         call main
  5         halt
  6 # Sample linked list
  7         .align 8
  8 ele1:
  9         .quad 0x00a
 10         .quad ele2
 11 ele2:
 12         .quad 0x0b0
 13         .quad ele3
 14 ele3:
 15         .quad 0xc00
 16         .quad 0
 17 main:
 18         irmovq ele1,%rdi
 19         call sum_list
 20         ret
 21 # long sum_lis(list_ptr ls)
 22 # ls in %rdi
 23 sum_list:
 24         xorq %rax,%rax
 25         andq %rdi,%rdi
 26         jmp test
 27 loop:
 28         mrmovq (%rdi),%rdx
 29         addq %rdx,%rax
 30         mrmovq 8(%rdi),%rdi
 31         andq %rdi,%rdi
 32 test:
 33         jne loop
 34         ret
 35 # Stack starts here and grows to lower addresses
 36         .pos 0x200
 37 stack:
```

该代码分为以下几个部分：

- 1 - 7行：使用 `.pos 0` 声明了程序段的起始地址为0x0，然后设置栈指针并调用主函数
- 6 - 16行：定义了一个三个元素组成的链表，其中 `.align 8` 指明了链表中数据的对齐长度为8，`.quad` 指明了链表中的值和地址各占8个字节。
- 17 - 20行：主函数中确定了其调用的 `sum_list` 的参数，并调用该函数
- 21 - 34行：一个利用Y86-84指令集编写的简单循环语句，其对链表中元素的值求和并返回。
- 35 - 37行：声明了程序栈的高地址为0x200，并向低地址增长。

### rsum_list

链表求和函数的递归版本，其C语言如下：

```
 1 long rsum_list(list_ptr ls)
 2 {
 3 	 if (!ls)
 4 		return 0;
 5 	 else {
 6 		long val = ls->val;
 7 		long rest = rsum_list(ls->next);
 8 		return val + rest;
 9 	 }
10 }
```

对应的汇编语句如下（在这里删去了与sum_list重复初始化、定义链表部分）：

```
  1 main:   
  2         irmovq ele1,%rdi
  3         call rsum_list
  4         ret
  5 # long rsum_lis(list_ptr ls)
  6 # ls in %rdi
  7 rsum_list:
  8         andq %rdi,%rdi
  9         jne  test
 10         xorq %rax,%rax
 11         ret
 12 test:   
 13         mrmovq (%rdi),%rdx
 14         pushq %rdx
 15         mrmovq 8(%rdi),%rdi
 16         call rsum_list
 17         popq %rdx
 18         addq %rdx,%rax
 19         ret
 20 # Stack starts here and grows to lower addresses
 21         .pos 0x300
 22 stack:  
```

在递归过程中，将当前的链表结点的值传入 `%rdx` ，并压入栈中进行保存，并把下一个链表的地址传入 `%rdi` 作为子函数的参数执行，将执行结果与保存的 `%rdx` 相加并返回。值得注意的是，由于递归程序需要更多的栈空间，因此需要为该程序分配更大的栈地址，在这里设置其为0x300。

### copy_block

该函数的功能是将数组 `src` 处存放的 `len`个元素复制到 `dest`，并返回 `src` 中各元素相互异或的值。

```
 1 /* copy_block - Copy src to dest and return xor checksum of src */
 2 long copy_block(long *src, long *dest, long len)
 3 {
 4 	 long result = 0;
 5   while (len > 0) {
 6   	long val = *src++;
 7   	*dest++ = val;
 8 		result ˆ= val;
 9 		len--;
10   }
11   return result;
12 }
```

其对应的汇编语句如下（同样删去了与sum_list相同的初始化和定义栈地址部分），是一个与sum_list类似的循环汇编语句，在这里省略对其的描述：

```
  1 # Source block
  2         .align 8
  3 src:    
  4         .quad 0x00a
  5         .quad 0x0b0
  6         .quad 0xc00
  7 # Destination block
  8 dest:
  9         .quad 0x111
 10         .quad 0x222
 11         .quad 0x333
 12 main:   
 13         irmovq src,%rdi
 14         irmovq dest,%rsi
 15         irmovq $3,%rdx
 16         call   copy_block
 17         ret
 18 # long copy_blcok(long *src,long *dest,long len)
 19 # src in %rdi,dest in %rsi,len in %rdx
 20 copy_block:
 21         xorq   %rax,%rax
 22         andq   %rdx,%rdx
 23         jmp    test
 24 loop:   
 25         mrmovq (%rdi),%rbx
 26         iaddq  $8,%rdi
 27         rmmovq %rbx,(%rsi)
 28         iaddq  $8,%rsi
 29         xorq   %rbx,%rax
 30         iaddq  $-1,%rdx
 31 test:
 32         jg loop
 33         ret
```

在编写完汇编代码后，可以用 `sim/misc` 中的编译器 `yas` 和模拟器 `yis` 编译运行代码，作为示例在这里展示对于 `rsum_list` 的运行过程及结果：

```
$ ./yas rsum.ys
$ ./yis rsum.yo
Stopped in 37 steps at PC = 0x13.  Status 'HLT', CC Z=0 S=0 O=0
Changes to registers:
%rax:	0x0000000000000000	0x0000000000000cba
%rdx:	0x0000000000000000	0x000000000000000a
%rsp:	0x0000000000000000	0x0000000000000300

Changes to memory:
0x02c0:	0x0000000000000000	0x0000000000000089
0x02c8:	0x0000000000000000	0x0000000000000c00
0x02d0:	0x0000000000000000	0x0000000000000089
0x02d8:	0x0000000000000000	0x00000000000000b0
0x02e0:	0x0000000000000000	0x0000000000000089
0x02e8:	0x0000000000000000	0x000000000000000a
0x02f0:	0x0000000000000000	0x000000000000005b
0x02f8:	0x0000000000000000	0x0000000000000013
```

值得注意的是，检验三个函数是否编写正确的标准是观察 `%rax` 中存放的返回值是否为 `0xcba`。

## Part B

在本部分中，需要在原有的Y86-84指令集的顺序实现SEQ中增添 `iaddq V, rB` 指令，其功能是将一个立即数与寄存器做加运算，并将运算结果储存在寄存器中，从而改善了原有指令集中仅能进行寄存器间运算的狭隘性。

为了在指令集中增添该指令，我们需要按照书中构造指令顺序实现的方式，将该指令分为**取指、译码、执行、访存、写回、更新PC**六个部分，如下所示：

```
           0       1        2            9  
iaddq V,rB | c | 0 | F | rB |       V   |
-----------------------------------------
phase     |        calculate            |
-----------------------------------------
          | icode : ifun <-- M1[pc]     |
fetch     | rA : rB      <-- M1[pc+1]   |
  	  | valC     <-- M8[pc+2]       |
  	  | valP     <-- pc + 10        |
————————---------------------------------
decode    | valB         <-- R[rB]      |
-----------------------------------------
execute   | valE         <-- valB + valC|
	  | set CC                      |
-----------------------------------------
write back| R[rB]        <-- valE       |
-----------------------------------------
pc update | pc           <-- valP       |
-----------------------------------------
```

仿照书中的 `IOP` 语句即可写出 `iaddq` 的顺序实现模型，需要注意的原则有以下几条：

* 取指阶段：用于解析指令的指令代码、指令功能，并取出可能的寄存器操作指示符；如指令中存在立即数，在valC中存放指令中的立即数；valP中需存放顺序执行过程中下一条指令的PC值，即当前指令值加指令长度；
* 译码阶段：读入取值过程中读入的寄存器字段对应的寄存器值，其中rA的值需存放在valA中、rB的值需存放在valB中；
* 执行阶段：使用ALU进行所需的一切运算，将运算结果存放在valE中。值得注意的是本指令也是一个类似于` IOP ` 的算数运算指令，因此需要对条件码进行设置
* 访存阶段：将数据存入内存或从内存中读出内存，读出的值应当存放在valM中，本指令不需要该阶段
* 写回阶段 ：将运算结果写回寄存器
* 更新PC 阶段：即更新PC值为valP值（对于异常控制流指令应当是另外的值）。

在分析完该指令各阶段的任务后，仅需在 `sim\seq\seq-full.hcl` 中增加 `iaddq` 相关的部分即可，应当添加的部分如下：

```
107 bool instr_valid = icode in
108     { INOP, IHALT, IRRMOVQ, IIRMOVQ, IRMMOVQ, IMRMOVQ,
109            IOPQ, IJXX, ICALL, IRET, IPUSHQ, IPOPQ, IIADDQ}; ## insert IIADDQ
```

在 `instr_valid` 字段的判断集合中加入`IIADDQ` （为该指令的宏表示），以代表该指令为合法指令。

---

```
112 bool need_regids =
113     icode in { IRRMOVQ, IOPQ, IPUSHQ, IPOPQ,
114              IIRMOVQ, IRMMOVQ, IMRMOVQ, IIADDQ }; ## insert IIADDQ
```

在 `need_regids` 的判断集合中加入 `IIADDQ` ，表示该指令需要寄存器字段

---

```
117 bool need_valC =
118     icode in { IIRMOVQ, IRMMOVQ, IMRMOVQ, IJXX, ICALL,IIADDQ }; ## insert IIADDQ
```

在 `need_valC` 的判断集合中加入 `IIADDQ`  ，表示该指令需要一个立即数字段

---

```
130 word srcB = [
131     icode in { IOPQ, IRMMOVQ, IMRMOVQ, IIADDQ  } : rB; ## insert IIADDQ
132     icode in { IPUSHQ, IPOPQ, ICALL, IRET } : RRSP;
133     1 : RNONE;  # Don't need register
134 ];
```

在译码阶段中，声明valB需要被赋以rB所对应的寄存器的值。

---

```
137 word dstE = [
138     icode in { IRRMOVQ } && Cnd : rB;
139     icode in { IIRMOVQ, IOPQ, IIADDQ} : rB; ## insert IIADDQ
140     icode in { IPUSHQ, IPOPQ, ICALL, IRET } : RRSP;
141     1 : RNONE;  # Don't write any register
142 ];
```

在写回阶段中，声明valE是要写入至rB所对应的寄存器中的值

---

```
153 word aluA = [
154     icode in { IRRMOVQ, IOPQ } : valA;
155     icode in { IIRMOVQ, IRMMOVQ, IMRMOVQ,IIADDQ } : valC; ## insert IIADDQ
156     icode in { ICALL, IPUSHQ } : -8;
157     icode in { IRET, IPOPQ } : 8;
```

在执行阶段中，立即数的值是传入至ALU的前一个值

---

```
162 word aluB = [
163     icode in { IRMMOVQ, IMRMOVQ, IOPQ, ICALL,
164               IPUSHQ, IRET, IPOPQ, IIADDQ } : valB; ## insert IIADDQ
165     icode in { IRRMOVQ, IIRMOVQ } : 0;
```

在执行阶段中，valB的值是传入至ALU的后一个值

----

```
176 bool set_cc = icode in { IOPQ,IIADDQ }; ## insert IIADDQ
```

在执行阶段中，需要更新条件码。

---

在对 `seq-full.hcl` 文件更改完成后，在终端输入 `cd ../ptest;make SIM=../seq/ssim TFLAGS=-i` 以检验是否成功添加了 `iaddq` 指令。以下的输出代表了成果添加：

```
./optest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 58 ISA Checks Succeed
./jtest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 96 ISA Checks Succeed
./ctest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 22 ISA Checks Succeed
./htest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 756 ISA Checks Succeed
```

## Part C

在该部分，需要对 `sim/pipe` 中的代码 `ncopy.ys` 和流水线形式的Y86-84指令集实现 `pipe-full.hcl` 进行改进，以尽可能的减少代码运行所需的CPE（每指令平均周期数）。

其中 `ncopy.ys` 的功能是将一个数组中的元素复制到另一个数组中，并返回数组中的正数数量，其C语言形式：

```
1 /* 
2 * ncopy - copy src to dst, returning number of positive ints
3 * contained in src array.
4 */ 
5 word_t ncopy(word_t *src, word_t *dst, word_t len)
6 { 
7   word_t count = 0;
8   word_t val;
9
10   while (len > 0) {
11   	val = *src++;
12 		*dst++ = val;
13 		if (val > 0)
14 			count++;
15 		len--;
16   }
17   return count;
18 }
```

在该部分中的一些必要命令如下：

* 每次对指令集实现 ` pipe-full.hcl` 进行更改之后，需要输入`make psim VERSION=full` 对编译器和执行器重新编译连接。

* 每次对代码 `ncopy.ys` 进行更改之后，需要输入 `make drivers` 对其重新编译连接。

* 当你想要对你的改进版本进行测试的时候，输入 `./correctness.pl` 以验证你的版本的正确性。当版本的功能正确时，可以得到以下输出：

  ```
  0	OK
  1	OK
  ...
  256	OK
  68/68 pass correctness test
  ```

* 当你已经验证你的版本可以实现该函数的正常功能后，输入 `.benchmark.pl` 以测试你的版本的性能，并得到相应的分数。对于最初的版本，会得到如下的输出：

  ```
  0	13
  1	29	29.00
  ...
  64	913	14.27
  Average CPE	15.18
  Score	0.0/60.0
  
  ```

  可以看出最初版本的平均CPE是15.18，得分为0分。

首先，我们来观察最初版本的 `ncopy.ys` ：

```
  1     xorq %rax,%rax      # count = 0;
  2     andq %rdx,%rdx      # len <= 0?
  3     jle Done        # if so, goto Done:
  4 
  5 Loop:   mrmovq (%rdi), %r10 # read val from src...
  6     rmmovq %r10, (%rsi) # ...and store it to dst
  7     andq %r10, %r10     # val <= 0?
  8     jle Npos        # if so, goto Npos:
  9     irmovq $1, %r10
 10     addq %r10, %rax     # count++
 11 Npos:   irmovq $1, %r10
 12     subq %r10, %rdx     # len--
 13     irmovq $8, %r10
 14     addq %r10, %rdi     # src++
 15     addq %r10, %rsi     # dst++
 16     andq %rdx,%rdx      # len > 0?
 17     jg Loop         # if so, goto Loop:
 18 Done:
 19 ret
```

可以看出该实现中没有使用 `iaddq` 指令，这导致了额外的寄存器被使用，以及额外的指令数目，在这里将其改为使用 `iaddq` 的实现：

```
 1     xorq %rax,%rax      # count = 0;
 2     andq %rdx,%rdx      # len <= 0?
 3     jle Done        # if so, goto Done:
 4 
 5 Loop:   mrmovq (%rdi), %r10 # read val from src...
 6     rmmovq %r10, (%rsi) # ...and store it to dst
 7     andq %r10, %r10     # val <= 0?
 8     jle Npos        # if so, goto Npos:
 9     iaddq $1, %rax      # count++
10 Npos:   irmovq $1, %r10 
11     iaddq $8, %rdi      # src++
12     iaddq $8, %rsi      # dst++
13     iaddq $-1, %rdx     # len--, len > 0?
14     jg Loop         # if so, goto Loop:
15 Done:
16     ret
17 End
```

值得注意的是，需要在 `pipe-full.hcl` 中添加 `iaddq` 的实现，该部分在 `Part B` 中已经实现了，其在流水线实现方式下的实现差别不大，在这不进行赘述。在进行了上述改进后，运行`./correctness.pl`验证实现的正确性，并运行 `./benchmark.pl` 测试该实现性能，得到如下结果：

```
Average CPE	12.70
Score	0.0/60.0
```

可以看出CPE有了一定的改善，但效果不大，得分仍是0分。我们再次对 `ncopy.ys` 进行改进，这次我们利用4×1循环展开的方法对其进行加速：

```
  1 ncopy:
  2         xorq %rax,%rax        # count1 = 0;
  3         iaddq $-3,%rdx        # limit = len -1 <= 0?
  4         jle Done1             # if so, goto Done:
  5 Loop1:  mrmovq (%rdi), %r10   # read val from src...
  6         rmmovq %r10, (%rsi)   # ...and store it to dst
  7         mrmovq 8(%rdi), %r9   # read val from src...
  8         rmmovq %r9, 8(%rsi)   # ...and store it to dst
  9         mrmovq 16(%rdi), %r8  # read val from src...
 10         rmmovq %r8, 16(%rsi)  # ...and store it to dst
 11         mrmovq 24(%rdi), %r11 # read val from src...
 12         rmmovq %r11, 24(%rsi) # ...and store it to dst
 13         andq %r10, %r10       # val <= 0?
 14         jle Npos1             # if so, goto Npos:
 15         iaddq $1, %rax        # count++
 16 Npos1:  
 17         andq %r9, %r9         # val <= 0?
 18         jle Npos2             # if so, goto Npos:
 19         iaddq $1, %rax        # count++
 20 Npos2:  
 21         andq %r8, %r8         # val <= 0?
 22         jle Npos3             # if so, goto Npos:
 23         iaddq $1, %rax        # count++
 24 Npos3:  
 25         andq %r11, %r11       # val <= 0?
 26         jle Npos4             # if so, goto Npos:
 27         iaddq $1, %rax        # count++
 28 Npos4:  
 29         iaddq $32, %rdi       # src += 32
 30         iaddq $32, %rsi       # dst += 32
 31         iaddq $-4, %rdx       # len -= 4,len > 0 ?
 32         jg Loop1              # if so, goto Loop:
 33 Done1:  
 34         iaddq $3,%rdx
 35         jle Done2
 36 Loop2:  
 37         mrmovq (%rdi), %r10   # read val from src...
 38         rmmovq %r10, (%rsi)   # ...and store it to dst
 39         andq %r10, %r10       # val <= 0?
 40         jle Npos5             # if so, goto Npos:
 41         iaddq $1, %rax        # count++
 42 Npos5:  
 43         iaddq $8, %rdi        # src += 32
 44         iaddq $8, %rsi        # dst += 32
 45         iaddq $-1, %rdx       # len -= 4
 46         jg Loop2               # if so, goto Loop:
 47 Done2: 
 48         ret
 49 End:
```

通过循环展开改进后，再次运行测试程序验证版本性能及正确性，得到如下性能结果：

```
Average CPE	9.30
Score	23.9/60.0
```

可以看出循环展开的方法有效的提升了程序的性能，终于得到了大于零的分数。在这里，我们已经对 `ncopy.ps`进行了充分的改进，我们需要思考如何对指令集实现 `pipe-full.hcl` 进行进一步的改进。通过阅读CSAPP可以得到，对于指令集实现CPI性能的改进，首先应该考虑对于分支预测的逻辑判断进行改进，在这里我们观察原版本的分支预测逻辑：

```
180 # Predict next value of PC
181 word f_predPC = [
182     f_icode in { IJXX, ICALL } : f_valC;
183     1 : f_valP;
184 ];
```

可以看出，原版本的预测逻辑是：对于跳转指令，总是选择跳转。这一逻辑对于循环语句来说是合理的，但是对于`if` 语句，尤其是汇编代码最开始判断数组元素个数是否大于零的时候，该跳转总是会得到错误的结果。因此我们将预测逻辑改为：对于循环指令跳转地址小于下一条指令地址，进行跳转，对于条件判断指令（一般是跳转地址大于下一条指令地址的），不进行跳转。逻辑语句如下：

```
181 word f_predPC = [
182     f_icode in { IJXX, ICALL } && (f_valC >= f_valP) : f_valC;
183     1 : f_valP;
184 ];
```

再次运行测试程序验证版本性能及正确性，得到以下结果：

```
Average CPE	3.50
Score	60.0/60.0
```

至此，我们已经得到了满分，实验结束。

