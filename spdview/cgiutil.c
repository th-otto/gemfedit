#include "linux/libcwrap.h"
#include "defs.h"
#include <sys/time.h>
#include <utime.h>
#include <unistd.h>
#include "cgic.h"
#include "cgiutil.h"
#include "version.h"

char const gl_program_version[] = PACKAGE_VERSION " - " PACKAGE_DATE;

char const spdview_css_name[] = "_spdview.css";
char const spdview_js_name[] = "_spdview.js";
char const charset[] = "UTF-8";

char const cgi_cachedir[] = "cache";

char const html_error_note_style[] = "spdview_error_note";
char const html_node_style[] = "spdview_node";
char const html_spd_info_id[] = "spd_info";
char const html_toolbar_style[] = "spdview_nav_toolbar";
char const html_dropdown_style[] = "spdview_dropdown";
char const html_dropdown_info_style[] = "spdview_dropdown_info";
char const html_nav_img_style[] = "spdview_nav_img";

char const html_nav_info_png[] = "images/iinfo.png";
char const html_nav_load_png[] = "images/iload.png";
char const html_nav_dimensions[] = " width=\"32\" height=\"21\"";

gboolean hidemenu;
gboolean debug;

const char *html_closer = " />";
char *output_dir;
char *output_url;
char *html_referer_url;
GString *errorout;
FILE *errorfile;
gboolean cgi_cached;

static size_t body_start;
static int force_crlf = FALSE;

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

void write_strout(GString *s, FILE *outfp)
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

/* ------------------------------------------------------------------------- */

char *html_quote_name(const char *name, unsigned int flags, size_t len)
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

gboolean html_out_stylesheet(GString *out)
{
	g_string_append_printf(out, "<link rel=\"stylesheet\" type=\"text/css\" href=\"%s\"%s\n", spdview_css_name, html_closer);
	return TRUE;
}

/* ------------------------------------------------------------------------- */

gboolean html_out_javascript(GString *out)
{
	g_string_append_printf(out, "<script type=\"text/javascript\" src=\"%s\" charset=\"%s\"></script>\n", spdview_js_name, charset);
	return TRUE;
}

/* ------------------------------------------------------------------------- */

static void html_out_nav_toolbar(GString *out)
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

void html_out_header(GString *out, GString *font_info, const char *title, gboolean for_error)
{
	const char *html_lang = "en-US";
	
	g_string_append(out, "<!DOCTYPE html>\n");
	g_string_append_printf(out, "<html xml:lang=\"%s\" lang=\"%s\">\n", html_lang, html_lang);
	g_string_append_printf(out, "<!-- This file was automatically generated by %s version %s -->\n", gl_program_name, gl_program_version);
	g_string_append_printf(out, "<!-- Copyright \302\251 1991-2018 by Thorsten Otto -->\n");
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
	} else if (font_info != NULL)
	{
		if (hidemenu)
		{
			g_string_append_printf(out, "<div class=\"%s\">\n", html_node_style);
		} else
		{
			html_out_nav_toolbar(out);
			g_string_append_printf(out, "<div class=\"%s\" style=\"position:absolute; top:32px;\">\n", html_node_style);
			body_start = out->len;
		}
	} else
	{
		g_string_append_printf(out, "<div class=\"%s\">\n", html_node_style);
	}
	
	if (font_info != NULL)
	{
		if (!hidemenu)
		{
			/*
			 * this element is displayed for "Info"
			 */
			g_string_append_printf(out, "<span class=\"%s\">", html_dropdown_style);
			g_string_append_printf(out, "<span class=\"%s\" id=\"%s_content\">\n", html_dropdown_info_style, html_spd_info_id);
			
			g_string_append(out, "</span></span>\n");
		}
	}
}

/* ------------------------------------------------------------------------- */

void html_out_trailer(GString *out, gboolean for_error)
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

gboolean uri_has_scheme(const char *uri)
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

void html_out_response_header(FILE *out, unsigned long len)
{
	fprintf(out, "Content-Type: %s;charset=%s\015\012", "text/html", charset);
	fprintf(out, "Content-Length: %lu\015\012", len);
	fprintf(out, "Cache-Control: no-cache\015\012");
	fprintf(out, "\015\012");
}

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

struct curl_parms {
	const char *filename;
	FILE *fp;
};

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

const char *currdate(void)
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

char *curl_download(CURL *curl, GString *body, const char *filename)
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

