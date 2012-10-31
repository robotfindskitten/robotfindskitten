/*
 *  Copyright (C) 2004-2005 Alexey Toptygin <alexeyt@freeshell.org>
 *  Based on sources by Leonard Richardson and others.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or EXISTENCE OF KITTEN.  See the GNU General
 *  Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include "config.h"

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#elif defined ( HAVE_CURSES_H )
#include <curses.h>
#else
#error "No (n)curses headers found, aborting"
#endif

#define SYSTEM_NKI_DIR "/usr/share/games/robotfindskitten"
#define USER_NKI_DIR ".robotfindskitten"

#define DEFAULT_NUM_BOGUS 20

/* option flags for state.options */
#define OPTION_HAS_COLOR        0x01
#define OPTION_DISPLAY_INTRO    0x02

#define DEFAULT_OPTIONS         (OPTION_DISPLAY_INTRO)

/* bits returned from test() */
#define BROBOT	0x01
#define BKITTEN	0x02
#define BBOGUS	0x04

/* Nethack keycodes */
#define NETHACK_down 'j'
#define NETHACK_DOWN 'J'
#define NETHACK_up 'k'
#define NETHACK_UP 'K'
#define NETHACK_left 'h'
#define NETHACK_LEFT 'H'
#define NETHACK_right 'l'
#define NETHACK_RIGHT 'L'
#define NETHACK_ul 'y'
#define NETHACK_UL 'Y'
#define NETHACK_ur 'u'
#define NETHACK_UR 'U'
#define NETHACK_dl 'b'
#define NETHACK_DL 'B'
#define NETHACK_dr 'n'
#define NETHACK_DR 'N'

/* When numlock is on, the keypad generates numbers */
#define NUMLOCK_UL	'7'
#define NUMLOCK_UP	'8'
#define NUMLOCK_UR	'9'
#define NUMLOCK_LEFT	'4'
#define NUMLOCK_RIGHT	'6'
#define NUMLOCK_DL	'1'
#define NUMLOCK_DOWN	'2'
#define NUMLOCK_DR	'3'

/* EMACS keycodes */
#define CTRL(key)	((key) & 0x1f)
#define EMACS_NEXT CTRL('N')
#define EMACS_PREVIOUS CTRL('P')
#define EMACS_BACKWARD CTRL('B')
#define EMACS_FORWARD CTRL('F')

/* allocate message readin buffer in chunks of this size */
#define MSG_ALLOC_CHUNK 80
/* allocate the messages array in chunks of this size */
#define MSGS_ALLOC_CHUNK 32

/* miscellaneous
 * I'm paranoid about collisions with curses in the KEY_ namespace */
#define MYKEY_REDRAW CTRL('L')
#define MYKEY_q 'q'
#define MYKEY_Q 'Q'

/* size of header area above frame and playing field */
#define HEADSIZE	2

/* thickness of frame - can be 1 or 0, 0 suppreses framing */
#define FRAME   	1

/* magic index of white color pair */
#define WHITE	7

/* special indices in the items array */
#define ROBOT   	0
#define KITTEN  	1
#define BOGUS	2

typedef struct {
	int x;
	int y;
	int color;
	bool bold;
	char character;
} screen_object;

typedef struct {
	int lines;
	int cols;
	unsigned int options;
	unsigned int num_items;
	unsigned int num_messages;
	unsigned int num_messages_alloc;
	screen_object *items;
	char **messages;
} game_state;

/* global state */
static game_state state;

void add_message ( char *msg, size_t len ) {
	char *buff, **nmess;

	if ( state.num_messages_alloc <= state.num_messages ) {
		nmess = calloc ( state.num_messages + MSGS_ALLOC_CHUNK,
			sizeof ( char * ) );
		if ( nmess ) {
			state.num_messages_alloc =
				state.num_messages + MSGS_ALLOC_CHUNK;
			memcpy ( nmess, state.messages, 
				( state.num_messages * sizeof ( char * ) ) );
			free ( state.messages );
			state.messages = nmess;
		} else {
			return; /* fail silently */
		}
	}

	if ( ( buff = malloc ( len ) ) ) {
		strcpy ( buff, msg );
		state.messages[state.num_messages] = buff;
		state.num_messages++;
	}
}

void read_file ( char *fname ) {
	int fd, ret;
	char ch, *buff, *buff2;
	size_t len, alloc;

	len = 0;
	alloc = 0;

	if ( ( fd = open ( fname, O_RDONLY ) ) ) {
		while ( true ) {
			ret = read ( fd, &ch, 1 );
			if ( ret < 0 ) /* an error */
				break;
			if ( alloc <= len ) { /* grow buff */
				buff2 = malloc ( alloc + MSG_ALLOC_CHUNK );
				if ( ! buff2 )
					break;
				memcpy ( buff2, buff, alloc );
				if ( alloc )
					free ( buff );
				buff = buff2;
				alloc = alloc + MSG_ALLOC_CHUNK;
			}
			/* end of line/file */
			if ( ( ret == 0 ) || ( ch == '\n' ) || ( ch == '\r' ) )
			{
				/* ignore blank lines and comments */
				if ( len && ( buff[0] != '#' ) ) {
					buff[len] = '\0';
					add_message ( buff, len + 1 );
				}
				if ( ret == 0 ) /* end of file */
					break;
				len = 0;
			} else {
				buff[len] = ch;
				len++;
			}
		} /* end while ( true ) */
		close ( fd );
	}
	if ( alloc )
		free ( buff );
}

void do_read_messages ( char *dname ) {
	char *fname;
	DIR *dir;
	size_t len, plen;
	struct dirent *dent;
	struct stat sb;

	if ( ! ( dir = opendir ( dname ) ) ) return;
	plen = strlen ( dname );
	while ( ( dent = readdir ( dir ) ) ) {
		len = plen + strlen ( dent->d_name ) + 2;
		if ( ( fname = malloc ( len ) ) ) {
			strcpy ( fname, dname );
			fname[plen] = '/';
			strcpy ( ( fname + plen + 1 ), dent->d_name );
			if ( ! stat ( fname, &sb ) &&
				( sb.st_mode & S_IFREG ) ) {
					read_file ( fname );
			}
			free ( fname );
		}
	}
	closedir ( dir );
}

void read_messages(void) {
	char *temp;
	size_t l;
	unsigned int i;
	extern char** environ;

	state.messages = 0;
	state.num_messages = 0;
	state.num_messages_alloc = 0;

	add_message("You found...yourself?", 21);
	add_message("You found kitten! Way to go, robot!", 35);

	do_read_messages ( SYSTEM_NKI_DIR );
	for ( i = 0; environ[i]; i++ ) {
		if ( ! strncmp ( environ[i], "HOME=", 5 ) ) {
			l = strlen ( environ[i] );
			temp = malloc ( l + strlen ( USER_NKI_DIR ) - 4 );
			if ( temp ) {
				strcpy ( temp, ( environ[i] + 5 ) );
				temp[( l - 5 )] = '/';
				strcpy ( ( temp + l - 4 ), USER_NKI_DIR );
				do_read_messages ( temp );
				free ( temp );
			}
			break;	
		}
	}
	do_read_messages ( "nki" );
}

void randomize_messages(void) {
	char *temp;
	unsigned int i, j;

	for ( i = BOGUS; i < ( state.num_messages - 1 ); i++ ) {
		j = i + ( random() % ( state.num_messages - i ) );
		if ( i != j ) {
			temp = state.messages[i];
			state.messages[i] = state.messages[j];
			state.messages[j] = temp;
		}
	}
}
 
/* convenient macros */
#define randx() ( FRAME + ( random() % ( state.cols - FRAME * 2 ) ) )
#define randy() ( HEADSIZE + FRAME + (random() % ( state.lines - HEADSIZE - FRAME * 2 ) ) )
#define randch() ( random() % ( '~' - '!' + 1 ) + '!' )
#define randbold() ( ( random() % 2 ) ? true : false )
#define randcolor() ( random() % 6 + 1 )

inline char randchar(void) {
	char ch;
	do { ch = randch(); } while ( ch == '#' );
	return ch;
}

inline int objcmp ( const screen_object a, const screen_object b ) {
	if ( a.x > b.x ) {
		return 1;
	} else if ( a.x < b.x ) {
		return -1;
	} else if ( a.y > b.y ) {
		return 1;
	} else if ( a.y < b.y ) {
		return -1;
	} else {
		return 0;
	}
}

/* returns a combination of robot kitten and bogus bits, based on what is 
	at the coordinates; if bogus bit is set, bnum will be set to the 
	index of the bogus object in the state.bogus array.
	more than one bit should never be returned, but shit happens */
/* I can't believe this is how I wrote it; but it makes so much sense */
unsigned int test ( int y, int x, unsigned int *bnum ) {
	int low, high, mid; /* have to be signed due to algorithm */
	screen_object s;
	unsigned int result;

	result = 0;
	s.x = x;
	s.y = y;
	if ( ! objcmp ( state.items[ROBOT], s ) )
		result |= BROBOT;
	if ( ! objcmp ( state.items[KITTEN], s ) )
		result |= BKITTEN;

	/* search items array */
	low = 0;
	high = state.num_items - 1;
	while ( low <= high ) {
		mid = ( low + high ) / 2;
		switch ( objcmp ( s, state.items[mid] ) ) {
		case 0:
			*bnum = mid;
			return ( result | BBOGUS );
			break;
		case 1:
			low = mid + 1;
			break;
		case -1:
			high = mid - 1;
			break;
		default:
			abort(); /* poop pants */
			break;
		}
	}
	return result;
}

void finish ( int sig ) {
	endwin();
	exit ( sig );
}

void init ( unsigned int num ) {
	screen_object tmpobj;
	unsigned int i, j, temp;

	/* allocate memory */
	if ( ! ( state.items = calloc ( num, sizeof ( screen_object ) ) ) ) {
		fprintf ( stderr, "Cannot malloc.\n" );
		exit ( EXIT_FAILURE );
	}

	/* install exit handler */
	signal ( SIGINT, finish );

	/* set up (n)curses */
	initscr();
	nonl();
	noecho();
	cbreak();
	intrflush ( stdscr, false );
	keypad ( stdscr, true );

	state.lines = LINES;
	state.cols = COLS;
	if ( ( ( state.lines - HEADSIZE - FRAME ) * state.cols ) < ( num + 2 ) ) {
		endwin();
		fprintf ( stderr, "Screen too small to fit all objects!\n" );
		exit ( EXIT_FAILURE );
	}

	/* set up robot */
	state.items[ROBOT].character = '#';
	state.items[ROBOT].bold = false; /* we are a timid robot */
	state.items[ROBOT].y = randy();
	state.items[ROBOT].x = randx();

	/* set up kitten */
	state.items[KITTEN].character = randchar();
	state.items[KITTEN].bold = randbold();
	do {
		state.items[KITTEN].y = randy();
		state.items[KITTEN].x = randx();
	} while ( ! objcmp ( state.items[ROBOT], state.items[KITTEN] ) );

	/* set up items */
	for ( i = BOGUS; i < BOGUS + num; i++ ) {
		state.items[i].character = randchar();
		state.items[i].bold = randbold();
		while ( true ) {
			state.items[i].y = randy();
			state.items[i].x = randx();
			if ( ! objcmp ( state.items[ROBOT], state.items[i] ) )
				continue;
			if ( ! objcmp ( state.items[KITTEN], state.items[i] ) )
				continue;
			for ( j = 0; j < i; j++ ) {
				if ( ! objcmp ( state.items[j], 
					state.items[i] ) ) break;
			}
			if ( j == i ) break;
		}
	}
	state.num_items = BOGUS + num;

	/* now sort the NKIs */ /* bubbly! */
	for ( i = BOGUS; i < state.num_items - 1; i++ ) {
		temp = i;
		for ( j = i; j < state.num_items; j++ ) {
			if ( objcmp ( state.items[j], state.items[temp] ) < 0 )
				temp = j;
		}
		if ( temp != i ) {
			tmpobj = state.items[i];
			state.items[i] = state.items[temp];
			state.items[temp] = tmpobj;
		}
	}

	/* set up colors */
	start_color();
	if ( has_colors() && ( COLOR_PAIRS > 7 ) ) {
		state.options |= OPTION_HAS_COLOR;
		init_pair ( 1, COLOR_GREEN, COLOR_BLACK );
		init_pair ( 2, COLOR_RED, COLOR_BLACK );
		init_pair ( 3, COLOR_YELLOW, COLOR_BLACK );
		init_pair ( 4, COLOR_BLUE, COLOR_BLACK );
		init_pair ( 5, COLOR_MAGENTA, COLOR_BLACK );
		init_pair ( 6, COLOR_CYAN, COLOR_BLACK );
		init_pair ( 7, COLOR_WHITE, COLOR_BLACK );
		bkgd ( COLOR_PAIR(WHITE) );

		state.items[ROBOT].color = WHITE;
		state.items[KITTEN].color = randcolor();
		for ( i = BOGUS; i < state.num_items; i++ ) {
			state.items[i].color = randcolor();
		}
	} else {
		state.options &= ~ OPTION_HAS_COLOR;
	}
}

void draw ( const screen_object *o ) {
	attr_t new;

	if ( state.options & OPTION_HAS_COLOR ) {
		new = COLOR_PAIR(o->color);
		if ( o->bold ) { new |= A_BOLD; }
		attrset ( new );
	}
	addch ( o->character );
}

void message ( char *message, int color ) {
	int y, x;

	getyx ( curscr, y, x );
	if ( state.options & OPTION_HAS_COLOR ) {
		attrset ( COLOR_PAIR(WHITE) );
	}
	move ( 1, 0 );
	clrtoeol();

	if ( state.options & OPTION_HAS_COLOR ) {
		attrset ( COLOR_PAIR(WHITE) );
	}

	move ( 1, 0 );
	printw ( "%.*s", state.cols, message );
	move ( y, x );
	refresh();
}

void draw_screen() {
	unsigned int i;

	if ( state.options & OPTION_HAS_COLOR )
		attrset ( COLOR_PAIR(WHITE) );
	clear();
#if FRAME > 0
	mvaddch(HEADSIZE, 0,      ACS_ULCORNER);
	mvaddch(HEADSIZE, COLS-1, ACS_URCORNER);
	mvaddch(LINES-1,  0,      ACS_LLCORNER);
	mvaddch(LINES-1,  COLS-1, ACS_LRCORNER);
	for (i = 1; i < COLS - 1; i++) {
	    mvaddch(HEADSIZE,  i, ACS_HLINE);
	    mvaddch(LINES - 1, i, ACS_HLINE);
	}
	for (i = FRAME + HEADSIZE; i < LINES - 1; i++) {
	    mvaddch(i, 0,      ACS_VLINE);
	    mvaddch(i, COLS-1, ACS_VLINE);
	}
#else
	for (i = 0; i < COLS; i++) {
	    mvaddch(HEADSIZE,  i, ACS_HLINE);
	}
#endif
	move ( 0, 0 );
	printw ( "robotfindskitten %s\n\n", PACKAGE_VERSION );
	for ( i = 0; i < state.num_items; i++ ) {
		move ( state.items[i].y, state.items[i].x );
		draw ( &state.items[i] );
	}
	move ( state.items[ROBOT].y, state.items[ROBOT].x );
	if ( state.options & OPTION_HAS_COLOR )
		attrset ( COLOR_PAIR(WHITE) );
	refresh();
}

/* captures control if the terminal shrank too much, otherwise does a refresh */
void handle_resize(void) {
    state.lines = LINES;
    state.cols = COLS;
    draw_screen();
}

void instructions(void) {
	clear();
	move ( 0, 0 );
	printw ( "robotfindskitten %s\n", PACKAGE_VERSION );
	printw ( 
"By the illustrious Leonard Richardson (C) 1997, 2000\n"\
"Written originally for the Nerth Pork robotfindskitten contest\n\n"\
"In this game, you are robot (#). Your job is to find kitten. This task\n"\
"is complicated by the existence of various things which are not kitten.\n"\
"Robot must touch items to determine if they are kitten or not. The game\n"\
"ends when robotfindskitten. Alternatively, you may end the game by hitting\n"
"the q key or a good old-fashioned Ctrl-C.\n\n"\
"See the documentation for more information.\n\n"\
"Press any key to start.\n"
	);
	refresh();
	if ( getch() == KEY_RESIZE )
		handle_resize();
	clear();
}

void play_animation ( bool fromright ) {
	int i, animation_meet;
	char kitty;

	if ( state.options & OPTION_HAS_COLOR )
		attrset ( COLOR_PAIR(WHITE) );
	move ( 1, 0 );
	clrtoeol();

	animation_meet = state.cols / 2;

	kitty = state.items[KITTEN].character;
	for ( i = 4; i > 0; i-- ) {
		state.items[ROBOT].character = ' ';
		state.items[KITTEN].character = ' ';

		move ( state.items[ROBOT].y, state.items[ROBOT].x );
		draw ( &state.items[ROBOT] );
		move ( state.items[KITTEN].y, state.items[KITTEN].x );
		draw ( &state.items[KITTEN] );

		state.items[ROBOT].character = '#';
		state.items[KITTEN].character = kitty;
		state.items[ROBOT].y = 1;
		state.items[KITTEN].y = 1;
		if ( fromright ) {
			state.items[ROBOT].x = animation_meet + i;
			state.items[KITTEN].x = animation_meet - i + 1;
		} else {
			state.items[ROBOT].x = animation_meet - i + 1;
			state.items[KITTEN].x = animation_meet + i;
		}

		move ( state.items[ROBOT].y, state.items[ROBOT].x );
		draw ( &state.items[ROBOT] );
		move ( state.items[KITTEN].y, state.items[KITTEN].x );
		draw ( &state.items[KITTEN] );
		move ( state.items[ROBOT].y, state.items[ROBOT].x );
		refresh();
		sleep ( 1 );
	}
	message ( "You found kitten! Way to go, robot!", WHITE );
}

void main_loop(void) {
	int ch, x, y;
	unsigned int bnum; 
	bool fromright;

	while ( ( ch = getch() ) ) {
		y = state.items[ROBOT].y;
		x = state.items[ROBOT].x;
		fromright = false;
		switch ( ch ) {
			case NETHACK_UL:
			case NETHACK_ul:
			case NUMLOCK_UL:
			case KEY_A1:
			case KEY_HOME:
				y--; x--; fromright = true; break;
			case EMACS_PREVIOUS:
			case NETHACK_UP:
			case NETHACK_up:
			case NUMLOCK_UP:
			case KEY_UP:
				/* fromright: special case */
				y--; fromright = true; break;
			case NETHACK_UR:
			case NETHACK_ur:
			case NUMLOCK_UR:
			case KEY_A3:
			case KEY_PPAGE:
				y--; x++; break;
			case EMACS_BACKWARD:
			case NETHACK_LEFT:
			case NETHACK_left:
			case NUMLOCK_LEFT:
			case KEY_LEFT:
				x--; fromright = true; break;
			case EMACS_FORWARD:
			case NETHACK_RIGHT:
			case NETHACK_right:
			case NUMLOCK_RIGHT:
			case KEY_RIGHT:
				x++; break;
			case NETHACK_DL:
			case NETHACK_dl:
			case NUMLOCK_DL:
			case KEY_C1:
			case KEY_END:
				y++; x--; fromright = true; break;
			case EMACS_NEXT:
			case NETHACK_DOWN:
			case NETHACK_down:
			case NUMLOCK_DOWN:
			case KEY_DOWN:
				y++; break;
			case NETHACK_DR:
			case NETHACK_dr:
			case NUMLOCK_DR:
			case KEY_C3:
			case KEY_NPAGE:
				y++; x++; break;
	                case MYKEY_Q:
	                case MYKEY_q:
	                        finish ( EXIT_FAILURE );
	                        break;
			case MYKEY_REDRAW:
				draw_screen();
				break;
			case KEY_RESIZE:
				handle_resize();
				break;
	                default:
	                        message ( "Invalid input: Use direction keys"\
					" or q.", WHITE );
	                        break;
		}

		/* it's the edge of the world as we know it... */
		if ( ( y < HEADSIZE + FRAME ) || ( y >= state.lines - FRAME ) ||
			( x < FRAME ) || ( x >= state.cols - FRAME) )
				continue;

		/* let's see where we've landed */
		switch ( test ( y, x, &bnum ) ) {
			case 0:
				/* robot moved */
				state.items[ROBOT].character = ' ';
				draw ( &state.items[ROBOT] );
				state.items[ROBOT].y = y;
				state.items[ROBOT].x = x;
				state.items[ROBOT].character = '#';
				move ( y, x );
				draw ( &state.items[ROBOT] );
				move ( y, x );
				refresh();
			case BROBOT:
				/* nothing happened */
				break;
			case BKITTEN:
				play_animation ( fromright );
				finish ( 0 );
				break;
			case BBOGUS:
				message ( state.messages[bnum], 
					state.items[bnum].color );
				break;
			default:
				message ( "Well, that was unexpected...", WHITE );
				break;
		}
	}
}

int main ( int argc, char **argv ) {
    int option, seed = time ( 0 ), nbogus = DEFAULT_NUM_BOGUS;

	while ((option = getopt(argc, argv, "n:s:V")) != -1) {
	    switch (option) {
	    case 'n':
		nbogus = atoi ( optarg );
		if ( nbogus <= 0 ) {
			fprintf ( stderr, "Argument must be positive.\n" );
			exit ( EXIT_FAILURE );
		}
		break;
	    case 's':
		seed = atoi(optarg);
		break;
	    case 'V':
		(void)printf("robotfindskitten: %s\n", PACKAGE_VERSION);
		exit(EXIT_SUCCESS);
	    case 'h':
	    case '?':
	    default:
		(void)printf("usage: %s [-n nitems] [-s seed] [-V]\n", argv[0]);
		exit(EXIT_SUCCESS);
	    }
	}

	state.options = DEFAULT_OPTIONS;
	srandom ( seed );
	read_messages();

	if (state.num_messages == 0) {
		fprintf ( stderr, "No NKIs found.\n" );
		exit ( EXIT_FAILURE );
	}

	randomize_messages();

	if ( nbogus > state.num_messages ) {
		fprintf ( stderr, "More NKIs used then messages available.\n" );
		exit ( EXIT_FAILURE );
	} else {
		init ( nbogus );
	}

	instructions();
	draw_screen();
	main_loop();
	finish ( EXIT_SUCCESS );
	return EXIT_SUCCESS;
}

/* end */
