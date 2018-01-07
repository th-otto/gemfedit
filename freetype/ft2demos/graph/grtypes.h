/***************************************************************************
 *
 *  grtypes.h
 *
 *    basic type definitions
 *
 *  Copyright 1999 - The FreeType Development Team - www.freetype.org
 *
 *
 *
 *
 ***************************************************************************/

#ifndef GRTYPES_H_
#define GRTYPES_H_

#include <stdint.h>

typedef uint8_t byte;

typedef struct grDimension_
{
	int x;
	int y;
} grDimension;

#define gr_err_ok                    0
#define gr_err_memory               -1
#define gr_err_bad_argument         -2
#define gr_err_bad_target_depth     -3
#define gr_err_bad_source_depth     -4
#define gr_err_saturation_overflow  -5
#define gr_err_conversion_overflow  -6
#define gr_err_invalid_device       -7


#endif /* GRTYPES_H_ */
