<?php
// get contents of a file into a string
$page = $_GET["subpage"];
$page = preg_replace("/\#.*$/", "", $page);
$dirname = $page;
$dirname = preg_replace("/^(.*\/).*$/m", "\\1", $dirname); 

$filename = $page;
$handle = fopen($filename, "r");
$contents = fread($handle, filesize($filename));

// $contents = require($page);
$contents = preg_replace("/^.*?<body.*?>/ims", "", $contents);
$contents = preg_replace("/<\s*\/\s*body\s*>.*$/ims", "", $contents);
$contents = preg_replace("/(<a\s*name=\"[^\"]+\"\s*>)/im", "\\1</a>", $contents); 
$contents = preg_replace("/href=\"([^\:]+?)\"/im", "href=\"/documentations/subpage/$dirname\\1\"", $contents);
$contents = preg_replace("/src=\"([^\:]+?)\"/im", "src=\"/$dirname\\1\"", $contents);
$contents = preg_replace("/style=\"clear\: both\"/im", "", $contents);
print($contents);
?>
