# Cache Lab

本实验由两部分组成，第一个部分要求我们编写一个基于LRU替换策略的cache模拟器，以模拟在经历一系列内存读取/存储任务时的cache命中、不命中及替换行为；第二个部分要求我们对矩阵转置传递函数进行优化，优化目标为尽可能低的cache不命中率。

## Part A

本部分的工作是设计一个基于LRU替换策略的cache模拟器，以模拟一系列内存访存任务时的cache行为。其中具体的内存访存任务存放在 `.trace` 格式的内存访存轨迹文件中，该文件中存放以下格式的字符串：

```
I 0400d7d4,8
 M 0421c7f0,4 
 L 04f6b868,8
 S 7ff0005c8,8
...
[space]operation address,size
```

其中 `operation` 代表了本次访存任务的类型：`I` 代表指令访存，其字符串的首个字符不为 `\0` ，且根据任务要求，本部分中不需要对指令访存进行处理；`L`、`S` 代表数据的加载和存储；`M` 代表紧挨着的对同一数据的加载并存储。`address` 代表访存地址的16进制字符串表示。`size` 表示该次访存的访存长度，由于任务要求中假设地址被完美的对其，不用考虑访存跨页现象，因此本部分中不需要对`size`进行处理。

cache模拟器以如下的形式在终端被调用：

`Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>`

*  `-h`：输出该帮助信息
*  `-v`：模拟中输出内存访存轨迹信息
*  `-s <s>`：组索引的二进制位数
*  `-E <E>`：每个组中的行数
*  `-b <b>`：块偏移数的二进制位数
*  `-t <tracefile>`：需要跟踪的内存访存轨迹文件的文件路径

本部分的主要难点在于以下两个方面：

* 命令行参数的读取；
* 对LRU替换策略的实现方式。

对于命令行参数的读取，可以通过 `getopt` 系列函数实现。对于LRU替换策略，在这里采用双向链表的方式实现，具体实现方式见代码描述：

```
  1 #include "cachelab.h"
  2 #include <getopt.h>
  3 #include <stdlib.h>
  4 #include <stdio.h>
  5 #include <string.h>
  6 #include <stdarg.h>
  7 #include <unistd.h>
  8 #include <stdbool.h>
  9 #include <math.h>
 10 
 11 #define MAXLINE 100
 12 int s,S,E,b;
 13 
 14 /*
 15  * error print and exit
 16  */
 17 void err_sys(const char *msg,...){
 18   char buf[MAXLINE];
 19   va_list ap;
 20   va_start(ap,msg);
 21   vsnprintf(buf,MAXLINE-1,msg,ap);
 22   va_end(ap);
 23   strcat(buf,"\n");
 24   fputs(buf,stderr);
 25   fflush(NULL);
 26   exit(0);
 27 }
```

**1 - 27行**：首先是头文件的导入以及一些全局变量（cache的组数、行数、块大小）和宏的定义，`err_sys` 是仿照APUE设计的异常处理函数，可以在主函数代码中以一行的长度插入。

```
 28 /*
 29  * the hex's string to int
 30  */
 31 int str2int(char* str){
 32   int len = strlen(str);
 33   int ret = 0,p = 1;
 34   for(int i=len-1;i>=0;i--){
 35     int val = 0;
 36     if(str[i] >= 'a' && str[i] <= 'f')
 37       val = str[i] - 'a' + 10;
 38     else
 39       val = str[i] - '0';
 40     ret += val*p;
 41     p *= 16;
 42   }
 43   return ret;
 44 }
 45 /*
 46  * simulate the cache with a queue
 47  */
 48 typedef struct cache_line{
 49   int tag;
 50   struct cache_line* prev;
 51   struct cache_line* next;
 52 } cache_line;
 53 typedef struct cache_set{
 54   int len;
 55   cache_line* head;
 56   cache_line* tail;
 57 } cache_set;
 58 /*
 59  *  queue insert
 60  */
 61 void cache_insert(cache_set* set,cache_line* ptr){
 62   if(set->len == 0){
 63     set->tail = ptr;
 64     set->head = ptr;
 65     return;
 66   } 
 67   set->tail->next = ptr;
 68   ptr->prev = set->tail;
 69   ptr->next = NULL;
 70   set->tail = ptr;
 71 } 
```

**28 - 71行**：`str2int` 是一个将字符串表示16进制数转化为 `int` 型数值的辅助函数；`cache_set` 为一个双向链表，是对cache中组的模拟形式，`cache_line` 为该双向链表中的结点，是对cache中行的模拟形式。`cache_insert` 是双向链表中的尾插函数。

```
 72 /*
 73  * return 0 : hit, return 1 : miss, return 2 : miss eviction
 74  */
 75 int cache_operate(cache_set* set,int address_tag){
 76   for(cache_line* ptr = set->head ; ptr!=NULL ; ptr = ptr->next){
 77     if(ptr->tag == address_tag){
 78       // cache hit : reset the line to the tail of queue
 79       if(ptr != set->tail){
 80         ptr->next->prev = ptr->prev;
 81         if(ptr == set->head)
 82           set->head = set->head->next;
 83         else
 84           ptr->prev->next = ptr->next;
 85         cache_insert(set,ptr);
 86       }
 87       return 0;
 88     }
 89   }
 90   // cache miss : insert the line to the tail of queue;
 91   cache_line* tmp = malloc(sizeof(cache_line));
 92   tmp->next = NULL;
 93   tmp->prev = NULL;
 94   tmp->tag = address_tag;
 95   cache_insert(set,tmp);
 96   set->len++;
 97   // if queue is full, do cache eviction : remove the head of queue
 98   if(set->len > E){
 99     tmp = set->head;
100     set->head = set->head->next;
101     set->head->prev = NULL;
102     free(tmp);
103     return 2;
104   }
105   return 1;
106 }
107 /*
108  * cache_set initialze
109  */
110 void cache_init(cache_set* set){
111   set->len = 0;
112   set->head = NULL;
113   set->tail = NULL;
114 }
```

**72 - 114行**：`cache_init` 为 `cache_set` 的构造函数。`cache_operate` 为cache行为模拟的主函数，当一次内存访存操作导致cache命中、不命中、不命中且替换时分别返回0、1、2。其输入 `set` 为 `main`函数中一次内存访存地址的组号所对应的双向链表、`address_tag` 为该内存访存地址的标记号，简称访存标记号。首先，函数寻找是否有与访存标记号相对应的cache行，如果存在相应的cache行，则代表cache命中，将该行移到双向链表的尾端并返回0代表cache命中；如果不存在，则代表cache不命中，需要将此访存记录行插入至链表尾端，当该链表的长度在插入后大于E时，删除链表头的行结点，并返回2代表cache不命中且替换；当链表长度在插入后不大于E时，返回1代表cache不命中。

```
115 int main(int argc,char *argv[])
116 {
117   // initialize the option arguments
118   char buf[MAXLINE];
119   int c;
120   FILE *fp;
121   bool flagV = false;
122   char filename[MAXLINE];
123   opterr = 0;
124   while((c = getopt(argc,argv,"hvs:E:b:t:")) != -1){
125       switch (c) {
126         case 'h':
127           if((fp = fopen("./help.txt","r")) == NULL)
128             err_sys("fopen the help.txt fail");
129           while(fgets(buf,MAXLINE,fp) != NULL){
130             if(fputs(buf,stdout) == EOF)
131               err_sys("output error");
132           }
133           exit(0);
134         case 'v':
135           flagV = true;
136           break;
137         case 's':
138           s = atoi(optarg);
139           S = pow(2,s);
140           break;
141         case 'E':
142           E = atoi(optarg);
143           break;
144         case 'b':
145           b = atoi(optarg);
146           break;
147         case 't':
148           strncpy(filename,optarg,sizeof(filename)-1);
149           break;
150         case '?':
151           err_sys("unrecognized option");
152       }
153   }
```

**115 - 153行**：主函数的首部分为对命令行选项参数进行解析，其中 `getopt` 函数的功能为：当存在下一个形式为`-<option>` 的命令行选项参数时，返回 `option` ，当不存在下一个命令行参数时，返回-1。该函数的第三个参数为可选的命令行选项参数对应的字符，当字符后存在 `:` 时代表该选项需要一个参数。将 `opterr` 置为0表示不需要`getopt`函数中的异常处理字符串。

​		利用 `switch` 语句对读入的命令行选项参数进行分类，当读入 `-h` 时，返回 `help.txt` 中存放的对于该命令的描述信息并退出进程；当读入 `-v`  时，代表本命令需要输出每次内存访存的信息和对应的cache行为；当读入`-s`、`-E`、`-b` 时，利用全局变量 `optarg`得到选项对应的参数并存储；当读入 `-t` ，将 `optarg` 存放的内存访存轨迹文件的路径存在 `filename` 中。 

```
154   int hits = 0,misses = 0,eviction = 0;
155   if((fp = fopen(filename,"r")) == NULL)
156       err_sys("open trace file fail");
157   //initialze the cache
158   cache_set* cache = calloc(S,sizeof(cache_set));
159   for(int i=0;i<S;i++){
160     cache_init(&cache[i]);
161   }
```

**154 - 161行**：创建保存cache命中、不命中、替换次数的局部变量，并打开内存访存轨迹文件的流描述符，创建大小为组数的双向链表数组以模拟整个cache，并对其进行初始化。

```
162   //simulate each cache operation
163   while(fgets(buf,MAXLINE,fp) != NULL){
164     if(buf[0] != ' ')
165       continue;
166     int len = strlen(buf);
167     buf[len-1] = '\0';
168     if(flagV) printf("%s",buf+1);
169     //split the operation,tag and set index
170     char operation = buf[1];
171     int address = str2int(strtok(buf+3,","));
172     int address_tag = address >> (s+b);
173     int set_mask = 1;
174     for(int i=1;i<s;i++)
175       set_mask = 1 | (set_mask<<1);
176     int address_set = (address >> b) & set_mask;
177     printf(" set:%d,tag:%d",address_set,address_tag);
```

**162 - 177行**：每次读入一行内存访存轨迹文件中的字符串，当字符串的首个字符为 `\0` 时，表示该次访存为指令访存，不需要对其进行模拟；接下来，从字符串中分割出访存方式、访存地址，并根据对应的组索引位数和块偏移量位数提取出访存地址中的组号和标记值，并将组号和标记值输出至标准输出（这一输出对下一部分的工作有很大帮助）。

```
178     //simulate the operation and log the info
179     switch(cache_operate(&cache[address_set],address_tag)){
180       case 1:
181         misses++;
182         if(flagV) printf(" miss");
183         break;
184       case 0:
185         hits++;
186         if(flagV) printf(" hit");
187         break;
188       case 2:
189         misses++;
190         eviction++;
191         if(flagV) printf(" miss eviction");
192         break;
193     }
194     if(operation == 'M'){
195       hits++;
196       if(flagV) printf(" hit");
197     }
198     if(flagV) printf("\n");
199   }
200   //handin the info
201   printSummary(hits, misses, eviction);
202   return 0;
203 }
```

**178 - 199行**：调用`cache_operate`函数并根据返回值得到此次内存访存操作导致的cache行为，对cache行为进行记录和输出。值得注意的是，当内存访存方式为 `M` 时，紧随内存加载后的内存访问一定会导致cache命中。最后，调用 `printSummary` 提交并输出此次访存任务的cache行为记录信息。

当编写完cache模拟器后，在终端输入 `make;./test-csim` 以检验模拟器的正确性，当得到以下输出时代表模拟器的功能正确：

```
                        Your simulator     Reference simulator
Points (s,E,b)    Hits  Misses  Evicts    Hits  Misses  Evicts
     3 (1,1,1)       9       8       6       9       8       6  traces/yi2.trace
     3 (4,2,4)       4       5       2       4       5       2  traces/yi.trace
     3 (2,1,4)       2       3       1       2       3       1  traces/dave.trace
     3 (2,1,3)     167      71      67     167      71      67  traces/trans.trace
     3 (2,2,3)     201      37      29     201      37      29  traces/trans.trace
     3 (2,4,3)     212      26      10     212      26      10  traces/trans.trace
     3 (5,1,5)     231       7       0     231       7       0  traces/trans.trace
     6 (5,1,5)  265189   21775   21743  265189   21775   21743  traces/long.trace
    27

TEST_CSIM_RESULTS=27
```

## Part B

本部分需要对矩阵转置传递函数进行优化，以达到在规格为`s = 5,E = 1,b = 5`的cache 中的最低的cache不命中率，并将优化后的函数存放在`transpose_submit` 中进行性能和正确性测试。值得注意的是，本部分中可以保存多个版本的矩阵转置传递函数，当在`registerFunctions()`调用`registerTransFunction(trans, trans_desc)`后（其中`trans`为定义在函数定义前，用于标记对应函数的标记字符串；`trans_desc`为矩阵转置传递函数的一个版本）将在执行测试命令时同时测试该版本的正确性和性能。

本部分中有以下命令可以对你编写的矩阵转置传递函数进行测试：

* `./test-trans -M <num> -N <num>` ：对固定规格的矩阵进行测试，将显示所有被登记的版本的正确性和性能；
* `./driver.py`：对`transpose_submit`函数以及上一部分的cache模拟器进行打分，打分的规则是对`32*32`，`64*64`，`61*67`规格的矩阵进行性能和正确性评价；
* `./tracegen -M <num> -N <num>`：当在`test-trans`中发现对某一规格的矩阵的转置错误时，该命令可以输出转置错误的具体位置。
* `./csim -v -s 5 -E 1 -b 5 -t trace.f0`

作为本部分的开始，我们首先对实验给定的cache规格进行分析：首先，cache的行数为1，即本cache为直接映射缓存，任意一次cache替换将导致对应的组中之前存放的cache记录全部被驱逐；并且，cache的块大小为32字节，因此，一个块中可以同时存储8个`int`型数据；cache的总大小为`S*E*B`，即相邻`32*32`字节的两个地址将具有相同的组索引号。

实验中给出的简单矩阵传递转置函数进行分析：

```
 char trans_desc[] = "Simple row-wise scan transpose";
 void trans(int M, int N, int A[N][M], int B[M][N])
 {   
     int i, j, tmp;
     for (i = 0; i < N; i++) {
         for (j = 0; j < M; j++) {
             tmp = A[i][j];
             B[j][i] = tmp;
         }
     }
}
```

可以看出，在简单的版本中，对于A（当`i,j`切换时则为B）的内存加载具有步长为1的良好空间局部性，且对于一个块中的所有元素的访问均在时间上相邻，也具有良好的时间局部性，而对于B的内存存储则具有步长为N的较差的空间局部性，同时B（当`i,j`切换时则为A）具有较差的时间局部性，由于每一次对B的内存存储导致的cache记录都在对该记录的另一次cache命中之前被替换出cache。可以看出，为了提高该函数的性能，需要在尽可能保留A的较好性能的前提下，提高B的时间局部性和空间局部性。因此，在这里我对其进行改进，写出了第一版改进函数：

```
 char trans_desc1[] = "block_size = 8"; 
 void trans1(int M, int N, int A[N][M], int B[M][N])
 {   
     int i,j,ii;
     for(ii = 0;ii < N;ii += 8){
       for(j = 0; j < M;j++){
         for(i = ii; i < N && i < ii+8;i++){
           B[j][i] = A[i][j];
         }
       }
   }
 }
```

其思想正如上文所述，由于一个cache块中最多可以保存8个`int`型数据，因此在对矩阵B中相邻8个元素进行访问后，下一次访问将落在下一个cache块中。因此，我们可以趁这个机会切换到B中的下一行，这使得在保持了对B访问的局部性的前提下提升了对A访问的时间局部性。A的时间局部性表现在：对A的一列8个的元素访问后，将导致cache中存放这8个元素对应的块的信息，步长为8的较短步长访问模式将使得这8个块在被驱逐之前可以被命中。为了方便理解，在这里绘制该版本的访问模式图（循环索引`i`代表了块中的移动）：

![figure1](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab5-cache/figure1.png)

在这里，输入`./driver.py`以检验该版本的性能（`transpose_submit` 仅需调用该函数即可）：

```
Cache Lab summary:
                        Points   Max pts      Misses
Csim correctness          27.0        27
Trans perf 32x32           6.9         8         343
Trans perf 64x64           0.0         8        4723
Trans perf 61x67          10.0        10        1931
          Total points    43.9        53
```

可以看出，该版本下对于`32x32`和`61x67`矩阵均有较好的性能，可奇怪的是对于`64x64`矩阵却有很坏的性能，在这里我们将其暂时放在一遍，先对规格较小的`32x32`矩阵进行观察，调用`./test-trans -M 32 -N 32;./csim -v -s 5 -E 1 -b 5 -t trace.f0`以观察该版本的cache行为：

```
S 0018d12c,1 set:9,tag:1588 miss
L 0018d140,8 set:10,tag:1588 miss
L 0018d124,4 set:9,tag:1588 hit
L 0018d120,4 set:9,tag:1588 hit
L 0010d120,4 set:9,tag:1076 miss eviction
S 0014d120,4 set:9,tag:1332 miss eviction
L 0010d1a0,4 set:13,tag:1076 miss
S 0014d124,4 set:9,tag:1332 hit
L 0010d220,4 set:17,tag:1076 miss
S 0014d128,4 set:9,tag:1332 hit
L 0010d2a0,4 set:21,tag:1076 miss
S 0014d12c,4 set:9,tag:1332 hit
L 0010d320,4 set:25,tag:1076 miss
S 0014d130,4 set:9,tag:1332 hit
L 0010d3a0,4 set:29,tag:1076 miss
S 0014d134,4 set:9,tag:1332 hit
L 0010d420,4 set:1,tag:1077 miss
S 0014d138,4 set:9,tag:1332 hit
L 0010d4a0,4 set:5,tag:1077 miss
S 0014d13c,4 set:9,tag:1332 hit
L 0010d124,4 set:9,tag:1076 miss eviction
S 0014d1a0,4 set:13,tag:1332 miss eviction
L 0010d1a4,4 set:13,tag:1076 miss eviction
S 0014d1a4,4 set:13,tag:1332 miss eviction
L 0010d224,4 set:17,tag:1076 hit
S 0014d1a8,4 set:13,tag:1332 hit
L 0010d2a4,4 set:21,tag:1076 hit
S 0014d1ac,4 set:13,tag:1332 hit
L 0010d324,4 set:25,tag:1076 hit
S 0014d1b0,4 set:13,tag:1332 hit
L 0010d3a4,4 set:29,tag:1076 hit
S 0014d1b4,4 set:13,tag:1332 hit
L 0010d424,4 set:1,tag:1077 hit
S 0014d1b8,4 set:13,tag:1332 hit
L 0010d4a4,4 set:5,tag:1077 hit
S 0014d1bc,4 set:13,tag:1332 hit
L 0010d128,4 set:9,tag:1076 hit
S 0014d220,4 set:17,tag:1332 miss eviction
L 0010d1a8,4 set:13,tag:1076 miss eviction
S 0014d224,4 set:17,tag:1332 hit
L 0010d228,4 set:17,tag:1076 miss eviction
S 0014d228,4 set:17,tag:1332 miss eviction
...
```

首先，我们可以看出，在对A矩阵的一列元素访问后的一系列cache不命中后，存入至cache的信息使得对A的下一列元素的访问cache命中。但是，我们观察到对于对角线上的元素，矩阵A和B相应位置的元素被映射到了同一个组中，因此导致了彼此的驱逐，其原因是A和B矩阵相应位置的元素的地址相差`32*32*4`个字节，正好为cache容量的整数倍，可以联想到`64*64`矩阵中也会导致该问题。在这里，我们利用局部变量存储元素的方法解决该问题，其代码如下：

```
 char transpose_desc2[] = "block size = 8 and register store";
 void trans2(int M, int N, int A[N][M], int B[M][N])
 {
   int i,j,t1,t2,t3,t4,t5,t6,t7,t8;
   for(i = 0;i < N-7;i+=8){
     for(j = 0; j < M;j++){
         t1 = A[i][j];
         t2 = A[i+1][j];
         t3 = A[i+2][j];
         t4 = A[i+3][j];
         t5 = A[i+4][j];
         t6 = A[i+5][j];
         t7 = A[i+6][j];
         t8 = A[i+7][j];
         B[j][i] = t1;
         B[j][i+1] = t2;
         B[j][i+2] = t3;
         B[j][i+3] = t4;
         B[j][i+4] = t5;
         B[j][i+5] = t6;
         B[j][i+6] = t7;
         B[j][i+7] = t8;
     }
   }
   for(;i<N;i++){
     for(j=0;j<M;j++){
       B[j][i] = A[i][j];
     }
   }
}
```

在这里，利用局部变量，将对A的访问和对B的访问分成两个单独的部分，我们运行cache模拟器来观察该版本导致的效果：

```
...
L 0010d124,4 set:9,tag:1076 miss eviction
L 0010d1a4,4 set:13,tag:1076 hit
L 0010d224,4 set:17,tag:1076 hit
L 0010d2a4,4 set:21,tag:1076 hit
L 0010d324,4 set:25,tag:1076 hit
L 0010d3a4,4 set:29,tag:1076 hit
L 0010d424,4 set:1,tag:1077 hit
L 0010d4a4,4 set:5,tag:1077 hit
S 0014d1a0,4 set:13,tag:1332 miss eviction
S 0014d1a4,4 set:13,tag:1332 hit
S 0014d1a8,4 set:13,tag:1332 hit
S 0014d1ac,4 set:13,tag:1332 hit
S 0014d1b0,4 set:13,tag:1332 hit
S 0014d1b4,4 set:13,tag:1332 hit
S 0014d1b8,4 set:13,tag:1332 hit
S 0014d1bc,4 set:13,tag:1332 hit
L 0010d128,4 set:9,tag:1076 hit
L 0010d1a8,4 set:13,tag:1076 miss eviction
L 0010d228,4 set:17,tag:1076 hit
L 0010d2a8,4 set:21,tag:1076 hit
L 0010d328,4 set:25,tag:1076 hit
L 0010d3a8,4 set:29,tag:1076 hit
L 0010d428,4 set:1,tag:1077 hit
L 0010d4a8,4 set:5,tag:1077 hit
...
```

可以看出，这种访问模式使得对角线上元素的互相驱逐之间的访问时间间隔延长了，这使得互相驱逐的行为与B数组中每个8个元素更换块的行为相重合，从而避免了互相驱逐导致的额外cache替换。运行分数测试程序可以得到：

```
Cache Lab summary:
                        Points   Max pts      Misses
Csim correctness          27.0        27
Trans perf 32x32           8.0         8         287
Trans perf 64x64           0.0         8        4611
Trans perf 61x67          10.0        10        1851
          Total points    45.0        53
```

可以看出该版本中对于`64*64`的性能有了一定的改善，且其余两个矩阵均得到了满分，在这里我们调用`./test-trans -M 64 -N 64;./csim -v -s 5 -E 1 -b 5 -t trace.f0`以观察`64*64`矩阵的cache行为：

```
...
L 0010d128,4 set:9,tag:1076 miss eviction
L 0010d228,4 set:17,tag:1076 miss eviction
L 0010d328,4 set:25,tag:1076 miss eviction
L 0010d428,4 set:1,tag:1077 miss eviction
L 0010d528,4 set:9,tag:1077 miss eviction
L 0010d628,4 set:17,tag:1077 miss eviction
L 0010d728,4 set:25,tag:1077 miss eviction
L 0010d828,4 set:1,tag:1078 miss eviction
S 0014d320,4 set:25,tag:1332 miss eviction
...
```

可以看出，数组A中相邻四行的元素被映射到了相同的组号，其原因是相邻四行的元素的地址相差`64*4*4`可以被cache的规格整除，导致了抖动现象。对于该现象的通常解决方法是在A的末尾存放另外的几个元素，但是本实验中明显不允许此操作，因此我们只能降低访问的步长为4，在牺牲对B访问的性能下避免抖动现象。在这里，我们需要思考的是，在步长为4的基础上，我们是否可以改进访问模式来提升A或B的时间局部性，在这里我设计了如下的访问模式：

![figure2](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab5-cache/figure2.png)

在这里，将以上版本中规格为`1*8`的 “元素片” 改变为规格为`8*8`的 “元素块”，在块中以步长为4的访问模式分三阶段进行访问。可以看出，相邻两个阶段之间在步长为4的前提下彼此相邻，因此在保证了不发生抖动的条件下，使得一个 “元素块” 中的访问过程中具有比单纯步长为4的 “元素片” 的访问模式有更好的时间局部性。该版本的代码如下：

```
 char transpose_desc3[] = "block size = 8*8 and register store";
 void trans3(int M, int N, int A[N][M], int B[M][N])
 {
   int i,j,ii,jj,t1,t2,t3,t4;
   for(i = 0;i < N;i+=8){
     for(j = 0;j < M;j+=8){
       for(jj = j;jj < j + 4;jj++){
         t1 = A[i][jj];
         t2 = A[i+1][jj];
        t3 = A[i+2][jj];
         t4 = A[i+3][jj];
         B[jj][i] = t1;
         B[jj][i+1] = t2;
         B[jj][i+2] = t3;
         B[jj][i+3] = t4;
       }
       for(ii = i;ii < i+8;ii++){
         t1 = A[ii][j+4]; t2 = A[ii][j+5];
         t3 = A[ii][j+6]; t4 = A[ii][j+7];
         B[j+4][ii] = t1; B[j+5][ii] = t2;
         B[j+6][ii] = t3; B[j+7][ii] = t4;
       }
       for(jj = j;jj < j + 4;jj++){
         t1 = A[i+4][jj];
         t2 = A[i+5][jj];
         t3 = A[i+6][jj];
         t4 = A[i+7][jj];
         B[jj][i+4] = t1;
         B[jj][i+5] = t2;
         B[jj][i+6] = t3;
         B[jj][i+7] = t4;
       }
     }
   }
}
```

可以证明的是，只有当矩阵的其中一个规格参数为64时，才会导致如上的抖动现象，因此我们在提交版本的函数中根据规格参数调用不同的矩阵转置传递函数：

```
 char transpose_submit_desc[] = "Transpose submission";
 void transpose_submit(int M, int N, int A[N][M], int B[M][N])
 {
  if(N == 64)
    trans3(M,N,A,B);
  else
    trans2(M,N,A,B);
 }
```

运行评分程序可以得到如下结果：

```
Cache Lab summary:
                        Points   Max pts      Misses
Csim correctness          27.0        27
Trans perf 32x32           8.0         8         287
Trans perf 64x64           6.4         8        1443
Trans perf 61x67          10.0        10        1851
          Total points    51.4        53
```

可以看出，`64*64`的性能得到了很大程度的改善，虽然还不能达到满分，但已经是一个比较好的分数了。由于本人近期的时间不是特别充足，对于本实验的分析就先到这里告一段落。



