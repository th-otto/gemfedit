<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN"
          "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xml:lang="en" lang="en" xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="content-type" content="text/html;charset=UTF-8" />
<title>SpdViewer Home</title>
<style type="text/css">
body {
	margin:1em;
	padding:1em;
	background-color: #cccccc;
}
</style>
<!-- BEGIN CONFIGURATION -->
<?php
error_reporting(E_ALL & ~E_WARNING);
ini_set("display_errors", 1);
$btfontdir = 'btfonts';
?>
<script type="text/javascript">
var btfontdir = 'btfonts';
</script>
<!-- END CONFIGURATION -->
<script type="text/javascript">
function submitUrl(url)
{
	var f = document.getElementById('spdviewform');
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
	var f = document.getElementById('spdviewform');
	f.method = 'post';
	f.enctype = "multipart/form-data";
	f.submit();
}
</script>
</head>
<body>
<div>
<h1 style="font-weight: bold;">
SpeedoFont Viewer Web Service<span style="font-size: 13pt"> - provided by <a href="http://www.tho-otto.de/">Thorsten Otto</a></span>
</h1>
<br/><br/>

<em>Want to browse SpeedoFont files in an HTML browser?</em><br />
<br />

<form action="spdview.cgi" method="get" id="spdviewform">

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
Type in URL of a SpeedoFont file (it must be remotely accessible from that URL<br />
for example <a href="javascript: submitUrl(&quot;<?php echo dirname($_SERVER['SCRIPT_NAME']) . "/$btfontdir/" ?>bx000003.spd&quot;);">http://www.tho-otto.de<?php echo dirname($_SERVER['SCRIPT_NAME']) . "/$btfontdir/" ?>bx000003.spd</a>:
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
Choose a SpeedoFont file for upload <br />
<input type="file" id="file" name="file" size="60" accept=".spd,.SPD" style="margin-top: 1ex;" />
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
<td colspan="2">
Quality:
</td>
<td>
<select id="quality" name="quality">
<option value="0">Normal</option>
<option value="1" selected="selected">Screen</option>
<!-- <option value="2">Outline</option> -->
<option value="3">2D</option>
</select>
</td>
</tr>

<tr>
<td colspan="2">
Pointsize:
</td>
<td>
<select id="points" name="points">
<option value="60">6</option>
<option value="80">8</option>
<option value="90">9</option>
<option value="100">10</option>
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

class spd {
	public $files;
	public function spd($opt) {
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

{
	$filelist = '';
	if ($dir = opendir($btfontdir))
	{
		$filelist .= "Local files:\n\n";
		$spd = new spd(array());
		while (false !== ($entry = readdir($dir))) {
			if ($entry == ".") continue;
			if ($entry == "..") continue;
			if (!fnmatch("*.spd", $entry)) continue;
	
			$spd->files[$entry] = array();
			$spd->files[$entry]['name'] = $entry;
			$info = fopen("$btfontdir/$entry", "rb");
			if (is_resource($info))
			{
				fseek($info, 24);
				$fontname = fread($info, 70);
				if (strpos($fontname, 0) >= 0)
					$fontname = substr($fontname, 0, strpos($fontname, 0));
				$spd->files[$entry]['fontname'] = $fontname;
				fclose($info);
			} else
			{
				$spd->files[$entry]['fontname'] = 'no such file';
			}
	    }
	
	    uksort($spd->files, array($spd, 'cmp_name'));
		$filelist .= "<ul>\n";
	    foreach ($spd->files as $name => $entry) {
	    	$filelist .= '<li><a href="javascript: submitUrl(&quot;' . dirname($_SERVER['SCRIPT_NAME']) . "/$btfontdir/" . js_escape($name) . '&quot;);">' . htmlspecialchars($name, ENT_QUOTES, 'UTF-8') . "</a>";
			if (isset($entry['fontname']))
				$filelist .= "&nbsp;" . htmlspecialchars($entry['fontname']);
			$filelist .= "</li>\n";
	    }
	 	closedir($dir);
		$filelist .= "</ul>\n";
	}
}
echo $filelist;
?>


<hr />

</div>

</body>
</html>
