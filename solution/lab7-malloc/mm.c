/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define MAX(x,y) ((x) > (y) ? (x) : (y))
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static char* heap_lists;
/*Given block ptr bp or ptr p,compute the size,compute size,allocated,next,prev fields*/
#define SIZE(p)  (GET(p) & ~0x7)
#define ALLOC(p) (GET(p) & 0x1)
#define GET_SIZE(bp)  (SIZE((char*)(bp) - 20))
#define GET_ALLOC(bp) (ALLOC((char*)(bp) - 20))
#define GET_NEXT(bp) (GET_ADR((char *)(bp) - 8))
#define GET_PREV(bp) (GET_ADR((char *)(bp) - 16))

#define CHUNKSIZE (1<<12)
#define PACK(size,alloc) ((size << 3) | (alloc))
#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p) = (val))
#define GET_ADR(p) (*(unsigned long *)(p))
#define PUT_ADR(p,val) (*(unsigned long *)(p) = (val))
#define ADR_LO ((heap_lists) + 8*1027)
#define ADR_HI ((char *)(mem_heap_hi()))

/*Put the next and prev fields*/
#define PUT_NEXT(bp,val) (PUT_ADR((char *)(bp) - 8,(val)))
#define PUT_PREV(bp,val) (PUT_ADR((char *)(bp) - 16,(val)))
#define PUT_HEAD(bp,val) (PUT((char*)(bp) - 20,(val)))
#define PUT_TAIL(bp,val) (PUT((char*)(bp) + GET_SIZE((char*)(bp) - 20) - 20,(val)))
/*Given block ptr bp,compute the size,compute address of next and prev blocks*/
#define NEXT_BLKP(bp) ((char*)(bp) + SIZE((char*)(bp) - 20))
#define PREV_BLKP(bp) ((char*)(bp) - SIZE((char*)(bp) - 24))

static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void insert(void *bp);
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heap_lists = mem_sbrk(ALIGN(1027*8))) == (void *)-1)
        return -1;
    char* bp = heap_lists;
    for(int i = 1;i <= 1027 ;i++,bp += 8){
        PUT_ADR(bp,0);
    }
    bp = extend_heap(CHUNKSIZE);
    insert(bp);
    return 0;
}
/*
 *
 */
static void *coalesce(void *bp){
    bool check_prev = !GET_ALLOC(PREV_BLKP(bp)) && (PREV_BLKP(bp) > ADR_LO);
    bool check_next = !GET_ALLOC(NEXT_BLKP(bp)) && (PREV_BLKP(bp) < ADR_HI);
    size_t size = GET_SIZE(bp);
    if(check_next && !check_prev){
        size += GET_SIZE(NEXT_BLKP(bp));
        erase(NEXT_BLKP(bp));
        erase(bp);
        insert(bp);
    }
    else if(!check_next && check_prev){
        size += GET_SIZE(PREV_BLKP(bp));
        erase(PREV_BLKP(bp));
        erase(bp);
        insert(PREV_BLKP(bp));
        bp = PREV_BLKP(bp);
    }
    else if(check_next && check_prev){
        size += GET_SIZE(PREV_BLKP(bp)) + GET_SIZE(NEXT_BLKP(bp));
        erase(bp);
        erase(NEXT_BLKP(bp));
        insert(PREV_BLKP(bp));
        bp = PREV_BLKP(bp);
    }
    PUT_HEAD(bp,PACK(size,0));
    PUT_TAIL(bp,PACK(size,0));
    return bp;
}
/*
 * erase the node pointed by bp from the free list
 */
void erase(void* bp){
    if(GET_PREV(bp) != 0)
        PUT_NEXT(GET_PREV(bp),GET_NEXT(bp));
    if(GET_NEXT(bp) != 0)
        PUT_PREV(GET_NEXT(bp),GET_PREV(bp));
    }
/*
 * match and insert the block pointed by bp into the head of suitable block list
 */
static void* find_head(size_t size){
    char* head;
    size -= 24;
    if(size <= 1024)
        head = heap_lists + 8*(size - 1);
    else if(size <= 2048)
        head = heap_lists + 1024*8;
    else if(size <= 4096)
        head = heap_lists + 1025*8;
    else
        head = heap_lists + 1026*8;
    return head;

}
static void insert(void* bp){
    size_t size = GET_SIZE(bp);
    char* head = find_head(size);
    head = GET(head);
    //insert the node at the front of free list
    PUT_NEXT(bp,head);
    PUT_PREV(bp,0);
    PUT_PREV(head,bp);
    PUT(head,bp);
}
static void* extend_heap(size_t size){
    char* bp;
    size = ALIGN(size);
    bp = mem_sbrk(size) + 20;
    PUT_HEAD(bp,PACK(size,0));
    PUT_TAIL(bp,PACK(size,0));
    bp = coalesce(bp);
    insert(bp);
    return bp;
}
void place(void *bp,size_t newsize){
    size_t currsize = GET_SIZE(bp);
    if(currsize - newsize > 24){
        PUT_HEAD(bp,PACK(newsize,1));
        PUT_TAIL(bp,PACK(newsize,1));
        PUT_HEAD((char*)(bp)+newsize,PACK(currsize - newsize,0));
        PUT_TAIL((char*)(bp)+newsize,PACK(currsize - newsize,0));
        insert((char*)(bp)+newsize);
    }
    else{
        PUT_HEAD(bp,PACK(currsize,1));
        PUT_TAIL(bp,PACK(currsize,1));
    }
    erase(bp);

}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    char* bp;
    size_t newsize,extendsize;
    if(size < 0)
        return NULL;
    if(size < ALIGNMENT)
        newsize = ALIGNMENT + 24;
    else
        newsize = ALIGN(24 + size);
    //find the block that enough big
    for(char* head = find_head(newsize);head < ADR_LO;head += 8){
        bp = GET(head);
        while(bp != 0){
            size_t currsize = GET_SIZE(bp);
            if(currsize > newsize){
                place(bp,newsize);
                return bp;
            }
            bp = GET_NEXT(bp);
        }
    }
    //no enough big block,extend the heap
    extendsize = MAX(CHUNKSIZE,newsize);
    bp = extend_heap(extendsize);
    extendsize = GET_SIZE(bp);
    place(bp,newsize);
    return bp;
}
/*
 * mm_free - Freeing a block,insert the block into the free list.
 */
void mm_free(void *ptr){
    PUT_HEAD(ptr,PACK(GET_SIZE(ptr),0));
    PUT_TAIL(ptr,PACK(GET_SIZE(ptr),0));
    insert(ptr);
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














