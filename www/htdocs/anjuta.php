
<html>
<head>
	<title>Anjuta Integrated Development Environment</title>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
	<meta content="programming, best, ide, free, user friendly, gnome, linux, anjuta, c, c++, php, versatile, gtk, website" name="keywords"/>
        <link href="/css/docs.css" type="text/css" rel="StyleSheet"/>
        <link href="/css/layout.css" rel="stylesheet" type="text/css" media="screen">
        <link href="/css/style.css" rel="stylesheet" type="text/css" media="all">
</head>

<body>
  <!-- site header -->
  <div id="page">
    <ul id="general">
      <li id="siteaction-gnome_home" class="home">
        <a href="http://anjuta.org/" title="Home">Home</a>
      </li>
      <li id="siteaction-gnome_news">
        <a href="http://sourceforge.net/news/?group_id=14222" title="News">News</a>
      </li>
      <li id="siteaction-gnome_projects">
        <a href="http://sourceforge.net/projects/anjuta" title="Project Site">Project Site</a>
      </li>
      <li id="siteaction-gnome_art">
        <a href="http://sourceforge.net/project/showfiles.php?group_id=14222" title="Releases">Releases</a>

      </li>
      <li id="siteaction-gnome_support">
        <a href="http://sourceforge.net/mail/?group_id=14222" title="Mailing lists">Mailing lists</a>
      </li>
      <li id="siteaction-gnome_development">
        <a href="http://sourceforge.net/forum/?group_id=14222" title="Forums">Forums</a>
      </li>
    </ul>

    <div id="header">
       <h1>Anjuta DevStudio: GNOME Integrated Development Environment</h1>
       <div id="tabs">
	<ul id="portal-globalnav">
		<li <?php if($_GET["page"] == "") echo("class=\"selected\"");?>><a href="/"><span>About</span></a></li>
		<li <?php if($_GET["page"] == "development") echo("class=\"selected\"");?>><a href="/development"><span>Development</span></a></li>
		<li <?php if($_GET["page"] == "downloads") echo("class=\"selected\"");?>><a href="/downloads"><span>Downloads</span></a></li>
		<li <?php if($_GET["page"] == "screenshots" || $_GET["page"] == "screen-shots" ) echo("class=\"selected\"");?>><a href="/screen-shots"><span>Screenshots</span></a></li>
		<li <?php if($_GET["page"] == "features") echo("class=\"selected\"");?>><a href="/features"><span>Features</span></a></li>
		<li <?php if($_GET["page"] == "credits") echo("class=\"selected\"");?>><a href="/credits"><span>Credits</span></a></li>
	</ul>
     </div> <!-- end of #tabs -->
    </div> <!-- end of #header -->
  </div> <!-- end of #page -->

<!-- end site header -->

<div id="sidebar">
	<h3>Project</h3>
	<ul>
		<li><a href="/changelog">ChangeLog</a></li>
		<li><a href="/roadmap">Roadmap</a></li>
		<li><a href="/hacking">Hacking</a></li>
		<li><a href="/tasks">Tasks & Bounties</a></li>
	</ul>
	<h3>Documentations</h3>
	<ul>
                <li><a href="/documentations/subpage/documents/libanjuta/index.html">Anjuta API docs</a></li>
		<li><a href="http://live.gnome.org/Anjuta">Anjuta wiki</a></li>
		<li><a href="/documentations/subpage/documents/C/anjuta-faqs/anjuta-faqs.html">FAQ [English]</a></li>
		<!--<li><a href="/documentations/subpage/documents/ja/anjuta-faqs/index.html">FAQ [Japanese]</a></li>
		<li><a href="/documentations/subpage/documents/C/anjuta-tutorial/index.html">Tutorial [English]</a></li>
		<li><a href="/documentations/subpage/documents/de/anjuta-tutorial/index.html">Tutorial [German]</a></li>
		<li><a href="/documentations/subpage/documents/zh_CN/anjuta-tutorial/index.html">Tutorial [Chinese]</a></li> -->
		<li><a href="/documentations/subpage/documents/C/anjuta-build-tutorial/index.html">Build Tutorial [English]</a></li> 
		<li><a href="/documentations/subpage/documents/C/anjuta-manual/anjuta-manual.html">Manual [English]</a></li>
		<!--<li><a href="/documentations/subpage/documents/ja/anjuta-manual/index.html">Manual [Japanese]</a></li> -->
	</ul>
	<h3>Project Status</h3>
        <ul>
        <li><a href="http://bugzilla.gnome.org/browse.cgi?product=anjuta">Bugs</a></li>
        <li><a href="http://bugzilla.gnome.org/reports/patch-report.cgi?product=anjuta">Patches</a></li>
        <li><a href="http://bugzilla.gnome.org/simple-bug-guide.cgi?product=anjuta">Submit</a></li>
        </ul>
        <p>
	<a href="http://sourceforge.net/donate/index.php?group_id=14222"><img src="http://images.sourceforge.net/images/project-support.jpg" width="88" height="32" border="0" alt="Support This Project" /></a>
        </p>
        <p>
        <A href="http://sourceforge.net"><IMG
           src="http://sourceforge.net/sflogo.php?group_id=14222&amp;type=5"
           width="210" height="62" border="0" alt="SourceForge.net Logo"/></A>
        </p>
</div> <!-- sidebar -->

<div id="body">
<div id="content">
	<!-- Content starts here -->
	<?php
		$page = $_GET["page"];
		if ($page == "credits"){
			require("authors.php");
		} else
		if ($page == "development"){
			require("development.php");
		} else
		if ($page == "downloads"){
			require("downloads.php");
		} else
		if ($page == 'screenshots' || $page == 'screen-shots'){
			require("screenshots.php");
		} else
		if ($page == 'features'){
			require("features.php");
		} else
		if ($page == 'changelog'){
			require("changelog.php");
		} else
		if ($page == 'roadmap'){
			require("roadmap.php");
		} else
		if ($page == 'hacking'){
			require("hacking.php");
		} else
		if ($page == 'tasks'){
			require("tasks.php");
                } else
                if ($page == 'documentations'){
		  $subpage = $_GET["subpage"];
		  require("proxy.php");
		} else
		{
			require("home.php");
		}
	?>
</div> <!-- content -->
</div> <!-- body -->

</body>
</html>
