static char* ver = "v1600003.201b";
/*
 *robotfindskitten: A Zen simulation
 *
 *Copyright (C) 1997,2000 Leonard Richardson 
 *                        leonardr@segfault.org
 *                        http://www.crummy.com/devel/
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or EXISTANCE OF KITTEN.  See the GNU General
 *   Public License for more details.
 *
 *   http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <curses.h>
#include <signal.h>

/*The messages go in a separate file because they are collectively
  huge, and you might want to modify them. It would be nice to load
  the messages from a text file at run time.*/
#include "messages.h"

/*Constants for our internal representation of the screen.*/
#define EMPTY -1
#define ROBOT 0
#define KITTEN 1

/*Keycode constants*/
#define DOWN_KEY 2
#define UP_KEY 3
#define LEFT_KEY 4
#define RIGHT_KEY 5
#define UP_LEFT_KEY 6
#define UP_RIGHT_KEY 83
#define DOWN_LEFT_KEY 104
#define DOWN_RIGHT_KEY 82

#define QUIT_KEY 27

/*Screen dimensions.*/
#define X_MIN 0
#define X_MAX 79
#define Y_MIN 3
#define Y_MAX 23

/*Macros for generating numbers in different ranges*/
#define randx() rand() % X_MAX+1
#define randy() rand() % (Y_MAX-Y_MIN+1)+Y_MIN /*I'm feeling randy()!*/
#define randchar() rand() % (254-'!'+1)+'!';
#define randcolor() rand() % 6 + 1
#define randbold() (rand() % 2 ? TRUE:FALSE)

/*Row constants for the animation*/
#define ADV_ROW 1
#define ANIMATION_MEET 50

/*This struct contains all the information we need to display an object
  on the screen*/
typedef struct
{
  int x;
  int y;
  int color;
  bool bold;
  char character;
} screen_object;

/*
 *Function definitions
 */

/*Initialization and setup functions*/
void initialize_ncurses();
void initialize_arrays();
void initialize_robot();
void initialize_kitten();
void initialize_bogus();
void initialize_screen();
void instructions();
static void finish(int sig);

/*Game functions*/
void play_game();
void process_input(char);

/*Helper functions*/
int validchar(char);

void play_animation(char);

/*Global variables. Bite me, it's fun.*/
screen_object robot;
screen_object kitten;

int num_bogus;
screen_object bogus[MESSAGES];
int bogus_messages[MESSAGES];
int used_messages[MESSAGES];

/* This array contains our internal representation of the screen. The
 array is bigger than it needs to be, as we don't need to keep track
 of the first few rows of the screen. But that requires making an
 offset function and using that everywhere. So not right now. */
int screen[X_MAX+1][Y_MAX+1];

#include "draw.h"

/******************************************************************************
 *
 * Begin meaty routines that do the dirty work.
 *
 *****************************************************************************/
int main(int argc, char *argv[])
{
  /*
   *Do general start-of-program stuff.
   */
  
  /*Get a number of non-kitten objects.*/
  if (argc == 1)
    {
      num_bogus = 20;
    } else {
      num_bogus = atoi(argv[1]);
      if (num_bogus < 0 || num_bogus > MESSAGES)
	{
	  printf("Run-time parameter must be between 0 and %d.\n",MESSAGES);
	  exit(0);
	}
    }

  /*Initialize the random number generator*/
  srand(time(0));

  /* Set up the screen to use the IBM character set. ncurses still won't
   cooperate with characters before '!', so we take care of that in the
   randchar() macro. */
   printf("%c%c%c",27,'(','U');

  initialize_arrays();

  /*
   *Now we initialize the various game objects.
   */
  initialize_robot();
  initialize_kitten();
  initialize_bogus();

  initialize_ncurses();  
  instructions();  

  initialize_screen();
  play_game();
}

/*
 *play_game waits in a loop getting input and sending it to process_input
 */
void play_game()
{
  int old_x = robot.x;
  int old_y = robot.y;
  char input;

  input = getch();
  while (input != QUIT_KEY)
    {
      process_input(input);
      
      /*Redraw robot, where applicable. We're your station, robot.*/
      if (!(old_x == robot.x && old_y == robot.y))
	{
	  /*Get rid of the old robot*/
	  mvaddch(old_y,old_x, ' ');
	  screen[old_x][old_y] = EMPTY;
	  
	  /*Meet the new robot, same as the old robot.*/
	  draw(robot);
	  refresh();
	  screen[robot.x][robot.y] = ROBOT;
	  
	  old_x = robot.x;
	  old_y = robot.y;
	}
      input = getch();
    } 
  message("Bye!");
  refresh();
  finish(0);
}

/*
 *Given the keyboard input, process_input interprets it in terms of moving,
 *touching objects, etc.
 */
void process_input(char input)
{
  int check_x = robot.x;
  int check_y = robot.y;

  switch ((int)input)
    {
    case UP_KEY:
      check_y--;
      break;
    case UP_LEFT_KEY:
      check_x--;
      check_y--;
      break;
    case UP_RIGHT_KEY:
      check_x++;
      check_y--;
      break;
    case DOWN_KEY:
      check_y++;
      break;
    case DOWN_LEFT_KEY:
      check_x--;
      check_y++;
      break;
    case DOWN_RIGHT_KEY:
      check_x++;
      check_y++;
      break;
    case LEFT_KEY:
      check_x--;
      break;
    case RIGHT_KEY:
      check_x++;
      break;
    case 0:
      break;
    default: /*Bad command or filename.*/
      message("Invalid input: Use direction keys or Esc.");
      return;
    }
  
  /*Check for going off the edge of the screen.*/
  if (check_y < Y_MIN || check_y > Y_MAX || check_x < X_MIN || check_x > X_MAX)
    {
      return; /*Do nothing.*/
    }
  
  /*Check for collision*/
  if (screen[check_x][check_y] != EMPTY)
    {
      switch (screen[check_x][check_y])
	{
	case ROBOT:
				/*We didn't move, or we're stuck in a
				  time warp or something.*/
	  break;
	case KITTEN: /*Found it!*/
	  move(1,0);
	  clrtoeol();
	  play_animation(input);
	  break;
	default: /*We hit a bogus object; print its message.*/
	  message(messages[bogus_messages[screen[check_x][check_y]-2]]);
	  break;
	}
      return;
    }

  /*Otherwise, move the robot.*/
  robot.x = check_x;
  robot.y = check_y;
}

/*finish is called upon signal or progam exit*/
void finish(int sig)
{
    endwin();
    printf("%c%c%c",27,'(','B'); /* Restore normal character set */
    exit(0);
}

/******************************************************************************
 *
 * Begin helper routines
 *
 *****************************************************************************/

int validchar(char a)
{
  switch(a)
    {
    case '#':
    case ' ':   
    case 127:
      return 0;
    }
  return 1;
}

void play_animation(char input)
{
  int counter;
  /*The grand cinema scene.*/
  for (counter = 4; counter >0; counter--)
    {
      /*Move the object on the right.*/
      mvaddch(ADV_ROW,ANIMATION_MEET+counter+1,' ');
      move(ADV_ROW,ANIMATION_MEET+counter);
      if (input == RIGHT_KEY || input == DOWN_KEY 
	  || input == DOWN_RIGHT_KEY  || input == UP_RIGHT_KEY)
	draw_in_place(kitten);
      else
	draw_in_place(robot);
      
      /*Move the object on the left.*/
      mvaddch(ADV_ROW,ANIMATION_MEET-counter,' ');
      move(ADV_ROW,ANIMATION_MEET-counter+1);
      if (input == RIGHT_KEY || input == DOWN_KEY
	  || input == DOWN_RIGHT_KEY  || input == UP_RIGHT_KEY)
	draw_in_place(robot);
      else
	draw_in_place(kitten);
      refresh();
      sleep (1);
    }

  move(1,0);
  addstr("You found kitten! Way to go, robot!");
  refresh();
  finish(0);
}

/******************************************************************************
 *
 * Begin initialization routines (called before play begins).
 *
 *****************************************************************************/

void instructions()
{
  char dummy;

  mvprintw(0,0,"robotfindskitten %s\n",ver);
  printw("By the illustrious Leonard Richardson (C) 1997, 2000\n");
  printw("Written originally for the Nerth Pork robotfindskitten contest\n\n");
  printw("In this game, you are robot (");
  draw_in_place(robot);
  printw("). Your job is to find kitten. This task\n"); 
  printw("is complicated by the existance of various things which are not kitten.\n");  
  printw("Robot must touch items to determine if they are kitten or not. The game\n");
  printw("ends when robotfindskitten. Alternatively, you may end the game by hitting\n");
  printw("the Esc key. See the documentation for more information.\n\n");
  printw("Press any key to start.\n");
  refresh();

  dummy = getch();
  clear();
}

void initialize_arrays()
{
  int counter, counter2;

  /*Initialize the empty object.*/
  screen_object empty;
  empty.x = -1;
  empty.y = -1;
  empty.color = 0;
  empty.bold = FALSE;
  empty.character = ' ';
  
  for (counter = 0; counter <= X_MAX; counter++)
    {
      for (counter2 = 0; counter2 <= Y_MAX; counter2++)
	{
	  screen[counter][counter2] = EMPTY;
	}
    }
  
  /*Initialize the other arrays.*/
  for (counter = 0; counter < MESSAGES; counter++)
    {
      used_messages[counter] = 0;
      bogus_messages[counter] = 0;
      bogus[counter] = empty;
    }
}

/*initialize_ncurses sets up ncurses for action. Much of this code 
 stolen from Raymond and Ben-Halim, "Writing Programs with NCURSES"*/
void initialize_ncurses()
{
  (void) signal(SIGINT, finish);
  (void) initscr();      /* initialize the curses library */
  keypad(stdscr, TRUE);  /* enable keyboard mapping */
  (void) nonl();         /* tell curses not to do NL->CR/NL on output */
  intrflush(stdscr, FALSE);
  (void) noecho;        /* don't echo characters */
  (void) cbreak;         /* don't wait for enter before accepting input */
    
  if (has_colors())
    {
      start_color();      
      init_pair(COLOR_BLACK, COLOR_BLACK, COLOR_BLACK);
      init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
      init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
      init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
      init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
      init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
      init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
      init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    }

}

/*initialize_robot initializes robot.*/
void initialize_robot()
{
  /*Assign a position to the player.*/
  robot.x = randx();
  robot.y = randy();

  robot.character = '#';
  robot.color = 0;
  robot.bold = FALSE;
  screen[robot.x][robot.y] = ROBOT;
}

/*initialize kitten, well, initializes kitten.*/
void initialize_kitten()
{
  /*Assign the kitten a unique position.*/
  do
    {
      kitten.x = randx();
      kitten.y = randy();
    } while (screen[kitten.x][kitten.y] != EMPTY);
  
  /*Assign the kitten a character and a color.*/
  do {
    kitten.character = randchar();
  } while (!(validchar(kitten.character))); 
  screen[kitten.x][kitten.y] = KITTEN;

  kitten.color = randcolor();
  kitten.bold = randbold();
}

/*initialize_bogus initializes all non-kitten objects to be used in this run.*/
void initialize_bogus()
{
  int counter, index;
  for (counter = 0; counter < num_bogus; counter++)
    {
      /*Give it a color.*/
      bogus[counter].color = randcolor();
      bogus[counter].bold = randbold();
      
      /*Give it a character.*/
      do {
	bogus[counter].character = randchar();
      } while (!(validchar(bogus[counter].character))); 
      
      /*Give it a position.*/
      do
	{
	  bogus[counter].x = randx();
	  bogus[counter].y = randy();
	} while (screen[bogus[counter].x][bogus[counter].y] != EMPTY);

      screen[bogus[counter].x][bogus[counter].y] = counter+2;
      
      /*Find a message for this object.*/
      do {
	index = rand() % MESSAGES;
      } while (used_messages[index] != 0);
      
      bogus_messages[counter] = index;
      used_messages[index] = 1;
    }

}

/*initialize_screen paints the screen.*/
void initialize_screen()
{
  int counter;

  /*
   *Print the status portion of the screen.
   */
  mvprintw(0,0,"robotfindskitten %s\n\n",ver);  
  
  /*Draw a line across the screen.*/
  for (counter = X_MIN; counter <= X_MAX; counter++)
    {
      printw("%c",196);
    }

  /*
   *Draw all the objects on the playing field.
   */
  for (counter = 0; counter < num_bogus; counter++)
    {
      draw(bogus[counter]);
    }

  draw(kitten);
  draw(robot);

  refresh();

}
