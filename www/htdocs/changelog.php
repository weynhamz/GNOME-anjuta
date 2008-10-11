	<p>
	<?php
		// get contents of a file into a string
		$filename = "svn/ChangeLog";
		$handle = fopen($filename, "r");
		// $contents = fread($handle, filesize($filename));
		$contents = fread($handle, 30000);
$contents = preg_replace("/^(.*\d{4}-\d{2}-\d{2}.*?\n).*$/sm", "\\1<br>...<br>\n", $contents);
		$contents = preg_replace("/#(\d+)/", "<a href=\"http://bugzilla.gnome.org/show_bug.cgi?id=\\1\">#\\1</a>", $contents);
		$contents = preg_replace("/(<.+?>)\n+(.+?)\n+(\d{4}-\d{2}-\d{2})/s", "\\1\n<table width=\"100%\" border=0><tr><td width=\"40\">&nbsp;</td><td>\n\t\\2</td></tr></table><br>\n\\3", $contents);
		$contents = preg_replace("/^(\d{4}-\d{2}-\d{2})(.+?)<(.+?)\@(.+?)>\s*$/m", "<font color=\"#ff0000\"><b>\\1</b></font> <font color=\"#008800\"><i><b>\\2</b></i></font> &lt;<i><b>\\3<script language=javascript>document.write(\"&#64;\");</script><noscript>_at_</noscript>\\4</b></i>&gt;", $contents);
		$contents = preg_replace("/^\s*\*(.+?)\:/sm", "<br>* <font color=\"#aa00aa\">\\1:</font> ", $contents);
$contents = preg_replace("/\=\=[\=]+([^\=]+?)\=\=[\=]+/sm", "\n<h3>=== \\1 ===</h3>\n", $contents);
		fclose($handle);
		print($contents);
	?>
	</p>
