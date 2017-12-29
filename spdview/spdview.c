#include "linux/libcwrap.h"
#include "spddefs.h"
#include <curl/curl.h>
#include <sys/time.h>
#include <utime.h>
#include <unistd.h>
#include <dirent.h>
#include "cgic.h"
#include "speedo.h"
#include "writepng.h"
#include "vdimaps.h"
#include "bics2uni.h"
#include "ucbycode.h"
#include "version.h"

char const gl_program_name[] = "spdview.cgi";
char const gl_program_version[] = PACKAGE_VERSION " - " PACKAGE_DATE;
static const char *cgi_scriptname = "spdview.cgi";
static char const spdview_css_name[] = "_spdview.css";
static char const spdview_js_name[] = "_spdview.js";
static char const charset[] = "UTF-8";

static char const cgi_cachedir[] = "cache";

static char const html_error_note_style[] = "spdview_error_note";
static char const html_node_style[] = "spdview_node";
static char const html_spd_info_id[] = "spd_info";
static char const html_toolbar_style[] = "spdview_nav_toolbar";
static char const html_dropdown_style[] = "spdview_dropdown";
static char const html_dropdown_info_style[] = "spdview_dropdown_info";
static char const html_nav_img_style[] = "spdview_nav_img";

static char const html_nav_info_png[] = "images/iinfo.png";
static char const html_nav_load_png[] = "images/iload.png";
static char const html_nav_load_href[] = "index.php";
static char const html_nav_dimensions[] = " width=\"32\" height=\"21\"";

struct curl_parms {
	const char *filename;
	FILE *fp;
};

#define ALLOWED_PROTOS ( \
	CURLPROTO_FTP | \
	CURLPROTO_FTPS | \
	CURLPROTO_HTTP | \
	CURLPROTO_HTTPS | \
	CURLPROTO_SCP | \
	CURLPROTO_SFTP | \
	CURLPROTO_TFTP)

static const char *html_closer = " />";
static char *output_dir;
static char *output_url;
static char *html_referer_url;
static GString *errorout;
static FILE *errorfile;
static size_t body_start;
static gboolean cgi_cached;
static gboolean hidemenu;
static gboolean debug;

#define _(x) x

#define DEBUG 0

static int force_crlf = FALSE;

#define	MAX_BITS	1024
#define CHAR_COLUMNS 16
#define PAGE_SIZE    128

static unsigned char framebuffer[MAX_BITS][MAX_BITS];
static uint16_t char_index, char_id;
static buff_t font;
static ufix8 *font_buffer;
static ufix8 *c_buffer;
static uint16_t mincharsize;
static fix15 bit_width, bit_height;
static char fontname[70 + 1];
static char fontfilename[70 + 1];
static uint16_t first_char_index;
static uint16_t num_chars;
static glyphinfo_t font_bb;

static long point_size = 120;
static int x_res = 72;
static int y_res = 72;
static int quality = 1;

static specs_t specs;

#define HAVE_MKSTEMPS

typedef struct {
	uint16_t char_index;
	uint16_t char_id;
	char *local_filename;
	char *url;
	glyphinfo_t bbox;
} charinfo;
static charinfo *infos;

#define QUOTE_CONVSLASH  0x0001
#define QUOTE_SPACE      0x0002
#define QUOTE_URI        0x0004
#define QUOTE_JS         0x0008
#define QUOTE_ALLOWUTF8  0x0010
#define QUOTE_LABEL      0x0020
#define QUOTE_UNICODE    0x0040
#define QUOTE_NOLTR      0x0080

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

static FILE *fp;

static void chomp(char *dst, const char *src, size_t maxlen)
{
	size_t len;
	
	strncpy(dst, src, maxlen);
	dst[maxlen - 1] = '\0';
	len = strlen(dst);
	while (len > 0 && dst[len - 1] == ' ')
		dst[--len] = '\0';
	while (len > 0 && dst[0] == ' ')
	{
		memmove(dst, dst + 1, len);
		len--;
	}
}

/* ------------------------------------------------------------------------- */

static void make_font_filename(char *dst, const char *src)
{
	unsigned char c;
	
	while ((c = *src++) != 0)
	{
		switch (c)
		{
		case ' ':
		case '(':
		case ')':
			c = '_';
			break;
		}
		*dst++ = c;
	}
	*dst = '\0';
}

/* ------------------------------------------------------------------------- */
/*
 * Reads 1-byte field from font buffer 
 */
#define read_1b(pointer) (*(pointer))

/* ------------------------------------------------------------------------- */

static fix15 read_2b(const ufix8 *ptr)
{
	fix15 tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) + *ptr;
	return tmp;
}

/* ------------------------------------------------------------------------- */

static fix31 read_4b(const ufix8 *ptr)
{
	fix31 tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) + *ptr++;
	tmp = (tmp << 8) + *ptr++;
	tmp = (tmp << 8) + *ptr;
	return tmp;
}

/* ------------------------------------------------------------------------- */

static char *html_quote_name(const char *name, unsigned int flags, size_t len)
{
	char *str, *ret;
	static char const hex[] = "0123456789ABCDEF";
	const char *end;
	
	if (name == NULL)
		return NULL;
	if (len == STR0TERM)
		len = strlen(name);
	str = ret = g_new(char, len * 20 + 1);
	if (str != NULL)
	{
		end = name + len;
		if (name < end && (flags & QUOTE_LABEL) && *name != '_' && !g_ascii_isalpha(*name))
			*str++ = '_';
		while (name < end)
		{
			unsigned char c = *name++;
#define STR(s) strcpy(str, s), str += sizeof(s) - 1
			switch (c)
			{
			case '\\':
				if (flags & QUOTE_URI)
				{
					STR("%2F");
				} else if (flags & QUOTE_CONVSLASH)
				{
					*str++ = '/';
				} else
				{
					*str++ = '\\';
				}
				break;
			case ' ':
				if (flags & QUOTE_URI)
				{
					STR("%20");
				} else if (flags & QUOTE_SPACE)
				{
					STR("&nbsp;");
				} else if (flags & QUOTE_LABEL)
				{
					*str++ = '_';
				} else
				{
					*str++ = ' ';
				}
				break;
			case '"':
				if (flags & QUOTE_JS)
				{
					STR("\\&quot;");
				} else if (flags & QUOTE_URI)
				{
					STR("%22");
				} else
				{
					STR("&quot;");
				}
				break;
			case '&':
				if (flags & QUOTE_URI)
				{
					STR("%26");
				} else
				{
					STR("&amp;");
				}
				break;
			case '\'':
				if (flags & QUOTE_URI)
				{
					STR("%27");
				} else
				{
					STR("&apos;");
				}
				break;
			case '<':
				if (flags & QUOTE_URI)
				{
					STR("%3C");
				} else
				{
					STR("&lt;");
				}
				break;
			case '>':
				if (flags & QUOTE_URI)
				{
					STR("%3E");
				} else
				{
					STR("&gt;");
				}
				break;
			case '-':
			case '.':
			case '_':
			case '~':
				*str++ = c;
				break;
			case 0x01:
				if (flags & QUOTE_URI)
				{
					STR("%01");
				} else
				{
					STR("&soh;");
				}
				break;
			case 0x02:
				if (flags & QUOTE_URI)
				{
					STR("%02");
				} else
				{
					STR("&stx;");
				}
				break;
			case 0x03:
				if (flags & QUOTE_URI)
				{
					STR("%03");
				} else
				{
					STR("&etx;");
				}
				break;
			case 0x04:
				if (flags & QUOTE_URI)
				{
					STR("%04");
				} else
				{
					STR("&eot;");
				}
				break;
			case 0x05:
				if (flags & QUOTE_URI)
				{
					STR("%05");
				} else
				{
					STR("&enq;");
				}
				break;
			case 0x06:
				if (flags & QUOTE_URI)
				{
					STR("%06");
				} else
				{
					STR("&ack;");
				}
				break;
			case 0x07:
				if (flags & QUOTE_URI)
				{
					STR("%07");
				} else
				{
					STR("&bel;");
				}
				break;
			case 0x08:
				if (flags & QUOTE_URI)
				{
					STR("%08");
				} else
				{
					STR("&bs;");
				}
				break;
			case 0x09:
				if (flags & QUOTE_URI)
				{
					STR("%09");
				} else
				{
					STR("&ht;");
				}
				break;
			case 0x0a:
				if (flags & QUOTE_URI)
				{
					STR("%0A");
				} else
				{
					STR("&lf;");
				}
				break;
			case 0x0b:
				if (flags & QUOTE_URI)
				{
					STR("%0B");
				} else
				{
					STR("&vt;");
				}
				break;
			case 0x0c:
				if (flags & QUOTE_URI)
				{
					STR("%0C");
				} else
				{
					STR("&ff;");
				}
				break;
			case 0x0d:
				if (flags & QUOTE_URI)
				{
					STR("%0D");
				} else
				{
					STR("&cr;");
				}
				break;
			case 0x0e:
				if (flags & QUOTE_URI)
				{
					STR("%0E");
				} else
				{
					STR("&so;");
				}
				break;
			case 0x0f:
				if (flags & QUOTE_URI)
				{
					STR("%0F");
				} else
				{
					STR("&si;");
				}
				break;
			case 0x10:
				if (flags & QUOTE_URI)
				{
					STR("%10");
				} else
				{
					STR("&dle;");
				}
				break;
			case 0x11:
				if (flags & QUOTE_URI)
				{
					STR("%11");
				} else
				{
					STR("&dc1;");
				}
				break;
			case 0x12:
				if (flags & QUOTE_URI)
				{
					STR("%12");
				} else
				{
					STR("&dc2;");
				}
				break;
			case 0x13:
				if (flags & QUOTE_URI)
				{
					STR("%13");
				} else
				{
					STR("&dc3;");
				}
				break;
			case 0x14:
				if (flags & QUOTE_URI)
				{
					STR("%14");
				} else
				{
					STR("&dc4;");
				}
				break;
			case 0x15:
				if (flags & QUOTE_URI)
				{
					STR("%15");
				} else
				{
					STR("&nak;");
				}
				break;
			case 0x16:
				if (flags & QUOTE_URI)
				{
					STR("%16");
				} else
				{
					STR("&syn;");
				}
				break;
			case 0x17:
				if (flags & QUOTE_URI)
				{
					STR("%17");
				} else
				{
					STR("&etb;");
				}
				break;
			case 0x18:
				if (flags & QUOTE_URI)
				{
					STR("%18");
				} else
				{
					STR("&can;");
				}
				break;
			case 0x19:
				if (flags & QUOTE_URI)
				{
					STR("%19");
				} else
				{
					STR("&em;");
				}
				break;
			case 0x1a:
				if (flags & QUOTE_URI)
				{
					STR("%1A");
				} else
				{
					STR("&sub;");
				}
				break;
			case 0x1b:
				if (flags & QUOTE_URI)
				{
					STR("%1B");
				} else
				{
					STR("&esc;");
				}
				break;
			case 0x1c:
				if (flags & QUOTE_URI)
				{
					STR("%1C");
				} else
				{
					STR("&fs;");
				}
				break;
			case 0x1D:
				if (flags & QUOTE_URI)
				{
					STR("%1D");
				} else
				{
					STR("&gs;");
				}
				break;
			case 0x1E:
				if (flags & QUOTE_URI)
				{
					STR("%1E");
				} else
				{
					STR("&rs;");
				}
				break;
			case 0x1F:
				if (flags & QUOTE_URI)
				{
					STR("%1F");
				} else
				{
					STR("&us;");
				}
				break;
			default:
				if (c >= 0x80 && (flags & QUOTE_ALLOWUTF8))
				{
					*str++ = c;
				} else if (g_ascii_isalnum(c))
				{
					*str++ = c;
				} else if (flags & QUOTE_URI)
				{
					*str++ = '%';
					*str++ = hex[c >> 4];
					*str++ = hex[c & 0x0f];
				} else if (c >= 0x80 && (flags & QUOTE_UNICODE))
				{
					wchar_t wc;
					--name;
					name = x_utf8_getchar(name, &wc);
					/*
					 * neccessary for hebrew characters to prevent switching to rtl
					 */
					if (wc >= 0x590 && wc <= 0x5ff && !(flags & QUOTE_NOLTR))
						str += sprintf(str, "<span dir=\"ltr\">&#x%lx;</span>", (unsigned long) wc);
					else
						str += sprintf(str, "&#x%lx;", (unsigned long) wc);
				} else
				{
					*str++ = c;
				}
				break;
			}
#undef STR
		}
		*str++ = '\0';
		ret = g_renew(char, ret, str - ret);
	}
	return ret;
}

/* ------------------------------------------------------------------------- */

static gboolean html_out_stylesheet(GString *out)
{
	g_string_append_printf(out, "<link rel=\"stylesheet\" type=\"text/css\" href=\"%s\"%s\n", spdview_css_name, html_closer);
	return TRUE;
}

/* ------------------------------------------------------------------------- */

static gboolean html_out_javascript(GString *out)
{
	g_string_append_printf(out, "<script type=\"text/javascript\" src=\"%s\" charset=\"%s\"></script>\n", spdview_js_name, charset);
	return TRUE;
}

/* ------------------------------------------------------------------------- */

#if 0
static char *html_cgi_params(void)
{
	return g_strdup_printf("%s%s&amp;quality=%d&amp;points=%ld",
		hidemenu ? "&amp;hidemenu=1" : "",
		cgi_cached ? "&amp;cached=1" : "",
		quality,
		point_size);
}
#endif
	
/* ------------------------------------------------------------------------- */

static void html_out_nav_toolbar(GString *out, const ufix8 *font)
{
	int xpos = 0;
	const int button_w = 40;
	const char *void_href = "javascript:void(0);";
	const char *alt;
	const char *disabled;

	g_string_append_printf(out, "<div class=\"%s\">\n", html_toolbar_style);
	g_string_append_printf(out, "<form action=\"%s\" method=\"get\">\n", cgi_scriptname);
	g_string_append(out, "<fieldset style=\"border:0;margin-left:0;margin-right:0;padding-top:0;padding-bottom:0;padding-left:0;padding-right:0;\">\n");
	g_string_append(out, "<legend></legend>\n");
	g_string_append(out, "<ul>\n");
	
	alt = _("Show info about font");
	disabled = "";
	g_string_append_printf(out,
		"<li style=\"position:absolute;left:%dpx;\">"
		"<a href=\"%s\" class=\"%s%s\" onclick=\"showInfo();\" onblur=\"hideInfo();\" accesskey=\"i\" rel=\"copyright\"><img src=\"%s\" alt=\"&nbsp;%s&nbsp;\" title=\"%s\"%s%s</a>"
		"</li>\n",
		xpos,
		void_href, html_nav_img_style, disabled, html_nav_info_png, alt, alt, html_nav_dimensions, html_closer);
	xpos += button_w + 20;

	alt = _("View a new file");
	disabled = "";
	g_string_append_printf(out,
		"<li style=\"position:absolute;left:%dpx;\">"
		"<a href=\"%s\" class=\"%s%s\" accesskey=\"o\"><img src=\"%s\" alt=\"&nbsp;%s&nbsp;\" title=\"%s\"%s%s</a>"
		"</li>\n",
		xpos,
		html_nav_load_href, html_nav_img_style, disabled, html_nav_load_png, alt, alt, html_nav_dimensions, html_closer);
	xpos += button_w;

	g_string_append(out, "</ul>\n");
	g_string_append(out, "</fieldset>\n");
	g_string_append(out, "</form>\n");

	g_string_append(out, "</div>\n");
}

/* ------------------------------------------------------------------------- */

static void html_out_header(GString *out, const ufix8 *font, const char *title, gboolean for_error)
{
	const char *html_lang = "en-US";
	char *str;
	
	g_string_append(out, "<!DOCTYPE html>\n");
	g_string_append_printf(out, "<html xml:lang=\"%s\" lang=\"%s\">\n", html_lang, html_lang);
	g_string_append_printf(out, "<!-- This file was automatically generated by %s version %s -->\n", gl_program_name, gl_program_version);
	g_string_append_printf(out, "<!-- Copyright \302\251 1991-2017 by Thorsten Otto -->\n");
	g_string_append(out, "<head>\n");
	g_string_append_printf(out, "<meta charset=\"%s\"%s\n", charset, html_closer);
	g_string_append_printf(out, "<meta name=\"GENERATOR\" content=\"%s %s\"%s\n", gl_program_name, gl_program_version, html_closer);
	if (title)
	{
		char *quoted = html_quote_name(title, QUOTE_ALLOWUTF8, STR0TERM);
		g_string_append_printf(out, "<title>%s</title>\n", quoted);
		g_free(quoted);
	}
	
	html_out_stylesheet(out);
	html_out_javascript(out);
	
	{
		g_string_append(out, "<!--[if lt IE 9]>\n");
		g_string_append(out, "<script src=\"http://html5shiv.googlecode.com/svn/trunk/html5.js\" type=\"text/javascript\"></script>\n");
		g_string_append(out, "<![endif]-->\n");
	}

	g_string_append(out, "</head>\n");
	g_string_append(out, "<body>\n");

	body_start = out->len;
	
	if (for_error)
	{
		g_string_append_printf(out, "<div class=\"%s\">\n", html_error_note_style);
		g_string_append(out, "<p>\n");
	} else if (font != NULL)
	{
		if (hidemenu)
		{
			g_string_append_printf(out, "<div class=\"%s\">\n", html_node_style);
		} else
		{
			html_out_nav_toolbar(out, font);
			g_string_append_printf(out, "<div class=\"%s\" style=\"position:absolute; top:32px;\">\n", html_node_style);
			body_start = out->len;
		}
	} else
	{
		g_string_append_printf(out, "<div class=\"%s\">\n", html_node_style);
	}
	
	if (font != NULL)
	{
		if (!hidemenu)
		{
			/*
			 * this element is displayed for "Info"
			 */
			g_string_append_printf(out, "<span class=\"%s\">", html_dropdown_style);
			g_string_append_printf(out, "<span class=\"%s\" id=\"%s_content\">\n", html_dropdown_info_style, html_spd_info_id);
			
			g_string_append_printf(out, "Font size: %ld<br />\n", (long)read_4b(font + FH_FNTSZ));
			g_string_append_printf(out, "Minimum font buffer size: %ld<br />\n", (long)read_4b(font + FH_FBFSZ));
			g_string_append_printf(out, "Minimum character buffer size: %u<br />\n", read_2b(font + FH_CBFSZ));
			g_string_append_printf(out, "Header size: %u<br />\n", read_2b(font + FH_HEDSZ));
			g_string_append_printf(out, "Font ID: %u<br />\n", read_2b(font + FH_FNTID));
			g_string_append_printf(out, "Font version number: %u<br />\n", read_2b(font + FH_SFVNR));
			str = html_quote_name((const char *)font + FH_FNTNM, QUOTE_UNICODE, 70);
			g_string_append_printf(out, "Font full name: %s<br />\n", str);
			g_free(str);
			str = html_quote_name((const char *)font + FH_SFNTN, QUOTE_UNICODE, 32);
			g_string_append_printf(out, "Short font name: %s<br />\n", str);
			g_free(str);
			str = html_quote_name((const char *)font + FH_SFACN, QUOTE_UNICODE, 16);
			g_string_append_printf(out, "Short face name: %s<br />\n", str);
			g_free(str);
			str = html_quote_name((const char *)font + FH_FNTFM, QUOTE_UNICODE, 14);
			g_string_append_printf(out, "Font form: %s<br />\n", str);
			g_free(str);
			str = html_quote_name((const char *)font + FH_MDATE, QUOTE_UNICODE, 10);
			g_string_append_printf(out, "Manufacturing date: %s<br />\n", str);
			g_free(str);
			str = html_quote_name((const char *)font + FH_LAYNM, QUOTE_UNICODE, 70);
			g_string_append_printf(out, "Layout name: %s<br />\n", str);
			g_free(str);
			g_string_append_printf(out, "Number of chars in layout: %u<br />\n", read_2b(font + FH_NCHRL));
			g_string_append_printf(out, "Total Number of chars in font: %u<br />\n", read_2b(font + FH_NCHRF));
			g_string_append_printf(out, "Index of first character: %u<br />\n", read_2b(font + FH_FCHRF));
			g_string_append_printf(out, "Number of Kerning Tracks: %u<br />\n", read_2b(font + FH_NKTKS));
			g_string_append_printf(out, "Number of Kerning Pairs: %u<br />\n", read_2b(font + FH_NKPRS));
			g_string_append_printf(out, "Flags: 0x%x<br />\n", read_1b(font + FH_FLAGS));
			g_string_append_printf(out, "Classification Flags: 0x%x<br />\n", read_1b(font + FH_CLFGS));
			g_string_append_printf(out, "Family Classification: 0x%x<br />\n", read_1b(font + FH_FAMCL));
			g_string_append_printf(out, "Font form classification: 0x%x<br />\n", read_1b(font + FH_FRMCL));
			g_string_append_printf(out, "Italic angle: %d<br />\n", read_2b(font + FH_ITANG));
			g_string_append_printf(out, "Number of ORUs per em: %d<br />\n", read_2b(font + FH_ORUPM));
			g_string_append_printf(out, "Width of Wordspace: %d<br />\n", read_2b(font + FH_WDWTH));
			g_string_append_printf(out, "Width of Emspace: %d<br />\n", read_2b(font + FH_EMWTH));
			g_string_append_printf(out, "Width of Enspace: %d<br />\n", read_2b(font + FH_ENWTH));
			g_string_append_printf(out, "Width of Thinspace: %d<br />\n", read_2b(font + FH_TNWTH));
			g_string_append_printf(out, "Width of Figspace: %d<br />\n", read_2b(font + FH_FGWTH));
			g_string_append_printf(out, "Font-wide bounding box: %d %d %d %d<br />\n", read_2b(font + FH_FXMIN), read_2b(font + FH_FYMIN), read_2b(font + FH_FXMAX), read_2b(font + FH_FYMAX));
			g_string_append_printf(out, "Underline position: %d<br />\n", read_2b(font + FH_ULPOS));
			g_string_append_printf(out, "Underline thickness: %d<br />\n", read_2b(font + FH_ULTHK));
			g_string_append_printf(out, "Small caps: %d<br />\n", read_2b(font + FH_SMCTR));
			g_string_append_printf(out, "Display Superiors: %d %7.2f %7.2f<br />\n", read_2b(font + FH_DPSTR), (double) read_2b(font + FH_DPSTR + 2) / 4096.0, (double) read_2b(font + FH_DPSTR + 4) / 4096.0);
			g_string_append_printf(out, "Footnote Superiors: %d %7.2f %7.2f<br />\n", read_2b(font + FH_FNSTR), (double) read_2b(font + FH_FNSTR + 2) / 4096.0, (double) read_2b(font + FH_FNSTR + 4) / 4096.0);
			g_string_append_printf(out, "Alpha Superiors: %d %7.2f %7.2f<br />\n", read_2b(font + FH_ALSTR), (double) read_2b(font + FH_ALSTR + 2) / 4096.0, (double) read_2b(font + FH_ALSTR + 4) / 4096.0);
			g_string_append_printf(out, "Chemical Inferiors: %d %7.2f %7.2f<br />\n", read_2b(font + FH_CMITR), (double) read_2b(font + FH_CMITR + 2) / 4096.0, (double) read_2b(font + FH_CMITR + 4) / 4096.0);
			g_string_append_printf(out, "Small Numerators: %d %7.2f %7.2f<br />\n", read_2b(font + FH_SNMTR), (double) read_2b(font + FH_SNMTR + 2) / 4096.0, (double) read_2b(font + FH_SNMTR + 4) / 4096.0);
			g_string_append_printf(out, "Small Denominators: %d %7.2f %7.2f<br />\n", read_2b(font + FH_SDNTR), (double) read_2b(font + FH_SDNTR + 2) / 4096.0, (double) read_2b(font + FH_SDNTR + 4) / 4096.0);
			g_string_append_printf(out, "Medium Numerators: %d %7.2f %7.2f<br />\n", read_2b(font + FH_MNMTR), (double) read_2b(font + FH_MNMTR + 2) / 4096.0, (double) read_2b(font + FH_MNMTR + 4) / 4096.0);
			g_string_append_printf(out, "Medium Denominators: %d %7.2f %7.2f<br />\n", read_2b(font + FH_MDNTR), (double) read_2b(font + FH_MDNTR + 2) / 4096.0, (double) read_2b(font + FH_MDNTR + 4) / 4096.0);
			g_string_append_printf(out, "Large Numerators: %d %7.2f %7.2f<br />\n", read_2b(font + FH_LNMTR), (double) read_2b(font + FH_LNMTR + 2) / 4096.0, (double) read_2b(font + FH_LNMTR + 4) / 4096.0);
			g_string_append_printf(out, "Large Denominators: %d %7.2f %7.2f<br />\n", read_2b(font + FH_LDNTR), (double) read_2b(font + FH_LDNTR + 2) / 4096.0, (double) read_2b(font + FH_LDNTR + 4) / 4096.0);
	
			g_string_append(out, "</span></span>\n");
		}
	}
}

/* ------------------------------------------------------------------------- */

static void html_out_trailer(GString *out, gboolean for_error)
{
	if (for_error)
	{
		g_string_append(out, "</p>\n");
		g_string_append(out, "</div>\n");
	} else
	{
		g_string_append(out, "</div>\n");
	}
	
	if (errorout->len)
	{
		g_string_insert_len(errorout, 0, 
			"<div class=\"spdview_error_note\">\n"
			"<pre>\n",
			-1);
		g_string_append(errorout,
			"</pre>\n"
			"</div>\n");
		g_string_insert_len(out, body_start, errorout->str, errorout->len);
		g_string_truncate(errorout, 0);
	}
	
	g_string_append(out, "</body>\n");
	g_string_append(out, "</html>\n");
}

/* ------------------------------------------------------------------------- */

/*
 * Called by Speedo character generator to report an error.
 */
void sp_write_error(const char *str, ...)
{
	va_list v;
	
	va_start(v, str);
	g_string_append_vprintf(errorout, str, v);
	g_string_append(errorout, "\n");
	va_end(v);
}

/* ------------------------------------------------------------------------- */

void sp_open_bitmap(fix31 xorg, fix31 yorg, fix15 xsize, fix15 ysize)
{
	fix31 width, pix_width;
	bbox_t bb;
	charinfo *c;

	UNUSED(xorg);
	UNUSED(yorg);
	bit_width = xsize;
	bit_height = ysize;

	if (bit_width > MAX_BITS)
	{
		if (debug)
			g_string_append_printf(errorout, "char 0x%x (0x%x): wider than max bits (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_width = MAX_BITS;
	}
	width = sp_get_char_width(char_index);
	pix_width = width * (specs.xxmult / 65536L) + ((ufix32) width * ((ufix32) specs.xxmult & 0xffff)) / 65536L;
	pix_width /= 65536L;

	width = (pix_width * 7200L) / (point_size * y_res);

	sp_get_char_bbox(char_index, &bb, TRUE);
	bb.xmin >>= 16;
	bb.ymin >>= 16;
	bb.xmax >>= 16;
	bb.ymax >>= 16;

#if DEBUG
	{
		fix15 off_horz;
		fix15 off_vert;
		
		off_horz = (fix15) ((xorg + 32768L) >> 16);
		off_vert = (fix15) ((yorg + 32768L) >> 16);

		if (((bb.xmax - bb.xmin)) != bit_width)
			g_string_append_printf(errorout, "char 0x%x (0x%x): bbox & width mismatch (%ld vs %d)\n",
					char_index, char_id, (unsigned long)(bb.xmax - bb.xmin), bit_width);
		if (((bb.ymax - bb.ymin)) != bit_height)
			g_string_append_printf(errorout, "char 0x%x (0x%x): bbox & height mismatch (%ld vs %d)\n",
					char_index, char_id, (unsigned long)(bb.ymax - bb.ymin), bit_height);
		if ((bb.xmin) != off_horz)
			g_string_append_printf(errorout, "char 0x%x (0x%x): x min mismatch (%ld vs %d)\n", char_index, char_id, (unsigned long)bb.xmin, off_horz);
		if ((bb.ymin) != off_vert)
			g_string_append_printf(errorout, "char 0x%x (0x%x): y min mismatch (%ld vs %d)\n", char_index, char_id, (unsigned long)bb.ymin, off_vert);
	}
#endif

	c = &infos[char_id];
	bit_width = c->bbox.width;
	bit_height = c->bbox.height;

	/* XXX kludge to handle space */
	if (bb.xmin == 0 && bb.ymin == 0 && bb.xmax == 0 && bb.ymax == 0 && width)
	{
		bit_width = 1;
		bit_height = 1;
	}

	if (bit_width > MAX_BITS)
	{
		if (debug)
			g_string_append_printf(errorout, "char 0x%x (0x%x): width too large (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_width = MAX_BITS;
	}
	
	if (bit_height > MAX_BITS)
	{
		if (debug)
			g_string_append_printf(errorout, "char 0x%x (0x%x): height too large (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_height = MAX_BITS;
	}
	
	memset(framebuffer, 0, sizeof(framebuffer));
}

/* ------------------------------------------------------------------------- */

static int trunc = 0;

void sp_set_bitmap_bits(fix15 y, fix15 xbit1, fix15 xbit2)
{
	fix15 i;

	if (xbit1 < 0 || xbit1 >= bit_width)
	{
		if (debug)
			g_string_append_printf(errorout, "char 0x%x (0x%x): bit1 %d wider than max bits %u -- truncated\n", char_index, char_id, xbit1, bit_width);
		xbit1 = MAX_BITS;
		trunc = 1;
	}
	if (xbit2 < 0 || xbit2 > bit_width)
	{
		if (debug)
			g_string_append_printf(errorout, "char 0x%x (0x%x): bit2 %d wider than max bits %u -- truncated\n", char_index, char_id, xbit2, bit_width);
		xbit2 = MAX_BITS;
		trunc = 1;
	}

	if (y < 0 || y >= bit_height)
	{
		if (debug)
			g_string_append_printf(errorout, "char 0x%x (0x%x): y value %d is larger than height %u -- truncated\n", char_index, char_id, y, bit_height);
		trunc = 1;
		return;
	}

	for (i = xbit1; i < xbit2; i++)
	{
		framebuffer[y][i] = vdi_maptab256[1];
	}
}

/* ------------------------------------------------------------------------- */

#if INCL_LCD
boolean sp_load_char_data(long file_offset, fix15 num, fix15 cb_offset, buff_t *char_data)
{
	if (fseek(fp, file_offset, SEEK_SET))
	{
		g_string_append_printf(errorout, "%x (%x): can't seek to char at %ld\n", char_index, char_id, file_offset);
		char_data->org = c_buffer;
		char_data->no_bytes = 0;
		return FALSE;
	}
	if ((num + cb_offset) > mincharsize)
	{
		g_string_append_printf(errorout, "char buf overflow\n");
		char_data->org = c_buffer;
		char_data->no_bytes = 0;
		return FALSE;
	}
	if (fread((c_buffer + cb_offset), sizeof(ufix8), num, fp) != num)
	{
		g_string_append_printf(errorout, "can't get char data\n");
		char_data->org = c_buffer;
		char_data->no_bytes = 0;
		return FALSE;
	}
	char_data->org = c_buffer + cb_offset;
	char_data->no_bytes = num;

	return TRUE;
}
#endif

/* ------------------------------------------------------------------------- */

static gboolean write_png(void)
{
	const charinfo *cinfo = &infos[char_id];
	int i;
	writepng_info *info;
	int rc;
	
	info = writepng_new();
	info->rowbytes = MAX_BITS;
	info->bpp = 8;
	info->image_data = &framebuffer[0][0];
	info->width = cinfo->bbox.width;
	info->height = cinfo->bbox.height;
	info->have_bg = vdi_maptab256[0];
	info->x_res = (x_res * 10000L) / 254;
	info->y_res = (y_res * 10000L) / 254;
	/*
	 * we cannot write a 0x0 image :(
	 */
	if (info->width == 0)
		info->width = 1;
	if (info->height == 0)
		info->height = 1;
	info->outfile = fopen(cinfo->local_filename, "wb");
	if (info->outfile == NULL)
	{
		rc = errno;
	} else
	{
		int num_colors = 256;
		
		info->num_palette = num_colors;
		for (i = 0; i < num_colors; i++)
		{
			int c;
			unsigned char pix;
			pix = vdi_maptab256[i];
			c = palette[i][0]; c = c * 255 / 1000;
			info->palette[pix].red = c;
			c = palette[i][1]; c = c * 255 / 1000;
			info->palette[pix].green = c;
			c = palette[i][2]; c = c * 255 / 1000;
			info->palette[pix].blue = c;
		}
		rc = writepng_output(info);
		fclose(info->outfile);
	}
	writepng_exit(info);
	return rc == 0;
}

/* ------------------------------------------------------------------------- */

void sp_close_bitmap(void)
{
	write_png();
	trunc = 0;
	memset(framebuffer, 0, sizeof(framebuffer));
}

/* ------------------------------------------------------------------------- */

/* outline stubs */
#if INCL_OUTLINE
void sp_open_outline(fix31 x_set_width, fix31 y_set_width, fix31 xmin, fix31 xmax, fix31 ymin, fix31 ymax)
{
	UNUSED(x_set_width);
	UNUSED(y_set_width);
	UNUSED(xmin);
	UNUSED(xmax);
	UNUSED(ymin);
	UNUSED(ymax);
}

/* ------------------------------------------------------------------------- */

void sp_start_sub_char(void)
{
}

/* ------------------------------------------------------------------------- */

void sp_end_sub_char(void)
{
}

/* ------------------------------------------------------------------------- */

void sp_start_contour(fix31 x, fix31 y, boolean outside)
{
	UNUSED(x);
	UNUSED(y);
	UNUSED(outside);
}

/* ------------------------------------------------------------------------- */

void sp_curve_to(fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3)
{
	UNUSED(x1);
	UNUSED(y1);
	UNUSED(x2);
	UNUSED(y2);
	UNUSED(x3);
	UNUSED(y3);
}

/* ------------------------------------------------------------------------- */

void sp_line_to(fix31 x1, fix31 y1)
{
	UNUSED(x1);
	UNUSED(y1);
}

/* ------------------------------------------------------------------------- */

void sp_close_contour(void)
{
}

/* ------------------------------------------------------------------------- */

void sp_close_outline(void)
{
}
#endif

/* ------------------------------------------------------------------------- */

#if 0
static int cmp_info(const void *_a, const void *_b)
{
	const charinfo *a = (const charinfo *)_a;
	const charinfo *b = (const charinfo *)_b;
	return (int16_t)(a->char_id - b->char_id);
}
#endif

/* ------------------------------------------------------------------------- */

static void gen_hor_line(GString *body, int columns)
{
	int i;
	
	g_string_append(body, "<tr>\n");
	for (i = 0; i < columns; i++)
	{
		g_string_append(body, "<td class=\"horizontal_line\" style=\"min-width: 1px;\"></td>\n");
		g_string_append(body, "<td class=\"horizontal_line\"></td>\n");
	}
	g_string_append(body, "<td class=\"horizontal_line\" style=\"min-width: 1px;\"></td>\n");
	g_string_append(body, "</tr>\n");
}

/* ------------------------------------------------------------------------- */

static void update_bbox(charinfo *c, glyphinfo_t *box)
{
	bbox_t bb;
	
	sp_get_char_bbox(c->char_index, &bb, TRUE);
	c->bbox.xmin = bb.xmin;
	c->bbox.ymin = bb.ymin;
	c->bbox.xmax = bb.xmax;
	c->bbox.ymax = bb.ymax;
	box->xmin = MIN(box->xmin, c->bbox.xmin);
	box->ymin = MIN(box->ymin, c->bbox.ymin);
	box->xmax = MIN(box->xmax, c->bbox.xmax);
	box->ymax = MIN(box->ymax, c->bbox.ymax);
	c->bbox.width = ((bb.xmax - bb.xmin) + 32768L) >> 16;
	c->bbox.height = ((bb.ymax - bb.ymin) + 32768L) >> 16;
	c->bbox.lbearing = (bb.xmin + 32768L) >> 16;
	c->bbox.off_vert = bb.ymin >> 16;
	box->lbearing = MIN(box->lbearing, c->bbox.lbearing);
	c->bbox.rbearing = c->bbox.width + c->bbox.lbearing;
	box->rbearing = MAX(box->rbearing, c->bbox.rbearing);
	c->bbox.ascent = c->bbox.height + c->bbox.off_vert;
	c->bbox.descent = c->bbox.height - c->bbox.ascent;
	box->width = MAX(box->width, c->bbox.width);
	box->height = MAX(box->height, c->bbox.height);
	box->ascent = MAX(box->ascent, c->bbox.ascent);
	box->descent = MAX(box->descent, c->bbox.descent);
}

/* ------------------------------------------------------------------------- */

static const char *get_uniname(uint16_t unicode)
{
	size_t a, b, c;
	const struct ucd *p;
	
	a = 0;
	b = sizeof(ucd_bycode) / sizeof(ucd_bycode[0]);
	while (a < b)
	{
		c = (a + b) >> 1;				/* == ((a + b) / 2) */
		p = &ucd_bycode[c];
		if (p->code == unicode)
			return p->name;
		if (p->code > unicode)
			b = c;
		else
			a = c + 1;
	}
	return "UNDEFINED";
}

/* ------------------------------------------------------------------------- */

static gboolean gen_speedo_font(const char *filename, GString *body)
{
	gboolean decode_ok = TRUE;
	const ufix8 *key;
	uint16_t i, id, j;
	uint16_t num_ids;
	
	/* init */
	sp_reset();

	key = sp_get_key(&font);
	if (key == NULL)
	{
		sp_write_error("Non-standard encryption");
#if 0
		decode_ok = FALSE;
#endif
	} else
	{
		sp_set_key(key);
	}
	
	first_char_index = read_2b(font_buffer + FH_FCHRF);
	num_chars = read_2b(font_buffer + FH_NCHRL);
	if (num_chars == 0)
	{
		g_string_append(errorout, "no characters in font\n");
		return FALSE;
	} 
	
	/* set up specs */
	/* Note that point size is in decipoints */
	specs.xxmult = point_size * x_res / 720 * (1L << 16);
	specs.xymult = 0L << 16;
	specs.xoffset = 0L << 16;
	specs.yxmult = 0L << 16;
	specs.yymult = point_size * y_res / 720 * (1L << 16);
	specs.yoffset = 0L << 16;
	switch (quality)
	{
	case 0:
		specs.flags = MODE_BLACK;
		break;
	case 1:
	default:
		specs.flags = MODE_SCREEN;
		break;
	case 2:
#if INCL_OUTLINE
		specs.flags = MODE_OUTLINE;
#else
		specs.flags = MODE_2D;
#endif
		break;
	case 3:
		specs.flags = MODE_2D;
		break;
	}

	chomp(fontname, (char *) (font_buffer + FH_FNTNM), sizeof(fontname));
	make_font_filename(fontfilename, fontname);
	html_out_header(body, font_buffer, fontname, FALSE);
	
	if (!sp_set_specs(&specs, &font))
	{
		decode_ok = FALSE;
	} else
	{
		time_t t;
		struct tm tm;
		char *basename;
		int columns = 0;
		
		t = time(NULL);
		tm = *gmtime(&t);
		basename = g_strdup_printf("%s_%04d%02d%02d%02d%02d%02d_",
			fontfilename,
			tm.tm_year + 1900,
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec);
		
		num_ids = 0;
		for (i = 0; i < num_chars; i++)
		{
			char_index = i + first_char_index;
			char_id = sp_get_char_id(char_index);
			if (char_id != SP_UNDEFINED && char_id != UNDEFINED && char_id >= num_ids)
				num_ids = char_id + 1;
		}
		
		if (num_ids == 0)
		{
			g_string_append(errorout, "no characters in font\n");
			return FALSE;
		} 
#if 0
		num_ids = sp_get_char_id(num_chars + first_char_index - 1) + 1;
		num_ids = ((num_ids + CHAR_COLUMNS - 1) / CHAR_COLUMNS) * CHAR_COLUMNS;
#endif
		
		infos = g_new(charinfo, num_ids);
		if (infos == NULL)
		{
			g_string_append_printf(errorout, "%s\n", strerror(errno));
			return FALSE;
		}
		
		for (i = 0; i < num_ids; i++)
		{
			infos[i].char_index = 0;
			infos[i].char_id = UNDEFINED;
			infos[i].local_filename = NULL;
			infos[i].url = NULL;
		}

		/*
		 * determine the average width of all the glyphs in the font
		 */
		{
			unsigned long total_width;
			uint16_t num_glyphs;
			glyphinfo_t max_bb;
			fix15 max_lbearing;
			fix15 min_descent;
			fix15 average_width;
			
			total_width = 0;
			num_glyphs = 0;
			max_bb.width = 0;
			max_bb.height = 0;
			max_bb.off_vert = 0;
			max_bb.ascent = -32000;
			max_bb.descent = -32000;
			max_bb.rbearing = -32000;
			max_bb.lbearing = 32000;
			max_bb.xmin = 32000L << 16;
			max_bb.ymin = 32000L << 16;
			max_bb.xmax = -(32000L << 16);
			max_bb.ymax = -(32000L << 16);
			max_lbearing = -32000;
			min_descent = 32000;
			for (i = 0; i < num_chars; i++)
			{
				char_index = i + first_char_index;
				char_id = sp_get_char_id(char_index);
				if (char_id != SP_UNDEFINED && char_id != UNDEFINED)
				{
					charinfo *c = &infos[char_id];
					
					if (c->char_id != UNDEFINED)
					{
						if (debug)
							g_string_append_printf(errorout, "char 0x%x (0x%x) already defined\n", char_index, char_id);
					} else
					{
						c->char_index = char_index;
						c->char_id = char_id;
						update_bbox(c, &max_bb);
						max_lbearing = MAX(max_lbearing, c->bbox.lbearing);
						min_descent = MIN(min_descent, c->bbox.descent);
						total_width += c->bbox.width;
						num_glyphs++;
					}
				}
			}
			if (num_glyphs == 0)
				average_width = max_bb.width;
			else
				average_width = (fix15)(total_width / num_glyphs);
			if (debug)
			{
				g_string_append_printf(errorout, "max height %d max width %d average width %d\n", max_bb.height, max_bb.width, average_width);
			}
			
			max_bb.height = max_bb.ascent + max_bb.descent;
			max_bb.width = max_bb.rbearing - max_bb.lbearing;

			if (debug)
			{
				fix15 xmin, ymin, xmax, ymax;
				long fwidth, fheight;
				long pixel_size = (point_size * x_res + 360) / 720;
				
				xmin = read_2b(font_buffer + FH_FXMIN);
				ymin = read_2b(font_buffer + FH_FYMIN);
				xmax = read_2b(font_buffer + FH_FXMAX);
				ymax = read_2b(font_buffer + FH_FYMAX);
				fwidth = xmax - xmin;
				fwidth = fwidth * pixel_size / sp_globals.orus_per_em;
				fheight = ymax - ymin;
				fheight = fheight * pixel_size / sp_globals.orus_per_em;
				g_string_append_printf(errorout, "bbox: %d %d ascent %d descent %d lb %d rb %d\n",
					max_bb.width, max_bb.height,
					max_bb.ascent, max_bb.descent,
					max_bb.lbearing, max_bb.rbearing);
				g_string_append_printf(errorout, "bbox (header): %d %d %d %d; %ld %ld %ld %ld\n",
					xmin, ymin,
					xmax, ymax,
					xmin * pixel_size / sp_globals.orus_per_em,
					ymin * pixel_size / sp_globals.orus_per_em,
					xmax * pixel_size / sp_globals.orus_per_em,
					ymax * pixel_size / sp_globals.orus_per_em);
			}
			
			font_bb = max_bb;
		}
		
		for (i = 0; i < num_chars; i++)
		{
			char_index = i + first_char_index;
			char_id = sp_get_char_id(char_index);
			if (char_id != SP_UNDEFINED && char_id != UNDEFINED)
			{
				infos[char_id].local_filename = g_strdup_printf("%s/%schr%04x.png", output_dir, basename, char_id);
				infos[char_id].url = g_strdup_printf("%s/%schr%04x.png", output_url, basename, char_id);
				if (!sp_make_char(char_index))
				{
					g_string_append_printf(errorout, "can't make char 0x%x (0x%x)\n", char_index, char_id);
				}
			}
		}
		
#if 0
		qsort(infos, num_chars, sizeof(infos[0]), cmp_info);
#endif
		
		g_string_append(body, "<table cellspacing=\"0\" cellpadding=\"0\">\n");
		for (id = 0; id < num_ids; id += CHAR_COLUMNS)
		{
			gboolean defined[CHAR_COLUMNS];
			const char *klass;
			const char *img[CHAR_COLUMNS];
			static char const vert_line[] = "<td class=\"vertical_line\"></td>\n";
			
			if (id != 0 && (id % PAGE_SIZE) == 0)
				g_string_append(body, "<tr><td width=\"1\" height=\"10\" style=\"padding: 0px; margin:0px;\"></td></tr>\n");

			columns = CHAR_COLUMNS;
			if ((id + columns) > num_ids)
				columns = num_ids - id;
			
			if ((id % PAGE_SIZE) == 0)
				gen_hor_line(body, columns);
			
			g_string_append(body, "<tr>\n");
			for (j = 0; j < columns; j++)
			{
				uint16_t char_id = id + j;
				charinfo *c = &infos[char_id];
				
				defined[j] = FALSE;
				img[j] = NULL;
				if (c->char_id != UNDEFINED)
				{
					defined[j] = TRUE;
					if (c->url != NULL)
						img[j] = c->url;
				}
				klass = defined[j] ? "spd_glyph_defined" : "spd_glyph_undefined";
				
				g_string_append(body, vert_line);
				
				g_string_append_printf(body,
					"<td class=\"%s\">%x</td>\n",
					klass,
					id + j);
			}				
				
			g_string_append(body, vert_line);
			g_string_append(body, "</tr>\n");
			gen_hor_line(body, columns);
				
			g_string_append(body, "<tr>\n");
			for (j = 0; j < columns; j++)
			{
				uint16_t char_id = id + j;
				charinfo *c = &infos[char_id];
				
				g_string_append(body, vert_line);
				
				if (img[j] != NULL)
				{
					uint16_t unicode;
					char *debuginfo = NULL;
					
					unicode = c->char_index < BICS_COUNT ? Bics2Unicode[c->char_index] : UNDEFINED;
					if (debug)
					{
						debuginfo = g_strdup_printf("Width: %u Height %u&#10;Ascent: %d Descent: %d lb: %d rb: %d&#10;",
							c->bbox.width, c->bbox.height, c->bbox.ascent, c->bbox.descent, c->bbox.lbearing, c->bbox.rbearing);
					}
					g_string_append_printf(body,
						"<td class=\"spd_glyph_image\" style=\"width: %dpx; height: %dpx; min-width: %dpx; min-height: %dpx;\" title=\""
						"Index: 0x%x (%u)&#10;"
						"ID: 0x%04x&#10;"
						"Unicode: 0x%04x &#%u;&#10;"
						"%s&#10;"
						"Xmin: %7.2f Ymin: %7.2f&#10;"
						"Xmax: %7.2f Ymax: %7.2f&#10;%s"
						"\"><img alt=\"\" style=\"text-align: left; vertical-align: top; position: relative; left: %dpx; top: %dpx\" src=\"%s\"></td>",
						font_bb.width, font_bb.height,
						font_bb.width, font_bb.height,
						c->char_index, c->char_index,
						c->char_id,
						unicode, unicode,
						get_uniname(unicode),
						(double)c->bbox.xmin / 65536.0,
						(double)c->bbox.ymin / 65536.0,
						(double)c->bbox.xmax / 65536.0,
						(double)c->bbox.ymax / 65536.0,
						debuginfo ? debuginfo : "",
						-(font_bb.lbearing - c->bbox.lbearing),
						font_bb.ascent - c->bbox.ascent,
						img[j]);
					g_free(debuginfo);
				} else
				{
					g_string_append_printf(body,
						"<td class=\"spd_glyph_image\" style=\"width: %dpx; height: %dpx;\"></td>", 0, 0);
				}
			}				
				
			g_string_append(body, vert_line);
			g_string_append(body, "</tr>\n");
			gen_hor_line(body, columns);
		}
		if ((id % PAGE_SIZE) != 0)
			gen_hor_line(body, columns);
		g_string_append(body, "</table>\n");

		for (i = 0; i < num_ids; i++)
		{
			g_free(infos[i].local_filename);
			g_free(infos[i].url);
		}
		g_free(infos);
		infos = NULL;
		g_free(basename);
	}
		
	return decode_ok;
}

/* ------------------------------------------------------------------------- */

static gboolean load_speedo_font(const char *filename, GString *body)
{
	gboolean ret;
	ufix8 tmp[FH_FBFSZ + 4];
	ufix32 minbufsize;
	const char *title = NULL;
	
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		g_string_append_printf(errorout, "%s: %s\n", filename, strerror(errno));
		ret = FALSE;
	} else
	{
		if (fread(tmp, sizeof(tmp), 1, fp) != 1)
		{
			g_string_append_printf(errorout, "%s: read error\n", filename);
			ret = FALSE;
		} else if (read_4b(tmp + FH_FMVER + 4) != 0x0d0a0000L)
		{
			sp_report_error(4);
		} else
		{
			g_free(font_buffer);
			g_free(c_buffer);
			c_buffer = NULL;
			minbufsize = (ufix32) read_4b(tmp + FH_FBFSZ);
			font_buffer = g_new(ufix8, minbufsize);
			if (font_buffer == NULL)
			{
				g_string_append_printf(errorout, "%s\n", strerror(errno));
				ret = FALSE;
			} else
			{
				fseek(fp, 0, SEEK_SET);
				if (fread(font_buffer, minbufsize, 1, fp) != 1)
				{
					g_free(font_buffer);
					font_buffer = NULL;
					g_string_append_printf(errorout, "%s: read error\n", filename);
					ret = FALSE;
				} else
				{
					mincharsize = read_2b(font_buffer + FH_CBFSZ);
				
					c_buffer = g_new(ufix8, mincharsize);
					if (c_buffer == NULL)
					{
						g_free(font_buffer);
						font_buffer = NULL;
						g_string_append_printf(errorout, "%s\n", strerror(errno));
						ret = FALSE;
					} else
					{
						font.org = font_buffer;
						font.no_bytes = minbufsize;
					
						title = fontname;
						ret = gen_speedo_font(filename, body);
					
						g_free(font_buffer);
						font_buffer = NULL;
						g_free(c_buffer);
						c_buffer = NULL;
					}
				}
			}
		}
		fclose(fp);
	}
	
	if (title == NULL)
		html_out_header(body, NULL, xbasename(filename), FALSE);
	html_out_trailer(body, FALSE);

	return ret;
}

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

static gboolean uri_has_scheme(const char *uri)
{
	gboolean colon = FALSE;
	
	if (uri == NULL)
		return FALSE;
	while (*uri)
	{
		if (*uri == ':')
			colon = TRUE;
		else if (*uri == '/')
			return colon;
		uri++;
	}
	return colon;
}

/* ------------------------------------------------------------------------- */

static void html_out_response_header(FILE *out, unsigned long len)
{
	fprintf(out, "Content-Type: %s;charset=%s\015\012", "text/html", charset);
	fprintf(out, "Content-Length: %lu\015\012", len);
	fprintf(out, "Cache-Control: no-cache\015\012");
	fprintf(out, "\015\012");
}

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

static size_t mycurl_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct curl_parms *parms = (struct curl_parms *) userdata;
	
	if (size == 0 || nmemb == 0)
		return 0;
	if (parms->fp == NULL)
	{
		parms->fp = fopen(parms->filename, "wb");
		if (parms->fp == NULL)
			g_string_append_printf(errorout, "%s: %s\n", parms->filename, strerror(errno));
	}
	if (parms->fp == NULL)
		return 0;
	return fwrite(ptr, size, nmemb, parms->fp);
}

/* ------------------------------------------------------------------------- */

static int mycurl_trace(CURL *handle, curl_infotype type, char *data, size_t size, void *userdata)
{
	struct curl_parms *parms = (struct curl_parms *) userdata;

	UNUSED(handle);
	UNUSED(parms);
	switch (type)
	{
	case CURLINFO_TEXT:
		fprintf(errorfile, "== Info: %s", data);
		if (size == 0 || data[size - 1] != '\n')
			fputc('\n', errorfile);
		break;
	case CURLINFO_HEADER_OUT:
		fprintf(errorfile, "=> Send header %ld\n", (long)size);
		fwrite(data, 1, size, errorfile);
		break;
	case CURLINFO_DATA_OUT:
		fprintf(errorfile, "=> Send data %ld\n", (long)size);
		break;
	case CURLINFO_SSL_DATA_OUT:
		fprintf(errorfile, "=> Send SSL data %ld\n", (long)size);
		break;
	case CURLINFO_HEADER_IN:
		fprintf(errorfile, "<= Recv header %ld\n", (long)size);
		fwrite(data, 1, size, errorfile);
		break;
	case CURLINFO_DATA_IN:
		fprintf(errorfile, "<= Recv data %ld\n", (long)size);
		break;
	case CURLINFO_SSL_DATA_IN:
		fprintf(errorfile, "<= Recv SSL data %ld\n", (long)size);
		break;
	case CURLINFO_END:
	default:
		break;
 	}
	return 0;
}

/* ------------------------------------------------------------------------- */

static const char *currdate(void)
{
	struct tm *tm;
	static char buf[40];
	time_t t;
	t = time(NULL);
	tm = localtime(&t);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
	return buf;
}
	
/* ------------------------------------------------------------------------- */

static char *curl_download(CURL *curl, GString *body, const char *filename)
{
	char *local_filename;
	struct curl_parms parms;
	struct stat st;
	long unmet;
	long respcode;
	CURLcode curlcode;
	double size;
	char err[CURL_ERROR_SIZE];
	char *content_type;

	curl_easy_setopt(curl, CURLOPT_URL, filename);
	curl_easy_setopt(curl, CURLOPT_REFERER, filename);
	local_filename = g_build_filename(output_dir, xbasename(filename), NULL);
	parms.filename = local_filename;
	parms.fp = NULL;
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mycurl_write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &parms);
	curl_easy_setopt(curl, CURLOPT_STDERR, errorfile);
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, ALLOWED_PROTOS);
	curl_easy_setopt(curl, CURLOPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, (long)1);
	curl_easy_setopt(curl, CURLOPT_FILETIME, (long)1);
	curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, mycurl_trace);
	curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &parms);
	*err = 0;
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err);
	
	/* set this to 1 to activate debug code above */
	curl_easy_setopt(curl, CURLOPT_VERBOSE, (long)0);

	if (stat(local_filename, &st) == 0)
	{
		curlcode = curl_easy_setopt(curl, CURLOPT_TIMECONDITION, (long)CURL_TIMECOND_IFMODSINCE);
		curlcode = curl_easy_setopt(curl, CURLOPT_TIMEVALUE, (long)st.st_mtime);
	}
	
	/*
	 * TODO: reject attempts to connect to local addresses
	 */
	curlcode = curl_easy_perform(curl);
	
	respcode = 0;
	unmet = -1;
	size = 0;
	content_type = NULL;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &respcode);
	curl_easy_getinfo(curl, CURLINFO_CONDITION_UNMET, &unmet);
	curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &size);
	curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
	fprintf(errorfile, "%s: GET from %s, url=%s, curl=%d, resp=%ld, size=%ld, content=%s, local=%s\n", currdate(), fixnull(cgiRemoteHost), filename, curlcode, respcode, (long)size, printnull(content_type), local_filename);
	
	if (parms.fp)
	{
		fclose(parms.fp);
		parms.fp = NULL;
	}
	
	if (curlcode != CURLE_OK || stat(local_filename, &st) != 0)
	{
		html_out_header(body, NULL, err, TRUE);
		g_string_append_printf(body, "%s:\n%s", _("Download error"), err);
		html_out_trailer(body, TRUE);
		unlink(local_filename);
		g_free(local_filename);
		local_filename = NULL;
	} else if ((respcode != 200 && respcode != 304) ||
		(respcode == 200 && content_type != NULL && strcmp(content_type, "text/plain") == 0))
	{
		/* most likely the downloaded data will contain the error page */
		parms.fp = fopen(local_filename, "rb");
		if (parms.fp != NULL)
		{
			size_t nread;
			
			while ((nread = fread(err, 1, sizeof(err), parms.fp)) > 0)
				g_string_append_len(body, err, nread);
			fclose(parms.fp);
		}
		unlink(local_filename);
		g_free(local_filename);
		local_filename = NULL;
	} else
	{
		long ft = -1;
		if (curl_easy_getinfo(curl, CURLINFO_FILETIME, &ft) == CURLE_OK && ft != -1)
		{
			struct utimbuf ut;
			ut.actime = ut.modtime = ft;
			utime(local_filename, &ut);
		}
	}
	
	return local_filename;
}

/* ------------------------------------------------------------------------- */

static void write_strout(GString *s, FILE *outfp)
{
	if (force_crlf)
	{
		const char *txt = s->str;
		while (*txt)
		{
			if (*txt == '\n')
			{
				fputc('\015', outfp);
				fputc('\012', outfp);
			}  else
			{
				fputc(*txt, outfp);
			}
			txt++;
		}
	} else
	{
		fwrite(s->str, 1, s->len, outfp);
	}
}

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

int main(void)
{
	int retval = EXIT_SUCCESS;
	FILE *out = stdout;
	GString *body;
	CURL *curl = NULL;
	char *val;
	
	errorfile = fopen("error.log", "a");
	if (errorfile == NULL)
		errorfile = stderr;
	errorout = g_string_new(NULL);
	
	body = g_string_new(NULL);
	cgiInit(body);

	{
		char *dir = spd_path_get_dirname(cgiScriptFilename);
		char *cache_dir = g_build_filename(dir, cgi_cachedir, NULL);
		DIR *dp;
		struct dirent *e;
		
		output_dir = g_build_filename(cache_dir, cgiRemoteAddr, NULL);
		output_url = g_build_filename(cgi_cachedir, cgiRemoteAddr, NULL);

		if (mkdir(cache_dir, 0750) < 0 && errno != EEXIST)
			g_string_append_printf(errorout, "%s: %s\n", cache_dir, strerror(errno));
		if (mkdir(output_dir, 0750) < 0 && errno != EEXIST)
			g_string_append_printf(errorout, "%s: %s\n", output_dir, strerror(errno));
		
		/*
		 * clean up from previous run(s),
		 * otherwise lots of files pile up there
		 */
		dp = opendir(output_dir);
		if (dp != NULL)
		{
			while ((e = readdir(dp)) != NULL)
			{
				char *f;
				const char *p;
				
				if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
					continue;
				if ((p = strrchr(e->d_name, '.')) != NULL && strcmp(p, ".png") == 0)
				{
					f = g_build_filename(output_dir, e->d_name, NULL);
					unlink(f);
					g_free(f);
				}
			}
			closedir(dp);
		}
		
		g_free(cache_dir);
		g_free(dir);
	}
	
	if (cgiScriptName)
		cgi_scriptname = cgiScriptName;
	
	point_size = 120;
	if ((val = cgiFormString("points")) != NULL)
	{
		point_size = strtol(val, NULL, 10);
		g_free(val);
		if (point_size < 40 || point_size > 10000)
			point_size = 120;
	}
	
	quality = 1;
	if ((val = cgiFormString("quality")) != NULL)
	{
		quality = (int)strtol(val, NULL, 10);
		g_free(val);
	}
	
	cgi_cached = FALSE;
	if ((val = cgiFormString("cached")) != NULL)
	{
		cgi_cached = (int)strtol(val, NULL, 10) != 0;
		g_free(val);
	}

	x_res = y_res = 72;
	if ((val = cgiFormString("resolution")) != NULL)
	{
		x_res = y_res = (int)strtol(val, NULL, 10);
		if (x_res < 10 || x_res > 10000)
			x_res = y_res = 72;
		g_free(val);
	}
	if ((val = cgiFormString("xresolution")) != NULL)
	{
		x_res = (int)strtol(val, NULL, 10);
		if (x_res < 10 || x_res > 10000)
			x_res = 72;
		g_free(val);
	}
	if ((val = cgiFormString("yresolution")) != NULL)
	{
		y_res = (int)strtol(val, NULL, 10);
		if (y_res < 10 || y_res > 10000)
			y_res = 72;
		g_free(val);
	}
	
	hidemenu = FALSE;
	if ((val = cgiFormString("hidemenu")) != NULL)
	{
		hidemenu = strtol(val, NULL, 10) != 0;
		g_free(val);
	}
	
	debug = FALSE;
	if ((val = cgiFormString("debug")) != NULL)
	{
		debug = strtol(val, NULL, 10) != 0;
		g_free(val);
	}
	
	if (g_ascii_strcasecmp(cgiRequestMethod, "GET") == 0)
	{
		char *url = cgiFormString("url");
		char *filename = g_strdup(url);
		char *scheme = empty(filename) ? g_strdup("undefined") : uri_has_scheme(filename) ? g_strndup(filename, strchr(filename, ':') - filename) : g_strdup("file");
		
		if (filename && filename[0] == '/')
		{
			html_referer_url = filename;
			filename = g_strconcat(cgiDocumentRoot, filename, NULL);
		} else if (empty(xbasename(filename)) || (!cgi_cached && g_ascii_strcasecmp(scheme, "file") == 0))
		{
			/*
			 * disallow file URIs, they would resolve to local files on the WEB server
			 */
			html_out_header(body, NULL, _("403 Forbidden"), TRUE);
			g_string_append_printf(body,
				_("Sorry, this type of\n"
				  "<a href=\"http://www.w3.org/Addressing/\">URL</a>\n"
				  "<a href=\"http://www.iana.org/assignments/uri-schemes.html\">scheme</a>\n"
				  "(<q>%s</q>) is not\n"
				  "supported by this service. Please check that you entered the URL correctly.\n"),
				scheme);
			html_out_trailer(body, TRUE);
			g_free(filename);
			filename = NULL;
		} else
		{
			if (!cgi_cached &&
				(curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK ||
				(curl = curl_easy_init()) == NULL))
			{
				html_out_header(body, NULL, _("500 Internal Server Error"), TRUE);
				g_string_append(body, _("could not initialize curl\n"));
				html_out_trailer(body, TRUE);
				retval = EXIT_FAILURE;
			} else
			{
				char *local_filename;
				
				if (cgi_cached)
				{
					html_referer_url = g_strdup(xbasename(filename));
					local_filename = g_build_filename(output_dir, html_referer_url, NULL);
					g_free(filename);
					filename = local_filename;
				} else
				{
					html_referer_url = g_strdup(filename);
					local_filename = curl_download(curl, body, filename);
					g_free(filename);
					filename = local_filename;
					if (filename)
						cgi_cached = TRUE;
				}
			}
		}
		if (filename && retval == EXIT_SUCCESS)
		{
			if (load_speedo_font(filename, body) == FALSE)
			{
				retval = EXIT_FAILURE;
			}
			g_free(filename);
		}
		g_free(scheme);
		g_free(html_referer_url);
		html_referer_url = NULL;
		
		g_free(url);
	} else if (g_ascii_strcasecmp(cgiRequestMethod, "POST") == 0 ||
		g_ascii_strcasecmp(cgiRequestMethod, "BOTH") == 0)
	{
		char *filename;
		int len;
		
		g_string_truncate(body, 0);
		filename = cgiFormFileName("file", &len);
		if (filename == NULL || len == 0)
		{
			const char *scheme = "undefined";
			html_out_header(body, NULL, _("403 Forbidden"), TRUE);
			g_string_append_printf(body,
				_("Sorry, this type of\n"
				  "<a href=\"http://www.w3.org/Addressing/\">URL</a>\n"
				  "<a href=\"http://www.iana.org/assignments/uri-schemes.html\">scheme</a>\n"
				  "(<q>%s</q>) is not\n"
				  "supported by this service. Please check that you entered the URL correctly.\n"),
				scheme);
			html_out_trailer(body, TRUE);
		} else
		{
			FILE *fp = NULL;
			char *local_filename;
			const char *data;
			
			if (*filename == '\0')
			{
				g_free(filename);
#if defined(HAVE_MKSTEMPS)
				{
				int fd;
				filename = g_strdup("tmpfile.XXXXXX.spd");
				local_filename = g_build_filename(output_dir, xbasename(filename), NULL);
				fd = mkstemps(local_filename, 4);
				if (fd > 0)
					fp = fdopen(fd, "wb");
				}
#elif defined(HAVE_MKSTEMP)
				{
				int fd;
				filename = g_strdup("tmpfile.spd.XXXXXX");
				local_filename = g_build_filename(output_dir, xbasename(filename), NULL);
				fd = mkstemp(local_filename);
				if (fd > 0)
					fp = fdopen(fd, "wb");
				}
#else
				filename = g_strdup("tmpfile.spd.XXXXXX");
				local_filename = g_build_filename(output_dir, xbasename(filename), NULL);
				mktemp(local_filename);
				fp = fopen(local_filename, "wb");
#endif
			} else
			{
				local_filename = g_build_filename(output_dir, xbasename(filename), NULL);
				fp = fopen(local_filename, "wb");
			}

			fprintf(errorfile, "%s: POST from %s, file=%s, size=%d\n", currdate(), fixnull(cgiRemoteHost), xbasename(filename), len);

			if (fp == NULL)
			{
				const char *err = strerror(errno);
				fprintf(errorfile, "%s: %s\n", local_filename, err);
				html_out_header(body, NULL, _("404 Not Found"), TRUE);
				g_string_append_printf(body, "%s: %s\n", xbasename(filename), err);
				html_out_trailer(body, TRUE);
				retval = EXIT_FAILURE;
			} else
			{
				data = cgiFormFileData("file", &len);
				fwrite(data, 1, len, fp);
				fclose(fp);
				cgi_cached = TRUE;
				html_referer_url = g_strdup(filename);
				if (load_speedo_font(local_filename, body) == FALSE)
				{
					retval = EXIT_FAILURE;
				}
			}
			g_free(local_filename);
		}
		g_free(filename);
		g_free(html_referer_url);
		html_referer_url = NULL;
	}
	
	html_out_response_header(out, body->len);
	cgiExit();
	write_strout(body, out);
	g_string_free(body, TRUE);
	g_string_free(errorout, TRUE);
	
	if (curl)
	{
		curl_easy_cleanup(curl);
		curl_global_cleanup();
	}
	
	fflush(errorfile);
	if (errorfile != stderr)
		fclose(errorfile);
	
	return retval;
}
