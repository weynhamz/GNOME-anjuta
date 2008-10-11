<?php
	// get contents of a file into a string
	$filename = "svn/ROADMAP";
	$handle = fopen($filename, "r");
	$contents = fread($handle, filesize($filename));
	$contents = preg_replace("/^([^\n]+):\n\-\-\-\-+$/m", "<h3>\\1</h3>", $contents);
	$contents = preg_replace("/\n\n+/m", "<p>", $contents);
	//$contents = preg_replace("/----+/m", "<hr>", $contents);
	$contents = preg_replace("/\n\s*[\*\-]/m", "<br>- ", $contents);
	$contents = preg_replace("/(http\:\/\/[\d\w\&\=\/\-\.\?]+)/", "<a href=\"\\1\">\\1</a>", $contents);
	print($contents);
?>
