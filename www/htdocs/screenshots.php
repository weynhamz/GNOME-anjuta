<h2>Anjuta DevStudio Screenshots</h2>
<table border=0 cellspacing=20 cellpadding=0>
<tbody>
		<?php
		$ss_database = "screenshots/screenshots.database";
		if (!($fp = fopen($ss_database, "r"))) {
			die("could not open Screenshots Database");
		}
		$xml_ss = xml_parser_create();
		$ss_data = fread($fp,filesize ($ss_database));
		xml_parser_set_option($xml_ss,XML_OPTION_CASE_FOLDING,0);
		xml_parse_into_struct($xml_ss,$ss_data,$vals,$index);
		echo "<tr>\n";
		$wrap_table = 'no';
		foreach ($vals as $key) {
			$desc = $key[attributes];
			if ($key[type] == 'complete') {
				echo "<td>\n";
				echo "<a href='$desc[big_pic_file]' target='_self'><img border='0' src='$desc[small_pic_file]'></a>\n";
				echo "<br>$desc[comment]</td>\n";
				if ($wrap_table == 'yes') {
					echo "</tr></tr>\n";
					$wrap_table = 'no';
				} else {
					$wrap_table = 'yes';
				}
			}
		}
		echo "</tr>\n";
		xml_parser_free($xml_ss);
		?>
</tbody></table>
