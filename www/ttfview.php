<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN"
          "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xml:lang="en" lang="en" xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="content-type" content="text/html;charset=UTF-8" />
<title>TtfViewer Home</title>
<style type="text/css">
body {
	margin:1em;
	padding:1em;
	background-color: #cccccc;
}
ul {
   padding: 0px;
   margin: 0px;
   border-spacing: 0px;
}
li {
   padding: 0px;
   margin: 0px;
   border-spacing: 0px;
}
</style>
<!-- BEGIN CONFIGURATION -->
<?php
error_reporting(E_ALL & ~E_WARNING);
ini_set("display_errors", 1);
$ttffontdir = 'ttffonts';
?>
<script type="text/javascript">
var ttffontdir = 'ttffonts';
</script>
<!-- END CONFIGURATION -->
<script type="text/javascript">
function submitUrl(url)
{
	var f = document.getElementById('ttfviewform');
	f.method = 'get';
	f.enctype = "application/x-www-form-urlencoded";
	var fileinput = document.getElementById('file');
	var urlinput = document.getElementById('url');
	fileinput.disabled = true;
	var oldvalue = urlinput.value;
	urlinput.value = url;
	f.submit();
	fileinput.disabled = false;
	urlinput.value = oldvalue;
}
function submitFile()
{
	var f = document.getElementById('ttfviewform');
	f.method = 'post';
	f.enctype = "multipart/form-data";
	f.submit();
}
</script>
</head>
<body>
<div>
<h1 style="font-weight: bold;">
TrueTypeFont Viewer Web Service<span style="font-size: 13pt"> - provided by <a href="http://www.tho-otto.de/">Thorsten Otto</a></span>
</h1>
<br/><br/>

<em>Want to browse TrueType-Font files in an HTML browser?</em><br />
<br />

<form action="ttfview.cgi" method="get" id="ttfviewform">

<noscript>
<p><span style="color:red">
<b>Your browser does not support JavaScript.</b>
<br />
Some features will not work without JavaScript enabled.
</span>
<br />
<br /></p>
</noscript>

<table>
<tr style="vertical-align: top;">
<td>
<fieldset>
Type in URL of a font file (it must be remotely accessible from that URL<br />
for example <a href="javascript: submitUrl(&quot;<?php echo dirname($_SERVER['SCRIPT_NAME']) . "/$ttffontdir/" ?>NotoSans-Regular.ttf&quot;);">http://www.tho-otto.de<?php echo dirname($_SERVER['SCRIPT_NAME']) . "/$ttffontdir/" ?>NotoSans-Regular.ttf</a>:
<br />
<input type="text" id="url" name="url" size="60" tabindex="1" style="margin-top: 1ex;" />
<input id="submiturl" style="background-color: #cccccc; font-weight: bold; visibility: hidden;" type="button" value="View" onclick="submitUrl(document.getElementById('url').value);" />
<noscript>
<div id="submitnoscript"><span><input type="submit" style="background-color: #cccccc; font-weight: bold;" value="View" /></span></div>
</noscript>
<script type="text/javascript">
document.getElementById('submiturl').style.visibility="visible";
</script>
</fieldset>
<div id="uploadbox" style="display:none;">
<br />
<b>OR</b><br />
<br />
<fieldset>
Choose a font file for upload <br />
<input type="file" id="file" name="file" size="60" accept=".ttf,.otf,.pfb,.pfa" style="margin-top: 1ex;" />
<input id="submitfile" style="background-color: #cccccc; font-weight: bold;" type="button" value="View" onclick="submitFile();" /><br />
</fieldset>
<br />
</div>
<script type="text/javascript">
document.getElementById('uploadbox').style.display="block";
</script>
<br />

<fieldset>
<table>
<tr>
<td>Quality:</td>
<td>
<select id="quality" name="quality">
<option value="0">Normal</option>
<option value="1" selected="selected">Screen</option>
<!-- <option value="2">Outline</option> -->
<!-- <option value="3">2D</option> -->
</select>
</td>
</tr>

<tr>
<td>Pointsize:</td>
<td>
<select id="points" name="points">
<option value="60">6</option>
<option value="80">8</option>
<option value="90">9</option>
<option value="100">10</option>
<option value="110">11</option>
<option value="120" selected="selected">12</option>
<option value="140">14</option>
<option value="160">16</option>
<option value="180">18</option>
<option value="240">24</option>
<option value="360">36</option>
<option value="480">48</option>
<option value="640">64</option>
<option value="720">72</option>
</select>
</td>
</tr>

<tr>
<td>Resolution:</td>
<td>
<input type="number" id="resolution" name="resolution" value="95" />
</td>
</tr>

</table>
</fieldset>

<br />
</td>

<td style="width:1ex;"> </td>
<td style="width:1ex; background-color: #888888;"> </td>
<td style="width:1ex;"> </td>

</tr>

</table>
</form>

<?php

function js_escape($string)
{
	$string = str_replace('&', '&amp;', $string);
	$string = str_replace('<', '&lt;', $string);
	$string = str_replace('>', '&gt;', $string);
	$string = str_replace('"', '\&quot;', $string);
	return $string;
}

class ttf {
	public $files;
	public function ttf($opt)
	{
		$this->files = array();
	}
	public function __destruct()
	{
	}

	public function cmp_name($a, $b)
	{
		if ($this->files[$a]['name'] > $this->files[$b]['name'])
			return 1;
		if ($this->files[$a]['name'] < $this->files[$b]['name'])
			return -1;
		return $a > $b;
	}
}

$filelist = '';

function add_files($dirname)
{
	global $filelist;
	global $ttf;
	global $ttffontdir;
	
	if ($dir = opendir("${ttffontdir}/${dirname}"))
	{
		while (false !== ($entry = readdir($dir)))
		{
			if ($entry == ".") continue;
			if ($entry == "..") continue;
			if (fnmatch(".*", $entry)) continue;
			if (fnmatch("*.dir", $entry)) continue;
			if (fnmatch("*.scale*", $entry)) continue;
			if (fnmatch("map-*", $entry)) continue;
			if (fnmatch("*.afm", $entry)) continue;
			if (fnmatch("*.pcf*", $entry)) continue;
			
			if ($dirname == ".")
			{
				$name = $entry;
				$namei = $entry;
			} else
			{
				$name = "${dirname}/${entry}";
				$namei = "${dirname}/${entry}";
			}
			if (is_dir("${ttffontdir}/${name}"))
			{
				add_files("$name");
				continue;
			}
			
			$info = fopen("${ttffontdir}/${name}", "rb");
			if (is_resource($info))
			{
				if (fnmatch("*.pfa", $entry) ||
					fnmatch("*.pfb", $entry))
				{
					$ttf->files[$namei] = array();
					$ttf->files[$namei]['name'] = $name;
					$test = fread($info, 4096);
					$pos = strpos($test, '/FullName (');
					if ($pos >= 0)
					{
						$end = strpos($test, ')', $pos);
						if ($end >= 0)
						{
							$pos += 11;
							$ttf->files[$namei]['fontname'] = htmlspecialchars(substr($test, $pos, $end - $pos));
						}
					}
				} else if (fnmatch("*.ttf", $entry) ||
					fnmatch("*.otf", $entry) ||
					fnmatch("*.ttc", $entry) ||
					fnmatch("*.otc", $entry))
				{
					$test = fread($info, 12);
					$header = unpack("Nversion/nnumtables/nsearchrange/nentryselector/nrangeshift", $test);
					if ($header['version'] == 0x10000 || $header['version'] == 0x4f54544f)
					{
						$ttf->files[$namei] = array();
						$ttf->files[$namei]['name'] = $name;
						$ttf->files[$namei]['fontname'] = '';
						for ($i = 0; $i < $header['numtables']; $i++)
						{
							$test = fread($info, 16);
							$offset = unpack("a4tag/Nchecksum/Noffset/Nlength", $test);
							$pos = ftell($info);
							if ($offset['tag'] == 'head')
							{
								fseek($info, $offset['offset']);
								$test = fread($info, $offset['length']);
								$head = unpack('nmajor/nminor/Nrevision/Nchecksum/Nmagic/nflags/nunitsperem/Jcreated/Jmodified/nxmin/nymin/nxmax/nymax/nmacstyle/nlowestppem/ndirection/nindexformat/ndataformat', $test);
								$ttf->files[$namei]['head'] = $head;
							}
							if ($offset['tag'] == 'name')
							{
								fseek($info, $offset['offset']);
								$test = fread($info, 6);
								$nameheader = unpack('nformat/ncount/noffset', $test);
								$ttf->files[$namei]['names'] = array();
								for ($j = 0; $j < $nameheader['count']; $j++)
								{
									$test = fread($info, 12);
									$rec = unpack('nplatform/nencoding/nlanguage/nname/nlength/noffset', $test);
									if ($rec['length'] > 0)
									{
										$pos2 = ftell($info);
										fseek($info, $offset['offset'] + $nameheader['offset'] + $rec['offset']);
										$rec['value'] = fread($info, $rec['length']);
										fseek($info, $pos2);
									} else
									{
										$rec['value'] = '';
									}
									if ($rec['name'] == 4)
									{
										if (($rec['platform'] == 0) ||
											($rec['platform'] == 3 && $rec['encoding'] == 1) ||
											($rec['platform'] == 2 && $rec['encoding'] == 1))
										{
											$rec['value'] = iconv('UTF-16BE', 'UTF-8', $rec['value']);
											$ttf->files[$namei]['fontname'] = htmlspecialchars($rec['value']);
//											$ttf->files[$namei]['fontname'] .= '<pre>' . htmlspecialchars(print_r($rec, 1)) . '</pre>';
//											$ttf->files[$namei]['fontname'] .= 'platform: ' . $rec['platform'] . "<br />\n";
//											$ttf->files[$namei]['fontname'] .= 'encoding: ' . $rec['encoding'] . "<br />\n";
										} else if ($rec['platform'] == 1 && $rec['encoding'] == 0)
										{
											$ttf->files[$namei]['fontname'] = htmlspecialchars($rec['value']);
										}
									}
									$ttf->files[$namei]['names'][$rec['name']] = $rec;
								}
							}
							fseek($info, $pos);
						}
					}
				} else
				{
					$ttf->files[$namei] = array();
					$ttf->files[$namei]['name'] = $name;
					$ttf->files[$namei]['fontname'] = 'unknown file format';
				}
				fclose($info);
			} else
			{
				$ttf->files[$namei] = array();
				$ttf->files[$namei]['name'] = $name;
				$ttf->files[$namei]['fontname'] = 'no such file';
			}
	    }
	 	closedir($dir);
	}
}

if (is_dir($ttffontdir))
{
	$filelist .= "Local files:\n\n";
	$ttf = new ttf(array());

	add_files(".");
	
    uksort($ttf->files, array($ttf, 'cmp_name'));
	$filelist .= "<table>\n";
    foreach ($ttf->files as $name => $entry)
    {
    	$filelist .= '<tr valign="top"><td valign="top"><ul><li>';
    	$filelist .= '<a href="javascript: submitUrl(&quot;' . dirname($_SERVER['SCRIPT_NAME']) . "/$ttffontdir/" . js_escape($entry['name']) . '&quot;);">' . htmlspecialchars($entry['name'], ENT_QUOTES, 'UTF-8') . "</a>";
		$filelist .= "</li></ul></td>\n";
		if (isset($entry['fontname']))
			$filelist .= '<td valign="top">' . $entry['fontname'] . '</td>';
		$filelist .= "</tr>\n";
    }
	$filelist .= "</table>\n";
	echo $filelist;
}
?>


<hr />

</div>

</body>
</html>
