# CSAPP-LABS  
_labs中存放了实验的原始数据，solution中存放了实验的解答和笔记，陆续更新中。

### 01 Data Lab  [datalab-note](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab1-data/datalab-note.md) 
本实验需要解决一系列的位运算函数，并通过给定的测试用例，考察了实验者对于整形数据和浮点数据在计算机中存储形式和位运算特点的知识。  

### 02 Bomb Lab [bomblab-note](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab2-bomb/bomblab-note.md)  
本实验由六个子部分组成，在每个子部分中需要输入对应格式的字符串从对应的 phase 函数中安全返回。具体的做法就是利用 gdb 来观察各 phase 的具体细节，从而确定应当输入字符串的格式（一串特定的文字或是以 \0 隔开的若干整数）。
### 03 Attack Lab [attacklab-note](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab3-attack/Attack%20Lab-note.md)  
在本实验中，实验者需要利用缓冲区攻击来破坏原有程序，以执行攻击者程序。本实验中可以进一步巩固汇编代码的阅读和分析能力，以及gdb工具的使用，并编写或组装自己的汇编代码。如果你独立完成了Bomb Lab的全部内容，本实验对于你来说应当不在话下。
### 04 Arch Lab [archlab-note](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab4-arch/Arch%20Lab.md)
本实验主要依托于CSAPP的第四章，为了完成实验，实验者需要对于书中所描述的Y86-84指令集具有较为清晰的理解。实验具体分为三个子部分：
* 利用书中所构造的Y86-64指令集编写汇编语言，以实现等效的三个C语言函数；
* 在书中SEQ（Y86-84处理器的顺序实现）上增加一条额外的指令；
* 优化书中的PIPE（Y86-84处理器的流水线实现）及用其来执行的汇编程序，使得执行该汇编程序的CPI（每指令周期数）最小。
### 05 Cache Lab [cacheLab-note](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab5-cache/Cache%20Lab.md)
本实验由两部分组成，第一个部分要求我们编写一个基于LRU替换策略的cache模拟器，以模拟在经历一系列内存读取/存储任务时的cache命中、不命中及替换行为；第二个部分要求我们对矩阵转置传递函数进行优化，优化目标为尽可能低的cache不命中率。
### 06 Shell Lab [shellLab-note](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab6-shell/Shell%20Lab.md)
本实验需要实现一个简易版本的Unix Shell，该Shell需要具有作业控制、僵死子进程回收、内置命令等基本功能，为了完成本实验，实验者需要对于进程行为、信号处理具有一定的了解。当实验中遇到困难时，可以通过阅读CSAPP第8章（或APUE）得到相应的解决方案。
### 07 Malloc lab
本实验的目的是编写一个动态内存分配器，尽可能的在空间利用率和时间复杂度上达到最优。在实验中的一切数据结构都要存放在堆中，对实验者对于指针的管理能力有一定考验，相较以上几个实验也更加困难。

注：在官网下载的源文件的测试用例tracefile不完整，可以在此实验的solution文件夹处下载完整的tracefile。
