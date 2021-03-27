SUBDIRS = src doc nki share
EXTRA_DIST = BUGS
ACLOCAL_AMFLAGS = -I m4

# Run the built code
run:
	src/robotfindskitten

# Count NKIs. Useful for generating the minor part of a release number
count:
	@wc -l nki/*.nki

# Make a ship.
ship:
	@echo ''
	@echo '          _!'
	@echo '     _  _| %\_'
	@echo '~~~~~\o______/~~~~~'
	@echo ''

