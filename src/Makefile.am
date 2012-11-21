execgamesdir= $(prefix)/games
execgames_PROGRAMS=robotfindskitten

AM_CPPFLAGS = -DSYSTEM_NKI_DIR=\"$(datadir)/games/robotfindskitten\"

robotfindskitten_SOURCES= messages.h robotfindskitten.c 
#robotfindskitten_LDADD= -lncurses 

cppcheck: robotfindskitten.c
	cppcheck --template gcc --enable=all robotfindskitten.c #--inline-suppr
