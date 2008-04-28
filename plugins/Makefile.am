
## The '.' is to make sure install-data-hook rule is executed first
SUBDIRS = . \
	gtodo \
	message-view \
	editor \
	gdb \
	debug-manager \
	file-manager \
	terminal \
	build-basic-autotools \
	file-loader \
	file-wizard \
	project-wizard \
	project-manager \
	sample1 \
	cvs-plugin \
	macro \
	class-gen \
	patch \
	subversion \
	project-import \
	tools \
	class-inheritance \
	document-manager \
	search \
	valgrind \
	language-support-cpp-java \
	glade \
	devhelp \
	profiler \
	language-manager \
	symbol-db \
	symbol-browser \
	sourceview \
	run-program

# indent

## This a temporary measure to insure anjuta does not crash due to old plugin.
## Make sure there are no old plugins left
install-data-hook:
	-rm -fr $(libdir)/anjuta/*

