// Yes, I have registered this Creator ID.  Yay.
#define rfkAppType 'Rfk1'
#define rfkAppID   'Rfk1'
#define rfkAppPrefID 0x00
#define rfkAppPrefVersion 0x01

// Size Of Screen.  160x160 pixels.  Standard PalmOS font.
// Height + width in pixels of one character ... in this case an 'M'
#define cell_H 11 /* was 10 */
#define cell_W 10 /* was 8 */
#define robot_H 7
#define robot_W 7
/* we can have   160 / cell_W  columns.  16.  0 thru 15. */
#define X_MIN 0
#define X_MAX 15
/* we can have   160 / cell_H  rows.  14.545454  0 thru 13.
   we will use top one for title. */
#define Y_MIN 0
#define Y_MAX 12


/*Constants for our internal representation of the screen.*/
#define EMPTY 0
#define ROBOT 1
#define KITTEN 2
#define STARTBOGUS 3

#define ROBOTCHAR '#'
#define HEARTCHAR 143

// MESSAGES is defined in messages.h
//#define MESSAGES 201
// (I had a note to myself that if MESSAGES exceeded 255 I would have to
// think, but that is no longer the case.)

// I will only allow 1-99 non-kitten items, because screen is small.
#define MAX_BOGUS 99

// many different ways we can obtain direction.
// hardkeys, hjkl, screen taps.  translate all to same format
#define DIR_NONE 0
#define DIR_W    1
#define DIR_S    2
#define DIR_N    3
#define DIR_E    4
#define DIR_NW   5
#define DIR_NE   6
#define DIR_SW   7
#define DIR_SE   8

/*Macros for generating numbers in different ranges*/
#define randx() (SysRandom(0) % X_MAX+1)
#define randy() (SysRandom(0) % (Y_MAX-Y_MIN+1)+Y_MIN) /*I'm feeling randy()!*/
/* I had better check on these characters. */
#define randchar() (SysRandom(0) % (254-'!'+1)+'!')
#ifdef I_AM_COLOR
/* here we want 0x00, 33, 66, 99, and CC values only (for a 'dark' R, G, or B.)
   so pick 0-4 inclusive and mul by 51. */
//#define randRGB() ((SysRandom(0) % 5) * 51)
//#define randcolor() rand() % 6 + 1
#endif /* I_AM_COLOR */
/* We could use bold but (a) it's a different font (b) 'M' is even wider in it
   Another possibility is reverse-video. */
//#define randbold() (rand() % 2 ? TRUE:FALSE)


/*This struct contains all the information we need to display an item
  on the screen*/
typedef struct
{
  UChar x;
  UChar y;
  //  bool bold; /* we're not using bold because it's a PAIN to change font */
  UChar character; // 33 to 255 inclusive, apart from a few skips
#ifdef I_AM_COLOR
  IndexedColorType fg_color; // this is a UInt8, or, as its friends used to
  IndexedColorType bg_color; // call it, a UChar.
#endif
} screen_object;



struct rfkPreferenceType {
  UChar default_num_bogus;
  Boolean relative_move;
  Boolean sound_on;
  Boolean use_buttons;
};
