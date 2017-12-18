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
		sp_write_error("Insufficient font data loaded\n");
		break;
	case 3:
		sp_write_error("Transformation matrix out of range\n");
		break;
	case 4:
		sp_write_error("Font format error\n");
		break;
	case 5:
		sp_write_error("Requested specs not compatible with output module\n");
		break;
	case 7:
		sp_write_error("Intelligent transformation requested but not supported\n");
		break;
	case 8:
		sp_write_error("Unsupported output mode requested\n");
		break;
	case 9:
		sp_write_error("Extended font loaded but only compact fonts supported\n");
		break;
	case 10:
		sp_write_error("Font specs not set prior to use of font\n");
		break;
	case 11:
		sp_write_error("Squeezing/Clipping requested but not supported\n");
		break;
	case 12:
		/* sp_write_error("Character data not available\n"); */
		break;
	case 13:
		sp_write_error("Track kerning data not available()\n");
		break;
	case 14:
		sp_write_error("Pair kerning data not available()\n");
		break;
	default:
		sp_write_error("report_error(%d)\n", n);
		break;
	}
}
