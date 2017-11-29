/*
 * Data for Atari ST to ISO Latin-1 code conversions.
 * Written by Thorsten Otto
 * based on http://www.unicode.org/Public/MAPPINGS/VENDORS/MISC/ATARIST.TXT
 */

static struct {
	unsigned char st;
	unsigned short uni;
} const known_pairs[] = {
	{ 0x80, 0xc7 },							/* C, LATIN CAPITAL LETTER C WITH CEDILLA */
	{ 0x81, 0xfc },							/* u" LATIN SMALL LETTER U WITH DIAERESIS */
	{ 0x82, 0xe9 },							/* e' LATIN SMALL LETTER E WITH ACUTE */
	{ 0x83, 0xe2 },							/* a^ LATIN SMALL LETTER A WITH CIRCUMFLEX */
	{ 0x84, 0xe4 },							/* a" LATIN SMALL LETTER A WITH DIAERESIS */
	{ 0x85, 0xe0 },							/* a` LATIN SMALL LETTER A WITH GRAVE */
	{ 0x86, 0xe5 },							/* aa LATIN SMALL LETTER A WITH RING ABOVE */
	{ 0x87, 0xe7 },							/* c, LATIN SMALL LETTER C WITH CEDILLA */
	{ 0x88, 0xea },							/* e^ LATIN SMALL LETTER E WITH CIRCUMFLEX */
	{ 0x89, 0xeb },							/* e" LATIN SMALL LETTER E WITH DIAERESIS */
	{ 0x8a, 0xe8 },							/* e` LATIN SMALL LETTER E WITH GRAVE */
	{ 0x8b, 0xef },							/* i" LATIN SMALL LETTER I WITH DIAERESIS */
	{ 0x8c, 0xee },							/* i^ LATIN SMALL LETTER I WITH CIRCUMFLEX */
	{ 0x8d, 0xec },							/* i` LATIN SMALL LETTER I WITH GRAVE */
	{ 0x8e, 0xc4 },							/* A" LATIN CAPITAL LETTER A WITH DIAERESIS */
	{ 0x8f, 0xc5 },							/* AA LATIN CAPITAL LETTER A WITH RING ABOVE */
	{ 0x90, 0xc9 },							/* E' LATIN CAPITAL LETTER E WITH ACUTE */
	{ 0x91, 0xe6 },							/* ae LATIN SMALL LETTER AE */
	{ 0x92, 0xc6 },							/* AE LATIN CAPITAL LETTER AE */
	{ 0x93, 0xf4 },							/* o^ LATIN SMALL LETTER O WITH CIRCUMFLEX */
	{ 0x94, 0xf6 },							/* o" LATIN SMALL LETTER O WITH DIAERESIS */
	{ 0x95, 0xf2 },							/* o` LATIN SMALL LETTER O WITH GRAVE */
	{ 0x96, 0xfb },							/* u^ LATIN SMALL LETTER U WITH CIRCUMFLEX */
	{ 0x97, 0xf9 },							/* u` LATIN SMALL LETTER U WITH GRAVE */
	{ 0x98, 0xff },							/* y" LATIN SMALL LETTER Y WITH DIAERESIS */
	{ 0x99, 0xd6 },							/* O" LATIN CAPITAL LETTER O WITH DIAERESIS */
	{ 0x9a, 0xdc },							/* U" LATIN CAPITAL LETTER U WITH DIAERESIS */
	{ 0x9b, 0xa2 },							/* \cent CENT SIGN */
	{ 0x9c, 0xa3 },							/* \pound POUND SIGN */
	{ 0x9d, 0xa5 },							/* \yen YEN SIGN */
	{ 0x9e, 0xdf },							/* \ss LATIN SMALL LETTER SHARP S */
	{ 0x9f, 0x192 },						/* LATIN SMALL LETTER F WITH HOOK */
	{ 0xa0, 0xe1 },							/* a' LATIN SMALL LETTER A WITH ACUTE */
	{ 0xa1, 0xed },							/* i' LATIN SMALL LETTER I WITH ACUTE */
	{ 0xa2, 0xf3 },							/* o' LATIN SMALL LETTER O WITH ACUTE */
	{ 0xa3, 0xfa },							/* u' LATIN SMALL LETTER U WITH ACUTE */
	{ 0xa4, 0xf1 },							/* n~ LATIN SMALL LETTER N WITH TILDE */
	{ 0xa5, 0xd1 },							/* N~ LATIN CAPITAL LETTER N WITH TILDE */
	{ 0xa6, 0xaa },							/* a_ FEMININE ORDINAL INDICATOR */
	{ 0xa7, 0xba },							/* o_ MASCULINE ORDINAL INDICATOR */
	{ 0xa8, 0xbf },							/* ?' INVERTED QUESTION MARK */
	{ 0xa9, 0x2310 },						/* REVERSED NOT SIGN */
	{ 0xaa, 0xac },							/* \neg NOT SIGN */
	{ 0xab, 0xbd },							/* 1/2 VULGAR FRACTION ONE HALF */
	{ 0xac, 0xbc },							/* 1/4 VULGAR FRACTION ONE QUARTER */
	{ 0xad, 0xa1 },							/* !` INVERTED EXCLAMATION MARK */
	{ 0xae, 0xab },							/* `` LEFT-POINTING DOUBLE ANGLE QUOTATION MARK */
	{ 0xaf, 0xbb },							/* '' RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK */
	{ 0xb0, 0xe3 },							/* a~ LATIN SMALL LETTER A WITH TILDE */
	{ 0xb1, 0xf5 },							/* o~ LATIN SMALL LETTER O WITH TILDE */
	{ 0xb2, 0xd8 },							/* O/ LATIN CAPITAL LETTER O WITH STROKE */
	{ 0xb3, 0xf8 },							/* o/ LATIN SMALL LETTER O WITH STROKE */
	{ 0xb4, 0x153 },						/* oe LATIN SMALL LIGATURE OE */
	{ 0xb5, 0x152 },						/* OE LATIN CAPITAL LIGATURE OE */
	{ 0xb6, 0xc0 },							/* A` LATIN CAPITAL LETTER A WITH GRAVE */
	{ 0xb7, 0xc3 },							/* A~ LATIN CAPITAL LETTER A WITH TILDE */
	{ 0xb8, 0xd5 },							/* O~ LATIN CAPITAL LETTER O WITH TILDE */
	{ 0xb9, 0xa8 },							/* DIAERESIS */
	{ 0xba, 0xb4 },							/* ACUTE ACCENT */
	{ 0xbb, 0x2020 },						/* DAGGER */
	{ 0xbc, 0xb6 },							/* PILCROW SIGN */
	{ 0xbd, 0xa9 },							/* COPYRIGHT SIGN */
	{ 0xbe, 0xae },							/* REGISTERED SIGN */
	{ 0xbf, 0x2122 },						/* TRADE MARK SIGN */
	{ 0xc0, 0x133 },						/* LATIN SMALL LIGATURE IJ */
	{ 0xc1, 0x132 },						/* LATIN CAPITAL LIGATURE IJ */
	{ 0xc2, 0x5d0 },						/* HEBREW LETTER ALEF */
	{ 0xc3, 0x5d1 },						/* HEBREW LETTER BET */
	{ 0xc4, 0x5d2 },						/* HEBREW LETTER GIMEL */
	{ 0xc5, 0x5d3 },						/* HEBREW LETTER DALET */
	{ 0xc6, 0x5d4 },						/* HEBREW LETTER HE */
	{ 0xc7, 0x5d5 },						/* HEBREW LETTER VAV */
	{ 0xc8, 0x5d6 },						/* HEBREW LETTER ZAYIN */
	{ 0xc9, 0x5d7 },						/* HEBREW LETTER HET */
	{ 0xca, 0x5d8 },						/* HEBREW LETTER TET */
	{ 0xcb, 0x5d9 },						/* HEBREW LETTER YOD */
	{ 0xcc, 0x5db },						/* HEBREW LETTER KAF */
	{ 0xcd, 0x5dc },						/* HEBREW LETTER LAMED */
	{ 0xce, 0x5de },						/* HEBREW LETTER MEM */
	{ 0xcf, 0x5e0 },						/* HEBREW LETTER NUN */
	{ 0xd0, 0x5e1 },						/* HEBREW LETTER SAMEKH */
	{ 0xd1, 0x5e2 },						/* HEBREW LETTER AYIN */
	{ 0xd2, 0x5e4 },						/* HEBREW LETTER PE */
	{ 0xd3, 0x5e6 },						/* HEBREW LETTER TSADI */
	{ 0xd4, 0x5e7 },						/* HEBREW LETTER QOF */
	{ 0xd5, 0x5e8 },						/* HEBREW LETTER RESH */ 
	{ 0xd6, 0x5e9 },						/* HEBREW LETTER SHIN */
	{ 0xd7, 0x5ea },						/* HEBREW LETTER TAV */
	{ 0xd8, 0x5df },						/* HEBREW LETTER FINAL NUN */
	{ 0xd9, 0x5da },						/* HEBREW LETTER FINAL KAF */
	{ 0xda, 0x5dd },						/* HEBREW LETTER FINAL MEM */
	{ 0xdb, 0x5e3 },						/* HEBREW LETTER FINAL PE */
	{ 0xdc, 0x5e5 },						/* HEBREW LETTER FINAL TSADI */
	{ 0xdd, 0xa7 },							/* PARAGRAPH SIGN, SECTION SIGN */
	{ 0xde, 0x2227 },						/* LOGICAL AND */
	{ 0xdf, 0x221e },						/* INFINITY */
	{ 0xe0, 0x3b1 },						/* GREEK SMALL LETTER ALPHA */
	{ 0xe1, 0x3b2 },						/* GREEK SMALL LETTER BETA */
	{ 0xe2, 0x393 },						/* GREEK CAPITAL LETTER GAMMA */
	{ 0xe3, 0x3c0 },						/* GREEK SMALL LETTER PI */
	{ 0xe4, 0x3a3 },						/* GREEK CAPITAL LETTER SIGMA */
	{ 0xe5, 0x3c3 },						/* GREEK SMALL LETTER SIGMA */
	{ 0xe6, 0xb5 },							/* MICRO SIGN */
	{ 0xe7, 0x3c4 },						/* GREEK SMALL LETTER TAU */
	{ 0xe8, 0x3a6 },						/* GREEK CAPITAL LETTER PHI */
	{ 0xe9, 0x398 },						/* GREEK CAPITAL LETTER THETA */
	{ 0xea, 0x3a9 },						/* GREEK CAPITAL LETTER OMEGA */
	{ 0xeb, 0x3b4 },						/* GREEK SMALL LETTER DELTA */
	{ 0xec, 0x222e },						/* CONTOUR INTEGRAL */
	{ 0xed, 0x3c6 },						/* GREEK SMALL LETTER PHI */
	{ 0xee, 0x2208 },						/* ELEMENT OF SIGN */
	{ 0xef, 0x2229 },						/* INTERSECTION */
	{ 0xf0, 0x2261 },						/* IDENTICAL TO */
	{ 0xf1, 0xb1 },							/* +- PLUS-MINUS SIGN */
	{ 0xf2, 0x22b5 },						/* >= GREATER-THAN OR EQUAL TO */
	{ 0xf3, 0x2264 },						/* <= LESS-THAN OR EQUAL TO */
	{ 0xf4, 0x2320 },						/* TOP HALF INTEGRAL */
	{ 0xf5, 0x2321 },						/* BOTTOM HALF INTEGRAL */
	{ 0xf6, 0xf7 },							/* \div DIVISION SIGN */
	{ 0xf7, 0x2248 },						/* ALMOST EQUAL TO */
	{ 0xf8, 0xb0 },							/* \deg DEGREE SIGN */
	{ 0xf9, 0x2219 },						/* BULLET OPERATOR */
	{ 0xfa, 0xb7 },							/* \cdot MIDDLE DOT */
	{ 0xfb, 0x221a },						/* SQUARE ROOT */
	{ 0xfc, 0x207f },						/* SUPERSCRIPT LATIN SMALL LETTER N */
	{ 0xfd, 0xb2 },							/* ^2 SUPERSCRIPT TWO */
	{ 0xfe, 0xb3 },							/* ^3 SUPERSCRIPT THREE */
	{ 0xff, 0xaf }							/* MACRON */
};

#define NUMBER_OF_PAIRS (sizeof (known_pairs) / sizeof (known_pairs[0]))

