SUBDIRS=src doc nki
EXTRA_DIST=BUGS
ACLOCAL_AMFLAGS=-I m4

# Run the built code
run:
	src/robotfindskitten

# Count NKIs. Useful for generating the minor part of a release number
count:
	@wc -l nki/*.nki

# This has the project location on SourceForge wired in.
# Furthermore it will only work if your SF ID is your local username.
# It just does the POSIX release; others have to be uploaded by hand.
ship:
	rsync -avP -e ssh \
		robotfindskitten-$(PACKAGE_VERSION).tar.gz \
		${LOGNAME},rfk@frs.sourceforge.net:'/home/frs/project/r/rf/rfk/robotfindskitten-POSIX/@TAGLINE@/robotfindskitten-$(PACKAGE_VERSION).tar.gz'

