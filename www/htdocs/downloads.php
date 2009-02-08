
<h3>Latest unstable Anjuta 2.25.x releases</h3>
	<p>
		Latest unstable release is
		<a href="http://ftp.gnome.org/pub/GNOME/sources/anjuta/2.25" >here</a>.
	
		Anjuta now follows GNOME <a href="http://live.gnome.org/TwoPointTwentyfive">release planning</a> and version numbering scheme. Unstable
		versions have a odd minor version number while stable version
		have a even minor version number.
		We encourage to use it and help us with <a href="http://bugzilla.gnome.org/enter_bug.cgi?product=anjuta">bug reports</a>.
                Some more information can be found at <a
                href="http://live.gnome.org/Anjuta">Anjuta wiki</a>.
        </p>

<h3>Latest stable Anjuta 2.24.x releases (Cyclone)</h3>

        <p>
                Latest stable release is <a
                href="http://ftp.gnome.org/pub/GNOME/sources/anjuta/2.24/anjuta-2.24.2.tar.bz2">
                Anjuta version 2.24.2</a>.
                Please see <a href="http://anjuta.org/features">features</a>
		for some details on this release.
                This is a stable release of 2.24.x (Cyclone) series and is under constant bugfix.
		We encourage to use it and help us with bug reports.
	</p><p>
		Older releases and documentations are available <a
		href="http://ftp.gnome.org/pub/GNOME/sources/anjuta/">here</a>
	</p>
<hr/>
<h3>Anjuta 2.x Dependencies</h3>
        <p> You must have these to install Anjuta 2.x. In case, you are installing binary package,
            please also install corresponding devel package as well.
        </p>
        <p>
                <table border=1 cellspacing=1 cellpadding=4 width="100%">
                        <tr><td nowrap><a href="http://ftp.gnome.org/pub/GNOME/sources/gdl/0.7/">gdl</a></td>
                        <td nowrap>0.7.7 or later</td>
                        <td>GNOME development library</td></tr>

                        <tr><td nowrap><a href="http://ftp.gnome.org/pub/GNOME/sources/gnome-build/0.2/">gnome-build</a></td>
                        <td nowrap>0.2.0 or later</td>
                        <td>GNOME build frame work (not needed anymore for Anjuta > 2.25.0)</td></tr>
                        <tr><td nowrap><a href="http://ftp.gnome.org/pub/GNOME/sources/libgda/3.99/">libgda</a></td>
                        <td nowrap>3.99.7 or later</td>
                        <td>GNOME DB library (need for Anjuta > 2.25.0)</td></tr>

                        <tr><td nowrap><a href="http://sourceforge.net/project/showfiles.php?group_id=3593">libopts</a></td>
                        <td nowrap>23.0.0 or later</td>
                        <td>Command options processing (required by autogen)</td></tr>

                        <tr><td nowrap><a href="ftp://ftp.gnu.org/gnu/guile/">guile</a></td>
                        <td nowrap>1.6.7 or later</td>
                        <td>Scripting engine (required by autogen)</td></tr>

                        <tr><td nowrap><a href="http://sourceforge.net/project/showfiles.php?group_id=3593">autogen</a></td>
                        <td nowrap>5.6.5 or later</td>
                        <td>Template processing engine</td></tr>

                        <tr><td nowrap><a href="http://www.pcre.org/">pcre</a></td>
                        <td nowrap>3.9 or later</td>
		        <td>Perl C regexp library. Most distributions already come with it.</td></tr>
               </table>
        <p>
        You can optionally have these to build extra (and very essential) plugins.
        </p>
        <p>
                <table border=1 cellspacing=1 cellpadding=4>
                        <tr><td nowrap><a href="http://ftp.gnome.org/pub/GNOME/sources/glade3/">glade3</a></td>
                        <td nowrap>3.1.3 or later</td>
                        <td>Next generation glade UI designer</td></tr>
						
                        <tr><td nowrap><a href="http://ftp.gnome.org/pub/GNOME/sources/devhelp/">devhelp</a></td>
                        <td nowrap>0.13 or later</td>
                        <td>Developers help system</td></tr>

                        <tr><td nowrap><a href="http://www.graphviz.org/Download.php">graphviz</a></td>
                        <td nowrap>2.6.0 or later</td>
                        <td>Graph processing library (required for class inheritance diagram plugin)</td></tr>
                        
			<tr><td nowrap><a href="http://subversion.tigris.org/project_packages.html">subversion</a></td>
                        <td nowrap>1.0.2 or later</td>
                        <td>Subversion version control (required for subversion plugin)</td></tr>
                </table>
        <p>
	<p>
<hr/>
<h3>Unofficial ubuntu feisty repository (only i386 and only binaries)</h3>
         <ol>
		<li> Add <b>deb http://anjuta.org/apt ./</b> in your <i>/etc/apt/sources.list</i> </li>
		<li> sudo apt-get update </li>
		<li> sudo apt-get install anjuta<br>
		<li> sudo apt-get install anjuta-dev libgbf-dev libgdl-dev (if you want to write anjuta plugins or report bugs)</li>
        </ol>
<p>
<h3>Ubuntu gusty repository</h3>
         <ol>
                <li> Add <b>deb http://ppa.launchpad.net/robster/ubuntu gutsy main</b> in your <i>/etc/apt/sources.list</i> </li>
                <li> sudo apt-get update </li>
                <li> sudo apt-get install anjuta<br>
                <li> sudo apt-get install anjuta-dev libgbf-dev libgdl-dev (if you want to write anjuta plugins or report bugs)</li>
           </ol>
</p>
<hr/>
	<h3>GNOME Development suite</h3>
	<p>
		Anjuta can be used alone for general development, but to be able to use the
		application wizards and to do GTK+/GNOME related projects,
		it is strongly recommended that you also get the following applications.
		Anjuta IDE works best in conjunction with other GNOME
		development tools (forming a development suite).
	</p><p>
		These are the packages you need to have
		at the least to form the development suite. With these, you should be able
		to develop GTK/GNOME applications in C language.
	</p><p>
		<table border=1 cellspacing=1 cellpadding=4>
			<tr><td nowrap><a href="http://anjuta.org/">Anjuta IDE</a></td>
			<td nowrap><i>current</i></td>
			<td>GNOME Integrated Development Environment.</td></tr>
			<tr><td><a href="http://glade.gnome.org/">Glade</a></td>
			<td nowrap>2.0.0 or later</td>
			<td>GTK/GNOME Graphical User Interface Editor.
			You need this for developing GTK/GNOME applications in Anjuta (for
			C language only).</td></tr>
			<tr><td><a href="http://www.imendio.com/projects/devhelp/">Devhelp</a></td>
			<td nowrap>0.6.0 or later</td>
			<td>Developer's Help system. Required for context
			sensitive API help and search.</td></tr>
		</table>
	<p>
	
	<h3>Application development requirements</h3>
	<p>
		These are required if you
		intend to do additional development (as mentioned alongside the
		packages). Anjuta does not have any restriction on versions of these
		packages, so choose the versions which your application require. Make sure
		to install their corresponding *-devel packages also.
	</p>
	<p>
	<table border=1 cellspacing=1 cellpadding=4>
		<tr><td><a href="http://home.wtal.de/petig/Gtk/">glademm</a></td>
		<td>Glade extention for c++ code generaltion.
		You need this if you want to develop C++ GTK/GNOME applications in Anjuta.
		You also need corresponding gtkmm/gnomemm libraries (see below).</td></tr>
		<tr><td><a href="http://gtkmm.sourceforge.net/">gtkmm</a></td>
		<td>C++ wrapper for GTK. You need this (in addition
		to Glade and glademm) for developing C++ GTK applications.</td></tr>
		<tr><td><a href="http://gtkmm.sourceforge.net/">gnomemm</a></td>
		<td>C++ wrapper for GNOME.You need this (in addition
		to Glade and glademm) for developing C++ GNOME applications.</td></tr>
		<tr><td><a href="http://www.daa.com.au/~james/software/libglade/">libglade</a></td>
		<td>Dynamic GUI loader/creator based on Glade. You
		need this (in addition to Glade) if you intend to create libglade based
		projects in Anjuta. Learn more about this library before you start using
		it (one of the things to learn is to avoid clicking 'Build' button in Glade).
		</td></tr>
		<tr><td><a href="http://www.wxwindows.org">WxWindows</a></td>
		<td>Cross platform GUI toolkit. It uses
		GTK+ for GNU/Linux systems. You need this for developing WxWindows based
		applications.</td></tr>
		<tr><td><a href="http://www.libsdl.org/">SDL</a></td>
		<td>Simple DirectMedia Layer graphics library. Required for
		developing SDL (graphics) applications.</td></tr>
	</table>
	</p>
<p>
<h3>Other known anjuta plugin projects</h3>
<table cellspacing="1" cellpadding="4" border="1">
    <tbody>
        <tr>
            <td nowrap="">
                <a href="http://labs.o-hand.com/anjuta-poky-sdk-plugin/"> Poky
                    SDK Plugin</a>
            </td>
            <td>
                Integrates Anjuta with the SDK toolchain built from the Poky
                Platform Builder allowing a rapid cross-compiled, build,
                deploy, test, debug cycle from within this easy to use
                environment.
            </td>

        </tr>
        <tr>
            <td nowrap="">
                <a href="http://anjuta-maemo.garage.maemo.org/">
                    Maemo SDK Plugin </a>
            </td>
            <td>
Anjuta maemo SDK+ plugin provides Anjuta IDE development environment for maemo based Internet tablets. The plugin generates a working C code template that is easy to extend into a full application. Code can be build, new source files added, and Debian packages generated directly from Anjuta.
            </td>
        </tr>

        <tr>
            <td nowrap="">
                <a href="http://projects.openmoko.org/projects/preity-plugin/">
                    Preity Plugin </a>
            </td>
            <td>
                Plugin for end-users to use, develop and package for OpenMoko.
                (Works only on version Anjuta 2.2)
            </td>

        </tr> 

        <tr>
            <td nowrap="">
                <a href="http://libanjutapython.sourceforge.net">
                    Libanjuta Python bindings </a>
            </td>
            <td>
                Python bindings for Anjuta. It includes a plugin allowing Anjuta
		to load plugins written in Python and a Python debugger plugin
		(using winpdb) written in Python. 
            </td>
        </tr>

    </tbody>
</table>
</p>
