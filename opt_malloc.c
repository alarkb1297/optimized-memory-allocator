#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

#include "ch02_malloc.h"

typedef struct node_t  {
    size_t size;
    struct node_t* next;
} node_t;

typedef struct header_t {
    size_t size;
} header_t;

const size_t PAGE_SIZE = 4096;
//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
__thread void* globalBlock;
__thread int globalPagesAvail = 0;
__thread node_t* bins[7];

static
size_t
div_up(size_t xx, size_t yy)
{
    // This is useful to calculate # of pages
    // for large allocations.
    size_t zz = xx / yy;
    
    if (zz * yy == xx) {
        return zz;
    }
    else {
        return zz + 1;
    }
}

node_t*
make_block(size_t numPages) {
    node_t* node = mmap(NULL, PAGE_SIZE * numPages, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
    node->size = PAGE_SIZE * numPages;
    node->next = NULL;
    return node;
    
}

void
insert_to_bins(node_t* blockToInsert) {
    
    int index = log2(blockToInsert->size / 32);
    
    blockToInsert->next = bins[index];
    bins[index] = blockToInsert;
    
}

void
return_extra(node_t* foundBlock, size_t sizeReq) {
    
    int numDivisions = (foundBlock->size / sizeReq) - 1;
    
    while (numDivisions != 0) {
        
        void* vpBlockToFreelist = (void*)foundBlock + (numDivisions * sizeReq);
        node_t* blockToFreelist = (node_t*)vpBlockToFreelist;
        blockToFreelist->size = sizeReq;
        blockToFreelist->next = NULL;
        insert_to_bins(blockToFreelist);
        
        foundBlock->size -= sizeReq;
        --numDivisions;
    }
}

node_t*
get_from_global_block(int pagesReq) {
    
    node_t* foundBlock = (node_t*)(globalBlock + ((globalPagesAvail - pagesReq) * PAGE_SIZE));
    globalPagesAvail -= pagesReq;
    foundBlock->size = PAGE_SIZE * pagesReq;
    foundBlock->next = NULL;
    return foundBlock;
    
}

node_t*
get_available_block(size_t sizeReq) {
    
    if (sizeReq > PAGE_SIZE) {
        int pagesReq = sizeReq / PAGE_SIZE;
        
        if (pagesReq < globalPagesAvail) {
            return get_from_global_block(pagesReq);
        }
        else {
            size_t numPages = div_up(sizeReq, PAGE_SIZE);
            return make_block(numPages);
        }
    }
    
    int index = log2(sizeReq / 32);
    
    for (int i = index; i < 7; ++i) {
        
        if (bins[i] != NULL && bins[i]->size == sizeReq) {
            node_t* foundBlock = bins[i];
            bins[i] = foundBlock->next;
            foundBlock->next = NULL;
            return foundBlock;
        }
        else if (bins[i] != NULL && bins[i]->size > sizeReq) {
            
            size_t sizeFound = bins[i]->size;
            node_t* foundBlock = bins[i];
            bins[i] = foundBlock->next;
            foundBlock->next = NULL;
            
            return_extra(foundBlock, sizeReq);
            
            return foundBlock;
        }
    }
    
    if (globalPagesAvail < 1) {
        globalBlock = mmap(NULL, PAGE_SIZE * 50, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
        globalPagesAvail = 50;
    }
    
    node_t* foundBlock = get_from_global_block(1);
    return_extra(foundBlock, sizeReq);
    return foundBlock;

}


void* opt_malloc(size_t sizeReq) {
    
    sizeReq += sizeof(size_t);
    
    if (sizeReq > 4096) {
        sizeReq = div_up(sizeReq, PAGE_SIZE) * PAGE_SIZE;
    }
    else if (sizeReq < 32) {
        sizeReq = 32;
    }
    else {
        sizeReq = pow(2, ceil(log2(sizeReq)));
    }
    node_t* returnedBlock = get_available_block(sizeReq);
    header_t* blockToUser = (header_t*)returnedBlock;
    blockToUser->size = returnedBlock->size;
    
    return (void*)blockToUser + sizeof(size_t);
}

void*
opt_realloc(void* prev, size_t bytes) {
    prev -= sizeof(size_t);
    header_t* prevHeader = (header_t*)prev;
    size_t prevSize = prevHeader->size;
    prev += sizeof(size_t);
    
    void* new = opt_malloc(bytes);
    
    memcpy(new, prev, prevSize - sizeof(size_t));
    
    opt_free(prev);
    return new;
}

void
opt_free(void* item) {
    
    item -= sizeof(size_t);
    header_t* itemToFree = (header_t*) item;
    
    if (itemToFree->size < PAGE_SIZE) {
        node_t* blockToFreelist = (node_t*)item;
        blockToFreelist->size = itemToFree->size;
        blockToFreelist->next = NULL;
        insert_to_bins(blockToFreelist);
    }
    else {
        int numPagesFree = itemToFree->size / PAGE_SIZE;
        munmap(itemToFree, numPagesFree * PAGE_SIZE);
    }
}









