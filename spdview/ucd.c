#include "linux/libcwrap.h"
#include "defs.h"
#include "ucd.h"
#include "ucbycode.h"
#include "ucblock.h"

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

static const struct ucd *ucd_find(uint32_t unicode)
{
	size_t a, b, c;
	const struct ucd *p;
	
	a = 0;
	b = sizeof(ucd_bycode) / sizeof(ucd_bycode[0]);
	while (a < b)
	{
		c = (a + b) >> 1;
		p = &ucd_bycode[c];
		if (p->code == unicode)
			return p;
		if (p->code > unicode)
			b = c;
		else
			a = c + 1;
	}
	return NULL;
}

/* ------------------------------------------------------------------------- */

const char *ucd_get_name(uint32_t unicode)
{
	const struct ucd *p = ucd_find(unicode);
	if (p)
		return &ucd_names[p->name_offset];
	return "UNDEFINED";
}

/* ------------------------------------------------------------------------- */

const char *ucd_get_block(uint32_t unicode)
{
	size_t a, b, c;
	const struct ucd_block *p;

	a = 0;
	b = sizeof(ucd_blocks) / sizeof(ucd_blocks[0]);
	while (a < b)
	{
		c = (a + b) >> 1;
		p = &ucd_blocks[c];
		if (unicode >= p->first && unicode <= p->last)
			return p->name;
		if (p->first > unicode)
			b = c;
		else
			a = c + 1;
	}
	return "No Block";
}
