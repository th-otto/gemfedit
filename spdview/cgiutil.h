#include <curl/curl.h>

#define _(x) x

extern const char *cgi_scriptname;
extern char const gl_program_name[];
extern char const gl_program_version[];

extern char const spdview_css_name[];
extern char const spdview_js_name[];
extern char const charset[];

extern char const cgi_cachedir[];

extern char const html_error_note_style[];
extern char const html_node_style[];
extern char const html_spd_info_id[];
extern char const html_toolbar_style[];
extern char const html_dropdown_style[];
extern char const html_dropdown_info_style[];
extern char const html_nav_img_style[];

extern char const html_nav_info_png[];
extern char const html_nav_load_png[];
extern char const html_nav_load_href[];
extern char const html_nav_dimensions[];

#define ALLOWED_PROTOS ( \
	CURLPROTO_FTP | \
	CURLPROTO_FTPS | \
	CURLPROTO_HTTP | \
	CURLPROTO_HTTPS | \
	CURLPROTO_SCP | \
	CURLPROTO_SFTP | \
	CURLPROTO_TFTP)

extern const char *html_closer;
extern char *output_dir;
extern char *output_url;
extern char *html_referer_url;
extern GString *errorout;
extern FILE *errorfile;
extern gboolean cgi_cached;
extern gboolean hidemenu;
extern gboolean debug;

void write_strout(GString *s, FILE *outfp);

#define QUOTE_CONVSLASH  0x0001
#define QUOTE_SPACE      0x0002
#define QUOTE_URI        0x0004
#define QUOTE_JS         0x0008
#define QUOTE_ALLOWUTF8  0x0010
#define QUOTE_LABEL      0x0020
#define QUOTE_UNICODE    0x0040
#define QUOTE_NOLTR      0x0080

char *html_quote_name(const char *name, unsigned int flags, size_t len);
gboolean html_out_stylesheet(GString *out);
gboolean html_out_javascript(GString *out);
void html_out_header(GString *out, GString *font_info, const char *title, gboolean for_error);
void html_out_trailer(GString *out, gboolean for_error);
gboolean uri_has_scheme(const char *uri);
void html_out_response_header(FILE *out, unsigned long len);
const char *currdate(void);

char *curl_download(CURL *curl, GString *body, const char *filename);
