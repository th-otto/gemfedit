#include "spddefs.h"
#include <curl/curl.h>
#include <sys/time.h>
#include <utime.h>
#include <unistd.h>
#include "cgic.h"

char const gl_program_name[] = "spdview.cgi";
char const gl_program_version[] = "1.0";
static const char *cgi_scriptname = "spdview.cgi";
static char const spdview_css_name[] = "_spdview.css";
static char const spdview_js_name[] = "_spdview.js";
static char const charset[] = "UTF-8";

static char const cgi_cachedir[] = "cache";

static char const html_error_note_style[] = "hypview_error_note";
static char const html_node_style[] = "hypview_node";

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
static char *html_referer_url;
static FILE *errorfile;

#define _(x) x

static int force_crlf = FALSE;

#define HAVE_MKSTEMPS

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
			fprintf(errorfile, "%s: %s\n", parms->filename, strerror(errno));
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

static void html_out_header(GString *out, const char *title, gboolean for_error)
{
	const char *html_lang = "en-US";
	
	g_string_append(out, "<!DOCTYPE html>\n");
	g_string_append_printf(out, "<html xml:lang=\"%s\" lang=\"%s\">\n", html_lang, html_lang);
	g_string_append_printf(out, "<!-- This file was automatically generated by %s version %s -->\n", gl_program_name, gl_program_version);
	g_string_append_printf(out, "<!-- Copyright \xC2\xA9 1991-2007 by Thorsten Otto -->\n");
	g_string_append(out, "<head>\n");
	g_string_append_printf(out, "<meta charset=\"%s\"%s\n", charset, html_closer);
	g_string_append_printf(out, "<meta name=\"GENERATOR\" content=\"%s %s\"%s\n", gl_program_name, gl_program_version, html_closer);
	if (title)
		g_string_append_printf(out, "<title>%s</title>\n", title);

	html_out_stylesheet(out);
	html_out_javascript(out);
	
	{
		g_string_append(out, "<!--[if lt IE 9]>\n");
		g_string_append(out, "<script src=\"http://html5shiv.googlecode.com/svn/trunk/html5.js\" type=\"text/javascript\"></script>\n");
		g_string_append(out, "<![endif]-->\n");
	}

	g_string_append(out, "</head>\n");
	g_string_append(out, "<body>\n");

	if (for_error)
	{
		g_string_append_printf(out, "<div class=\"%s\">\n", html_error_note_style);
		g_string_append(out, "<p>\n");
	} else
	{
		g_string_append_printf(out, "<div class=\"%s\">\n", html_node_style);
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
	
	g_string_append(out, "</body>\n");
	g_string_append(out, "</html>\n");
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
	fprintf(errorfile, "%s: GET from %s, url=%s, curl=%d, resp=%ld, size=%ld, content=%s\n", currdate(), fixnull(cgiRemoteHost), filename, curlcode, respcode, (long)size, printnull(content_type));
	
	if (parms.fp)
	{
		fclose(parms.fp);
		parms.fp = NULL;
	}
	
	if (curlcode != CURLE_OK)
	{
		html_out_header(body, err, TRUE);
		g_string_append_printf(body, "%s:\n%s", _("Download error"), err);
		html_out_trailer(body, TRUE);
		unlink(local_filename);
		g_free(local_filename);
		local_filename = NULL;
	} else if ((respcode != 200 && respcode != 304) ||
		(respcode == 200 && (content_type == NULL || strcmp(content_type, "text/plain") == 0)))
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

static gboolean gen_speedo_font(const char *filename, GString *body)
{
	html_out_header(body, NULL, FALSE);
	g_string_append_printf(body, "<p>outdir: %s</p>\n", output_dir);
	g_string_append_printf(body, "<p>filename: %s</p>\n", filename);
	html_out_trailer(body, FALSE);
	return TRUE;
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
	char *lang;
	
	errorfile = stderr;
	
	body = g_string_new(NULL);
	cgiInit(body);

	{
		char *dir = hyp_path_get_dirname(cgiScriptFilename);
		char *cache_dir = g_build_filename(dir, cgi_cachedir, NULL);
		output_dir = g_build_filename(cache_dir, cgiRemoteAddr, NULL);

		if (mkdir(cache_dir, 0750) < 0 && errno != EEXIST)
			fprintf(errorfile, "%s: %s\n", cache_dir, strerror(errno));
		if (mkdir(output_dir, 0750) < 0 && errno != EEXIST)
			fprintf(errorfile, "%s: %s\n", output_dir, strerror(errno));
		
		g_free(cache_dir);
		g_free(dir);
	}
	
	if (cgiScriptName)
		cgi_scriptname = cgiScriptName;
	
	lang = cgiFormString("lang");
	
	if (g_ascii_strcasecmp(cgiRequestMethod, "GET") == 0)
	{
		char *url = cgiFormString("url");
		char *filename = g_strdup(url);
		char *scheme = empty(filename) ? g_strdup("undefined") : uri_has_scheme(filename) ? g_strndup(filename, strchr(filename, ':') - filename) : g_strdup("file");
		
		if (filename && filename[0] == '/')
		{
			html_referer_url = filename;
			filename = g_strconcat(cgiDocumentRoot, filename, NULL);
		} else if (empty(xbasename(filename)) || (g_ascii_strcasecmp(scheme, "file") == 0))
		{
			/*
			 * disallow file URIs, they would resolve to local files on the WEB server
			 */
			html_out_header(body, _("403 Forbidden"), TRUE);
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
			if ((curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK ||
				(curl = curl_easy_init()) == NULL))
			{
				html_out_header(body, _("500 Internal Server Error"), TRUE);
				g_string_append(body, _("could not initialize curl\n"));
				html_out_trailer(body, TRUE);
				retval = EXIT_FAILURE;
			} else
			{
				char *local_filename;
				
				html_referer_url = g_strdup(filename);
				local_filename = curl_download(curl, body, filename);
				g_free(filename);
				filename = local_filename;
			}
		}
		if (filename && retval == EXIT_SUCCESS)
		{
			if (gen_speedo_font(filename, body) == FALSE)
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
			html_out_header(body, _("403 Forbidden"), TRUE);
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
			FILE *fp;
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
					close(fd);
				}
#elif defined(HAVE_MKSTEMP)
				{
				int fd;
				filename = g_strdup("tmpfile.spd.XXXXXX");
				local_filename = g_build_filename(output_dir, xbasename(filename), NULL);
				fd = mkstemp(local_filename);
				if (fd > 0)
					close(fd);
				}
#else
				filename = g_strdup("tmpfile.spd.XXXXXX");
				local_filename = g_build_filename(output_dir, xbasename(filename), NULL);
				mktemp(local_filename);
#endif
			} else
			{
				local_filename = g_build_filename(output_dir, xbasename(filename), NULL);
			}

			fprintf(errorfile, "%s: POST from %s, file=%s, size=%d\n", currdate(), fixnull(cgiRemoteHost), xbasename(filename), len);

			fp = fopen(local_filename, "wb");
			if (fp == NULL)
			{
				const char *err = strerror(errno);
				fprintf(errorfile, "%s: %s\n", local_filename, err);
				html_out_header(body, _("404 Not Found"), TRUE);
				g_string_append_printf(body, "%s: %s\n", xbasename(filename), err);
				html_out_trailer(body, TRUE);
				retval = EXIT_FAILURE;
			} else
			{
				data = cgiFormFileData("file", &len);
				fwrite(data, 1, len, fp);
				fclose(fp);
				html_referer_url = g_strdup(filename);
				if (gen_speedo_font(local_filename, body) == FALSE)
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
	
	g_free(lang);
	
	if (curl)
	{
		curl_easy_cleanup(curl);
		curl_global_cleanup();
	}
	
	return retval;
}


/*
HTTP_HOST=127.0.0.2
HTTP_USER_AGENT=Mozilla/5.0 (X11; Linux x86_64; rv:45.0) Gecko/20100101 Firefox/45.0
HTTP_ACCEPT=text/html,application/xhtml+xml,application/xml;q=0.9;q=0.8
HTTP_ACCEPT_LANGUAGE=en-US,en;q=0.8,de-DE;q=0.5,de;q=0.3
HTTP_ACCEPT_ENCODING=gzip, deflate
HTTP_DNT=1
HTTP_REFERER=http://127.0.0.2/spdview/
HTTP_CONNECTION=keep-alive
PATH=/usr/bin:/bin:/usr/sbin:/sbin
SERVER_SIGNATURE=
Apache/2.2.17 (Linux/SUSE) Server at 127.0.0.2 Port 80

SERVER_SOFTWARE=Apache/2.2.17 (Linux/SUSE)
SERVER_NAME=127.0.0.2
SERVER_ADDR=127.0.0.2
SERVER_PORT=80
REMOTE_ADDR=127.0.0.1
DOCUMENT_ROOT=/srv/www/htdocs
SERVER_ADMIN=[no address given]
SCRIPT_FILENAME=/srv/www/htdocs/spdview/spdview.cgi
REMOTE_PORT=48150
GATEWAY_INTERFACE=CGI/1.1
SERVER_PROTOCOL=HTTP/1.1
REQUEST_METHOD=GET
QUERY_STRING=url=&hideimages=0&hidemenu=0&dstenc=
REQUEST_URI=/spdview/spdview.cgi?url=&hideimages=0&hidemenu=0&dstenc=
SCRIPT_NAME=/spdview/spdview.cgi
*/
