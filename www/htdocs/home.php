	<p>
		<?php
			error_reporting(E_ERROR);

			# check the return value of fetch_rss()

			$rss = fetch_rss("http://blogs.gnome.org/anjuta");

			if ( $rss ) {
				echo "<ul>";
				foreach ($rss->items as $item) {
					$href = $item['link'];
					$title = $item['title'];
					echo "<li><a href=$href>$title</a></li>";
				}
				echo "</ul>";
			}
			else {
				echo "Could not fetch news!" .
					 "<br>Error Message: "	. magpie_error();
			}
		?>
	</p>
	<h3>Introduction</h3>
		<p>
		Anjuta is a versatile Integrated Development Environment (IDE)
		for C and C++ on GNU/Linux.  It has been written for GTK/GNOME
		and features a number of advanced programming facilities
		including project management, application wizards, an
		interactive debugger and a powerful source editor
		with source browsing and syntax highlighting.
		</p><p>
		Anjuta is an effort to marry the flexibility and power of text-based
		command-line tools with the ease of use of the GNOME graphical user
		interface.  That is why it has been made as user-friendly as possible.
		</p><p>
		Any sort of suggestions or patches for Anjuta are most welcome.
		</p><p>
		  Anjuta is licensed under the GNU GPL.
		  Please read the file COPYING that comes with the distribution for details.
		</p>
<!--
	<h3>Copyright (C) <a href="http://naba.co.in/">Naba Kumar</a></h3>
		<p>
		  This program is free software; you can redistribute it and/or modify it under the terms of
		  the GNU General Public License as published by the Free Software Foundation, either
		  version 2 of the License, or (at your discretion) any later version.
		</p>
		<p>
		  This program is distributed in the hope that it will be useful, but WITHOUT ANY
		  WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
		  PARTICULAR PURPOSE. See the GNU General Public License for more details.
		<p></p>
		  You should have received a copy of the GNU General Public License along with this program;
		  if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
		  Boston, MA 02110-1301 USA
		</p>
-->
	<h3>Help wanted!</h3>
		<p>
		We welcome help from
		<ul>
			<li>software developers (new code, bug fixes)
			<li>editors (manuals, articles)
			<li>artists (icons, images, splash screens)
		</ul>
		Please go to the mailing list section to join or subscribe to the
		development mailing list to start contributing.
		</p>
