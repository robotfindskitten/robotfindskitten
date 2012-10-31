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

typedef struct {
	int x;
	int y;
	int color;
	bool bold;
	char character;
} screen_object;

typedef struct {
	screen_object robot;
	screen_object kitten;
	int lines;
	int cols;
	unsigned int options;
	unsigned int num_bogus;
	unsigned int num_messages;
	unsigned int num_messages_alloc;
	screen_object *bogus;
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

	for ( i = 0; i < ( state.num_messages - 1 ); i++ ) {
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
	if ( ! objcmp ( state.robot, s ) )
		result |= BROBOT;
	if ( ! objcmp ( state.kitten, s ) )
		result |= BKITTEN;

	/* search bogus array */
	low = 0;
	high = state.num_bogus - 1;
	while ( low <= high ) {
		mid = ( low + high ) / 2;
		switch ( objcmp ( s, state.bogus[mid] ) ) {
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
	if ( ! ( state.bogus = calloc ( num, sizeof ( screen_object ) ) ) ) {
		fprintf ( stderr, "Cannot malloc.\n" );
		exit ( 1 );
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
		exit ( 1 );
	}

	/* set up robot */
	state.robot.character = '#';
	state.robot.bold = 0; /* we are a timid robot */
	state.robot.y = randy();
	state.robot.x = randx();

	/* set up kitten */
	state.kitten.character = randchar();
	state.kitten.bold = randbold();
	do {
		state.kitten.y = randy();
		state.kitten.x = randx();
	} while ( ! objcmp ( state.robot, state.kitten ) );

	/* set up bogus */
	for ( i = 0; i < num; i++ ) {
		state.bogus[i].character = randchar();
		state.bogus[i].bold = randbold();
		while ( true ) {
			state.bogus[i].y = randy();
			state.bogus[i].x = randx();
			if ( ! objcmp ( state.robot, state.bogus[i] ) )
				continue;
			if ( ! objcmp ( state.kitten, state.bogus[i] ) )
				continue;
			for ( j = 0; j < i; j++ ) {
				if ( ! objcmp ( state.bogus[j], 
					state.bogus[i] ) ) break;
			}
			if ( j == i ) break;
		}
	}
	state.num_bogus = num;

	/* now sort the bogus items */ /* bubbly! */
	for ( i = 0; i < state.num_bogus - 1; i++ ) {
		temp = i;
		for ( j = i; j < state.num_bogus; j++ ) {
			if ( objcmp ( state.bogus[j], state.bogus[temp] ) < 0 )
				temp = j;
		}
		if ( temp != i ) {
			tmpobj = state.bogus[i];
			state.bogus[i] = state.bogus[temp];
			state.bogus[temp] = tmpobj;
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
		bkgd ( COLOR_PAIR(7) );

		state.robot.color = 7;
		state.kitten.color = randcolor();
		for ( i = 0; i < state.num_bogus; i++ ) {
			state.bogus[i].color = randcolor();
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
		attrset ( COLOR_PAIR(7) );
	}
	move ( 1, 0 );
	clrtoeol();

	if ( state.options & OPTION_HAS_COLOR ) {
		attrset ( COLOR_PAIR(7) );
	}

	move ( 1, 0 );
	printw ( "%.*s", state.cols, message );
	move ( y, x );
	refresh();
}

void draw_screen() {
	unsigned int i;

	if ( state.options & OPTION_HAS_COLOR )
		attrset ( COLOR_PAIR(7) );
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
#endif
	move ( 0, 0 );
	printw ( "robotfindskitten %s\n\n", PACKAGE_VERSION );
	move ( state.kitten.y, state.kitten.x );
	draw ( &state.kitten );
	for ( i = 0; i < state.num_bogus; i++ ) {
		move ( state.bogus[i].y, state.bogus[i].x );
		draw ( &state.bogus[i] );
	}
	move ( state.robot.y, state.robot.x );
	draw ( &state.robot );
	move ( state.robot.y, state.robot.x );
	refresh();
}

/* captures control if the terminal shrank too much, otherwise does a refresh */
void handle_resize(void) {
	int ch;

	while ( ( LINES < state.lines ) || ( COLS < state.cols ) ) {
		ch = MYKEY_REDRAW;
		do {
			if ( ch == KEY_RESIZE ) {
				break;
			} else if ( ch == MYKEY_REDRAW ) {
				clear();
				if ( state.options & OPTION_HAS_COLOR )
					attrset ( COLOR_PAIR(7) );
				move ( 0, 0 );
				printw ( "Stop it, you're crushing me!" );
				refresh();
			}
		} while  ( ( ch = getch() ) );
				
	}
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

void play_animation ( unsigned int fromright ) {
	int i, animation_meet;
	char kitty;

	if ( state.options & OPTION_HAS_COLOR )
		attrset ( COLOR_PAIR(7) );
	move ( 1, 0 );
	clrtoeol();

	animation_meet = state.cols / 2;

	kitty = state.kitten.character;
	for ( i = 4; i > 0; i-- ) {
		state.robot.character = ' ';
		state.kitten.character = ' ';

		move ( state.robot.y, state.robot.x );
		draw ( &state.robot );
		move ( state.kitten.y, state.kitten.x );
		draw ( &state.kitten );

		state.robot.character = '#';
		state.kitten.character = kitty;
		state.robot.y = 1;
		state.kitten.y = 1;
		if ( fromright ) {
			state.robot.x = animation_meet + i;
			state.kitten.x = animation_meet - i + 1;
		} else {
			state.robot.x = animation_meet - i + 1;
			state.kitten.x = animation_meet + i;
		}

		move ( state.robot.y, state.robot.x );
		draw ( &state.robot );
		move ( state.kitten.y, state.kitten.x );
		draw ( &state.kitten );
		move ( state.robot.y, state.robot.x );
		refresh();
		sleep ( 1 );
	}
	message ( "You found kitten! Way to go, robot!", 7 );
}

void main_loop(void) {
	int ch, x, y;
	unsigned int bnum, fromright;

	while ( ( ch = getch() ) ) {
		y = state.robot.y;
		x = state.robot.x;
		fromright = 0;
		switch ( ch ) {
			case NETHACK_UL:
			case NETHACK_ul:
			case NUMLOCK_UL:
			case KEY_A1:
			case KEY_HOME:
				y--; x--; fromright = 1; break;
			case EMACS_PREVIOUS:
			case NETHACK_UP:
			case NETHACK_up:
			case NUMLOCK_UP:
			case KEY_UP:
				/* fromright: special case */
				y--; fromright = 1; break;
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
				x--; fromright = 1; break;
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
				y++; x--; fromright = 1; break;
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
	                        finish ( 1 );
	                        break;
			case MYKEY_REDRAW:
				draw_screen();
				break;
			case KEY_RESIZE:
				handle_resize();
				break;
	                default:
	                        message ( "Invalid input: Use direction keys"\
					" or q.", 7 );
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
				state.robot.character = ' ';
				draw ( &state.robot );
				state.robot.y = y;
				state.robot.x = x;
				state.robot.character = '#';
				move ( y, x );
				draw ( &state.robot );
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
					state.bogus[bnum].color );
				break;
			default:
				message ( "Well, that was unexpected...", 7 );
				break;
		}
	}
}

int main ( int argc, char **argv ) {
	int nbogus;

	state.options = DEFAULT_OPTIONS;
	srandom ( time ( 0 ) );
	read_messages();

	if (state.num_messages == 0) {
		fprintf ( stderr, "No NKIs found\n" );
		exit ( 1 );
	}

	randomize_messages();

	if ( argc > 1 ) {
		nbogus = atoi ( argv[1] );
		if ( nbogus <= 0 ) {
			fprintf ( stderr, "Argument must be positive\n" );
			exit ( 1 );
		}
	} else {
		nbogus = DEFAULT_NUM_BOGUS;
	}
	if ( nbogus > state.num_messages ) {
		fprintf ( stderr, "More NKIs used then messages available\n" );
		exit ( 1 );
	} else {
		init ( nbogus );
	}

	instructions();
	draw_screen();
	main_loop();
	finish ( 0 );
	return 0;
}

/* end */
