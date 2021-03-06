#include "linux/libcwrap.h"
#include "speedo.h"

/*
 * Called by Speedo character generator to report an error.
 *
 *  Since character data not available is one of those errors
 *  that happens many times, don't report it to user
 */
void sp_report_error(SPD_PROTO_DECL2 fix15 n)
{
	switch (n)
	{
	case 1:
		sp_write_error(SPD_GARGS "Insufficient font data loaded");
		break;
	case 3:
		sp_write_error(SPD_GARGS "Transformation matrix out of range");
		break;
	case 4:
		sp_write_error(SPD_GARGS "Font format error");
		break;
	case 5:
		sp_write_error(SPD_GARGS "Requested specs not compatible with output module");
		break;
	case 7:
		sp_write_error(SPD_GARGS "Intelligent transformation requested but not supported");
		break;
	case 8:
		sp_write_error(SPD_GARGS "Unsupported output mode requested");
		break;
	case 9:
		sp_write_error(SPD_GARGS "Extended font loaded but only compact fonts supported");
		break;
	case 10:
		sp_write_error(SPD_GARGS "Font specs not set prior to use of font");
		break;
	case 11:
		sp_write_error(SPD_GARGS "Squeezing/Clipping requested but not supported");
		break;
	case 12:
		/* sp_write_error(SPD_GARGS "Character data not available"); */
		break;
	case 13:
		sp_write_error(SPD_GARGS "Track kerning data not available()");
		break;
	case 14:
		sp_write_error(SPD_GARGS "Pair kerning data not available()");
		break;
	default:
		sp_write_error(SPD_GARGS "report_error(%d)", n);
		break;
	}
}
