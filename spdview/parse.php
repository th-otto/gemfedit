<?php


class ucd {
	private $opt = NULL;
	private $localdir;
	
	private $download_location = 'ftp://www.unicode.org/Public/10.0.0/ucd/';

	private $category = array(
		'Lu' => array('long' => 'Uppercase_Letter', 'description' => 'an uppercase letter'),
		'Ll' => array('long' => 'Lowercase_Letter', 'description' => 'a lowercase letter'),
		'Lt' => array('long' => 'Titlecase_Letter', 'description' => 'a digraphic character, with first part uppercase'),
		'LC' => array('long' => 'Cased_Letter', 'description' => 'Lu | Ll | Lt'),
		'Lm' => array('long' => 'Modifier_Letter', 'description' => 'a modifier letter'),
		'Lo' => array('long' => 'Other_Letter', 'description' => 'other letters, including syllables and ideographs'),
		'L'  => array('long' => 'Letter', 'description' => 'Lu | Ll | Lt | Lm | Lo'),
		'Mn' => array('long' => 'Nonspacing_Mark', 'description' => 'a nonspacing combining mark (zero advance width)'),
		'Mc' => array('long' => 'Spacing_Mark', 'description' => 'a spacing combining mark (positive advance width)'),
		'Me' => array('long' => 'Enclosing_Mark', 'description' => 'an enclosing combining mark'),
		'M'  => array('long' => 'Mark', 'description' => 'Mn | Mc | Me'),
		'Nd' => array('long' => 'Decimal_Number', 'description' => 'a decimal digit'),
		'Nl' => array('long' => 'Letter_Number', 'description' => 'a letterlike numeric character'),
		'No' => array('long' => 'Other_Number', 'description' => 'a numeric character of other type'),
		'N'  => array('long' => 'Number', 'description' => 'Nd | Nl | No'),
		'Pc' => array('long' => 'Connector_Punctuation', 'description' => 'a connecting punctuation mark, like a tie'),
		'Pd' => array('long' => 'Dash_Punctuation', 'description' => 'a dash or hyphen punctuation mark'),
		'Ps' => array('long' => 'Open_Punctuation', 'description' => 'an opening punctuation mark (of a pair)'),
		'Pe' => array('long' => 'Close_Punctuation', 'description' => 'a closing punctuation mark (of a pair)'),
		'Pi' => array('long' => 'Initial_Punctuation', 'description' => 'an initial quotation mark'),
		'Pf' => array('long' => 'Final_Punctuation', 'description' => 'a final quotation mark'),
		'Po' => array('long' => 'Other_Punctuation', 'description' => 'a punctuation mark of other type'),
		'P'  => array('long' => 'Punctuation', 'description' => 'Pc | Pd | Ps | Pe | Pi | Pf | Po'),
		'Sm' => array('long' => 'Math_Symbol', 'description' => 'a symbol of primarily mathematical use'),
		'Sc' => array('long' => 'Currency_Symbol', 'description' => 'a currency sign'),
		'Sk' => array('long' => 'Modifier_Symbol', 'description' => 'a non-letterlike modifier symbol'),
		'So' => array('long' => 'Other_Symbol', 'description' => 'a symbol of other type'),
		'S'  => array('long' => 'Symbol', 'description' => 'Sm | Sc | Sk | So'),
		'Zs' => array('long' => 'Space_Separator', 'description' => 'a space character (of various non-zero widths)'),
		'Zl' => array('long' => 'Line_Separator', 'description' => 'U+2028 LINE SEPARATOR only'),
		'Zp' => array('long' => 'Paragraph_Separator', 'description' => 'U+2029 PARAGRAPH SEPARATOR only'),
		'Z'  => array('long' => 'Separator', 'description' => 'Zs | Zl | Zp'),
		'Cc' => array('long' => 'Control', 'description' => 'a C0 or C1 control code'),
		'Cf' => array('long' => 'Format', 'description' => 'a format control character'),
		'Cs' => array('long' => 'Surrogate', 'description' => 'a surrogate code point'),
		'Co' => array('long' => 'Private_Use', 'description' => 'a private-use character'),
		'Cn' => array('long' => 'Unassigned', 'description' => 'a reserved unassigned code point or a noncharacter'),
		'C'  => array('long' => 'Other', 'description' => 'Cc | Cf | Cs | Co | Cn'),
	);

	private $canonical_combining_class = array(
		'0'   => array('long' => 'Not_Reordered', 'description' => 'Spacing and enclosing marks; also many vowel and consonant signs, even if nonspacing'),
		'1'   => array('long' => 'Overlay', 'description' => 'Marks which overlay a base letter or symbol'),
		'7'   => array('long' => 'Nukta', 'description' => 'Diacritic nukta marks in Brahmi-derived scripts'),
		'8'   => array('long' => 'Kana_Voicing', 'description' => 'Hiragana/Katakana voicing marks'),
		'9'   => array('long' => 'Virama', 'description' => 'Viramas'),
		'10'  => array('long' => 'Ccc10', 'description' => 'Start of fixed position classes'),
		'11'  => array('long' => 'Ccc11', 'description' => ''),
		'12'  => array('long' => 'Ccc12', 'description' => ''),
		'13'  => array('long' => 'Ccc13', 'description' => ''),
		'14'  => array('long' => 'Ccc14', 'description' => ''),
		'15'  => array('long' => 'Ccc15', 'description' => ''),
		'16'  => array('long' => 'Ccc16', 'description' => ''),
		'17'  => array('long' => 'Ccc17', 'description' => ''),
		'18'  => array('long' => 'Ccc18', 'description' => ''),
		'19'  => array('long' => 'Ccc19', 'description' => ''),
		'20'  => array('long' => 'Ccc20', 'description' => ''),
		'21'  => array('long' => 'Ccc21', 'description' => ''),
		'22'  => array('long' => 'Ccc22', 'description' => ''),
		'23'  => array('long' => 'Ccc23', 'description' => ''),
		'24'  => array('long' => 'Ccc24', 'description' => ''),
		'25'  => array('long' => 'Ccc25', 'description' => ''),
		'26'  => array('long' => 'Ccc26', 'description' => ''),
		'27'  => array('long' => 'Ccc27', 'description' => ''),
		'28'  => array('long' => 'Ccc28', 'description' => ''),
		'29'  => array('long' => 'Ccc29', 'description' => ''),
		'30'  => array('long' => 'Ccc30', 'description' => ''),
		'31'  => array('long' => 'Ccc31', 'description' => ''),
		'32'  => array('long' => 'Ccc32', 'description' => ''),
		'33'  => array('long' => 'Ccc33', 'description' => ''),
		'34'  => array('long' => 'Ccc34', 'description' => ''),
		'35'  => array('long' => 'Ccc35', 'description' => ''),
		'36'  => array('long' => 'Ccc36', 'description' => ''),
		'37'  => array('long' => 'Ccc37', 'description' => ''),
		'38'  => array('long' => 'Ccc38', 'description' => ''),
		'39'  => array('long' => 'Ccc39', 'description' => ''),
		'40'  => array('long' => 'Ccc40', 'description' => ''),
		'41'  => array('long' => 'Ccc41', 'description' => ''),
		'42'  => array('long' => 'Ccc42', 'description' => ''),
		'43'  => array('long' => 'Ccc43', 'description' => ''),
		'44'  => array('long' => 'Ccc44', 'description' => ''),
		'45'  => array('long' => 'Ccc45', 'description' => ''),
		'46'  => array('long' => 'Ccc46', 'description' => ''),
		'47'  => array('long' => 'Ccc47', 'description' => ''),
		'48'  => array('long' => 'Ccc48', 'description' => ''),
		'49'  => array('long' => 'Ccc49', 'description' => ''),
		'50'  => array('long' => 'Ccc50', 'description' => ''),
		'51'  => array('long' => 'Ccc51', 'description' => ''),
		'52'  => array('long' => 'Ccc52', 'description' => ''),
		'53'  => array('long' => 'Ccc53', 'description' => ''),
		'54'  => array('long' => 'Ccc54', 'description' => ''),
		'55'  => array('long' => 'Ccc55', 'description' => ''),
		'56'  => array('long' => 'Ccc56', 'description' => ''),
		'57'  => array('long' => 'Ccc57', 'description' => ''),
		'58'  => array('long' => 'Ccc58', 'description' => ''),
		'59'  => array('long' => 'Ccc59', 'description' => ''),
		'60'  => array('long' => 'Ccc60', 'description' => ''),
		'61'  => array('long' => 'Ccc61', 'description' => ''),
		'62'  => array('long' => 'Ccc62', 'description' => ''),
		'63'  => array('long' => 'Ccc63', 'description' => ''),
		'64'  => array('long' => 'Ccc64', 'description' => ''),
		'65'  => array('long' => 'Ccc65', 'description' => ''),
		'66'  => array('long' => 'Ccc66', 'description' => ''),
		'67'  => array('long' => 'Ccc67', 'description' => ''),
		'68'  => array('long' => 'Ccc68', 'description' => ''),
		'69'  => array('long' => 'Ccc69', 'description' => ''),
		'70'  => array('long' => 'Ccc70', 'description' => ''),
		'71'  => array('long' => 'Ccc71', 'description' => ''),
		'72'  => array('long' => 'Ccc72', 'description' => ''),
		'73'  => array('long' => 'Ccc73', 'description' => ''),
		'74'  => array('long' => 'Ccc74', 'description' => ''),
		'75'  => array('long' => 'Ccc75', 'description' => ''),
		'76'  => array('long' => 'Ccc76', 'description' => ''),
		'77'  => array('long' => 'Ccc77', 'description' => ''),
		'78'  => array('long' => 'Ccc78', 'description' => ''),
		'79'  => array('long' => 'Ccc79', 'description' => ''),
		'80'  => array('long' => 'Ccc80', 'description' => ''),
		'81'  => array('long' => 'Ccc81', 'description' => ''),
		'82'  => array('long' => 'Ccc82', 'description' => ''),
		'83'  => array('long' => 'Ccc83', 'description' => ''),
		'84'  => array('long' => 'Ccc84', 'description' => ''),
		'85'  => array('long' => 'Ccc85', 'description' => ''),
		'86'  => array('long' => 'Ccc86', 'description' => ''),
		'87'  => array('long' => 'Ccc87', 'description' => ''),
		'88'  => array('long' => 'Ccc88', 'description' => ''),
		'89'  => array('long' => 'Ccc89', 'description' => ''),
		'90'  => array('long' => 'Ccc90', 'description' => ''),
		'91'  => array('long' => 'Ccc91', 'description' => ''),
		'92'  => array('long' => 'Ccc92', 'description' => ''),
		'93'  => array('long' => 'Ccc93', 'description' => ''),
		'94'  => array('long' => 'Ccc94', 'description' => ''),
		'95'  => array('long' => 'Ccc95', 'description' => ''),
		'96'  => array('long' => 'Ccc96', 'description' => ''),
		'97'  => array('long' => 'Ccc97', 'description' => ''),
		'98'  => array('long' => 'Ccc98', 'description' => ''),
		'99'  => array('long' => 'Ccc99', 'description' => ''),
		'100'  => array('long' => 'Ccc100', 'description' => ''),
		'101'  => array('long' => 'Ccc101', 'description' => ''),
		'102'  => array('long' => 'Ccc102', 'description' => ''),
		'103'  => array('long' => 'Ccc103', 'description' => ''),
		'104'  => array('long' => 'Ccc104', 'description' => ''),
		'105'  => array('long' => 'Ccc105', 'description' => ''),
		'106'  => array('long' => 'Ccc106', 'description' => ''),
		'107'  => array('long' => 'Ccc107', 'description' => ''),
		'108'  => array('long' => 'Ccc108', 'description' => ''),
		'109'  => array('long' => 'Ccc109', 'description' => ''),
		'110'  => array('long' => 'Ccc110', 'description' => ''),
		'111'  => array('long' => 'Ccc111', 'description' => ''),
		'112'  => array('long' => 'Ccc112', 'description' => ''),
		'113'  => array('long' => 'Ccc113', 'description' => ''),
		'114'  => array('long' => 'Ccc114', 'description' => ''),
		'115'  => array('long' => 'Ccc115', 'description' => ''),
		'116'  => array('long' => 'Ccc116', 'description' => ''),
		'117'  => array('long' => 'Ccc117', 'description' => ''),
		'118'  => array('long' => 'Ccc118', 'description' => ''),
		'119'  => array('long' => 'Ccc119', 'description' => ''),
		'120'  => array('long' => 'Ccc120', 'description' => ''),
		'121'  => array('long' => 'Ccc121', 'description' => ''),
		'122'  => array('long' => 'Ccc122', 'description' => ''),
		'123'  => array('long' => 'Ccc123', 'description' => ''),
		'124'  => array('long' => 'Ccc124', 'description' => ''),
		'125'  => array('long' => 'Ccc125', 'description' => ''),
		'126'  => array('long' => 'Ccc126', 'description' => ''),
		'127'  => array('long' => 'Ccc127', 'description' => ''),
		'128'  => array('long' => 'Ccc128', 'description' => ''),
		'129'  => array('long' => 'Ccc129', 'description' => ''),
		'130'  => array('long' => 'Ccc130', 'description' => ''),
		'131'  => array('long' => 'Ccc131', 'description' => ''),
		'132'  => array('long' => 'Ccc132', 'description' => ''),
		'133'  => array('long' => 'Ccc133', 'description' => ''),
		'134'  => array('long' => 'Ccc134', 'description' => ''),
		'135'  => array('long' => 'Ccc135', 'description' => ''),
		'136'  => array('long' => 'Ccc136', 'description' => ''),
		'137'  => array('long' => 'Ccc137', 'description' => ''),
		'138'  => array('long' => 'Ccc138', 'description' => ''),
		'139'  => array('long' => 'Ccc139', 'description' => ''),
		'140'  => array('long' => 'Ccc140', 'description' => ''),
		'141'  => array('long' => 'Ccc141', 'description' => ''),
		'142'  => array('long' => 'Ccc142', 'description' => ''),
		'143'  => array('long' => 'Ccc143', 'description' => ''),
		'144'  => array('long' => 'Ccc144', 'description' => ''),
		'145'  => array('long' => 'Ccc145', 'description' => ''),
		'146'  => array('long' => 'Ccc146', 'description' => ''),
		'147'  => array('long' => 'Ccc147', 'description' => ''),
		'148'  => array('long' => 'Ccc148', 'description' => ''),
		'149'  => array('long' => 'Ccc149', 'description' => ''),
		'150'  => array('long' => 'Ccc150', 'description' => ''),
		'151'  => array('long' => 'Ccc151', 'description' => ''),
		'152'  => array('long' => 'Ccc152', 'description' => ''),
		'153'  => array('long' => 'Ccc153', 'description' => ''),
		'154'  => array('long' => 'Ccc154', 'description' => ''),
		'155'  => array('long' => 'Ccc155', 'description' => ''),
		'156'  => array('long' => 'Ccc156', 'description' => ''),
		'157'  => array('long' => 'Ccc157', 'description' => ''),
		'158'  => array('long' => 'Ccc158', 'description' => ''),
		'159'  => array('long' => 'Ccc159', 'description' => ''),
		'160'  => array('long' => 'Ccc160', 'description' => ''),
		'161'  => array('long' => 'Ccc161', 'description' => ''),
		'162'  => array('long' => 'Ccc162', 'description' => ''),
		'163'  => array('long' => 'Ccc163', 'description' => ''),
		'164'  => array('long' => 'Ccc164', 'description' => ''),
		'165'  => array('long' => 'Ccc165', 'description' => ''),
		'166'  => array('long' => 'Ccc166', 'description' => ''),
		'167'  => array('long' => 'Ccc167', 'description' => ''),
		'168'  => array('long' => 'Ccc168', 'description' => ''),
		'169'  => array('long' => 'Ccc169', 'description' => ''),
		'170'  => array('long' => 'Ccc170', 'description' => ''),
		'171'  => array('long' => 'Ccc171', 'description' => ''),
		'172'  => array('long' => 'Ccc172', 'description' => ''),
		'173'  => array('long' => 'Ccc173', 'description' => ''),
		'174'  => array('long' => 'Ccc174', 'description' => ''),
		'175'  => array('long' => 'Ccc175', 'description' => ''),
		'176'  => array('long' => 'Ccc176', 'description' => ''),
		'177'  => array('long' => 'Ccc177', 'description' => ''),
		'178'  => array('long' => 'Ccc178', 'description' => ''),
		'179'  => array('long' => 'Ccc179', 'description' => ''),
		'180'  => array('long' => 'Ccc180', 'description' => ''),
		'181'  => array('long' => 'Ccc181', 'description' => ''),
		'182'  => array('long' => 'Ccc182', 'description' => ''),
		'183'  => array('long' => 'Ccc183', 'description' => ''),
		'184'  => array('long' => 'Ccc184', 'description' => ''),
		'185'  => array('long' => 'Ccc185', 'description' => ''),
		'186'  => array('long' => 'Ccc186', 'description' => ''),
		'187'  => array('long' => 'Ccc187', 'description' => ''),
		'188'  => array('long' => 'Ccc188', 'description' => ''),
		'189'  => array('long' => 'Ccc189', 'description' => ''),
		'190'  => array('long' => 'Ccc190', 'description' => ''),
		'191'  => array('long' => 'Ccc191', 'description' => ''),
		'192'  => array('long' => 'Ccc192', 'description' => ''),
		'193'  => array('long' => 'Ccc193', 'description' => ''),
		'194'  => array('long' => 'Ccc194', 'description' => ''),
		'195'  => array('long' => 'Ccc195', 'description' => ''),
		'196'  => array('long' => 'Ccc196', 'description' => ''),
		'197'  => array('long' => 'Ccc197', 'description' => ''),
		'198'  => array('long' => 'Ccc198', 'description' => ''),
		'199' => array('long' => 'Ccc199', 'description' => 'End of fixed position classes'),
		'200' => array('long' => 'Attached_Below_Left', 'description' => 'Marks attached at the bottom left'),
		'202' => array('long' => 'Attached_Below', 'description' => 'Marks attached directly below'),
		'204' => array('long' => 'Attached_Bottom', 'description' => 'Marks attached at the bottom right'),
		'208' => array('long' => 'Attached_Left', 'description' => 'Marks attached to the left'),
		'210' => array('long' => 'Attached_Right', 'description' => 'Marks attached to the right'),
		'212' => array('long' => 'Attached_Top_Left', 'description' => 'Marks attached at the top left'),
		'214' => array('long' => 'Attached_Above', 'description' => 'Marks attached directly above'),
		'216' => array('long' => 'Attached_Above_Right', 'description' => 'Marks attached at the top right'),
		'218' => array('long' => 'Below_Left', 'description' => 'Distinct marks at the bottom left'),
		'220' => array('long' => 'Below', 'description' => 'Distinct marks directly below'),
		'222' => array('long' => 'Below_Right', 'description' => 'Distinct marks at the bottom right'),
		'224' => array('long' => 'Left', 'description' => 'Distinct marks to the left'),
		'226' => array('long' => 'Right', 'description' => 'Distinct marks to the right'),
		'228' => array('long' => 'Above_Left', 'description' => 'Distinct marks at the top left'),
		'230' => array('long' => 'Above', 'description' => 'Distinct marks directly above'),
		'232' => array('long' => 'Above_Right', 'description' => 'Distinct marks at the top right'),
		'233' => array('long' => 'Double_Below', 'description' => 'Distinct marks subtending two bases'),
		'234' => array('long' => 'Double_Above', 'description' => 'Distinct marks extending above two bases'),
		'240' => array('long' => 'Iota_Subscript', 'description' => 'Greek iota subscript only'),
	);

	private $bidi_class = array(
		/* Strong Types */
		'L' => array('long' => 'Left_To_Right', 'description' => 'any strong left-to-right character'),
		'R' => array('long' => 'Right_To_Left', 'description' => 'any strong right-to-left (non-Arabic-type) character'),
		'AL' => array('long' => 'Arabic_Letter', 'description' => 'any strong right-to-left (Arabic-type) character'),
		/* Weak Types */
		'EN' => array('long' => 'European_Number', 'description' => 'any ASCII digit or Eastern Arabic-Indic digit'),
		'ES' => array('long' => 'European_Separator', 'description' => 'plus and minus signs'),
		'ET' => array('long' => 'European_Terminator', 'description' => 'a terminator in a numeric format context, includes currency signs'),
		'AN' => array('long' => 'Arabic_Number', 'description' => 'any Arabic-Indic digit'),
		'CS' => array('long' => 'Common_Separator', 'description' => 'commas, colons, and slashes'),
		'NSM' => array('long' => 'Nonspacing_Mark', 'description' => 'any nonspacing mark'),
		'BN' => array('long' => 'Boundary_Neutral', 'description' => 'most format characters, control codes, or noncharacters'),
		/* Neutral Types */
		'B' => array('long' => 'Paragraph_Separator', 'description' => 'various newline characters'),
		'S' => array('long' => 'Segment_Separator', 'description' => 'various segment-related control codes'),
		'WS' => array('long' => 'White_Space', 'description' => 'spaces'),
		'ON' => array('long' => 'Other_Neutral', 'description' => 'most other symbols and punctuation marks'),
		/* Explicit Formatting Types */
		'LRE' => array('long' => 'Left_To_Right_Embedding', 'description' => 'U+202A: the LR embedding control'),
		'LRO' => array('long' => 'Left_To_Right_Override', 'description' => 'U+202D: the LR override control'),
		'RLE' => array('long' => 'Right_To_Left_Embedding', 'description' => 'U+202B: the RL embedding control'),
		'RLO' => array('long' => 'Right_To_Left_Override', 'description' => 'U+202E: the RL override control'),
		'PDF' => array('long' => 'Pop_Directional_Format', 'description' => 'U+202C: terminates an embedding or override control'),
		'LRI' => array('long' => 'Left_To_Right_Isolate', 'description' => 'U+2066: the LR isolate control'),
		'RLI' => array('long' => 'Right_To_Left_Isolate', 'description' => 'U+2067: the RL isolate control'),
		'FSI' => array('long' => 'First_Strong_Isolate', 'description' => 'U+2068: the first strong isolate control'),
		'PDI' => array('long' => 'Pop_Directional_Isolate', 'description' => 'U+2069: terminates an isolate control'),
	);

	private $numeric_type = array(
		'None' => array('long' => 'None'),
		'Digit' => array('long' => 'Digit'),
		'Decimal' => array('long' => 'Decimal'),
		'Numeric' => array('long' => 'Numeric'),
	);


	static public $bycode;
	static public $byname;
	
	static function program()
	{
		return $_SERVER["argv"][0];
	}
	
	
	static function error_log($msg)
	{
		$program = ucd::program();
		error_log("$program: $msg");
	}

	/*
	 * Constructor
	 */
	public function ucd(&$opt)
	{
		$this->opt = $opt;
		$this->localdir = './';
	}
	
	/*
	 * Destructor
	 */
	public function __destruct()
	{
	}
	
	
	private function parse_database($filename)
	{
		$out = NULL;
		$localname = $this->localdir . basename($filename);
		ucd::$bycode = array();
		ucd::$byname = array();
		$filename = $localname;
		$fh = fopen($filename, "r");
		if (!$fh)
		{
			throw(new Exception("can't open $filename"));
		}
		printf("parsing %s\n", $filename);
		$lineno = 0;
		while (!feof($fh))
		{
			$line = fgets($fh);
			if ($out)
				fputs($out, $line);
			$lineno++;
			if (preg_match("/^#/", $line)) continue;
			if (preg_match("/^[ \t]*$/", $line)) continue;
			if (preg_match('/^[ \t]*([0-9a-fA-F]+);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*)/', $line, $matches))
			{
				$ent = array();
				$code = intval($matches[1], 16);
				$name = $matches[2];
				$ent['gc'] = $matches[3];                       /* general category */
				$ent['ccc'] = $matches[4];                      /* canonical combining class */
				$ent['bidi'] = $matches[5];                     /* bidi class */
				$ent['decomp'] = $matches[6];					/* decomposition type */
				$ent['numeric1'] = $matches[7];					/* numeric type */
				$ent['numeric2'] = $matches[8];
				$ent['numeric3'] = $matches[9];
				$ent['mirrored'] = $matches[10];				/* bidi mirrored */
				$ent['oldname'] = $matches[11];					/* unicode 1.0 name */
				$ent['comment'] = $matches[12];					/* ISO comment */
				$ent['uppercase'] = intval($matches[13], 16);	/* simple uppercase mapping */
				$ent['lowercase'] = intval($matches[14], 16);	/* simple lowercase mapping */
				$ent['titlecase'] = intval($matches[15], 16);	/* simple titlecase mapping */
				switch ($code)
				{
					case 0x1e9e: $ent['lowercase'] = $code; break;
					case 0x212a: $ent['lowercase'] = $code; break;
					case 0x212b: $ent['lowercase'] = $code; break;
					case 0x017f: $ent['uppercase'] = $code; $ent['titlecase'] = $code; break;
				}
				if ($ent['uppercase'] == 0)
					$ent['uppercase'] = $code;
				if ($ent['lowercase'] == 0)
					$ent['lowercase'] = $code;
				if ($ent['titlecase'] == 0)
					$ent['titlecase'] = $ent['uppercase'];
				if ($name == '<control>')
					$name = $ent['oldname'];
				if ($name == '')
					$name = 'U+' . $matches[1];
				if ($ent['gc'] == '')
					$ent['gc'] = 'Cn';
				if ($ent['ccc'] == '')
					$ent['ccc'] = '0';
				$ent['code'] = $code;
				$ent['name'] = $name;
				if ($this->opt['debug']) { printf("0x%08x %s\n", $code, $name); }

				if (!isset($this->category[$ent['gc']])) { ucd::error_log(sprintf("%s:$lineno: unknown category %s", $filename, $ent['gc'])); }
				if (!isset($this->canonical_combining_class[$ent['ccc']])) { ucd::error_log(sprintf("%s:$lineno: unknown combining class %s", $filename, $ent['ccc'])); }
				if (!isset($this->bidi_class[$ent['bidi']])) { ucd::error_log(sprintf("%s:$lineno: unknown bidi class %s", $filename, $ent['bidi'])); }

				if (isset(ucd::$bycode[$code])) { ucd::error_log(sprintf("%s:$lineno: duplicate character code 0x%x", $filename, $code)); }
				if (isset(ucd::$byname[$name]))
				{
					ucd::error_log("$filename:$lineno: duplicate character name $name");
					if ($name == 'BELL')
						$ent = ucd::$byname[$name];
				}
				ucd::$bycode[$code] = $ent;
				ucd::$byname[$name] = $ent;
			} else
			{
				fprintf(STDERR, "ignoring line %d: %s\n", $lineno, $line);
			}
		}
		fclose($fh);
		if ($out)
			fclose($out);
	}

	
	private function dump_database()
	{
		$localname = 'ucbycode.h';
		$out = fopen($localname, "w");
		if (!$out)
		{
			throw(new Exception("can't create $localname"));
		}
		fputs($out, "/* automatically generated from UnicodeData.txt - DO NOT EDIT */\n\n");
		fputs($out, "struct ucd {\n");
		fputs($out, "\tuint32_t code;\n");
		fputs($out, "\tuint32_t name_offset;\n");
		fputs($out, "\tuint16_t canonical_combining_class;\n");
		fputs($out, "\tuint8_t  general_class;\n");
		fputs($out, "\tuint8_t  bidi_class;\n");
		fputs($out, "};\n");

		fputs($out, "static char const ucd_names[] = {\n");
		foreach (ucd::$bycode as $code => $ent)
		{
			// if ($code < 0x10000)
			{
				$code = sprintf('0x%06x', $code);
				fputs($out, "\t" . '"' . $ent['name'] . '\\0"' . "\n");
			}
		}
		fputs($out, "};\n");

		$offset = 0;
		fputs($out, "static struct ucd const ucd_bycode[] = {\n");
		foreach (ucd::$bycode as $code => $ent)
		{
			// if ($code < 0x10000)
			{
				$code = sprintf('0x%06x', $code);
				fputs($out, "\t{ " . $code . ', ' . $offset . ', ' . $ent['ccc'] . ', UCD_GC_' . strtoupper($ent['gc']) . ', UCD_BIDI_' . strtoupper($ent['bidi']) . ' },' . "\n");
				$offset += strlen($ent['name']) + 1;
			}
		}
		fputs($out, "};\n");
		fclose($out);
	}

	public function create_database()
	{
		try {
			$this->parse_database("UnicodeData.txt");
			$this->dump_database();
		} catch (Exception $e)
		{
			ucd::error_log($e->getMessage());
			return false;
		}
		return true;
	}
}



class ucd_main {
	private $is_cli;
	private $ucd;
	
	private $opt = array(
		'debug' => false,
		'verbose' => false,
		'help' => false,
	);
	
	
	public function ucd_main()
	{
		$this->is_cli = substr(php_sapi_name(), 0, 3) == 'cli';
		if ($this->is_cli)
		{
			ini_set("log_errors", "On");
			ini_set("display_errors", "Off");
			ini_set("enable_dl", "0"); /* enable dl(); should be safe wehen using cli version */
			error_reporting(E_ALL | E_STRICT);
		} else
		{
			ini_set("log_errors", "Off");
			ini_set("display_errors", "Off");
			/* leave "enable_dl" as it was */
			error_reporting(0);
		}
		$this->ucd = NULL;
	}


	public function __destruct()
	{
		unset($this->opt);
	}


	private function usage()
	{
		if (function_exists('dl'))
		{
			if (!extension_loaded('PDO'))
				dl('php_pdo');
		}
		print '
usage: ' . ucd::program() . ' [<options>] <command>

options:
  -?, --help           display this helpscreen and exit
  
  --debug              enable debug
  --verbose            print progress messages
';
	}
	
	
	private function usage_error($msg)
	{
		ucd::error_log($msg);
		if ($this->is_cli)
			exit(1);
	}
	
	/*
	 * FIXME: should really use getopt(), but seems
	 * to be not available on Windows, and sometimes
	 * doesn't support long options on unix
	 */
	
	private function parse_args()
	{
		for ($i = 1; $i < $_SERVER["argc"]; $i++)
		{
			switch($_SERVER["argv"][$i])
			{
			case "-v":
			case "-verbose":
			case "--verbose":
				$this->opt['verbose'] = true;
				break;
		
			case "+verbose":
				$this->opt['verbose'] = false;
				break;
		
			case "-d":
			case "-debug":
			case "--debug":
				$this->opt['debug'] = true;
				break;
		
			case "+debug":
				$this->opt['debug'] = false;
				break;
		
			case "-?":
			case "-h":
			case "-help":
			case "--help":
				$this->opt['help'] = true;
				break;
			
			default:
				$this->usage_error("unrecognised option " . $_SERVER["argv"][$i]);
				break;
			}
		}
	}
	
	
	public function run()
	{
		$this->parse_args();
		
		if ($this->opt['help'])
		{
			$this->usage();
			exit(0);
		}
		
		try {
			$this->ucd = new ucd($this->opt);
		} catch (Exception $e) {
			ucd::error_log($e->getMessage());
		}
		$status = isset($this->ucd);
		
		if ($status)
		{
			$status &= $this->ucd->create_database();
		}
			
		unset($this->ucd);
		
		if ($this->is_cli)
			exit($status ? 0 : 1);
	}
}

{
	$main = new ucd_main();
	$main->run();
}
?>
