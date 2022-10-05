#ifndef __DBGMEM_H__
#define __DBGMEM_H__

/*
 * values for DEBUG_ALLOC:
 * 0 - disable
 * 1 - track number of blocks and total allocated size
 * 2 - track every block and where it is allocated
 * 3 - same as 2 and track maximum allocated size & number
 * 4 - same as 3 and write log file to "dbgalloc.trc"
 */
#ifndef DEBUG_ALLOC
#  define DEBUG_ALLOC 0
#endif

#ifndef STDC
extern voidp  malloc OF((uInt size));
extern voidp  calloc OF((uInt items, uInt size));
extern void   free   OF((voidpf ptr));
#endif

void mem_test_start(void);
int mem_test_end(void);

#if DEBUG_ALLOC

void *mem_debug_get(size_t size, const char *from, long line);
void mem_debug_free(void *block, const char *from, long line);
void *mem_debug_0get(size_t size, const char *from, long line);
char *mem_debug_str_dup(const char *str, const char *from, long line);
void *mem_debug_realloc(void *block, size_t newsize, const char *from, long line);
int mem_debug_check(void *block, const char *where);
#if DEBUG_ALLOC >= 2
void mem_debug_check_all(const char *where, int report_ok);
#endif

#define z_malloc(size) mem_debug_get(size, __FILE__, __LINE__)
#define z_calloc(nitems, size) mem_debug_0get((size_t)(nitems) * (size_t)(size), __FILE__, __LINE__)
#define z_strdup(str) mem_debug_str_dup(str, __FILE__, __LINE__)
#define z_free(block) mem_debug_free(block, __FILE__, __LINE__)
#define z_realloc(block, newsize) mem_debug_realloc(block, newsize, __FILE__, __LINE__)

#else

#define z_malloc(size) malloc(size)
#define z_calloc(nitems, size) calloc(nitems, size)
#define z_strdup(str) strdup(str)
#define z_free(block) free(block)
#define z_realloc(block, newsize) realloc(block, newsize)
#define mem_debug_check(ptr, where)
#define mem_debug_check_all(where, report_ok)
#define mem_test_start()
#define mem_test_end()

#endif /* DEBUG_ALLOC */

#endif /* __DBGMEM_H__ */
