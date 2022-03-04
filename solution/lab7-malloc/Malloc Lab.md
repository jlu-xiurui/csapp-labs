# Malloc Lab

本实验的目的是编写一个动态内存分配器，尽可能的在空间利用率和时间复杂度上达到最优。在实验中的一切数据结构都要存放在堆中，对实验者对于指针的管理能力有一定考验，相较以上几个实验也更加困难。

注：在官网下载的源文件的测试用例`tracefile`不完整，可以在此实验的`solution`文件夹处下载完整的`tracefile`。

### 实现方法及源代码

**1. 内存块的管理方式**：在本实验中利用了分离显示空闲链表的方式管理空闲内存块，将各大小范围的空闲块存放在各自的空闲链表之中，在本实验中设置了9个空闲链表，其大小范围分别为：16-31，32-63，64-127，128-255，256-511，512-1023，1024-2047，2048-4095，4096-无穷大。内存块的格式如下所示：

![figure1](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab7-malloc/figure1.png)

值得注意的是，用于存放前继和后继结点地址的指针被存放在块指针bp后，即工作负载之中。当块未被分配时，该处地址用于存放结点在空闲链表中的前后结点信息；当块被分配时，块从空间链表中被删除，其前后结点信息也就不用被保存了，该处地址直接被用做工作负载。

**2.用于地址运算的宏定义**：

```
 39 #define MAX(x,y) ((x) > (y) ? (x) : (y))
 40 /* rounds up to the nearest multiple of ALIGNMENT */
 41 #define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
 42 
 43 /*Given block ptr bp or ptr p,compute the size,compute size,allocated,next,prev fields*/
 44 #define SIZE(p)  (GET(p) & ~0x7)
 45 #define ALLOC(p) (GET(p) & 0x1)
 46 #define GET_SIZE(bp)  (SIZE((char*)(bp) - 4))
 47 #define GET_ALLOC(bp) (ALLOC((char*)(bp) - 4))
 48 #define GET_NEXT(bp) (GET((char *)(bp)))
 49 #define GET_PREV(bp) (GET((char *)(bp)+4))
 50 
 51 #define CHUNKSIZE (1<<12)
 52 #define PACK(size,alloc) ((size) | (alloc))
 53 #define GET(p) (*(unsigned int *)(p))
 54 #define PUT(p,val) (*(unsigned int *)(p) = (val))
 55 /*Put the next and prev fields*/
 56 #define PUT_NEXT(bp,val) (PUT((char *)(bp),(val)))
 57 #define PUT_PREV(bp,val) (PUT((char *)(bp)+4,(val)))
 58 #define PUT_HEAD(bp,val) (PUT((char*)(bp)-4,(val)))
 59 #define PUT_TAIL(bp,val) (PUT((char*)(bp) + GET_SIZE(bp) - 8,(val)))
 60 /*Given block ptr bp,compute the size,compute address of next and prev blocks*/
 61 #define NEXT_BLKP(bp) ((char*)(bp) + SIZE((char*)(bp) - 4))
 62 #define PREV_BLKP(bp) ((char*)(bp) - SIZE((char*)(bp) - 8))

```

通过如上宏定义，使得可以利用块指针bp来完成对于空闲块的所有操作。值得注意的是，`NEXT_BLKP`和`PREV_BLKP`得到的是在内存空间上紧邻的空闲块，`GET_NEXT`以及其余三个宏的操作对象是空闲块结点在空闲链表中的相邻结点。

**2.内存分配器的初始化**：

```
 89 static char* heap_lists;
 90 int mm_init(void)
 91 {
 92     //printf("\n\nmm_init called\n");
 93     if((heap_lists = mem_sbrk(9*4 + 12)) == NULL)
 94         return -1;
 95     //printf("heap_lists is 0x%x\n",heap_lists);
 96     char* bp = heap_lists;
 97     for(int i = 1;i <= 9;i++,bp += 4){
 98         PUT(bp,0);
 99     }
100     PUT(bp,PACK(8,1));
101     PUT(bp+4,PACK(8,1));
102     PUT(bp+8,PACK(0,1));
103     bp = extend_heap(CHUNKSIZE);
104     //check();
105     return 0;
106 }
```

初始化内存分配器时，在堆中分配N（空闲链表数量）个32位地址指针，作为各空闲链表的哨兵结点。在这里比较巧妙的地方在于，后继结点指针next与块指针bp地址相同，因此各哨兵结点处存放的指针即为哨兵结点的后继结点地址，即可以直接对哨兵结点指针调用`GET_NEXT(list_ptr)`来获取其后继结点地址，这使得空闲链表的插入和删除操作更加简单。

![figure2](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab7-malloc/figure2.png)

同时，为了方便空闲块合并时的边界检查，在初始化内存分配器时同时分配了序言块和结尾块，作用与书中的对应单元相同。

**3.查找空闲链表**：

根据空闲块的大小，寻找相应的空闲链表的哨兵结点

```
148 static void* find_list(size_t size){
149     char* head = heap_lists;
150     int idx = 8,tmp = 4096;
151     while(size < tmp){
152         idx--;
153         tmp /= 2;
154     }
155     return head + 4*idx;
156 }
```

**4.插入和删除空闲块**：

将空闲块插入至相应的空闲链表中，以及将空闲块从空闲链表中删除。这两个操作是显示空闲链表法和隐式空闲链表法之间的最主要的差别，如何合理的在其他函数中安排插入和删除操作是本实验的难点之一。

插入空闲块有两种插入方式：

- 先入后出法：直接将空闲块插入至哨兵结点之后。
- 地址顺序法：将空闲块按块地址顺序从高到低的顺序进行排列，即将空闲块插入至比其块地址低的第一个空闲块之前。

```

134 /*
135  * erase the node pointed by bp from the free list
136  */
137 void erase(void* bp){
138     //printf("erase bp 0x%x,size is %d\n",bp,GET_SIZE(bp));
139     if(GET_PREV(bp) != 0)
140         PUT_NEXT(GET_PREV(bp),GET_NEXT(bp));
141     if(GET_NEXT(bp) != 0)
142         PUT_PREV(GET_NEXT(bp),GET_PREV(bp));
143 
144 }
157 /*
158  * insert the block into the free list
159  */
160
161 static void insert_FILO(void* bp){
162     size_t size = GET_SIZE(bp);
163     //printf("insert block at 0x%x,size is %d\n",bp,size);
164     char* list_ptr = find_list(size);
165     //insert the node at the front of free list
166     if(GET_NEXT(list_ptr) != 0)
167         PUT_PREV(GET_NEXT(list_ptr),bp);
168     PUT_NEXT(bp,GET_NEXT(list_ptr));
169     PUT_NEXT(list_ptr,bp);
170     PUT_PREV(bp,list_ptr);
171 }
172 static void insert_address(void* bp){
173     size_t size = GET_SIZE(bp);
174     char* prev = find_list(size);
175     char* ptr = GET_NEXT(prev);
176     while(ptr != 0 && ptr > bp){
177         prev = ptr;
178         ptr = GET_NEXT(ptr);
179     }
180     if(GET_NEXT(prev) != 0)
181         PUT_PREV(GET_NEXT(prev),bp);
182     PUT_NEXT(bp,ptr);
183     PUT_NEXT(prev,bp);
184     PUT_PREV(bp,prev);
185 }
```

**5.合并空闲块**：

查看被合并空闲块的紧邻内存块的分配状态，并将紧邻的未分配的空闲块合并成一块。当空闲块被合并时，要注意将其从空闲链表中删除。

```
107 /*
108  * coalesce the free block
109  */
110 static void *coalesce(void *bp){
111     bool check_prev = !GET_ALLOC(PREV_BLKP(bp));
112     bool check_next = !GET_ALLOC(NEXT_BLKP(bp));
113     //printf("coalesce called : check_prev = %d,check_next = %d\n",check_prev,check_next);
114     size_t size = GET_SIZE(bp);
115     if(check_next && !check_prev){
116         size += GET_SIZE(NEXT_BLKP(bp));
117         erase(NEXT_BLKP(bp));
118     }
119     else if(!check_next && check_prev){
120         size += GET_SIZE(PREV_BLKP(bp));
121         erase(PREV_BLKP(bp));
122         bp = PREV_BLKP(bp);
123     }
124     else if(check_next && check_prev){
125         size += GET_SIZE(PREV_BLKP(bp)) + GET_SIZE(NEXT_BLKP(bp));
126         erase(PREV_BLKP(bp));
127         erase(NEXT_BLKP(bp));
128         bp = PREV_BLKP(bp);
129     }
130     PUT_HEAD(bp,PACK(size,0));
131     PUT_TAIL(bp,PACK(size,0));
132     return bp;
133 }
```

**6.寻找合适空闲块**：

在空闲链表中寻找满足大小的空闲块并将其分配给用户，如当前链表中无满足大小的空闲块时，则在更大的空闲链表中寻找。在空闲链表中的寻找方式又可以分为两种：

- 首次适配法：当寻找到第一个满足大小的空闲块时，立即返回该空闲块。
- 最佳适配法：当寻找到满足大小的空闲块时，继续搜索该空闲链表，寻找满足大小的更小空闲块，当搜索完该空闲链表时，返回最小的且满足大小的空闲块。

```
224 /*
225  * find the suitable blk
226  */
227 static void* first_fit(size_t size){
228     char* bp;
229     for(char* head = find_list(size);head < heap_lists + 4*9;head += 4){
230         bp = GET_NEXT(head);
231         while(bp != 0){
232             size_t currsize = GET_SIZE(bp);
233             if(currsize >= size){
234                 return bp;
235             }
236             bp = GET_NEXT(bp);
237         }
238     }
239     return NULL;
240 }
241 static void* best_fit(size_t size){
242     char* bp;
243     char* best = NULL;
244     for(char* head = find_list(size);head < heap_lists + 4*9;head += 4){
245         bp = GET_NEXT(head);
246         size_t min_size = 0xffffffff;
247         while(bp != 0){
248             size_t currsize = GET_SIZE(bp);
249             if(currsize >= size){
250                 if(currsize < min_size){
251                     best = bp;
252                     min_size = currsize;
253                 }
254             }
255             bp = GET_NEXT(bp);
256         }
257         if(best != NULL)
258             return best;
259     }
260     return NULL;
261 }
```

**7.放置空闲块**：

在找到合适大小的空闲块后，调用`place`函数对空闲块进行分配，如果剩余部分足够大，则将其插入至空闲列表中。注意需要将被放置的空闲块从空闲链表中删除。

```
205 /*
206  * Alloc the block and insert the remaining part into the free list
207  */
208 void place(void *bp,size_t newsize){
209     size_t currsize = GET_SIZE(bp);
210     erase(bp);
211     if(currsize - newsize >= 16){
212         PUT_HEAD(bp,PACK(newsize,1));
213         PUT_TAIL(bp,PACK(newsize,1));
214         PUT_HEAD((char*)(bp)+newsize,PACK(currsize - newsize,0));
215         PUT_TAIL((char*)(bp)+newsize,PACK(currsize - newsize,0));
216         insert((char*)(bp)+newsize);
217     }
218     else{
219         PUT_HEAD(bp,PACK(currsize,1));
220         PUT_TAIL(bp,PACK(currsize,1));
221     }
223 }
```

**8.分配和释放空闲块**

- 分配空闲块：首先根据所需空间的大小，确定对应的空闲链表，并在该链表中寻找满足大小的空闲块并将其分配给用户，当对于链表中无满足大小的空闲块时，则在更大的空闲链表中寻找。当所有空闲链表中均无满足大小的空闲块时，就扩展足够大小的堆空间将其分配给用户。

```
262 /* 
263  * mm_malloc - Allocate a block by incrementing the brk pointer.
264  *     Always allocate a block whose size is a multiple of the alignment.
265  */
266 void *mm_malloc(size_t size)
267 {
268     char* bp;
269     size_t newsize,extendsize;
270     if(size < 0)
271         return NULL;
272     newsize = ALIGN(16 + size);
273     //printf("malloc %d and newsize is %d\n",size,newsize);
274     //find the block that enough big
275     if((bp = best_fit(newsize)) != NULL){
276         place(bp,newsize);
277         //check();
278         return bp;
279     }
280     //no enough big block,extend the heap
281     extendsize = MAX(CHUNKSIZE,newsize);
282     bp = extend_heap(extendsize);
283     //printf("no suitable blcok and extendheap %d,bp is 0x%x\n",extendsize,bp);
284     place(bp,newsize);
285     //check();
286     //printf("malloc return bp 0x%x size is %d\n",bp,GET_SIZE(bp));
287     return bp;
288 }
```

- 释放空闲块：对块指针进行合并操作，并将合并得到的空闲块插入至空闲链表

```
289 /*
290  * mm_free - Freeing a block,insert the block into the free list.
291  */
292 void mm_free(void *ptr){
293     //printf("free 0x%x,size is %d,alloc is %d\n",(unsigned int)ptr,GET_SIZE(ptr),GET_ALLOC(ptr)    );
294     size_t size = GET_SIZE(ptr);
295     ptr = coalesce(ptr);
296     insert(ptr);
297     //check();
298 }
```

**9.内存块的重分配**

本实验的难点之一，当仅利用`mm_malloc`和`mm_free`函数实现本功能时，得到的内存使用性能非常差。

- 首先对于输入参数的特殊性进行判断，当ptr为空或size为0时，调用`mm_malloc`或`mm_free`函数。
- 当原内存块的大小大于重分配所需大小时，直接原内存块返回即可。值得注意的是，对于本函数将内存块的剩余部分重新插入空闲列表会导致内存使用性能变差，造成的原因可能是对于内存块的分割导致了内部碎片。
- 当原内存块的大小小于重分配所需大小时，尝试将其与紧邻的下一内存块进行合并，如可以合并且合并大小大于重分配所需大小时，返回合并后的空闲块。
- 当无法合并或合并大小不足时，调用`mm_malloc`和`mm_free`函数重新分配内存块并返回用户

```
302 void *mm_realloc(void *ptr, size_t size)
303 {
304     //printf("realloc ptr 0x%x to size %d,oldsize is %d\n",ptr,ALIGN(size + 16),GET_SIZE(ptr));
305     if(ptr == NULL){
306         return mm_malloc(size);
307     }   
308     if(size == 0){
309         mm_free(ptr);
310         return NULL;
311     }   
312     size_t asize = ALIGN(size + 16);
313     size_t oldsize = GET_SIZE(ptr);
314     if(oldsize >= asize)
315         return ptr;
316     if(!GET_ALLOC(NEXT_BLKP(ptr)) && GET_SIZE(NEXT_BLKP(ptr)) + oldsize >= asize){
317         oldsize += GET_SIZE(NEXT_BLKP(ptr));
318         erase(NEXT_BLKP(ptr));
319         PUT_HEAD(ptr,PACK(oldsize,1));
320         PUT_TAIL(ptr,PACK(oldsize,1));
321         return ptr;
322     }   
323     char* newptr = mm_malloc(size);
324     memcpy(newptr,ptr,oldsize-8);
325     mm_free(ptr);
326     return newptr;
327 }   
```

**10.空闲链表的监控函数**

该函数可以打印出所有空闲链表，对于调试代码和检测内存泄漏有一定的帮助

```
 68 /*
 69  * print every free list
 70  */
 71 static void check(){
 72     char *bp = heap_lists;
 73     for(int i = 1;i <= 9;i++,bp += 4){
 74         char* ptr = GET(bp);
 75         if(ptr != 0){
 76             printf("list %d:",i,ptr);
 77             while(ptr != NULL){
 78                 printf("0x%x:%d-->",ptr,GET_SIZE(ptr));
 79                 ptr = GET_NEXT(ptr);
 80             }
 81             printf("\n");
 82         }
 83     }
 84 }
```

### 实验结果

当采用最佳适配法和地址顺序法时，得到的性能最佳，可以看出本实验设计的分配器在时间复杂度上已经达到了较好水平，但在空间利用率上仍有改善空间：：

![figure3](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab7-malloc/figure3.png)

首次适配法+地址顺序法

![figure4](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab7-malloc/figure4.png)

首次适配法+LIFO法

![figure5](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab7-malloc/figure5.png)
