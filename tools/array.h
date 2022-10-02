#ifndef TABLE_H
#define TABLE_H

typedef struct {
	unsigned char *data;
	size_t len;
} array;

typedef uint32_t unichar_t;

array *array_new(void);
void array_delete(array *a);

void array_addByte(array *a, unsigned char b);
void array_addShort(array *a, unsigned short s);
void array_addLong(array *a, unsigned long s);

#define array_getLen(a) ((a)->len)
#define array_getBuf(a) ((a)->data)

void array_addArray(array *a, array *from);
void array_addString(array *a, const char *str);
void array_addWideString(array *a, const unichar_t *str);

typedef struct table {
	unsigned long chk;
	unsigned long off;
	array *ary;
	size_t len;
	struct table *mapTo;
} table;

table *table_new(void);
void table_delete(table *t);

unsigned long table_calcSum(table *t);
unsigned long table_setTableLen(table *t, unsigned long len);
unsigned long table_getTableLen(table *t);
void table_setMapTable(table *t, table *value);
table *table_getMapTable(table *t);
void table_write(table *t, FILE *fn);

void *xmalloc(size_t size);
void *xrealloc(void *p, size_t size);

#endif /* TABLE_H */
