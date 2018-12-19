#ifndef CH02_MALLOC_H
#define CH02_MALLOC_H

typedef struct opt_stats {
    long pages_mapped;
    long pages_unmapped;
    long chunks_allocated;
    long chunks_freed;
    long free_length;
} opt_stats;

opt_stats* opt_getstats();
void opt_printstats();

void* opt_malloc(size_t size);
void opt_free(void* item);
void* opt_realloc(void* prev, size_t bytes);

#endif
