<?php

$compiled_langs = array();
$interprt_langs = array();
$flag = '';
$count = 0;
$task_done = 0;
$text_html = "";

$item_category = "";
$item_status = "";
$item_summary ="";
$item_comment = "";
$item_html = "";

$category_html = "";
$category_items = 0;

$index_html = "";
$index_completed_html = "";
$details_html = "";

function opening_element($parser, $element, $attributes) {
  // opening XML element callback function

  global $flag;
  global $task_done;
  global $item_html;
  global $category_html;
  global $text_html;
  global $item_category;
  global $item_summary;
  global $item_status;
  global $item_comment;
  global $details_html;
  global $count;
  
  if ($element == 'item')
    {
      $count++;
      $item_html = "<li><p><a name='$count'></a>";
    }
  if ($element == 'summary')
    {
      $flag = $element;
      if ($task_done == "1")
	{
	  $item_html .= "<strike><font color=\"#8f8f8f\">";
	}
      $item_html .= "<b>";
    }
  if ($element == 'comment')
    {
      $flag = $element;
      // echo ("");
    }
  if ($element == 'attribute')
    {
      $task_done = $attributes['done'];
    }
  if ($element == 'category')
    {
      $category_html = "<h3>";
      $category_html .= $attributes['title'];
      $category_html .= "</h3><ol>\n";
      $item_category = $attributes['title'];
    }
}

function closing_element($parser, $element) {
  // closing XML element callback function

  global $flag;
  global $task_done;
  global $item_html;
  global $category_html;
  global $text_html;
  global $category_items;
  global $index_html;
  global $index_completed_html;
  global $item_category;
  global $item_summary;
  global $item_status;
  global $item_comment;
  global $details_html;
  global $count;

  if ($element == 'item')
    {
      $item_html .= $text_html;
      $item_bounty = "-";
      $item_bugzilla = "-";

      if (preg_match('/\s*\[([^\]]+)\]/', $item_comment, $match)) {
        $item_info = $match[1];
      }

      if (preg_match('/Skill: (\w+)/', $item_info, $match)) {
	$item_skill = $match[1];
      }
      if (preg_match('/Bounty: \$(\d+)/', $item_info, $match)) {
	$item_bounty = "<font color=red>$" . $match[1] . " USD</font>";
      }
      if (preg_match('/Status: (\w+)/', $item_info, $match)) {
	$item_status = $match[1];
      }
      if (preg_match('/Bug: \#(\d+)/', $item_info, $match)) {
	$item_bugzilla = "<a href='http://bugzilla.gnome.org/show_bug.cgi?id=$match[1]'>#" . $match[1] . "</a>";
      }

      if ($task_done == 1)
	{
	  $item_status = "Completed";
	  $index_completed_html .= "<tr><td> $item_category</td>";
	  $index_completed_html .= "<td> $item_summary</td>";
	  $index_completed_html .= "<td> $item_bounty</td>";
	  $index_completed_html .= "<td> $item_status</td>";
	  $index_completed_html .= "<td> $item_bugzilla</td>";
	  $index_completed_html .= "</tr>\n";
	}
      else
	{
	  if (!$item_status)
	    $item_status = "Pending";

	  $index_html .= "<tr><td> $item_category</td>";
	  $index_html .= "<td> <a href='#$count'>$item_summary</a></td>";
	  $index_html .= "<td> $item_bounty</td>";
	  if ($item_status == "Assigned")
	    $index_html .= "<td> <font color=green>$item_status</font></td>";
	  else if ($item_status == "Pending")
	    $index_html .= "<td> <font color=grey>$item_status</font></td>";
	  else
	    $index_html .= "<td> $item_status</td>";

	  $index_html .= "<td> $item_bugzilla</td>";
	  $index_html .= "</tr>\n";
	}

      $text_html = "";
      $item_status = "";
      $item_summary = "";

      $item_html .= ("</p></li>\n");

      $category_items++;

      // Hide completed items
      if ($task_done == 1)
	{
	  $item_html = "";
	  $category_items--;
	}
      $category_html .= $item_html;
      $item_html = "";
    }
  if ($element == 'category')
    {
      $category_html .= $text_html;
      $text_html = "";

      $category_html .= "</ol>\n";

      // Hide empty categories
      if ($category_items > 0)
	$details_html .= $category_html;
      $category_items = 0;
      $category_html = "";
    }
  if ($element == 'summary')
    {
      $flag = '';

      $item_html .= $text_html;
      $item_summary = $text_html;
      $text_html = "";

      $item_html .= $text_html;
      $text_html = "";
      $item_html .= ": </b><br>";
    }
  if ($element == 'comment')
    {
      $flag = '';
      $item_comment = $text_html;
      $text_html = preg_replace("/\n/m", "<br>", $text_html);
      $text_html = preg_replace("/Bounty: \\$(\d+)/im", "<font color=red>Bounty: $\\1</font>", $text_html);
      $text_html = preg_replace("/#(\d+)/", "<a href=\"http://bugzilla.gnome.org/show_bug.cgi?id=\\1\">#\\1</a>", $text_html);
      $item_html .= $text_html;
      $text_html = "";
      // echo ("<!-- done == $task_done -->");
      if ($task_done == "1")
	{
	  $item_html .= "</font></strike>";
	}
      $item_html .= "\n";
    }
}

function character_data($parser, $data) {
  // callback function for character data

  global $flag;
  global $text_html;

  if ($flag == 'summary' || $flag == 'comment')
    {
      $text_html .= $data;
    }
}

$parser = xml_parser_create();
xml_parser_set_option($parser, XML_OPTION_CASE_FOLDING, false);
xml_set_element_handler($parser, 'opening_element', 'closing_element');
xml_set_character_data_handler($parser, 'character_data');

$document = file('svn/TODO.tasks');

foreach ($document as $line) {
  xml_parse($parser, $line);
}

?>
<p>
These are Anjuta development tasks that have been defined and are being undertaken. Many tasks have bounties assigned to prioritize their development and to encourage more contribution to critical tasks in Anjuta. Anyone interested can pick up any tasks and contribute to Anjuta. Before undertaking any task, it is advisable to announce and discuss it first on the anjuta-devel mailing list to ensure that no duplicate work is being done. This also ensures that tasks in progress are marked 'Assigned' so that other people know who are working on them (and can possibly collaborate on the work).
</p>
<p>
In order for a task to be marked as assigned, you must <a href='http://bugzilla.gnome.org/enter_bug.cgi?product=anjuta'>create a Bugzilla entry</a> for tracking its development. Enter the task title as the Bugzilla summary and the task description as the Bugzilla description. If the task has a bounty, please also suffix the Bugzilla summary with 'Bounty: '.
</p>
<p>
Tasks with bounties are little different than other general tasks. Before these can be marked Assigned we need to first make sure that the contributor is indeed suitable for the task. If you are new to Anjuta development, we suggest that you convince the lead developer in the mailing list discussion that you can perform the task. Your contribution or patch is subject to code reviews and quality checks. This means that you will receive feedback and suggestions until the code is acceptable. 
</p>
<?php
echo "<h1>Pending Tasks</h1>\n";
echo "Click on task titles for full descriptions and on Bugzilla ids for development updates.<br>";
echo "<table cellspacing=0 cellpadding=5 border=1 width='100%'>";
echo "<tr><th>Category</th><th>Summary</th><th>Bounty</th><th>Status</th><th>Bugzilla</th>";
echo $index_html;
echo "</table>\n";

echo "<h1>Task Descriptions</h1>\n";
echo $details_html;

echo "<h1>Completed Tasks</h1>\n";
echo "<table cellspacing=0 cellpadding=5 border=1 width='100%'>";
echo "<tr><th>Category</th><th>Summary</th><th>Bounty</th><th>Status</th><th>Bugzilla</th>";
echo $index_completed_html;
echo "</table>\n";

xml_parser_free($parser);

?> 
