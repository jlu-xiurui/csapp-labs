# Proxy Lab

本实验需要实验者实现一个Web服务器的简单代理工具，并在实验中尝试和学习比较简单的网络套接字编程、并发编程。在学习理解书中后三章后，相信本实验对你来说一定不是困难。

## 实验代码及讲解

本实验分为三个部分：

- 完成代理工具的基本功能，即转发请求至Web服务器；
- 对代理工具进行优化，使其可以并发的处理客户端请求（生产者-消费者问题）；
- 对代理工具进一步优化，使其具有并发条件下的缓存功能（读写者问题）；

在这里，我将按照这三个部分的顺序，直接对具有完整功能的最终版代码进行讲解：

### main 主函数

```
 42 buf_t poll;
 43 cache_t cache;
 44 int main(int argc,char **argv)
 45 {
 46     int port,listenfd,connfd,clientlen;
 47     struct sockaddr_in clientaddr;
 48     pthread_t tid;
 49     if(argc != 2)
 50         unix_error("proxy <port>\n");
 51     listenfd = Open_listenfd(argv[1]);
 52     buf_init(&poll,POLL_SIZE);
 53     cache_init(&cache);
 54     for(int i = 0; i < POLL_SIZE; i++)
 55         Pthread_create(&tid,NULL,serve,NULL);
 56     while(1){
 57         clientlen = sizeof(clientaddr);
 58         connfd = Accept(listenfd,(SA *)(&clientaddr),&clientlen);
 59         buf_insert(&poll,connfd);
 60     }
 61     buf_free(&poll);
 62     cache_free(&cache);
 63 }

```

定义并初始化线程池、代理缓存，并运行`POLL_SIZE`个工作线程等待为客户端服务，其中`POLL_SIZE`被设置为16。初始化结束后，创建监听套接字，并进入循环等待客户端请求，当接收到客户端请求后，将`Accept`函数返回的已连接套接字输入至线程池，等待工作线程将其提取。在其中`buf_insert`为线程池的“生产者”函数，当线程池中没有空闲线程时，主函数阻塞直到出现空闲工作线程。最后，释放线程池及缓存。

### serve 工作线程函数

```
177 static void serve(void *vargp){
178     pthread_detach(pthread_self());
179     while(1){
180     int listenfd = buf_remove(&poll),clientfd;
181     char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE],line[MAXLINE];
182     rio_t rio;
183     rio_readinitb(&rio,listenfd);
184     rio_readlineb(&rio,buf,MAXLINE);
185     sscanf(buf,"%s %s %s",method,uri,version);
186     char *host = strstr(uri,"//");
187     if(host == NULL)
188         host = uri;
189     else
190         host += 2;
191     char *request = strstr(host,"/");
192     *request = '\0';
193     request += 1;
194     char* port;
195     if(port = strstr(host,":")){
196         *port = '\0';
197         port = port + 1;
198     }
199     else
200         port = "80";
201     cache_node* ptr;
202     char key[MAXLINE];
203     sprintf(key,"GET /%s HTTP/1.0\r\n",request);
204     if(cache_find(&cache,buf,key)){
205         rio_writen(listenfd,buf,MAXLINE);
206     }
207     else{
208         clientfd = open_clientfd(host,port);
209         sprintf(buf,"GET /%s HTTP/1.0\r\n",request);
210         sprintf(buf,"%sHost: %s\r\n%s",buf,host,user_agent_hdr);
211         sprintf(buf,"%sConnection: close\r\nProxy-Connection: close\r\n",buf);
212         rio_readlineb(&rio,line,MAXLINE);
213         while(strcmp(line,"\r\n")){
214             sprintf(buf,"%s%s",buf,line);
215             rio_readlineb(&rio,line,MAXLINE);
216         }
217         sprintf(buf,"%s\r\n",buf);
218         rio_writen(clientfd,buf,MAXLINE);
219         sprintf(buf,"");
220         while(rio_readn(clientfd,line,MAXLINE) > 0){
221             sprintf(buf,"%s%s",buf,line);
222             rio_writen(listenfd,line,MAXLINE);
223         }
224         cache_insert(&cache,buf,key);
225     }
226     close(listenfd);
227     }
```

代理工具的核心函数，为客户端提供代理服务。

**178-181行**：初始化函数的一些局部变量。首先调用`pthread_detach(pthread_self())`使得工作线程分离，以防止内存泄漏，然后进入无限循环。`buf_remove`为线程池的“消费者”函数，当线程池内没有需要被服务的已连接套接字时，工作线程在此阻塞，直到已连接套接字被插入线程池。

**182-200行**：利用书中提供的RIO包读入来自客户端的请求行，并将其分解成method、host、version，以及尝试提取请求行中的端口号，当请求行无端口号时采用默认端口号80。

**201-206行**：利用请求行作为缓存的键，查找缓存中是否存在当前请求，如缓存命中，则直接将缓存中的数据发送至客户端。

**207-227行：**当缓存不命中时，根据客户端请求行确定服务器端口号并请求连接。然后，构造传输至服务器的新请求行和请求报头，并合并客户端发送的其余请求报头，一同发送至服务器。将来自服务器的请求插入至缓存，并发送至客户端。最后，关闭已连接套接字。

### 线程池相关定义及函数

```
  9 typedef struct {
 10     int size;
 11     int *buf;
 12     int front;
 13     int tail;
 14     sem_t items;
 15     sem_t mutex;
 16     sem_t slots;
 17 } buf_t;
 ...
150 static void buf_init(buf_t* p,int n){
151     p->size = n;
152     p->buf = calloc(n,sizeof(int));
153     p->front = p->tail = 0;
154     Sem_init(&p->items,0,0);
155     Sem_init(&p->slots,0,n);
156     Sem_init(&p->mutex,0,1);
157 }
158 static void buf_free(buf_t* p){
159     Free(p->buf);
160 }
```

线程池即一个“生产者-消费者”模型。其中，`size、buf、front、tail`定义了一个循环数组，以存放等待被服务的已连接套接字。`items、slots`分别表示套接字的剩余数量及循环数组的空余容量，分别被初始化为0和`size`。`mutex`作为整个模型的互斥锁，被初始化为1。

```
161 static void buf_insert(buf_t* p,int item){
162     P(&p->slots);
163     P(&p->mutex);
164     p->buf[(p->tail++)%(p->size)] = item;
165     V(&p->mutex);
166     V(&p->items);
167 }
168 static int buf_remove(buf_t* p){
169     P(&p->items);
170     P(&p->mutex);
171     int ret;
172     ret = p->buf[(p->front++)%(p->size)];
173     V(&p->mutex);
174     V(&p->slots);
175     return ret;
176 }
```

线程池的生产者，消费者函数，具体实现原理见书即可。

### 代理缓存及相关函数

```
 18 typedef struct cache_node{
 19     int size;
 20     char* request;
 21     char* item;
 22     struct cache_node* next;
 23     struct cache_node* prev;
 24 }cache_node;
 25 typedef struct cache_t{
 26     int capacity;
 27     sem_t mutex,w;
 28     int readcnt;
 29     struct cache_node* head;
 30     struct cache_node* tail;
 31 }cache_t;
 ...
 65 static void cache_init(cache_t* p){
 66     p->capacity = MAX_CACHE_SIZE;
 67     Sem_init(&p->mutex,0,1);
 68     Sem_init(&p->w,0,1);
 69     p->readcnt = 0;
 70     p->head = NULL;
 71     p->tail = NULL;
 72 }
 73 static void cache_free(cache_t* p){
 74     cache_node* ptr;
 75     for(ptr = p->head;ptr != NULL;ptr = ptr->next){
 76         Free(ptr->request);
 77             Free(ptr->item);
 78     }
 79     Free(p);
 80 }
```

在并发条件下的代理缓存，本质上是一个读写者模型。在这里，利用双向链表来模拟LRU功能的缓存，当缓存中某个结点被读取或写入时，将其从链表中删除并插入至表尾；当缓存空间不足需要驱逐块时，驱逐表头的结点。在这里，用`capacity`显示的存储缓存的剩余容积。`w`作为保护链表的互斥锁、`mutex`作为保护当前读者数`readcnt`的互斥锁。

```
 81 static void list_delete(cache_t* p,cache_node* ptr){
 82     p->capacity += ptr->size;
 83     if(ptr == p->head)
 84         p->head = p->head->next;
 85     if(ptr == p->tail)
 86         p->tail = p->tail->prev;
 87     if(ptr->prev)
 88         ptr->prev->next = ptr->next;
 89     if(ptr->next)
 90         ptr->next->prev = ptr->prev;
 91 }
 92 static void list_insert(cache_t* p,cache_node* ptr){
 93     p->capacity -= ptr->size;
 94     if(p->head == NULL){
 95         p->head = ptr;
 96         p->tail = ptr;
 97     }
 98     else{
 99         ptr->prev = p->tail;
100         p->tail->next = ptr;
101         p->tail = ptr;
102     }
103 }
```

链表的插入与删除操作，在插入和删除时更新缓存的容积。

```
104 static void cache_insert(cache_t* p,char* buf,char* request){
105     size_t buf_size = strlen(buf);
106     if(buf_size > MAX_OBJECT_SIZE)
107         return;
108     size_t request_size = strlen(request);
109     cache_node* node = malloc(sizeof(cache_node));
110     node->size = buf_size;
111     node->request = malloc(request_size + 10);
112     sprintf(node->request,"%s",request);
113     node->item = malloc(buf_size + 10);
114     node->next = NULL;
115     node->prev = NULL;
116     sprintf(node->item,"%s",buf);
117     P(&p->w);
118     cache_node* ptr;
119     for(ptr = p->head;ptr != NULL;ptr = ptr->next){
120         if(!strcmp(request,ptr->request)){
121             list_delete(p,ptr);
122             break;
123         }   
124     }
125     while(p->capacity < buf_size){
126         free(p->head->request);
127         free(p->head->item);
128         list_delete(p,p->head);
129     }   
130     list_insert(p,node);
131     V(&p->w);
132 }
```

缓存的插入函数，也是读写者模型中的写者。每次从服务器中读取响应时，以响应块作为值参数、请求行作为键参数，并调用本函数将其插入至缓存。

**105-107行**：检查响应块的大小，当其超出缓存中单个缓存的最大容积时，将其忽略；

**108-117行**：根据函数参数，初始化缓存块，并对链表加锁；

**119-124行**：检查链表中是否存在与将要插入的缓存块键相同的块，如果有则将其删除。

**125-131行**：当前缓存容积不足以存储该缓存块时，删除表头元素直至容积足够。最后，插入缓存块至表尾，并对链表解锁。

```
133 static int cache_find(cache_t* p,char* buf,char* request){
134     P(&p->mutex);
135     p->readcnt++;
136     if(p->readcnt == 1)
137         P(&p->w); 
138     cache_node* ptr;
139     for(ptr = p->head; ptr!=NULL; ptr = ptr->next){
140         if(!strcmp(request,ptr->request))
141             break;
142     }       
143     int ret = 0;
144     if(ptr != NULL){
145         ret = 1;
146         sprintf(buf,"%s",ptr->item);
147         list_delete(p,ptr);
148         list_insert(p,ptr);
149     }
150     p->readcnt--;
151     if(p->readcnt == 0)
152         V(&p->w);
153     V(&p->mutex);
154     return ret;
155 }
```

缓存的查找并读取函数，当存在对应键的缓存块时返回1，否则返回0；也是读写者模型中的读者。

**134-137行**：对当前读者数量加锁，并加一；如果当前读者是首个进入的读者，则尝试对链表加锁，如存在写者则阻塞；

**138-149行**：查找与请求键相同的缓存块，如查找命中则将其重新插入至表尾，并通过`buf`参数返回缓存块中的响应块；

**150-154行**：对当前读者数量减一，如果当前读者是最后离开的读者，则对链表解锁。最后，对读者数量解锁并返回。

## 实验结果

调用`./driver.sh`测试用例，可以得到以下输出：

```
*** Basic ***
Starting tiny on 9593
Starting proxy on 25815
1: home.html
   Fetching ./tiny/home.html into ./.proxy using the proxy
   Fetching ./tiny/home.html into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
2: csapp.c
   Fetching ./tiny/csapp.c into ./.proxy using the proxy
   Fetching ./tiny/csapp.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
3: tiny.c
   Fetching ./tiny/tiny.c into ./.proxy using the proxy
   Fetching ./tiny/tiny.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
4: godzilla.jpg
   Fetching ./tiny/godzilla.jpg into ./.proxy using the proxy
   Fetching ./tiny/godzilla.jpg into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
5: tiny
   Fetching ./tiny/tiny into ./.proxy using the proxy
   Fetching ./tiny/tiny into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
Killing tiny and proxy
basicScore: 40/40

*** Concurrency ***
Starting tiny on port 8545
Starting proxy on port 15903
Starting the blocking NOP server on port 9251
Trying to fetch a file from the blocking nop-server
Fetching ./tiny/home.html into ./.noproxy directly from Tiny
Fetching ./tiny/home.html into ./.proxy using the proxy
Checking whether the proxy fetch succeeded
Success: Was able to fetch tiny/home.html from the proxy.
Killing tiny, proxy, and nop-server
concurrencyScore: 15/15

*** Cache ***
Starting tiny on port 29585
Starting proxy on port 30912
Fetching ./tiny/tiny.c into ./.proxy using the proxy
Fetching ./tiny/home.html into ./.proxy using the proxy
Fetching ./tiny/csapp.c into ./.proxy using the proxy
Killing tiny
Fetching a cached copy of ./tiny/home.html into ./.noproxy
Success: Was able to fetch tiny/home.html from the cache.
Killing proxy
cacheScore: 15/15

totalScore: 70/70
```

在浏览器中设置代理并访问服务器时，输出正常：

![figure1](https://github.com/jlu-xiurui/csapp-labs/blob/master/solution/lab8-proxy/figure1.png)



