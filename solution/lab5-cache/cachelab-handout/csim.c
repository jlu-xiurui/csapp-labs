#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>

#define MAXLINE 100
int s,S,E,b;

/*
 * error print and exit
 */
void err_sys(const char *msg,...){
  char buf[MAXLINE];

  va_list ap;
  va_start(ap,msg);
  vsnprintf(buf,MAXLINE-1,msg,ap);
  va_end(ap);
  strcat(buf,"\n");
  fputs(buf,stderr);
  fflush(NULL);
  exit(0);
}
/*
 * the hex's string to int
 */
int str2int(char* str){
  int len = strlen(str);
  int ret = 0,p = 1;
  for(int i=len-1;i>=0;i--){
    int val = 0;
    if(str[i] >= 'a' && str[i] <= 'f')
      val = str[i] - 'a' + 10;
    else
      val = str[i] - '0';
    ret += val*p;
    p *= 16;
  }
  return ret;
}
/*
 * simulate the cache with a queue
 */

typedef struct cache_line{
  int tag;
  struct cache_line* prev;
  struct cache_line* next;
}cache_line;
typedef struct cache_set{
  int len;
  cache_line* head;
  cache_line* tail;
}cache_set;
/*
 *  queue insert
 */
void cache_insert(cache_set* set,cache_line* ptr){
  if(set->len == 0){
    set->tail = ptr;
    set->head = ptr;
    return;
  }
  set->tail->next = ptr;
  ptr->prev = set->tail;
  ptr->next = NULL;
  set->tail = ptr;
}
/*
 * return 0 : hit, return 1 : miss, return 2 : miss eviction
 */
int cache_operate(cache_set* set,int address_tag){
  for(cache_line* ptr = set->head ; ptr!=NULL ; ptr = ptr->next){
    if(ptr->tag == address_tag){
      // cache hit : reset the line to the tail of queue
      if(ptr != set->tail){
        ptr->next->prev = ptr->prev;
        if(ptr == set->head)
          set->head = set->head->next;
        else
          ptr->prev->next = ptr->next;
        cache_insert(set,ptr);
      }
      return 0;
    }
  }
  // cache miss : insert the line to the tail of queue;
  cache_line* tmp = malloc(sizeof(cache_line));
  tmp->next = NULL;
  tmp->prev = NULL;
  tmp->tag = address_tag;
  cache_insert(set,tmp);
  set->len++;
  // if queue is full, do cache eviction : remove the head of queue
  if(set->len > E){
    tmp = set->head;
    set->head = set->head->next;
    set->head->prev = NULL;
    free(tmp);
    return 2;
  }
  return 1;
}
/*
 * cache_set initialze
 */
void cache_init(cache_set* set){
  set->len = 0;
  set->head = NULL;
  set->tail = NULL;
}
int main(int argc,char *argv[])
{
  // initialize the option arguments
  char buf[MAXLINE];
  int c;
  FILE *fp;
  bool flagV = false;
  char filename[MAXLINE];
  opterr = 0;
  while((c = getopt(argc,argv,"hvs:E:b:t:")) != -1){
      switch (c) {
        case 'h':
          if((fp = fopen("./help.txt","r")) == NULL)
            err_sys("fopen the help.txt fail");
          while(fgets(buf,MAXLINE,fp) != NULL){
            if(fputs(buf,stdout) == EOF)
              err_sys("output error");
          }
          exit(0);
        case 'v':
          flagV = true;
          break;
        case 's':
          s = atoi(optarg);
          S = pow(2,s);
          break;
        case 'E':
          E = atoi(optarg);
          break;
        case 'b':
          b = atoi(optarg);
          break;
        case 't':
          strncpy(filename,optarg,sizeof(filename)-1);
          break;
        case '?':
          err_sys("unrecognized option");
      }
  }
  int hits = 0,misses = 0,eviction = 0;
  if((fp = fopen(filename,"r")) == NULL)
      err_sys("open trace file fail");
  //initialze the cache
  cache_set* cache = calloc(S,sizeof(cache_set));
  for(int i=0;i<S;i++){
    cache_init(&cache[i]);
  }
  //simulate each cache operation
  while(fgets(buf,MAXLINE,fp) != NULL){
    if(buf[0] != ' ')
      continue; 
    int len = strlen(buf);
    buf[len-1] = '\0';
    if(flagV) printf("%s",buf+1);
    //split the operation,tag and set index
    char operation = buf[1];
    int address = str2int(strtok(buf+3,","));
    int address_tag = address >> (s+b);
    int set_mask = 1;
    for(int i=1;i<s;i++)
      set_mask = 1 | (set_mask<<1);
    int address_set = (address >> b) & set_mask;
    printf(" set:%d,tag:%d",address_set,address_tag);
    //simulate the operation and log the info
    switch(cache_operate(&cache[address_set],address_tag)){
      case 1:
        misses++;
        if(flagV) printf(" miss");
        break;
      case 0:
        hits++;
        if(flagV) printf(" hit");
        break;
      case 2:
        misses++;
        eviction++;
        if(flagV) printf(" miss eviction");
        break;
    }
    if(operation == 'M'){
      hits++;
      if(flagV) printf(" hit");
    }
    if(flagV) printf("\n");
  }
  //handin the info
  printSummary(hits, misses, eviction);
  return 0;
}
