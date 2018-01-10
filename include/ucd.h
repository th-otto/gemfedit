#define UCD_GC_LU 0 /* an uppercase letter */
#define UCD_GC_LL 1 /* a lowercase letter */
#define UCD_GC_LT 2 /* a digraphic character, with first part uppercase */
#define UCD_GC_LC 3 /* Lu | Ll | Lt */
#define UCD_GC_LM 4 /* a modifier letter */
#define UCD_GC_LO 5 /* other letters, including syllables and ideographs */
#define UCD_GC_L  6 /* Lu | Ll | Lt | Lm | Lo */
#define UCD_GC_MN 7 /* a nonspacing combining mark (zero advance width) */
#define UCD_GC_MC 8 /* a spacing combining mark (positive advance width) */
#define UCD_GC_ME 9 /* an enclosing combining mark */
#define UCD_GC_M  10 /* Mn | Mc | Me */
#define UCD_GC_ND 11 /* a decimal digit */
#define UCD_GC_NL 12 /* a letterlike numeric character */
#define UCD_GC_NO 13 /* a numeric character of other type */
#define UCD_GC_N  14 /* Nd | Nl | No */
#define UCD_GC_PC 15 /* a connecting punctuation mark, like a tie */
#define UCD_GC_PD 16 /* a dash or hyphen punctuation mark */
#define UCD_GC_PS 17 /* an opening punctuation mark (of a pair) */
#define UCD_GC_PE 18 /* a closing punctuation mark (of a pair) */
#define UCD_GC_PI 19 /* an initial quotation mark */
#define UCD_GC_PF 20 /* a final quotation mark */
#define UCD_GC_PO 21 /* a punctuation mark of other type */
#define UCD_GC_P  22 /* Pc | Pd | Ps | Pe | Pi | Pf | Po */
#define UCD_GC_SM 23 /* a symbol of primarily mathematical use */
#define UCD_GC_SC 24 /* a currency sign */
#define UCD_GC_SK 25 /* a non-letterlike modifier symbol */
#define UCD_GC_SO 26 /* a symbol of other type */
#define UCD_GC_S  27 /* Sm | Sc | Sk | So */
#define UCD_GC_ZS 28 /* a space character (of various non-zero widths) */
#define UCD_GC_ZL 29 /* U+2028 LINE SEPARATOR only */
#define UCD_GC_ZP 30 /* U+2029 PARAGRAPH SEPARATOR only */
#define UCD_GC_Z  31 /* Zs | Zl | Zp */
#define UCD_GC_CC 32 /* a C0 or C1 control code */
#define UCD_GC_CF 33 /* a format control character */
#define UCD_GC_CS 34 /* a surrogate code point */
#define UCD_GC_CO 35 /* a private-use character */
#define UCD_GC_CN 36 /* a reserved unassigned code point or a noncharacter */
#define UCD_GC_C  37 /* Cc | Cf | Cs | Co | Cn */

#define UCD_BIDI_L   0 /* any strong left-to-right character */
#define UCD_BIDI_R   1 /* any strong right-to-left (non-Arabic-type) character */
#define UCD_BIDI_AL  2 /* any strong right-to-left (Arabic-type) character */
#define UCD_BIDI_EN  3 /* any ASCII digit or Eastern Arabic-Indic digit */
#define UCD_BIDI_ES  4 /* plus and minus signs */
#define UCD_BIDI_ET  5 /* a terminator in a numeric format context, includes currency signs */
#define UCD_BIDI_AN  6 /* any Arabic-Indic digit */
#define UCD_BIDI_CS  7 /* commas, colons, and slashes */
#define UCD_BIDI_NSM 8 /* any nonspacing mark */
#define UCD_BIDI_BN  9 /* most format characters, control codes, or noncharacters */
#define UCD_BIDI_B   10 /* various newline characters */
#define UCD_BIDI_S   11 /* various segment-related control codes */
#define UCD_BIDI_WS  12 /* spaces */
#define UCD_BIDI_ON  13 /* most other symbols and punctuation marks */
#define UCD_BIDI_LRE 14 /* U+202A: the LR embedding control */
#define UCD_BIDI_LRO 15 /* U+202D: the LR override control */
#define UCD_BIDI_RLE 16 /* U+202B: the RL embedding control */
#define UCD_BIDI_RLO 17 /* U+202E: the RL override control */
#define UCD_BIDI_PDF 18 /* U+202C: terminates an embedding or override control */
#define UCD_BIDI_LRI 19 /* U+2066: the LR isolate control */
#define UCD_BIDI_RLI 20 /* U+2067: the RL isolate control */
#define UCD_BIDI_FSI 21 /* U+2068: the first strong isolate control */
#define UCD_BIDI_PDI 22 /* U+2069: terminates an isolate control */

const char *ucd_get_name(uint32_t unicode);
const char *ucd_get_block(uint32_t unicode);
