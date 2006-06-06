
## The '.' is to make sure install-data-hook rule is executed first
SUBDIRS = . \
	gtodo \
	message-view \
	editor \
	gdb \
	debug-manager \
	devhelp \
	file-manager \
	terminal \
	build-basic-autotools \
	file-loader \
	profile-default \
	file-wizard \
	project-wizard \
	project-manager \
	symbol-browser \
	sample1 \
	cvs-plugin \
	macro \
	class-gen \
	patch \
	subversion \
	project-import \
	glade \
	tools \
	class-inheritance \
	document-manager \
	search \
	valgrind \
	sourceview \
	language-support-cpp-java

## This a temporary measure to insure anjuta does not crash due to old plugin.
## Make sure there are no old plugins left
install-data-hook:
	-rm -fr $(libdir)/anjuta/*
