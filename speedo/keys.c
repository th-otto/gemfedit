#include "speedo.h"
#include "keys.h"


#ifdef EXTRAFONTS
static ufix8 const skey[] = {
	SKEY0,
	SKEY1,
	SKEY2,
	SKEY3,
	SKEY4,
	SKEY5,
	SKEY6,
	SKEY7,
	SKEY8
};										/* Sample Font decryption key */

static ufix8 const rkey[] = {
	RKEY0,
	RKEY1,
	RKEY2,
	RKEY3,
	RKEY4,
	RKEY5,
	RKEY6,
	RKEY7,
	RKEY8
};										/* Retail Font decryption key */

#endif /* EXTRAFONTS */

#ifdef XSAMPLEFONTS
static ufix8 const xkey[] = {
	XKEY0,
	XKEY1,
	XKEY2,
	XKEY3,
	XKEY4,
	XKEY5,
	XKEY6,
	XKEY7,
	XKEY8
};										/* Sample Font decryption key */
#endif

static ufix8 const mkey[] = {
	KEY0,
	KEY1,
	KEY2,
	KEY3,
	KEY4,
	KEY5,
	KEY6,
	KEY7,
	KEY8
};										/* Font decryption key */



const ufix8 *sp_get_key(buff_t font_buff)
{
	ufix16 cust_no;
	const ufix8 *key;

	cust_no = sp_get_cust_no(font_buff);
#ifdef EXTRAFONTS
	if (cust_no == SCUS0)
	{
		key = skey;
	} else if (cust_no == RCUS0)
	{
		key = rkey;
	} else
#endif

#ifdef XSAMPLEFONTS
	if (cust_no == XCUS0)
	{
		key = xkey;
	} else
#endif

	if (cust_no == CUS0)
	{
		key = mkey;
	} else
	{
		key = NULL;
	}
	return key;
}
