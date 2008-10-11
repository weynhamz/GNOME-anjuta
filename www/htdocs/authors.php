<!--
	<table border="0" width="100%" cellspacing="0" cellpadding="5">
	<tbody>
	<tr>
	<td><img border="0" src="image_web/members_icon_ws.jpg" width="48" height="50"></td>
	<td width="100%"><img border="0" src="image_web/authors_title.jpg" width="315"
	height="37"></td></tr></tbody></table>
-->

<?php

// get contents of a file into a string
$filename = "svn/AUTHORS";
$handle = fopen($filename, "r");
$contents = fread($handle, filesize($filename));
$contents = preg_replace("/note[^\~]*/im", "", $contents);
$contents = preg_replace("/^([^\n]+:)\n\-\-\-\-+$/m", "<h3>\\1</h3><ul>", $contents);
$contents = preg_replace("/^\t([^\<]+)\<([^\>]+\@[^\>]+)\>.*?\(([^\)]+)\).*?$/m", "<li>\\1 &lt;\\2&gt; (\\3)</li>\n", $contents);
$contents = preg_replace("/^\t([^\<]+)\<([^\>]+\@[^\>]+)\>.*?$/m", "<li>\\1 &lt;\\2&gt;</li>\n", $contents);
$contents = preg_replace("/^\t(.+?)$/m", "<li>\\1</li>\n", $contents);
$contents = preg_replace("/\n\t\n/m", "</ul>\n", $contents);
/* $contents = preg_replace("/\n/m", "<br>", $contents);
$contents = preg_replace("/\n\t(*)\n+/m", "\n<li>\\1</li>\n", $contents);
*/

$contents = preg_replace("/\@/", "<i><script language=javascript>document.write(\"&#64;\");</script><noscript>_at_</noscript></i>", $contents);
print($contents);

echo("<p>");
$members_database = "databases/members.database";
if (!($fp = fopen($members_database, "r"))) {
	die("could not open Authors Database");
}
$xml_authors = xml_parser_create();
$authors_data = fread($fp,filesize ($members_database));
xml_parser_set_option($xml_authors,XML_OPTION_CASE_FOLDING,0);
xml_parse_into_struct($xml_authors,$authors_data,$vals,$index);

foreach ($vals as $key) {
	$desc = $key[attributes];
	if ($key[level] >= 2) {
		if ($key[type] == 'open') {
			echo "<h3>$desc[name]</h3><ul>\n";
		}
	}
	if ($key[level] >= 3) {	
		if ($key[type] == 'complete') {
			echo "<li>$desc[name] &lt;$desc[email]&gt; ($desc[country]): $desc[contribution]</li>\n";
		}
	}
	if ($key[level] >= 2) {
		if ($key[type] == 'close') {
			echo "</ul>\n";
		}
	}
}
xml_parser_free($xml_authors);

echo ("<p><b>Note: </b><i>If you have contributed something significant to Anjuta and your name is missing here, please email Naba Kumar &lt;naba<i><script language=javascript>document.write(\"&#64;\");</script><noscript>_at_</noscript></i>gnome.org&gt; with your Full name, email address, country and a brief reminder of what you have done. This page is auto-generated from AUTHORS file in CVS.</i><p>");

?>
