#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>
#include "array.h"

/* ************************************************************************** */
/* -------------------------------------------------------------------------- */
/* ************************************************************************** */

void array_addWideString(array *a, const unichar_t *str)
{
	while (*str)
	{
		array_addShort(a, *str);
		++str;
	}
}

/* -------------------------------------------------------------------------- */

void array_addString(array *a, const char *str)
{
	while (*str)
		array_addByte(a, *str++);
}

/* -------------------------------------------------------------------------- */

array *array_new(void)
{
	array *a = (array *)xmalloc(sizeof(*a));
	a->len = 0;
	a->data = NULL;
	return a;
}

/* -------------------------------------------------------------------------- */

void array_delete(array *a)
{
	free(a->data);
	free(a);
}

/* -------------------------------------------------------------------------- */

static void array_extend(array *a)
{
	unsigned char *nd;

	if ((a->len & 63) == 0)
	{
		nd = (unsigned char *) xrealloc(a->data, a->len + 64);

		a->data = nd;
	}
}

/* -------------------------------------------------------------------------- */

void array_addByte(array *a, unsigned char b)
{
	array_extend(a);
	a->data[a->len++] = b;
}

/* -------------------------------------------------------------------------- */

void array_addShort(array *a, unsigned short s)
{
	array_addByte(a, (unsigned char)(s >> 8));
	array_addByte(a, (unsigned char)s);
}

/* -------------------------------------------------------------------------- */

void array_setShort(array *a, size_t offset, unsigned short s)
{
	assert(offset + 2 <= a->len);
	a->data[offset + 0] = (unsigned char)(s >> 8);
	a->data[offset + 1] = (unsigned char)s;
}

/* -------------------------------------------------------------------------- */

void array_addArray(array *a, array *from)
{
	int len = (int) array_getLen(from);
	int i;

	for (i = 0; i < len; ++i)
		array_addByte(a, from->data[i]);
}

/* -------------------------------------------------------------------------- */

void array_addLong(array *a, unsigned long s)
{
	array_addShort(a, (unsigned short)(s >> 16));
	array_addShort(a, (unsigned short)s);
}

/* -------------------------------------------------------------------------- */

void array_addQuad(array *a, uint64_t s)
{
	array_addLong(a, s >> 32);
	array_addLong(a, s & 0xffffffffUL);
}

/* ************************************************************************** */
/* -------------------------------------------------------------------------- */
/* ************************************************************************** */

table *table_new(void)
{
	table *t = (table *)xmalloc(sizeof(*t));
	
	t->chk = t->off = t->len = 0;
	t->mapTo = NULL;
	t->ary = array_new();
	return t;
}

/* -------------------------------------------------------------------------- */

void table_delete(table *t)
{
	array_delete(t->ary);
	free(t);
}

/* -------------------------------------------------------------------------- */

size_t table_setTableLen(table *t, size_t newlen)
{
#if 0
	/* Keep 4 byte alignment. */
	t->len = (newlen + 3) & ~3L;
#else
	t->len = newlen;
#endif
	return t->len;
}

/* -------------------------------------------------------------------------- */

unsigned long table_calcSum(table *t)
{
	int i;
	int c;
	unsigned long l;
	size_t len = array_getLen(t->ary);
	const unsigned char *buf = array_getBuf(t->ary);
	const unsigned char *eot = buf + len;

	t->len = len;
	t->chk = 0;
	while (buf < eot)
	{
		for (l = i = 0; i < 4; i++)
		{
			c = (buf < eot) ? *buf++ : 0;
			l = ((l >> 8) & 0xffffff) + (c << 24);
		}
		t->chk += l;
		t->chk &= 0xffffffff;
	}
	return t->chk;
}

/* -------------------------------------------------------------------------- */

unsigned long table_getTableLen(table *t)
{
	return t->len;
}

/* -------------------------------------------------------------------------- */

void table_setMapTable(table *t, table *value)
{
	t->mapTo = value;
}

/* -------------------------------------------------------------------------- */

table *table_getMapTable(table *t)
{
	return t->mapTo;
}

/* -------------------------------------------------------------------------- */

void table_write(table *t, FILE *fn)
{
	size_t maxlen = array_getLen(t->ary);
	size_t i;

	for (i = 0; i < maxlen; ++i)
		putc(t->ary->data[i], fn);

	/* Padding with 0 (LOCA and HMTX) */
	for (; i < t->len; ++i)
		putc(0, fn);
}
