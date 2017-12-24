<?php


class ucd {
	private $opt = NULL;
	private $localdir;
	
	private $categories = array(
		'Lu' => array('long' => 'Uppercase_Letter', 'description' => 'an uppercase letter'),
		'Ll' => array('long' => 'Lowercase_Letter', 'description' => 'a lowercase letter'),
		'Lt' => array('long' => 'Titlecase_Letter', 'description' => 'a digraphic character, with first part uppercase'),
		'Lm' => array('long' => 'Modifier_Letter', 'description' => 'a modifier letter'),
		'Lo' => array('long' => 'Other_Letter', 'description' => 'other letters, including syllables and ideographs'),
		'Mn' => array('long' => 'Nonspacing_Mark', 'description' => 'a nonspacing combining mark (zero advance width)'),
		'Mc' => array('long' => 'Spacing_Mark', 'description' => 'a spacing combining mark (positive advance width)'),
		'Me' => array('long' => 'Enclosing_Mark', 'description' => 'an enclosing combining mark'),
		'Nd' => array('long' => 'Decimal_Number', 'description' => 'a decimal digit'),
		'Nl' => array('long' => 'Letter_Number', 'description' => 'a letterlike numeric character'),
		'No' => array('long' => 'Other_Number', 'description' => 'a numeric character of other type'),
		'Pc' => array('long' => 'Connector_Punctuation', 'description' => 'a connecting punctuation mark, like a tie'),
		'Pd' => array('long' => 'Dash_Punctuation', 'description' => 'a dash or hyphen punctuation mark'),
		'Ps' => array('long' => 'Open_Punctuation', 'description' => 'an opening punctuation mark (of a pair)'),
		'Pe' => array('long' => 'Close_Punctuation', 'description' => 'a closing punctuation mark (of a pair)'),
		'Pi' => array('long' => 'Initial_Punctuation', 'description' => 'an initial quotation mark'),
		'Pf' => array('long' => 'Final_Punctuation', 'description' => 'a final quotation mark'),
		'Po' => array('long' => 'Other_Punctuation', 'description' => 'a punctuation mark of other type'),
		'Sm' => array('long' => 'Math_Symbol', 'description' => 'a symbol of primarily mathematical use'),
		'Sc' => array('long' => 'Currency_Symbol', 'description' => 'a currency sign'),
		'Sk' => array('long' => 'Modifier_Symbol', 'description' => 'a non-letterlike modifier symbol'),
		'So' => array('long' => 'Other_Symbol', 'description' => 'a symbol of other type'),
		'Zs' => array('long' => 'Space_Separator', 'description' => 'a space character (of various non-zero widths)'),
		'Zl' => array('long' => 'Line_Separator', 'description' => 'U+2028 LINE SEPARATOR only'),
		'Zp' => array('long' => 'Paragraph_Separator', 'description' => 'U+2029 PARAGRAPH SEPARATOR only'),
		'Cc' => array('long' => 'Control', 'description' => 'a C0 or C1 control code'),
		'Cf' => array('long' => 'Format', 'description' => 'a format control character'),
		'Cs' => array('long' => 'Surrogate', 'description' => 'a surrogate code point'),
		'Co' => array('long' => 'Private_Use', 'description' => 'a private-use character'),
		'Cn' => array('long' => 'Unassigned', 'description' => 'a reserved unassigned code point or a noncharacter'),
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
				$ent['gc'] = $matches[3];                         /* general category */
				$ent['ccc'] = $matches[4];                        /* canonical combining class */
				$ent['bidi'] = $matches[5];                       /* bidi class */
				$ent['decomp'] = $matches[6];
				$ent['numeric1'] = $matches[7];
				$ent['numeric2'] = $matches[8];
				$ent['numeric3'] = $matches[9];
				$ent['mirrored'] = $matches[10];
				$ent['oldname'] = $matches[11];
				$ent['comment'] = $matches[12];
				$ent['uppercase'] = intval($matches[13], 16);
				$ent['lowercase'] = intval($matches[14], 16);
				$ent['titlecase'] = intval($matches[15], 16);
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
				$ent['code'] = $code;
				$ent['name'] = $name;
				if ($this->opt['debug']) { printf("0x%08x %s\n", $code, $name); }
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
		fputs($out, "\tuint16_t code;\n");
		fputs($out, "\tconst char *name;\n");
		fputs($out, "};\n");
		fputs($out, "static struct ucd const ucd_bycode[] = {\n");
		foreach (ucd::$bycode as $code => $ent)
		{
			if ($code < 0x10000)
			{
				$code = sprintf('0x%x', $code);
				fputs($out, "\t{ " . $code . ', "' . $ent['name'] . '" },' . "\n");
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
