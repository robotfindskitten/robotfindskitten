/**************************************************************************
 * robotfindskitten: A Zen simulation (ported to PalmOS)
 * Copyright (C) 2001 Bridget Spitznagel
 *                    i0lanthe@robotfindskitten.org
 *
 * Original robotfindskitten is
 * Copyright (C) 1997,2000 Leonard Richardson 
 *                         leonardr@segfault.org
 * 
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License as
 *    published by the Free Software Foundation; either version 2 of
 *    the License, or (at your option) any later version.
 * 
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or EXISTENCE OF KITTEN.  See the GNU General
 *    Public License for more details.
 * 
 *    http://www.gnu.org/copyleft/gpl.html
 *
 **************************************************************************/

#ifndef I_AM_COLOR
#include <Pilot.h>
#else /* I_AM_COLOR */
#include <PalmOS.h>
#include <PalmCompatibility.h>
#endif /* I_AM_COLOR */
#include "robotfindskittenRsc.h"
#include "constants.h"

// this is so that we can include an *unaltered* messages.h:
#define char Char
#include "messages.h"

#ifndef I_AM_COLOR
Boolean color = false;
#else /* I_AM_COLOR */
Boolean color = true;
#endif /* I_AM_COLOR */

screen_object robot;
screen_object kitten;

Short num_bogus = 20;

// 4*99 + 1*99 + 1*201 = 696 B
screen_object bogus[MAX_BOGUS];  // Representations of non-kittem items
bogus_message bogus_messages[MAX_BOGUS]; // Has an index into messages[]
Boolean used_messages[MESSAGES]; // Which of messages[] are used as bogus_msgs
Short MESSAGES_real;
// Might also take some NKI messages from a memo.
#include "rfk-memo.h"

// Our internal representation of the screen.
// This is 300 B bringing us to about 1K in globals: Within acceptable limits.
UChar screen[X_MAX+1][Y_MAX+1]; /* EMPTY, R, K, or an index into the bogus[].
				   Note that MAX_BOGUS+3 < 255. */

// The routine that moves robot will set 'found' if kitten is found
// (after playing a little "animation").  The HuntForm event handler
// will check 'found' and modify its behavior to restart game.
Boolean found = false;
// The routine that moves robot will set this if the robot encounters
// a non-kitten thing, then it will pop up a message form.
// This is how it tells the message form what message to print:
Short atthing;

struct rfkPreferenceType my_prefs = {
  20,     // default number of screen objects
  true,   // relative move
  true,   // sound on
  true,   // hardware buttons on
  false,  // append memo (don't replace with it)
  true,   // objects are color if you've got it
  true    // messages are color if you've got it
};

// remember the last hardware or graffiti key pressed (0 if last action nonkey)
Word last_key = 0;

/*****************************************************************************
 *                      the Hunt form                                        *
 *****************************************************************************/

#ifdef I_AM_COLOR
/* Note - these most probably need work. */
// 00, 33, 66, 99, cc, ff
#define randRGB() ((SysRandom(0) % 6) * 51)
IndexedColorType random_fg_color()
{
  RGBColorType c;
  //  Short i = SysRandom(0) % 3;
  // Try to pick something with decent contrast..
  c.r = (SysRandom(0) % 3) * 51;  // 00, 33, 66
  c.g = (SysRandom(0) % 2) * 51;  // 00, 33, 66
  c.b = (SysRandom(0) % 3) * 51;  // 00, 33, 66
  if (c.b >= c.g && c.b >= c.r) c.b += (SysRandom(0) % 3) * 51;
  else if (c.g >= c.r) c.g += (SysRandom(0) % 3) * 51;
  else c.r += (SysRandom(0) % 3) * 51;
  return WinRGBToIndex(&c);
}
IndexedColorType random_bg_color()
{
  RGBColorType my_tmp_color;
  my_tmp_color.r = my_tmp_color.g = my_tmp_color.b = 255;
  if (0 == (SysRandom(0) % 10))
    my_tmp_color.r -= 51;
  if (0 == (SysRandom(0) % 10))
    my_tmp_color.g -= 51;
  if (0 == (SysRandom(0) % 10))
    my_tmp_color.b -= 51;
  return WinRGBToIndex(&my_tmp_color);
}
#endif /* I_AM_COLOR */


// if color, the caller promises to save and restore screen state
void draw(screen_object o)
{
  Short row = o.y, col = o.x;
  UChar ch = o.character;
  Short cheat;
    
  // calculate pixel position of "row, col" and put char there
  cheat = cell_W - FntCharWidth(ch); // center the variable width characters

  if (cheat <= 1)   cheat = 0;
  else              cheat /= 2;

#ifdef I_AM_COLOR
  if (color && my_prefs.color_nki) {
    WinSetTextColor(o.fg_color);
    //  WinSetBackColor(o.bg_color); // It's possible.  But kind of uglified.
  }
#endif /* I_AM_COLOR */

  WinDrawChars(&ch, 1, col * cell_W + cheat, (row+1) * cell_H + 2);

}


/*initialize_screen draws all the objects on the playing field.*/
// it's safe to call this more than once.
void initialize_screen()
{
  Short counter;
  WinDrawChars("robotfindskitten", 16, 0, 0);
  WinDrawGrayLine(0, 11, 160, 11);

#ifdef I_AM_COLOR
  if (color) {
    WinPushDrawState();
  }
#endif /* I_AM_COLOR */

  for (counter = 0; counter < num_bogus; counter++)
    draw(bogus[counter]);
  draw(kitten);
  draw(robot);

#ifdef I_AM_COLOR
  if (color) {
    WinPopDrawState();
  }
#endif /* I_AM_COLOR */

}

// Erase robot's old location and redraw at new location
// maybe this will save some tails getting erased
// Robot is always black on white so we don't care about color here.
void redraw_robot(UChar x0, UChar y0, UChar x1, UChar y1)
{
  Short cheat = (cell_W - robot_W) / 2;
  RectangleType r;
  RctSetRectangle(&r, x0 * cell_W + cheat, ((y0+1) * cell_H + 2) + 2,
		  robot_W, robot_H); /* Left, Top, W, H */
  WinEraseRectangle(&r, 0); /* 0 for square corners */
  WinDrawChars(&robot.character, 1, x1 * cell_W + cheat, (y1+1) * cell_H + 2);
}

// note: if you are on POSE, at least on my computer which could stand an
// upgrade, this is mindbogglingly slow.  just fine on the real thing though.
void play_animation()
{
  Short rx, kx, rcheat, kcheat, moves = 3;
  RectangleType r;
  UChar c = HEARTCHAR; // heart, I hope.
  // robot starts at the right, kitten starts at the left.
  // we have 90 pixels of width
  rx = X_MAX;
  kx = X_MAX - (1 + 2*moves); // there's only room for 3 moves.
  rcheat = (cell_W - robot_W) / 2;
  kcheat = cell_W - FntCharWidth(kitten.character);
  kcheat = ((kcheat <= 1) ? 0 : kcheat/2);
  // draw them at initial position..
  WinDrawChars(&robot.character,  1, rx * cell_W + rcheat, Y_MIN*cell_H);
  WinDrawChars(&kitten.character, 1, kx * cell_W + kcheat, Y_MIN*cell_H);
  while (moves-- > 0) {
    // wait for a second or two
    SysTaskDelay(SysTicksPerSecond());
    // erase
    RctSetRectangle(&r, rx * cell_W + rcheat, Y_MIN*cell_H, cell_W, cell_H);
    WinEraseRectangle(&r, 0);
    RctSetRectangle(&r, kx * cell_W + kcheat, Y_MIN*cell_H, cell_W, cell_H);
    WinEraseRectangle(&r, 0);
    // redraw
    rx--; kx++;
    WinDrawChars(&robot.character,  1, rx * cell_W + rcheat, Y_MIN*cell_H);
    WinDrawChars(&kitten.character, 1, kx * cell_W + kcheat, Y_MIN*cell_H);
  }
  SysTaskDelay(SysTicksPerSecond());
  RctSetRectangle(&r, 0, 12, 160, 160-12);
  WinEraseRectangle(&r, 0);
  // draw nice red heart
#ifdef I_AM_COLOR
  if (color) {
    IndexedColorType my_red;
    RGBColorType my_tmp_color;
    WinPushDrawState();
    my_tmp_color.r = 255;
    my_tmp_color.g = 0;
    my_tmp_color.b = 0;
    my_red = WinRGBToIndex(&my_tmp_color);
    WinSetTextColor(my_red);
  }
#endif /* I_AM_COLOR */
  WinDrawChars(&c, 1, kx*cell_W + cell_W/2, (Y_MIN+1)*cell_H + 2);  
#ifdef I_AM_COLOR
  if (color) {
    WinPopDrawState();
  }
#endif /* I_AM_COLOR */
  SysTaskDelay(SysTicksPerSecond());
  WinEraseRectangle(&r, 0);
  WinDrawChars("play again?", 11, 56, (160-12)/2); // approximately centered
  return;
}

// move robot..
static Short delta_x[9] = { 0, -1, 0,  0, 1, -1,  1, -1, 1 };
static Short delta_y[9] = { 0,  0, 1, -1, 0, -1, -1,  1, 1 };
void do_feep(Long frq, UInt dur);
void move_me(Short dir)
{
  Short x, y;
  if (dir < 1 || dir > 8) return; // should not happen

  x = robot.x + delta_x[dir];
  y = robot.y + delta_y[dir];

  /*Check for going off the edge of the screen.*/
  if (y < Y_MIN || y > Y_MAX || x < X_MIN || x > X_MAX)
    return; /*Do nothing.*/
  
  /*Check for collision*/
  if (screen[x][y] != EMPTY) {
    switch (screen[x][y]) {
    case ROBOT:
      /*We didn't move, or we're stuck in a time warp or something.*/
      do_feep(200,9);
      return;
    case KITTEN: /*Found it!*/
      do_feep(400,9);
      play_animation();
      found = true; // so the HuntForm will change its handling behavior
      break;
    default: /*We hit a bogus object; print its message.*/
      do_feep(100,9);
      atthing = screen[x][y] - STARTBOGUS;
      // message(messages[bogus_messages[screen[x][y]-2]]);
      FrmPopupForm(ThingForm);
      break;
    }
    return;
  }

  /*Otherwise, move the robot.*/
  do_feep(200,9);
  redraw_robot(robot.x, robot.y, x, y);
  screen[robot.x][robot.y] = EMPTY;
  robot.x = x;
  robot.y = y;
  screen[robot.x][robot.y] = ROBOT;
}

Boolean do_keyboard(Char c)
{
  Short dir = DIR_NONE;
  switch(c) {
  case 'h': dir = DIR_W ; break;
  case 'j': dir = DIR_S ; break;
  case 'k': dir = DIR_N ; break;
  case 'l': dir = DIR_E ; break;
  case 'y': dir = DIR_NW; break;
  case 'u': dir = DIR_NE; break;
  case 'b': dir = DIR_SW; break;
  case 'n': dir = DIR_SE; break;
  default: return false;
  }
  move_me(dir);
  return true;
}

void LeaveForm()
{
   FormPtr frm;
   frm = FrmGetActiveForm();
   FrmEraseForm (frm);
   FrmDeleteForm (frm);
   FrmSetActiveForm (FrmGetFirstForm ());
}

Boolean buttonsHandleEvent(EventPtr e)
{
  Short dir = DIR_NONE;
  
  if ((e->data.keyDown.chr != hard1Chr)
      && (e->data.keyDown.chr != hard2Chr)
      && (e->data.keyDown.chr != hard3Chr)
      && (e->data.keyDown.chr != hard4Chr)
      && (e->data.keyDown.chr != pageUpChr)
      && (e->data.keyDown.chr != pageDownChr))
    return false; // not a hardware button (e.g. graffiti) or not one we want

  last_key = 0;

  if (FrmGetActiveFormID() != HuntForm) {
    if (FrmGetActiveFormID() == ThingForm &&
	(e->data.keyDown.chr == hard1Chr || e->data.keyDown.chr == hard4Chr))
      LeaveForm(); // Datebook and Memo will leave a Thing-popup.  else....
    return true; // we override these buttons BUT do nothing!
  }
  // ok we are in the HuntForm for sure now, handling only the
  // hard?Chr where ?==1..4, plus the pageup/pagedown.
  
  if (found) {
    // we're in "hit any key to start a new game" state..
    LeaveForm();
    FrmGotoForm(StartForm);
    return true;
  }
  last_key = e->data.keyDown.chr; // could just set it to 0..

  // <incs>/UI/Chars.h is useful....   ChrIsHardKey(c)
  // hard[1-4]Chr, calcChr, findChr.
  switch (e->data.keyDown.chr) {
  case hard2Chr:      // address
    dir = DIR_W;
    break;
  case hard3Chr:      // todo
    dir = DIR_E;
    break;
  case pageUpChr:
    dir = DIR_N;
    break;
  case pageDownChr:
    dir = DIR_S;
    break;
  case hard1Chr: case hard4Chr:
    return true; // we override these buttons BUT do nothing!
  default:
    return false;
  }

  // move_me will do all the redrawing needed, 
  // and check whether you have found the kitten and stuff
  move_me(dir);

  return true;
}


/*
 * Given input SCREEN coordinates x,y,
 * Translate x,y such that the onscreen location of robot is mapped to 0,0
 */
void relativize_move(Short *x, Short *y)
{
  Short center_x = robot.x, center_y = robot.y;
  center_x *= cell_W;
  center_y = (center_y + 1) * cell_H + 2;
  // that is the top-left corner of the cell.  SHOULD add cell_W/2, cell_H/2.
  *x -= center_x;
  *y -= center_y;
}

#define abs(a)   (((a) >= 0) ? (a) : (-(a)))
// This is probably not perfect, especially near the center part.
Short convert_to_dir(Short x, Short y)
{
  // mostly x?
  if ((abs(x)) > 2 * (abs(y)))
    return ( (x > 0) ? DIR_E : DIR_W ); // east, west
  // mostly y?
  if ((abs(y)) > 2 * (abs(x)))
    return ( (y > 0) ? DIR_S : DIR_N ); // south, north
  // diagonal?  (x ==approx== y)
  if (y > 0)      // southish
    return ( (x > 0) ? DIR_SE : DIR_SW ); // se, sw
  else            // northish
    return ( (x > 0) ? DIR_NE : DIR_NW ); // ne, nw
}


Boolean Hunt_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
  Short tmp_x, tmp_y, dir;

  switch (e->eType) {

  case frmOpenEvent:
    last_key = 0;
    frm = FrmGetActiveForm();
    FrmDrawForm(frm);
    // draw the robot, kitten, and non-kitten things.
    initialize_screen();
    return true;

  case menuEvent:
    last_key = 0;
    MenuEraseStatus(NULL); // clear menu status from display
    if (!found) // if kitten is found, game is over, disable menu
      switch(e->data.menu.itemID) {
      case menu_h_about:
	FrmPopupForm(AboutForm);
	break;
      case menu_h_instruct:
	FrmHelp(InstructStr);
	break;
      case menu_h_prefs:
	FrmPopupForm(PrefsForm);
	break;
      case menu_h_quit:
	LeaveForm();
	FrmGotoForm(StartForm);
	break;
      default:
	break;
      }
    handled = true;
    break;

    // we will allow hjklyubn keys to move robot
    // (or to restart the game if kitten is found)
  case keyDownEvent:
    last_key = 0;
    if (found) {
      LeaveForm();
      FrmGotoForm(StartForm);
      handled = true;
    } else {
      last_key = e->data.keyDown.chr;
      handled = do_keyboard(e->data.keyDown.chr);
    }
    break;

    // we will allow screen-taps to move robot
    // (or to restart the game if kitten is found)
  case penDownEvent:
    last_key = 0;
    if (found) {
      LeaveForm();
      FrmGotoForm(StartForm);
    } else {
      tmp_x = e->screenX;
      tmp_y = e->screenY;
      if (my_prefs.relative_move) {
	// translate tap coordinates so that robot's screen location = 0,0.
	relativize_move(&tmp_x, &tmp_y);
      } else {
	tmp_x -= 80; tmp_y -= 80;
      }
      dir = convert_to_dir(tmp_x, tmp_y);
      // move_me will do all the redrawing needed, 
      // and check whether you have found the kitten
      // and do animation and going to start_form foo.
      move_me(dir);
    }
    handled = true;
    break;

    // we will also allow hardware buttons (address, up, todo, down) to move.
    // but they must be caught and handled elsewhere!!

  default:
    return true;
  }
  return handled;
}


/*****************************************************************************
 *                      the Thing form                                       *
 *****************************************************************************/


// All The News That Fits, We Print.
void print_descr(Short i)
{
  Char *msg, *p;
  Word wwlen, maxwid=152;
  Short w, maxw=0, x, y = 0;

#ifdef I_AM_COLOR
  if (color && my_prefs.color_msgs) {
    WinPushDrawState();
    WinSetTextColor(bogus[i].fg_color);
  }
#endif /* I_AM_COLOR */

  if (bogus_messages[i].is_memo) {
    if (!lookup_fortune( bogus_messages[i].index ))
      StrPrintF(fortune_buf, "It's a bug!"); // This ``should never happen''
    msg = fortune_buf;
  } else {
    msg = messages[ bogus_messages[i].index ];
  }
  p = msg;

  while (y < THINGFORM_H) {
    wwlen = FntWordWrap(p, maxwid);
    w = FntCharsWidth(p, wwlen);
    if (w > maxw) maxw = w;
    y += 11;
    if (wwlen >= StrLen(p)) break;
    p += wwlen;
  }
  
  x = (160 - maxw) / 2;
  y = ((y >= THINGFORM_H) ? 0 : (THINGFORM_H-y)/2 );
  p = msg; // rewind...
  while (y < THINGFORM_H) {
    wwlen = FntWordWrap(p, maxwid);
    WinDrawChars(p, wwlen, x, y);
    if (wwlen >= StrLen(p)) break; // whew, we printed the whole message
    p += wwlen;
    y += 11;
  }

#ifdef I_AM_COLOR
  if (color && my_prefs.color_msgs) {
    WinPopDrawState();
  }
#endif /* I_AM_COLOR */
}

// This form just displays a message.
// I have found that the widest message, word-wrapped, will take 3 lines.
// If wider messages are added, change the size of this form, though it
// is currently tall enough to fit 4 lines.
Boolean Thing_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;

  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    FrmDrawForm(frm);
    // remember that 'atthing' is an index into bogus_messages or bogus
    print_descr(atthing);
    return true;

    // Note: you have to tap WITHIN the form in order to leave the form.
    // or write any character; or (see buttonsHandleEvent) certain hw buttons.
  case keyDownEvent:
    // ignore the key that they pressed to "get here" in case it's held down
    if (e->data.keyDown.chr == last_key) return true;
    // else Fall Through.    
  case penDownEvent:
    LeaveForm();
    handled = true;
    break;

  default:
    return true;
  } // end switch event type!
  return handled;
}


/*****************************************************************************
 *                      the About form                                       *
 *****************************************************************************/

// not very exciting but someone's got to handle it

Boolean About_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
    
  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    FrmDrawForm(frm);
    handled = true;
    break;

  case ctlSelectEvent:
    switch(e->data.ctlSelect.controlID) {
    case btn_about_ok:
      LeaveForm();
      handled = true;
      break;
    case btn_about_more:
      FrmHelp(AboutStr);
      handled = true;
      break;
    }
    break;

  default:
    break;
  }

  return handled;
}



/*****************************************************************************
 *                      the Start form                                       *
 *****************************************************************************/

void initialize_arrays()
{
  Short i, j;
  screen_object empty;

#ifdef I_AM_COLOR
  if (color) {
    RGBColorType my_tmp_color;
    my_tmp_color.r = my_tmp_color.g = my_tmp_color.b = 0; // black
    empty.fg_color = WinRGBToIndex(&my_tmp_color);
    my_tmp_color.r = my_tmp_color.g = my_tmp_color.b = 255; // white
    empty.bg_color = WinRGBToIndex(&my_tmp_color);
  }
#endif /* I_AM_COLOR */

  empty.x = 0; empty.y = 0;  // these were -1
  empty.character = ' ';
  empty.memo = false;
  
  for (i = 0; i <= X_MAX; i++)
    for (j = 0; j <= Y_MAX; j++)
      screen[i][j] = EMPTY;
  
  for (i = 0; i < MESSAGES; i++)
    used_messages[i] = false;
  reinit_used_fortunes();

  for (i = 0; i < MAX_BOGUS; i++) {
    bogus_messages[i].index = 0;
    bogus_messages[i].is_memo = false;
    bogus[i] = empty;
  }
}

/*initialize_robot initializes robot.*/
void initialize_robot()
{
#ifdef I_AM_COLOR
  if (color) {
    RGBColorType my_tmp_color;
    my_tmp_color.r = my_tmp_color.g = my_tmp_color.b = 0; // black
    robot.fg_color = WinRGBToIndex(&my_tmp_color);
    my_tmp_color.r = my_tmp_color.g = my_tmp_color.b = 255; // white
    robot.bg_color = WinRGBToIndex(&my_tmp_color);
  }
#endif /* I_AM_COLOR */

  robot.x = randx();  /*Assign a position to the player.*/
  robot.y = randy();

  robot.character = ROBOTCHAR;
  screen[robot.x][robot.y] = ROBOT;
  // if color, robot stays black on white.
}

Boolean validchar(UChar a)
{
  if (a < '!') return false;
  switch(a) {
  case ROBOTCHAR: // 35 '#'
  case HEARTCHAR: // 143
  case 127: case 129: case 160: // boxes and whitespace
  case 128: // whitespace on OS 2
    //  case 37: // This one is too wide if it's bold!  But we're not bolding.
  case 137: // an interesting character, but too wide (way too wide if bold)
    return false;
  }
  return true;
}

/*initialize kitten, well, initializes kitten.*/
void initialize_kitten()
{
  /*Assign the kitten a unique position.*/
  do {
    kitten.x = randx();
    kitten.y = randy();
  } while (screen[kitten.x][kitten.y] != EMPTY);
  
  /*Assign the kitten a character (and a color if we had color).*/
  do {
    kitten.character = randchar();
  } while (!(validchar(kitten.character))); 
  screen[kitten.x][kitten.y] = KITTEN;
#ifdef I_AM_COLOR
  if (color) {
    kitten.bg_color = random_bg_color();
    kitten.fg_color = random_fg_color();
  }
#endif /* I_AM_COLOR */
  found = false;
}


/*initialize_bogus initializes all non-kitten objects to be used in this run.*/
void initialize_bogus()
{
  Short j, index, x, y, msg_count, total_count;
  UChar c;
  for (j = 0; j < num_bogus; j++) {
    /*Give it a character.*/
    do {  c = randchar(); }
    while (!validchar(c)); 
    bogus[j].character = c;

    // also a color
#ifdef I_AM_COLOR
    if (color) {
      bogus[j].bg_color = random_bg_color();
      bogus[j].fg_color = random_fg_color();
    }
#endif /* I_AM_COLOR */

    /*Give it a position.*/
    do { x = randx();  y = randy(); }
    while (screen[x][y] != EMPTY);
    bogus[j].x = x; bogus[j].y = y;
    screen[x][y] = j + STARTBOGUS;
      
    /*Find a message for this object.*/
    /* Treat MESSAGES as just a hint; don't rely on it for # of nki..
       could be too large or too small accidentally. */
    msg_count = (MESSAGES_real < MESSAGES) ? MESSAGES_real : MESSAGES;
    // Might use some fortunes from a database record.
    total_count = (fortune_exists ? msg_count+fortune_ctr : msg_count);
    // Decide which of the 1 or 2 pools to draw the message from
    if (!fortune_exists ||  // [props to Richard Hoelscher]
	(!my_prefs.replace_nki &&
	 ((SysRandom(0) % total_count) < msg_count)) ) {
      // Draw from the messages.h pool
      do {
	index = SysRandom(0) % msg_count;
      } while (used_messages[index]);
      bogus_messages[j].index = index;
      bogus_messages[j].is_memo = false;
      used_messages[index] = true;
    } else {
      // Draw from the memo pool - see rfk-memo.c
      bogus_messages[j].index = random_fortune(num_bogus);
      bogus_messages[j].is_memo = true;
      // (random_fortune() will track used_fortunes on its own.)
    }
  }
}



void init_bogus_field(FieldPtr fld, Short n_bogus)
{
  CharPtr p;
  VoidHand vh;
  vh = (VoidHand) FldGetTextHandle(fld);
  if (!vh) // in .rcp we restricted fld to 2 chars; but may as well be safe:
    vh = MemHandleNew(8 * sizeof(Char));
  FldSetTextHandle(fld, (Handle) 0);
  p = MemHandleLock(vh);
  StrPrintF(p, "%d", n_bogus);
  MemHandleUnlock(vh);
  FldSetTextHandle(fld, (Handle) vh);
}

Boolean read_bogus_field(FieldPtr fld, Short *n_bogus)
{
  CharPtr textp;
  Short n;
  textp = FldGetTextPtr(fld);
  if (textp && StrLen(textp) > 0) {
    n = StrAToI(textp);
    if (n > 0 && n <= MAX_BOGUS) {
      *n_bogus = n;
      FldReleaseFocus(fld);
      FldSetSelection(fld, 0, 0);
      FldFreeMemory(fld);
      return true;
    }
    // Else the number is out of range and we silently reject it.
    // Note: It might be a better idea to noisily reject it.
  }
  return false;
}


Boolean Start_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
  FieldPtr fld;
    
  switch (e->eType) {

  case frmOpenEvent:
    initialize_arrays();
    initialize_robot();
    initialize_kitten();
    frm = FrmGetActiveForm();
    fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, field_start));
    init_bogus_field(fld, num_bogus);
    FrmDrawForm(frm);
    // I should read prefs.num_bogus and set the ui widgets..
#ifdef DEBUG
    {
      Char buf[80];
      if (MESSAGES_real != MESSAGES)
	StrPrintF(buf, "[WARNING: there's %d nki, not %d]",
		  MESSAGES_real, MESSAGES);
      else
	StrPrintF(buf, "[yep, messages.h has %d nki]", MESSAGES);
      WinDrawChars(buf, StrLen(buf), 5, 103);
      if (fortune_exists)
	StrPrintF(buf, "%d fortunes found", fortune_ctr);
      else 
	StrPrintF(buf, "No fortunes found", fortune_ctr);
      WinDrawChars(buf, StrLen(buf), 5, 103-11);
    }
#endif /* DEBUG */
    handled = true;
    break;

  case menuEvent:
    MenuEraseStatus(NULL); // clear menu status from display
      switch(e->data.menu.itemID) {
      case menu_s_about:
	FrmPopupForm(AboutForm);
	break;
      case menu_s_prefs:
	FrmPopupForm(PrefsForm);
	break;
      default:
	break;
      }
    handled = true;
    break;

  case ctlSelectEvent:
    switch(e->data.ctlSelect.controlID) {
    case btn_start_ok:
      frm = FrmGetActiveForm();
      fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, field_start));
      if (read_bogus_field(fld, &num_bogus)) {
	initialize_bogus();
	LeaveForm();
	FrmGotoForm(HuntForm);
      }
      handled = true;
      break;
    }
    break;

  default:
    break;
  }

  return handled;
}



/*****************************************************************************
 *                      the Prefs form                                       *
 *****************************************************************************/

void readPrefs()
{
  Word prefsSize;
  SWord prefsVersion;

  prefsSize = sizeof(struct rfkPreferenceType);
  prefsVersion = PrefGetAppPreferences(rfkAppID, rfkAppPrefID, &my_prefs,
                                       &prefsSize, true);
  num_bogus = my_prefs.default_num_bogus;
}
void writePrefs()
{
  Word prefsSize;

  prefsSize = sizeof(struct rfkPreferenceType);
  PrefSetAppPreferences(rfkAppID, rfkAppPrefID, rfkAppPrefVersion,
                        &my_prefs, prefsSize, true);
}

Boolean Prefs_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false, redraw = false;
  FormPtr frm;
  FieldPtr fld;
  ControlPtr checkbox;
  Short n;
    
  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, field_prefs));
    init_bogus_field(fld, my_prefs.default_num_bogus);
    checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prefs_1));
    CtlSetValue(checkbox, (my_prefs.relative_move ? 1 : 0));
    checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prefs_2));
    CtlSetValue(checkbox, (my_prefs.sound_on ? 1 : 0));
    checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prefs_3));
    CtlSetValue(checkbox, (my_prefs.use_buttons ? 1 : 0));
    checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prefs_4));
    CtlSetValue(checkbox, (my_prefs.replace_nki ? 1 : 0));
    FrmDrawForm(frm);
    if (color) {
      // make the color-OS-only checkboxes usable.. AFTER drawing the form..
      checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prefs_5));
      CtlSetValue(checkbox, (my_prefs.color_nki ? 1 : 0));
      CtlShowControl(checkbox);
      checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prefs_6));
      CtlSetValue(checkbox, (my_prefs.color_msgs ? 1 : 0));
      CtlShowControl(checkbox);
    }
    handled = true;
    break;

  case ctlSelectEvent:
    switch(e->data.ctlSelect.controlID) {
    case btn_prefs_ok:
      frm = FrmGetActiveForm();
      fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, field_prefs));
      if (read_bogus_field(fld, &n)) {
	my_prefs.default_num_bogus = n;
	checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prefs_1));
	my_prefs.relative_move = (CtlGetValue(checkbox) != 0);	
	checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prefs_2));
	my_prefs.sound_on = (CtlGetValue(checkbox) != 0);	
	checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prefs_3));
	my_prefs.use_buttons = (CtlGetValue(checkbox) != 0);	
	checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prefs_4));
	my_prefs.replace_nki = (CtlGetValue(checkbox) != 0);	
	if (color) {
	  Boolean old = my_prefs.color_nki;
	  checkbox = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,check_prefs_5));
	  my_prefs.color_nki = (CtlGetValue(checkbox) != 0);
	  redraw = (old != my_prefs.color_nki); // if changed, redraw.. later..
	  checkbox = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,check_prefs_6));
	  my_prefs.color_msgs = (CtlGetValue(checkbox) != 0);	
	}
	writePrefs();
	LeaveForm();
	if (redraw) initialize_screen(); // since we turned 'color' on or off 
      }
      handled = true;
      break;
    case btn_prefs_cancel:
      LeaveForm();
      handled = true;
      break;
    }
    break;

  default:
    break;
  }

  return handled;
}

/**********************************************************************
                       DO_FEEP
 IN:
 frq = frequency
 dur = duration
 OUT:
 nothing
 PURPOSE:
 This will produce a beep of the requested frequency and duration,
 if the owner has "game sound" turned on in the Preferences app.
 (Try to keep it down to chirps...)
 **********************************************************************/
/* SoundLevelType: slOn, slOff
   gameSoundLevel found in SystemPreferencesType
 */
void do_feep(Long frq, UInt dur)
{
  SystemPreferencesChoice allgamesound;

  if (!my_prefs.sound_on)
    return;

#ifdef I_AM_OS_2
  allgamesound = prefGameSoundLevel;
#else
  allgamesound = prefGameSoundLevelV20;
#endif

  if (PrefGetPreference(allgamesound) != slOff) {
    /* click: 200, 9
       confirmation: 500, 70  */
    SndCommandType sndCmd;
    sndCmd.cmd = sndCmdFreqDurationAmp; /* "play a sound" */
    sndCmd.param1 = frq; /* frequency in Hz */
    sndCmd.param2 = dur; /* duration in milliseconds */
    sndCmd.param3 = sndDefaultAmp; /* amplitude (0 to sndMaxAmp) */
    SndDoCmd(0, &sndCmd, true);
  }
}

/*****************************************************************************
 *                      ApplicationHandleEvent                               *
 *****************************************************************************/
Boolean ApplicationHandleEvent(EventPtr e)
{
    FormPtr frm;
    Word    formId;
    Boolean handled = false;

    if (e->eType == frmLoadEvent) {
	formId = e->data.frmLoad.formID;
	frm = FrmInitForm(formId);
	FrmSetActiveForm(frm);

	switch(formId) {
	case StartForm:
	    FrmSetEventHandler(frm, Start_Form_HandleEvent);
	    break;
	case HuntForm:
	    FrmSetEventHandler(frm, Hunt_Form_HandleEvent);
	    break;
	case AboutForm:
	    FrmSetEventHandler(frm, About_Form_HandleEvent);
	    break;
	case PrefsForm:
	    FrmSetEventHandler(frm, Prefs_Form_HandleEvent);
	    break;
	case ThingForm:
	    FrmSetEventHandler(frm, Thing_Form_HandleEvent);
	    break;
	}
	handled = true;
    }

    return handled;
}


/*****************************************************************************
 *                      StartApplication                                     *
 *****************************************************************************/
static Word StartApplication(void)
{
  Word error = 0;

  // Decide whether we have color an'at
  DWord version;
  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &version);
#ifdef I_AM_COLOR
  // well,,, we don't NECESSARILY have color, but we have the API calls anyway
  if (version >= 0x03503000L)
    color = true;
  else
    color = false; // Better test to see if it will run on OS2..
#endif /* I_AM_COLOR */

  // Make sure that messages.h has not got a fencepost error (again).
  MESSAGES_real = sizeof(messages) / sizeof(Char *);

  // Read user preferences.
  readPrefs();
  // Open the memo database and look for a fortune memo
  find_fortune();
  // Seed the random number generator.
  SysRandom(TimGetSeconds());
  
  FrmGotoForm(StartForm);
  
  return error;
}


/*****************************************************************************
 *                      StopApplication                                      *
 *****************************************************************************/
static void StopApplication(void)
{
  FrmSaveAllForms();
  FrmCloseAllForms();
  // close databases, free stuff.  maybe we don't need to.
  cleanup_fortune();
}


/*****************************************************************************
 *                      EventLoop                                            *
 *****************************************************************************/

/* The main event loop */

static void EventLoop(void)
{
    Word err;
    EventType e;
     
    do {
	EvtGetEvent(&e, evtWaitForever);
	// first see if it's a hardware button thing!!!
	// poweredOnKeyMask: we don't handle a keypress if it caused power-on.
	if ( (e.eType != keyDownEvent)
	     || !my_prefs.use_buttons
	     || (e.data.keyDown.modifiers & poweredOnKeyMask)
	     || !buttonsHandleEvent(&e) )
	  // now proceed with usual handling
	  if (! SysHandleEvent (&e))
	    if (! MenuHandleEvent (NULL, &e, &err))
	      if (! ApplicationHandleEvent (&e))
		FrmDispatchEvent (&e);
    } while (e.eType != appStopEvent);
}


/* Main entry point; it is unlikely you will need to change this except to
   handle other launch command codes */
/*****************************************************************************
 *                      PilotMain                                            *
 *****************************************************************************/
DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
    Word err;

    if (cmd == sysAppLaunchCmdNormalLaunch) {

      err = StartApplication();
      if (err) return err;

      EventLoop();
      StopApplication();

    } else {
      return sysErrParamErr;
    }

    return 0;
}
