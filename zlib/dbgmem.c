#include <linux/libcwrap.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "zlib.h"
#include "dbgmem.h"

#if DEBUG_ALLOC
static int got_errors;
static long memUse;
static size_t memUseSize;
static size_t startMemCount;
static size_t startMemSize;
static int mem_test_ignored;
#if DEBUG_ALLOC >= 3
static size_t memMaxUse;
static size_t memMaxUseSize;
static size_t memMaxUseBlock;
#endif
#if DEBUG_ALLOC >= 4
static FILE *trc_file;
#define TRC_FILENAME "dbgalloc.trc"
#endif
#endif

#define printnull(p) ((p) ? (p) : "(null)")


#define MEM_MAGIC_SIZE 4

typedef struct _mem_control
{
	size_t size;
#if DEBUG_ALLOC >= 2
	const char *who;
	long line;
	struct _mem_control *next;
#endif
	long magic;
	unsigned char checker[MEM_MAGIC_SIZE];
} MEM_CONTROL;

#define MEM_MAGIC_START_INTERN 8154711L

/*
 * bytevalue put in memory just before and beyond the
 * allocated block
 */
#define MEM_MAGIC_END			0xaa
/*
 * bytevalue put in newly allocate blocks
 */
#define MEM_MAGIC_NEW_ALLOCATED 0xbb
/*
 * bytevalue put in blocks that are going to be freed
 */
#define MEM_MAGIC_FREED         0x99

#ifndef ALLOC_ALIGN_SIZE
#  define ALLOC_ALIGN_SIZE (sizeof(double) > 8 ? 16 : 8)
#endif

#define SIZEOF_MEM_CONTROL (((sizeof(MEM_CONTROL) + ALLOC_ALIGN_SIZE - 1) / ALLOC_ALIGN_SIZE) * ALLOC_ALIGN_SIZE)

#define EXTRA_MEM (SIZEOF_MEM_CONTROL + sizeof(unsigned char) * MEM_MAGIC_SIZE)

#define Pling() fputc(7, stderr)



static void oom(size_t size)
{
	fprintf(stderr, "out of memory allocating %lu bytes\n", (unsigned long) size);
}

/* ********************************************************************** */
/* ---------------------------------------------------------------------- */
/* ********************************************************************** */

#if DEBUG_ALLOC >= 2
static MEM_CONTROL *alloc_list;

static int delete_alloc_list(MEM_CONTROL *block, const char *who, long line)
{
	MEM_CONTROL *search = alloc_list;

	if (block == search)
	{
		alloc_list = search->next;
	} else
	{
		while (search && search->next != block)
			search = search->next;
		if (search && search->next == block)
		{
			search->next = search->next->next;
		} else
		{
			fprintf(stderr, "%s:%ld: Memory Block not found: %p %-5.5s\n", printnull(who), line, (char *)block + SIZEOF_MEM_CONTROL, (const char *)block + SIZEOF_MEM_CONTROL);
#if DEBUG_ALLOC >= 4
			if (trc_file != NULL)
			{
				int i, maxl = 30;
				const unsigned char *p = (const unsigned char *)block + SIZEOF_MEM_CONTROL;
				fprintf(trc_file, "%s:%ld: Memory Block not found: %p ", printnull(who), line, (const char *)block + SIZEOF_MEM_CONTROL);
				for (i = 0; i < maxl && *p; i++)
				{
					if (*p >= 0x20 && *p < 0x7f)
						putc(*p, trc_file);
					else
						fprintf(trc_file, "\\x%02x", *p);
					p++;
				}
				fprintf(trc_file, "\n");
				fflush(trc_file);
			}
#endif
			got_errors = TRUE;
			return FALSE;
		}
	}
	return TRUE;
}

/* ---------------------------------------------------------------------- */

static int in_alloc_list(MEM_CONTROL *block)
{
	MEM_CONTROL *search = alloc_list;

	while (search)
	{
		if (search == block)
			return TRUE;
		search = search->next;
	}
	got_errors = TRUE;
	return FALSE;
}

#endif							/* DEBUG_ALLOC >= 2 */

/* ---------------------------------------------------------------------- */

#if DEBUG_ALLOC

static void *mem_debug_getit(size_t size, int zero, const char *who, long line)
{
	MEM_CONTROL *cntl;
	
	if (size == 0)
	{
		fprintf(stderr, "malloc(0) ? (%s:%ld)\n", printnull(who), line);
		return NULL;
	}

	cntl = malloc(size + EXTRA_MEM);

	if (cntl == NULL)
	{
		oom(size);
		return NULL;
	}
	
	cntl->magic = MEM_MAGIC_START_INTERN;
	
	cntl->size = size;
	{
		size_t i;
		
		for (i = 0; i < SIZEOF_MEM_CONTROL - offsetof(MEM_CONTROL, checker); i++)
			cntl->checker[i] = MEM_MAGIC_END;
	}
#if DEBUG_ALLOC >= 2
	cntl->next = alloc_list;
	alloc_list = cntl;
	cntl->who = who;
	cntl->line = line;
#endif
	cntl = (MEM_CONTROL *)((char *)cntl + SIZEOF_MEM_CONTROL);

	if (zero)
		memset(cntl, 0, size);
	else
		memset(cntl, MEM_MAGIC_NEW_ALLOCATED, size);

	{
		unsigned char *test;
		size_t i;
		
		test = (unsigned char *) cntl + size;
		for (i = 0; i < MEM_MAGIC_SIZE; i++)
			test[i] = MEM_MAGIC_END;
	}
	memUseSize += size;
	memUse++;
#if DEBUG_ALLOC >= 3
	if (memUse > memMaxUse)
		memMaxUse = memUse;
	if (memUseSize > memMaxUseSize)
		memMaxUseSize = memUseSize;
	if (size > memMaxUseBlock)
		memMaxUseBlock = size;
#endif


#if DEBUG_ALLOC >= 4
	if (trc_file != NULL)
	{
		fprintf(trc_file, "Trace: allocated %lu bytes memory at %p in %s:%ld\n", (unsigned long)size, cntl, printnull(who), line);
		fflush(trc_file);
	}
#endif
	return (void *) cntl;
}

/* ********************************************************************** */
/* ---------------------------------------------------------------------- */
/* ********************************************************************** */

void *mem_debug_realloc(void *ptr, size_t size, const char *who, long line)
{
	MEM_CONTROL *cntl;
	MEM_CONTROL *newcntl;
	int defect;
	size_t oldsize;
	
	if (size == 0)
	{
#if DEBUG_ALLOC >= 4
		if (trc_file != NULL)
		{
			fprintf(trc_file, "Trace: realloc: free %p\n", ptr);
			fflush(trc_file);
		}
#endif
		mem_debug_free(ptr, who, line);
		return NULL;
	}
	if (ptr == NULL)
	{
#if DEBUG_ALLOC >= 4
		if (trc_file != NULL)
		{
			fprintf(trc_file, "Trace: realloc: ptr is NULL\n");
			fflush(trc_file);
		}
#endif
		return mem_debug_getit(size, FALSE, who, line);
	}
	
	cntl = (MEM_CONTROL *) ((char *) (ptr) - SIZEOF_MEM_CONTROL);

#if DEBUG_ALLOC >= 2
	if (!in_alloc_list(cntl))
	{
		fprintf(stderr, "%s:%ld: Memory Block not found: %p\n", printnull(who), line, ptr);
#if DEBUG_ALLOC >= 4
		if (trc_file != NULL)
		{
			fprintf(trc_file, "%s:%ld: Memory Block not found: %p\n", printnull(who), line, ptr);
			fflush(trc_file);
		}
#endif
		return NULL;
	}
#endif
	
	defect = FALSE;
	if (cntl->magic != MEM_MAGIC_START_INTERN)
		defect = TRUE;

	{
		size_t i;
		unsigned char *test;
	
		test = cntl->checker;
		for (i = 0; i < SIZEOF_MEM_CONTROL - offsetof(MEM_CONTROL, checker); i++)
			if (test[i] != MEM_MAGIC_END)
				defect = TRUE;
	}

	if (defect)
	{
		got_errors = TRUE;
#if DEBUG_ALLOC >= 2
		fprintf(stderr, "%s:%ld: MemStart defekt: %p(%lu)\n", printnull(who), line, ptr, (unsigned long)cntl->size);
#else
		fprintf(stderr, "MemStart defekt: %p\n", ptr);
#endif
		return NULL;
	}

	{
		size_t i;
		unsigned char *test;
	
		defect = FALSE;
		test = ((unsigned char *) (cntl)) + cntl->size + SIZEOF_MEM_CONTROL;
		for (i = 0; i < MEM_MAGIC_SIZE; i++)
			if (test[i] != MEM_MAGIC_END)
				defect = TRUE;
		if (defect)
		{
			got_errors = TRUE;
#if DEBUG_ALLOC >= 2
			fprintf(stderr, "%s:%ld: MemEnd defekt: %p(%lu)\n", printnull(who), line, ptr, (unsigned long)cntl->size);
#else
			fprintf(stderr, "MemEnd defekt: %p\n", ptr);
#endif
			return NULL;
		}
	}

	oldsize = cntl->size;
	
	newcntl = realloc(cntl, size + EXTRA_MEM);
	if (newcntl == NULL)
	{
		oom(size);
		return NULL;
	}
	newcntl->magic = MEM_MAGIC_START_INTERN;
	
	newcntl->size = size;
	{
		size_t i;
		
		for (i = 0; i < SIZEOF_MEM_CONTROL - offsetof(MEM_CONTROL, checker); i++)
			newcntl->checker[i] = MEM_MAGIC_END;
	}
#if DEBUG_ALLOC >= 2
	if (cntl != newcntl)
	{
		if (!delete_alloc_list(cntl, who, line))
			return NULL;
		newcntl->next = alloc_list;
		alloc_list = newcntl;
	}
	newcntl->who = who;
	newcntl->line = line;
#endif
	
	newcntl = (MEM_CONTROL *)((char *)newcntl + SIZEOF_MEM_CONTROL);

	if (size > oldsize)
		memset((char *)newcntl + oldsize, MEM_MAGIC_NEW_ALLOCATED, size - oldsize);

	{
		unsigned char *test;
		size_t i;
		
		test = (unsigned char *) newcntl + size;
		for (i = 0; i < MEM_MAGIC_SIZE; i++)
			test[i] = MEM_MAGIC_END;
	}
	
	if (size > oldsize)
		memUseSize += size - oldsize;
	else
		memUseSize -= oldsize - size;
#if DEBUG_ALLOC >= 3
	if (memUseSize > memMaxUseSize)
		memMaxUseSize = memUseSize;
	if (size > memMaxUseBlock)
		memMaxUseBlock = size;
#endif
#if DEBUG_ALLOC >= 4
	if (trc_file != NULL)
	{
		fprintf(trc_file, "Trace: realloc (%p,%lu) to (%p,%lu) in %s:%ld\n", ((const char *)cntl + SIZEOF_MEM_CONTROL), oldsize, newcntl, size, printnull(who), line);
		fflush(trc_file);
	}
#endif
	
	return (void *) newcntl;
}

/* ---------------------------------------------------------------------- */

void mem_debug_free(void *ptr, const char *who, long line)
{
	MEM_CONTROL *memCntl;
	int defect;
	
	if (ptr == NULL)
	{
		return;
	}

	memCntl = (MEM_CONTROL *)((char *)ptr - SIZEOF_MEM_CONTROL);

#if DEBUG_ALLOC >= 2
	if (!delete_alloc_list(memCntl, who, line))
		return;
#endif
	defect = FALSE;
	if (memCntl->magic != MEM_MAGIC_START_INTERN)
		defect = TRUE;

	{
		size_t i;
		
		for (i = 0; i < SIZEOF_MEM_CONTROL - offsetof(MEM_CONTROL, checker); i++)
			if (memCntl->checker[i] != MEM_MAGIC_END)
				defect = TRUE;
	}

	if (defect)
	{
		got_errors = TRUE;
#if DEBUG_ALLOC >= 2
		fprintf(stderr, "%s:%ld: MemStart defekt: %p(%lu)\n", printnull(memCntl->who), memCntl->line, ptr, (unsigned long)memCntl->size);
#else
		fprintf(stderr, "%s:%ld: MemStart defekt: %p\n", ptr, printnull(who), line);
#endif
		return;
	}

#if DEBUG_ALLOC >= 2
	{
		unsigned char *test = ((unsigned char *)memCntl) + memCntl->size + SIZEOF_MEM_CONTROL;
		size_t i;
		
		defect = FALSE;
		for (i = 0; i < MEM_MAGIC_SIZE; i++)
			if (test[i] != MEM_MAGIC_END)
				defect = TRUE;
		if (defect)
		{
			got_errors = TRUE;
			/* assumes MAGIC_SIZE >= 4 */
			fprintf(stderr, "%s:%ld: MemEnd defekt %p(%lu): %02x %02x %02x %02x\n", printnull(memCntl->who), memCntl->line, ptr,
				(unsigned long)memCntl->size,
				test[0], test[1], test[2], test[3]);
		}
	}
#endif

#if DEBUG_ALLOC >= 4
	if (trc_file != NULL)
	{
		fprintf(trc_file, "Trace: free %p(%ld) %s:%ld\n", ptr, memCntl->size, printnull(who), line);
		fflush(trc_file);
	}
#endif

	memUse--;
	memUseSize -= memCntl->size;
	memset(memCntl, MEM_MAGIC_FREED, memCntl->size + EXTRA_MEM);

	free(memCntl);
}

/* ---------------------------------------------------------------------- */

void *mem_debug_get(size_t size, const char *who, long line)
{
	return mem_debug_getit(size, FALSE, who, line);
}

/* ---------------------------------------------------------------------- */

void *mem_debug_0get(size_t size, const char *who, long line)
{
	return mem_debug_getit(size, TRUE, who, line);
}

/* ---------------------------------------------------------------------- */

char *mem_debug_str_dup(const char *str, const char *from, long line)
{
	char *new;

	if (str == NULL)
		return NULL;
	new = mem_debug_getit(strlen(str) + 1, FALSE, from, line);
	if (new != NULL)
		strcpy(new, str);
	return new;
}

/* ---------------------------------------------------------------------- */

int mem_debug_check(void *ptr, const char *where)
{
	MEM_CONTROL *memCntl;
	int defect;
	
	if (ptr == NULL)
	{
		fprintf(stderr, "mem_check(NULL)? (%s)\n", where);
		return FALSE;
	}

	memCntl = (MEM_CONTROL *)((char *)ptr - SIZEOF_MEM_CONTROL);

#if DEBUG_ALLOC >= 2
	if (!in_alloc_list(memCntl))
	{
		fprintf(stderr, "%s: Memory Block not found: %p\n", where, ptr);
#if DEBUG_ALLOC >= 4
		if (trc_file != NULL)
		{
			fprintf(trc_file, "%s: Memory Block not found: %p\n", where, ptr);
			fflush(trc_file);
		}
#endif
		return FALSE;
	}
#endif
	defect = FALSE;
	if (memCntl->magic != MEM_MAGIC_START_INTERN)
		defect |= 1;
	
	{
		size_t i;
		
		for (i = 0; i < SIZEOF_MEM_CONTROL - offsetof(MEM_CONTROL, checker); i++)
			if (memCntl->checker[i] != MEM_MAGIC_END)
				defect |= 1;
	}

	if (defect & 1)
	{
		got_errors = TRUE;
#if DEBUG_ALLOC >= 2
		fprintf(stderr, "%s: MemStart defekt %p(%lu): %s:%ld\n", where, ptr, (unsigned long)memCntl->size, memCntl->who, memCntl->line);
#else
		fprintf(stderr, "%s: MemStart defekt: %p\n", where, ptr);
#endif
		return FALSE;
	}

#if DEBUG_ALLOC >= 2
	{
		unsigned char *test = ((unsigned char *)memCntl) + memCntl->size + SIZEOF_MEM_CONTROL;
		size_t i;
		
		for (i = 0; i < MEM_MAGIC_SIZE; i++)
			if (test[i] != MEM_MAGIC_END)
				defect |= 2;
		if (defect & 2)
		{
			got_errors = TRUE;
			/* assumes MAGIC_SIZE >= 4 */
			fprintf(stderr, "%s: MemEnd defekt %p(%lu): %s%ld: %02x %02x %02x %02x\n", where,
				ptr,
				(unsigned long)memCntl->size,
				printnull(memCntl->who), memCntl->line,
				test[0], test[1], test[2], test[3]);
			return FALSE;
		}
	}
#endif

	return defect == 0;
}

/* ---------------------------------------------------------------------- */

#if DEBUG_ALLOC >= 2
void mem_debug_check_all(const char *where, int report_ok)
{
	MEM_CONTROL *search = alloc_list;
	int error;
	
	error = FALSE;
	while (search != NULL)
	{
		if (!mem_debug_check((char *)search + SIZEOF_MEM_CONTROL, where))
			error = TRUE;
		search = search->next;
	}
	if (!error && report_ok)
		fprintf(stderr, "%s: mem ok\n", where);
}
#endif


/* ---------------------------------------------------------------------- */

int mem_test_end(void)
{
#if DEBUG_ALLOC
	if ((memUse != startMemCount || memUseSize != startMemSize) && !mem_test_ignored)
	{
		Pling();
		fprintf(stderr, "MemTest A:%ld S:%ld\n", (long)(memUse - startMemCount), (long)(memUseSize - startMemSize));
		got_errors = TRUE;
#if DEBUG_ALLOC >= 4
		if (trc_file != NULL)
		{
			MEM_CONTROL *list;
			
			fprintf(trc_file, "MemTest A:%ld S:%ld\n", (long)(memUse - startMemCount), (long)(memUseSize - startMemSize));
			for (list = alloc_list; list != NULL; list = list->next)
			{
				fprintf(trc_file, "-> %s:%ld: %p(%lu)\n", printnull(list->who), list->line, (const char *)list + SIZEOF_MEM_CONTROL, (unsigned long)list->size);
			}
		}
#endif
#if DEBUG_ALLOC >= 2
		while (alloc_list != NULL)
		{
			fprintf(stderr, "-> %s:%ld: %p(%lu)\n", printnull(alloc_list->who), alloc_list->line, (const char *)alloc_list + SIZEOF_MEM_CONTROL, (unsigned long)alloc_list->size);
			alloc_list = alloc_list->next;
		}
#endif
	}
#endif
#if DEBUG_ALLOC >= 3
	fprintf(stderr, "Max Memory usage: %lu blocks, %lu bytes, largest %lu\n",
		(unsigned long)memMaxUse,
		(unsigned long)memMaxUseSize,
		(unsigned long)memMaxUseBlock);
#endif
#if DEBUG_ALLOC >= 4
	if (trc_file != NULL)
	{
		fprintf(trc_file, "Max Memory usage: %lu blocks, %lu bytes, largest %lu\n",
			(unsigned long)memMaxUse,
			(unsigned long)memMaxUseSize,
			(unsigned long)memMaxUseBlock);
		fclose(trc_file);
		trc_file = NULL;
	}
#endif
	return !got_errors;
}

/* ---------------------------------------------------------------------- */

void mem_test_start(void)
{
	got_errors = FALSE;
#if DEBUG_ALLOC
	startMemCount = memUse;
	startMemSize = memUseSize;
#endif
#if DEBUG_ALLOC >= 3
	memMaxUse = 0;
	memMaxUseSize = 0;
#endif
#if DEBUG_ALLOC >= 4
	trc_file = fopen(TRC_FILENAME, "w");
#endif
}

#endif /* DEBUG_ALLOC */
