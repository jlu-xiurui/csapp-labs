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
