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

#define MAX(x,y) ((x) > (y) ? (x) : (y))
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + 7) & ~0x7)

/*Given block ptr bp or ptr p,compute the size,compute size,allocated,next,prev fields*/
#define SIZE(p)  (GET(p) & ~0x7)
#define ALLOC(p) (GET(p) & 0x1)
#define GET_SIZE(bp)  (SIZE((char*)(bp) - 4))
#define GET_ALLOC(bp) (ALLOC((char*)(bp) - 4))
#define GET_NEXT(bp) (GET((char *)(bp)))
#define GET_PREV(bp) (GET((char *)(bp)+4))

#define CHUNKSIZE (1<<12)
#define PACK(size,alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p) = (val))
/*Put the next and prev fields*/
#define PUT_NEXT(bp,val) (PUT((char *)(bp),(val)))
#define PUT_PREV(bp,val) (PUT((char *)(bp)+4,(val)))
#define PUT_HEAD(bp,val) (PUT((char*)(bp)-4,(val)))
#define PUT_TAIL(bp,val) (PUT((char*)(bp) + GET_SIZE(bp) - 8,(val)))
/*Given block ptr bp,compute the size,compute address of next and prev blocks*/
#define NEXT_BLKP(bp) ((char*)(bp) + SIZE((char*)(bp) - 4))
#define PREV_BLKP(bp) ((char*)(bp) - SIZE((char*)(bp) - 8))

static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void insert(void *bp);
static void* find_list(size_t size);
static char* heap_lists;
/*
 * print every free list
 */
static void check(){
    char *bp = heap_lists;
    for(int i = 1;i <= 9;i++,bp += 4){
        char* ptr = GET(bp);
        if(ptr != 0){
            printf("list %d:",i,ptr);
            while(ptr != NULL){
                printf("0x%x:%d-->",ptr,GET_SIZE(ptr));
                ptr = GET_NEXT(ptr);
            }
            printf("\n");
        }
    }
}
/* 
 * mm_init - initialize the malloc package. 
 */

int mm_init(void)
{
    //printf("\n\nmm_init called\n");
    if((heap_lists = mem_sbrk(9*4 + 12)) == NULL)
        return -1;
    //printf("heap_lists is 0x%x\n",heap_lists);
    char* bp = heap_lists;
    for(int i = 1;i <= 9;i++,bp += 4){
        PUT(bp,0);
    }
    PUT(bp,PACK(8,1));
    PUT(bp+4,PACK(8,1));
    PUT(bp+8,PACK(0,1));
    bp = extend_heap(CHUNKSIZE);
    //check();
    return 0;
}
/*
 * coalesce the free block
 */
static void *coalesce(void *bp){
    bool check_prev = !GET_ALLOC(PREV_BLKP(bp));
    bool check_next = !GET_ALLOC(NEXT_BLKP(bp));
    //printf("coalesce called : check_prev = %d,check_next = %d\n",check_prev,check_next);
    size_t size = GET_SIZE(bp);
    if(check_next && !check_prev){
        size += GET_SIZE(NEXT_BLKP(bp));
        erase(NEXT_BLKP(bp));
    }
    else if(!check_next && check_prev){
        size += GET_SIZE(PREV_BLKP(bp));
        erase(PREV_BLKP(bp));
        bp = PREV_BLKP(bp);
    }
    else if(check_next && check_prev){
        size += GET_SIZE(PREV_BLKP(bp)) + GET_SIZE(NEXT_BLKP(bp));
        erase(PREV_BLKP(bp));
        erase(NEXT_BLKP(bp));
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
    //printf("erase bp 0x%x,size is %d\n",bp,GET_SIZE(bp));
    if(GET_PREV(bp) != 0)
        PUT_NEXT(GET_PREV(bp),GET_NEXT(bp));
    if(GET_NEXT(bp) != 0)
        PUT_PREV(GET_NEXT(bp),GET_PREV(bp));

}
/*
 * match and insert the block pointed by bp into the head of suitable block list
 */
static void* find_list(size_t size){
    char* head = heap_lists;
    int idx = 8,tmp = 4096;
    while(size < tmp){
        idx--;
        tmp /= 2;
    }
    return head + 4*idx;
}
/*
 * insert the block into the free list
 */

static void insert_FILO(void* bp){
    size_t size = GET_SIZE(bp);
    //printf("insert block at 0x%x,size is %d\n",bp,size);
    char* list_ptr = find_list(size);
    //insert the node at the front of free list
    if(GET_NEXT(list_ptr) != 0)
        PUT_PREV(GET_NEXT(list_ptr),bp);
    PUT_NEXT(bp,GET_NEXT(list_ptr));
    PUT_NEXT(list_ptr,bp);
    PUT_PREV(bp,list_ptr);
}
static void insert_address(void* bp){
    size_t size = GET_SIZE(bp);
    char* prev = find_list(size);
    char* ptr = GET_NEXT(prev);
    while(ptr != 0 && ptr > bp){
        prev = ptr;
        ptr = GET_NEXT(ptr);
    }
    if(GET_NEXT(prev) != 0)
        PUT_PREV(GET_NEXT(prev),bp);
    PUT_NEXT(bp,ptr);
    PUT_NEXT(prev,bp);
    PUT_PREV(bp,prev);
}
static void insert(void* bp){
    insert_address(bp);
}
/*
 * extend the heap by size and return the block ptr
 */
static void* extend_heap(size_t size){
    char* bp;
    size = ALIGN(size);
    bp = mem_sbrk(size);
    PUT_HEAD(bp,PACK(size,1));
    PUT_TAIL(bp,PACK(size,1));
    PUT_HEAD(NEXT_BLKP(bp),PACK(size,1));
    PUT_NEXT(bp,0);
    PUT_PREV(bp,0);
    bp = coalesce(bp);
    insert(bp);
    return bp;
}
/*
 * Alloc the block and insert the remaining part into the free list
 */
void place(void *bp,size_t newsize){
    size_t currsize = GET_SIZE(bp);
    erase(bp);
    if(currsize - newsize >= 16){
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

}
/*
 * find the suitable blk
 */
static void* first_fit(size_t size){
    char* bp;
    for(char* head = find_list(size);head < heap_lists + 4*9;head += 4){
        bp = GET_NEXT(head);
        while(bp != 0){
            size_t currsize = GET_SIZE(bp);
            if(currsize >= size){
                return bp;
            }
            bp = GET_NEXT(bp);
        }
    }
    return NULL;
}
static void* best_fit(size_t size){
    char* bp;
    char* best = NULL;
    for(char* head = find_list(size);head < heap_lists + 4*9;head += 4){
        bp = GET_NEXT(head);
        size_t min_size = 0xffffffff;
        while(bp != 0){
            size_t currsize = GET_SIZE(bp);
            if(currsize >= size){
                if(currsize < min_size){
                    best = bp;
                    min_size = currsize;
                }
            }
            bp = GET_NEXT(bp);
        }
        if(best != NULL)
            return best;
    }
    return NULL;
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
    newsize = ALIGN(16 + size);
    //printf("malloc %d and newsize is %d\n",size,newsize);
    //find the block that enough big
    if((bp = best_fit(newsize)) != NULL){
        place(bp,newsize);
        //check();
        return bp;
    }
    //no enough big block,extend the heap
    extendsize = MAX(CHUNKSIZE,newsize);
    bp = extend_heap(extendsize);
    //printf("no suitable blcok and extendheap %d,bp is 0x%x\n",extendsize,bp);
    place(bp,newsize);
    //check();
    //printf("malloc return bp 0x%x size is %d\n",bp,GET_SIZE(bp));
    return bp;
}
/*
 * mm_free - Freeing a block,insert the block into the free list.
 */
void mm_free(void *ptr){
    //printf("free 0x%x,size is %d,alloc is %d\n",(unsigned int)ptr,GET_SIZE(ptr),GET_ALLOC(ptr));
    size_t size = GET_SIZE(ptr);
    ptr = coalesce(ptr);
    insert(ptr);
    //check();
}
/*
 * mm_realloc - resize the malloced blk
 */
void *mm_realloc(void *ptr, size_t size)
{
    //printf("realloc ptr 0x%x to size %d,oldsize is %d\n",ptr,ALIGN(size + 16),GET_SIZE(ptr));
    if(ptr == NULL){
        return mm_malloc(size);
    }
    if(size == 0){
        mm_free(ptr);
        return NULL;
    }
    size_t asize = ALIGN(size + 16);
    size_t oldsize = GET_SIZE(ptr);
    if(oldsize >= asize)
        return ptr;
    if(!GET_ALLOC(NEXT_BLKP(ptr)) && GET_SIZE(NEXT_BLKP(ptr)) + oldsize >= asize){
        oldsize += GET_SIZE(NEXT_BLKP(ptr));
        erase(NEXT_BLKP(ptr));
        PUT_HEAD(ptr,PACK(oldsize,1));
        PUT_TAIL(ptr,PACK(oldsize,1));
        return ptr;
    } 
    char* newptr = mm_malloc(size);
    memcpy(newptr,ptr,oldsize-8);
    mm_free(ptr);
    return newptr;
}












