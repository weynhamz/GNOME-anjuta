<h1>The scoop on Anjuta DevStudio</h1>
<p>
Anjuta DevStudio has been designed to be simple to operate yet powerful enough to fullfil all your programming needs. Many features have evolved since the early days and several very attractive ones added. Our focus is on delivering power and usability at the same time without overloading your senses and making your software development a pleasurable process. We will always be busy getting you one of the best IDE experiences by straighting up all the neat features and stablizing it. We hope you will have a good time using Anjuta. If not, please help us make it better by reporting bugs and suggestions. Here are some of the things you will find in Anjuta.
</p>

<h2>User Interface</h2>
<p>
<a href="screenshots/anjuta-2.1/anjuta-2.1.2-1.png"><img style="border: 0; padding: 10px" src="screenshots/anjuta-2.1/anjuta-2.1.2-1-thumb.png" align="right"></a>
Anjuta has a very flexible and advanced docking system that allows you to layout all views in whatever way you like. You can drag and drop the views using drag bars and re-arrange the layout. The layouts are persistent for each project so you can maintain different layouts required by projects. All dock views are minimizable to avoid clutter in main window. Minimized views appear as icons on left side of main window. All menu actions are configurable either by type-in-the-menu (the usual GNOME way) or by a dedicated shortcuts configuration UI.
</p>
<h2>Plugins</h2>
<p>
Anjuta is very extensible with plugins. Almost all features in Anjuta are implemented using plugins which can be dynamicaly enabled or disabled. You can select which plugins to be active for your projects and the default mode (no-project mode). Like UI layout, plugins selection are also persistent for each project making it easy to work on different levels of project complexities.
</p>
<p>
Anjuta also has a highly extensible plugin framework for those who want to extend it with their own features. Plugins can be developed and installed independent of main Anjuta development. Anjuta being written in C, the plugin framework and the API are all in C. However, c++ and python bindings are under active development. In not so distant future, it would be possible to write c++ and python plugins for Anjuta.
</p>
<p>
All plugins in Anjuta are easily substitutable with different plugins implementing the same features. This allows, for example, to have multiple editors to choose from (so far, we have scintilla and gtksourceview editors) or implement a new one (vim/emacs anyone?) suiting your taste. This applies to any plugin. If Anjuta finds that there are multiple plugins satisfying the same feature requirement, it will prompt the user to select one and remember it for rest of the time. It is posible to 'forget' this selection by clearing a gconf key (sometime soon we will have a preferences UI to make it easier).
</p>

<h2>File Manager</h2>
<p>
Integrated file manager plugin behaves more or less like your typical file manager in a tree view. It lists all directories and files in current project (or a pre-configured directory if there is no project open). It is possible to not list hidden files and/or files that are ignored by version control system. A custom regexp can also be set in file manager preferences to hide additional files.
</p>
<p>
Default action (double-click) on any of the files would open it, either within Anjuta if there is a plugin capable of handling it or with external application configured in your standard desktop. The file can also be opened with other applications/plugins from the context menu which lists all plugins and applications that are able to open it.
</p>
<p>
In addintion, File Manager context menu also lists actions associated with other plugins, such as build actions (associated with build system plugin), CVS/Subversion actions (associated version control system plugins) and project actions (associated with Project Manager plugin). This allows to conveniently perform all actions from within the File Manager.
</p>

<h2>Project Manager</h2>
<p>
Anjuta has a very powerful Project Manager plugin which can open pretty much any automake/autoconf based project on the planet. It might fail on some oddly configured projects, but as long as the project is done by using automake/autoconf properly it should work.
</p>
<p>
The neat thing is that it does not store any project information beyond what is already available in project struture. That is, there is no separate project data maintained by Anjuta and all project processing are done directly within the project structure. This allows the project to be maintained or developed outside Anjuta without having to, so called, 'porting' or 'switching' to a new platform. Since technically Anjuta projects are just automake projects, mixed development of it (Anjuta and non-Anjuta users) or switching back and forth between Anjuta and other tools is quite possible without any hindrance.
</p>
<p>
Project Manager lists the project in standard automake heirarchy organized into groups of targets. Groups corresponds to directories in your project and targets corresponds to normal automake targets (not to be confused with make targets). Project manager view actually has two parts; one part (lower one) shows complete project heirarchy and the other part (upper one) lists important targets directly. Important targets constitute executable and library targets -- making them extremely accessible in the project view. This is particularly useful in big projects where the heirarchy could be deep and hard to navigate from the tree alone. Targets are in turn composed of source files.
</p>
<p>
Each of the project groups and targets is configurable in standard automake way. You can set complier and linker flags directly for each target, or set configure variables. Groups allow setting installation destinations for the targets.
</p>
<p>
Just like file manager, project manager view also has convenience actions for the source files and targets accessible from the context menu.
</p>

<h2>Project wizards</h2>
<p>
The project wizard plugin uses a powerful template processing engine called <a href="http://autogen.sourceforge.net/">autogen</a>. All new projects are created from templates that are written in autogen syntax. Project wizard lets you create new projects from a selection of project templates. The selection includes simple generic, flat (no subdirectory), gtk+, gnome, java, python projects and more. New templates can be easily downloaded and installed since they are just a collection of text files.
</p>

<h2>Source Code Editor</h2>
<p>
There are two editors currently available in Anjuta; scintilla based (classic) editor and the new gtksourceview based editor. Except for some minor differences, both are equally good and can be used interchangably. Depending on your taste of editing, either of them can be choosen. Some of the editor features are:
</p>

<ul>
    <li> Syntax highlighting: Supports syntax highlighting for almost all common programing languages. Syntax highlighting for new languages can be easily added by added proprties file, lexer parser (for scintilla editor) or lang files (for sourceview editor).</li>
    
    <li> Smart Indentation: Code automatically indentated as you type based on the language and your indentation settings. Currently only available for C and C++. For the rest, it only does basic indentation.</li>
    
    <li> Autoindentation: Indent current line or a selection of lines as per your indentation settings as if they were re-typed taking into account Smart Indentation.</li>
    
    <li> Automatic code formatting (only C and C++): Source code reformatting using 'indent' program. Full range of indent options avaiable.</li>
    
    <li> Code folding/hiding: Fold code blocks, functions, balanced texts to hide them in heirachial order. Unfold to unhide them.</li>
    
    <li> Line numbers/markers display: Left margins to show line numbers, markers and folding points.</li>
    
    <li> Text zooming: Zoom texts using scrollwheel or menu.</li>
    
    <li> Code autocompletion: Automatic code completion for known symbols. Type ahead suggestions to choose for completion.</li>
    
    <li> Calltips for function prototypes: Provides helpful tips for typing function calls with their prototype and arguments hint.</li>
    
    <li> Indentation guides: Guides to make it easier to see indentation levels.</li>
    
    <li> Bookmarks: Set or unset bookmarks and navigation.</li>
    
    <li> Multiple split views: Multiple views for the same file (split inside the same editor). Useful when typing in a file while referencing the same file at another location or copy-pasting within the same file to/from different locations without having to scroll back and forth.</li>
    
    <li> Incremental Search: Search as you type in the search field for instant search. Useful when you want to avoid typing the full search when the first few characters are enough to reach the point.</li>
    
	<li> Powerful search and replace: Supports string and regexp search expressions, search in files, search in project etc.</li>

    <li> Jump to line number: Instant jump to line number.</li>
    
    <li> Build messages highlight: Error/warning/information messages are indicated in the editor with helpful (and appropriately colored) underlines. Very useful when you are strolling throught the file correcting all build errors without having to use build outputs to navigate indivitual errors.</li>

    <li> Tabs reordering: Reorder editor tabs as per your convenience.</li>
    
    <li> Notifications for files modified outside Anjuta while they are open.</li>
</ul>
   
<h2>Symbols view and navigation</h2>
<p>
The symbol browser plugin shows all symbols in your project organized into their types. There are tree views in symbol browser plugin; one showing the global symbol tree, another showing symbols in current file and the third one providing a view to search the symbols. All the symbols can be navigated to their respective definitions and declarations. Symbols in current file can also be navigated quickly from toolbar.
</p>
<p>
When anjuta is started for first time, it also indexes symbols from all installed libraries for autocompletion and calltips. This provides instant reference to library functions used in the project. The libraries that should be referenced can be selected from symbol browser preferences.
</p>
<h2>Integrated Debugger</h2>
<p>
Anjuta provides a full source level debugger (currently backed by gdb, but will have other debugger backends soon). The debugger provides everything that can be expected from a typical source debugger such as breakpoints, watches, source navigations, stack trace, threads, disassembly view, registers, local variables, memory dumps etc. You can also set up breakpoints and watches without first having the debugger running. They are saved in session so the next debugging session would still have them.
</p>
<p>
Program execution under the debugger can be performed in single step, step over, step out, continued execution, run to cursor, pause program or attach to process etc. All programs in the project can be started in termial and can be provided arguments. Programs linking shared libraries within the project are also started correctly using libtool to ensure non-installed libraries are picked up rather than installed ones.
</p>

<h2>Integrated glade user interface designer</h2>
<p>
<a href="screenshots/anjuta-2.1/anjuta-2.1.2-9.png"><img style="border: 0; padding: 10px" src="screenshots/anjuta-2.1/anjuta-2.1.2-9-thumb.png" align="right"></a>
<a href="http://glade.gnome.org/">Glade</a> is the GTK+/GNOME wysiwyg graphical user interface designer which lets you create UIs (dialogs and windows) for your application visualy. Glade files can be directly edited within Anjuta. When a glade file is opened or created, glade plugin is started and brings up the designer view, palettes, properties editor and widgets view. The project can have any number of glade files and can be conveniently opened simultaneous (however, only one can be edited at a time).
</p>
<h2>Class generator and file wizard</h2>
<p>
With class generator plugin, you can create c++ and GObject classes easily and add them to your projects. Similarly, file wizard can create templates for your new source files.
</p>

<h2>Valgrind plugin and gprof profiler plugin</h2>
<p>
Integrated valgrind plugin can be used to profile programs for memory leaks and corruptions.
</p>

<h2>Integrated devhelp API help browser</h2>
<p>
<a href="screenshots/anjuta-2.1/anjuta-2.1.2-8.png"><img style="border: 0; padding: 10px" src="screenshots/anjuta-2.1/anjuta-2.1.2-8-thumb.png" align="right"></a>
<a href="http://developer.imendio.com/projects/devhelp">Devhelp</a> is the GTK+/GNOME developer's help browser. It is very conveniently integrated into Anjuta to give instant API help. Press Shift+F1 and you get the API documentation of the current symbol in editor. Make sure you have enabled Devhelp plugin for the project to use it. You can browse all the installed help documents from the tree view or search for symbols in the search view.
</p>
<!--
    * Bookmarks: Navigate through all the bookmarks in the file.
    
    * Instant symbol navigation to its definition or declaration, ether by
    using shortcut keys, or symbol views.
    
    * Local symbol views: A seperate views for local symbols of current
    file available both in symbol view and toolbar.
    
    * Navigate past navigation histories.
    
    * Navigate error messages.

3. Interactive source-level debugger (built over gdb).
    * Interactive execution.
    * Breakpoints/watches/signal/stack manipulation.
    * ... and much more.

4. Built-in application wizards to create terminal/GTK/GNOME applications on-the-fly. 


6. Full project and build files management. 

6. Bookmark management.

9. Support for other languages 
    * Java, Perl, Pascal, etc. (only file mode, no project management).

10. Interactive messaging system.

-->
(to be updated more)
