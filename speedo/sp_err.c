#include "linux/libcwrap.h"
#include "speedo.h"

/*
 * Called by Speedo character generator to report an error.
 *
 *  Since character data not available is one of those errors
 *  that happens many times, don't report it to user
 */
void sp_report_error(fix15 n)
{
	switch (n)
	{
	case 1:
		sp_write_error("Insufficient font data loaded");
		break;
	case 3:
		sp_write_error("Transformation matrix out of range");
		break;
	case 4:
		sp_write_error("Font format error");
		break;
	case 5:
		sp_write_error("Requested specs not compatible with output module");
		break;
	case 7:
		sp_write_error("Intelligent transformation requested but not supported");
		break;
	case 8:
		sp_write_error("Unsupported output mode requested");
		break;
	case 9:
		sp_write_error("Extended font loaded but only compact fonts supported");
		break;
	case 10:
		sp_write_error("Font specs not set prior to use of font");
		break;
	case 11:
		sp_write_error("Squeezing/Clipping requested but not supported");
		break;
	case 12:
		/* sp_write_error("Character data not available"); */
		break;
	case 13:
		sp_write_error("Track kerning data not available()");
		break;
	case 14:
		sp_write_error("Pair kerning data not available()");
		break;
	default:
		sp_write_error("report_error(%d)", n);
		break;
	}
}
