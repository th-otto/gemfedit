#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "fonthdr.h"



void decode_gemfnt(unsigned char *dst, const unsigned char *src, unsigned short form_width, unsigned short form_height)
{
	size_t remaining_bytes;
	int count_bits;			/* bits still to be done in src word */
	unsigned char current_bit;
	int count_leading_zeroes;
	int odd_bits;
	int zero_count;
	unsigned short source;
	unsigned short accum;
	unsigned short carry;
	size_t data_size;
	unsigned char *dstp;
	unsigned char *dstp2;
	
	data_size = (size_t)form_width * form_height;
	dstp = dst - 1;
	
	/* Must start with count+1 */
	remaining_bytes = data_size + 1;
	source = 0;
	
	/* bits still to be done in src word */
	count_bits = 0;
	
	/*
	 * Select current bit which is the bit
	 * before the first word, as we have an
	 * imagined zero there
	 */
	current_bit = 1;
	
	for (;;)
	{
		count_leading_zeroes = -1;
	
		/*
		 * count a string of zero bits which leads zeros string
		 */
		do
		{
			if (--count_bits < 0)
			{
				/* get next word of source */
				source = src[0] | (src[1] << 8);
				src += 2;
				count_bits = 15;
			}
		
			/* Count times round this loop + 1 */
			++count_leading_zeroes;
			/* Any more zeros? */
			carry = source & 0x8000;
			source <<= 1;
		} while (!carry);
		
		/* Preload the 1 bit implicit for n>8 */
		accum = 1;
		/* Is string length going to be >8 */
		if (count_leading_zeroes <= 0)
		{
			count_leading_zeroes = 1;
			accum = 0;
		}
		/* We need 2 bits more */
		count_leading_zeroes += 2;
	
		do
		{
			if (--count_bits < 0)
			{
				/* get next word of source */
				source = src[0] | (src[1] << 8);
				src += 2;
				count_bits = 15;
			}
			carry = source & 0x8000;
			source <<= 1;
			accum = (accum << 1) | (carry >> 15);
		} while (--count_leading_zeroes);
		
		count_leading_zeroes = accum + 1;
		if (count_leading_zeroes == 0)
			--count_leading_zeroes;
		/* Isolate odd bits */
		odd_bits = count_leading_zeroes & 7;
		/* Convert to a byte count */
		zero_count = count_leading_zeroes >> 3;
		/* Do whole bytes, if any */
		if (zero_count != 0)
		{
			do
			{
				if (--remaining_bytes == 0)
					goto Huff_done;
				*++dstp = 0;
			} while (--zero_count != 0);
		}
		while (--odd_bits >= 0)
		{
			carry = current_bit & 1;
	 		current_bit = (current_bit >> 1) | (carry << 7);
			if (carry)
			{
				if (--remaining_bytes == 0)
					goto Huff_done;
				*++dstp = 0;
			}
		}
		
		/* Get back zero count. */
		count_leading_zeroes = accum + 1;
		if (count_leading_zeroes == 0)
			continue;
		
		do
		{
			if (--count_bits < 0)
			{
				/* get next word of source */
				source = src[0] | (src[1] << 8);
				src += 2;
				count_bits = 15;
			}
			/* Set bit in font */
			*dstp |= current_bit;
			/* advance to next bit */
			carry = current_bit & 1;
	 		current_bit = (current_bit >> 1) | (carry << 7);
			if (carry)
			{
				if (--remaining_bytes == 0)
					goto Huff_done;
				*++dstp = 0;
			}
			carry = source & 0x8000;
			source <<= 1;
		} while (carry);
	}
	
Huff_done:
	/* remaining_bytes has just reached 0 so we are at the end of the form */
	if (form_height > 1)
	{
		/* Undo the XOR */
		dstp = dst;
		/* ptr of second line of form. */
		dstp2 = dstp + form_width;
		remaining_bytes = data_size - form_width;
		while (remaining_bytes > 0)
		{
			*dstp2 ^= *dstp;
			dstp++;
			dstp2++;
			remaining_bytes--;
		}
	}
}
