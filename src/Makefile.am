execgamesdir= $(prefix)/games
execgames_PROGRAMS=robotfindskitten

AM_CPPFLAGS = -DSYSTEM_NKI_DIR=\"$(datadir)/games/robotfindskitten\"

robotfindskitten_SOURCES= robotfindskitten.c 
#robotfindskitten_LDADD= -lncurses 

# Run a static type check on the code
cppcheck: robotfindskitten.c
	cppcheck -I .. --template gcc --enable=all robotfindskitten.c

SPLINT_SUPPRESSIONS = +charintliteral +charindex
splint:
	splint +quiet +posixlib $(SPLINT_SUPPRESSIONS) -I .. *.c
