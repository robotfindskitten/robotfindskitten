execgamesdir= $(prefix)/games
execgames_PROGRAMS=robotfindskitten

AM_CPPFLAGS = -DSYSTEM_NKI_DIR=\"$(datadir)/games/robotfindskitten\"

robotfindskitten_SOURCES= messages.h robotfindskitten.c 
#robotfindskitten_LDADD= -lncurses 

# Test the code in place
test:
	src/robotfindskitten

# Run a static type check on the code
cppcheck: robotfindskitten.c
	cppcheck -I .. --template gcc --enable=all robotfindskitten.c
