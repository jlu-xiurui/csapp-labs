#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define POLL_SIZE 16
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
typedef struct {
    int size;
    int *buf;
    int front;
    int tail;
    sem_t items;
    sem_t mutex;
    sem_t slots;
} buf_t;
typedef struct cache_node{
    int size;
    char* request;
    char* item;
    struct cache_node* next;
    struct cache_node* prev;
}cache_node;
typedef struct cache_t{
    int capacity;
    sem_t mutex,w;
    int readcnt;
    struct cache_node* head;
    struct cache_node* tail;
}cache_t;
static void serve(void*);
static void buf_init(buf_t* p,int n);
static void buf_free(buf_t* p);
static void buf_insert(buf_t* p,int item);
static int buf_remove(buf_t* p);
static void cache_init(cache_t* p);
static void list_delete(cache_t* p,cache_node* ptr);
static void list_insert(cache_t* p,cache_node* ptr);
static void cache_insert(cache_t* p,char* buf,char* request);
static int cache_find(cache_t* p,char* buf,char* request);
static void cache_free(cache_t* p);
buf_t poll;
cache_t cache;
int main(int argc,char **argv)
{
    int port,listenfd,connfd,clientlen;
    struct sockaddr_in clientaddr;
    pthread_t tid;
    if(argc != 2)
        unix_error("proxy <port>\n");
    listenfd = Open_listenfd(argv[1]);
    buf_init(&poll,POLL_SIZE);
    cache_init(&cache);
    for(int i = 0; i < POLL_SIZE; i++)
        Pthread_create(&tid,NULL,serve,NULL);
    while(1){
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd,(SA *)(&clientaddr),&clientlen);
        buf_insert(&poll,connfd);
    }
    buf_free(&poll);
    cache_free(&cache);
}
static void cache_init(cache_t* p){
    p->capacity = MAX_CACHE_SIZE;
    Sem_init(&p->mutex,0,1);
    Sem_init(&p->w,0,1);
    p->readcnt = 0;
    p->head = NULL;
    p->tail = NULL;
}
static void cache_free(cache_t* p){
    cache_node* ptr;
    for(ptr = p->head;ptr != NULL;ptr = ptr->next){
        Free(ptr->request);
            Free(ptr->item);
    }
    Free(p);
}
static void list_delete(cache_t* p,cache_node* ptr){
    p->capacity += ptr->size;
    if(ptr == p->head)
        p->head = p->head->next;
    if(ptr == p->tail)
        p->tail = p->tail->prev;
    if(ptr->prev)
        ptr->prev->next = ptr->next;
    if(ptr->next)
        ptr->next->prev = ptr->prev;
}
static void list_insert(cache_t* p,cache_node* ptr){
    p->capacity -= ptr->size;
    if(p->head == NULL){
        p->head = ptr;
        p->tail = ptr;
    }
    else{
        ptr->prev = p->tail;
        p->tail->next = ptr;
        p->tail = ptr;
    }
}
static void cache_insert(cache_t* p,char* buf,char* request){
    size_t buf_size = strlen(buf);
    if(buf_size > MAX_OBJECT_SIZE)
        return;
    size_t request_size = strlen(request);
    cache_node* node = malloc(sizeof(cache_node));
    node->size = buf_size;
    node->request = malloc(request_size + 10);
    sprintf(node->request,"%s",request);
    node->item = malloc(buf_size + 10);
    node->next = NULL;
    node->prev = NULL;
    sprintf(node->item,"%s",buf);
    P(&p->w);
    cache_node* ptr;
    for(ptr = p->head;ptr != NULL;ptr = ptr->next){
        if(!strcmp(request,ptr->request)){
            list_delete(p,ptr);
            break;
        }
    }
    while(p->capacity < buf_size){
        free(p->head->request);
        free(p->head->item);
        list_delete(p,p->head);
    }
    list_insert(p,node);
    V(&p->w);
}
static int cache_find(cache_t* p,char* buf,char* request){
    P(&p->mutex);
    p->readcnt++;
    if(p->readcnt == 1)
        P(&p->w);
    cache_node* ptr;
    for(ptr = p->head; ptr!=NULL; ptr = ptr->next){
        if(!strcmp(request,ptr->request))
            break;
    }
    int ret = 0;
    if(ptr != NULL){
        ret = 1;
        sprintf(buf,"%s",ptr->item);
        list_delete(p,ptr);
        list_insert(p,ptr);
    }
    p->readcnt--;
    if(p->readcnt == 0)
        V(&p->w);
    V(&p->mutex);
    return ret;
}
static void buf_init(buf_t* p,int n){
    p->size = n;
    p->buf = calloc(n,sizeof(int));
    p->front = p->tail = 0;
    Sem_init(&p->items,0,0);
    Sem_init(&p->slots,0,n);
    Sem_init(&p->mutex,0,1);
}
static void buf_free(buf_t* p){
    Free(p->buf);
}
static void buf_insert(buf_t* p,int item){
    P(&p->slots);
    P(&p->mutex);
    p->buf[(p->tail++)%(p->size)] = item;
    V(&p->mutex);
    V(&p->items);
}
static int buf_remove(buf_t* p){
    P(&p->items);
    P(&p->mutex);
    int ret;
    ret = p->buf[(p->front++)%(p->size)];
    V(&p->mutex);
    V(&p->slots);
    return ret;
}
static void serve(void *vargp){
    pthread_detach(pthread_self());
    while(1){
    int listenfd = buf_remove(&poll),clientfd;
    char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE],line[MAXLINE];
    rio_t rio;
    rio_readinitb(&rio,listenfd);
    rio_readlineb(&rio,buf,MAXLINE);
    sscanf(buf,"%s %s %s",method,uri,version);
    char *host = strstr(uri,"//");
    if(host == NULL)
        host = uri;
    else
        host += 2;
    char *request = strstr(host,"/");
    *request = '\0';
    request += 1;
    char* port;
    if(port = strstr(host,":")){
        *port = '\0'; 
        port = port + 1;
    }
    else
        port = "80";
    cache_node* ptr;
    char key[MAXLINE];
    sprintf(key,"GET /%s HTTP/1.0\r\n",request);
    if(cache_find(&cache,buf,key)){
        rio_writen(listenfd,buf,MAXLINE);
    }
    else{
        clientfd = open_clientfd(host,port);
        sprintf(buf,"GET /%s HTTP/1.0\r\n",request);
        sprintf(buf,"%sHost: %s\r\n%s",buf,host,user_agent_hdr);
        sprintf(buf,"%sConnection: close\r\nProxy-Connection: close\r\n",buf);
        rio_readlineb(&rio,line,MAXLINE);
        while(strcmp(line,"\r\n")){
            sprintf(buf,"%s%s",buf,line);
            rio_readlineb(&rio,line,MAXLINE);
        }
        sprintf(buf,"%s\r\n",buf);
        rio_writen(clientfd,buf,MAXLINE);
        sprintf(buf,"");
        while(rio_readn(clientfd,line,MAXLINE) > 0){
            sprintf(buf,"%s%s",buf,line);
            rio_writen(listenfd,line,MAXLINE);
        }
        cache_insert(&cache,buf,key);
    }
    close(listenfd);
    }
}
