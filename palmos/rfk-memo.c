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
//#include "robotfindskittenRsc.h" // not needed, no UI stuff in here
#include "constants.h"
// but not "messages.h"

/*
 * Find the "rfk-fortune" Memo, if one exists.
 * set a boolean, and lock the record for reading.
 * count the number of lines.
 * bogus_messages[] is an array of Short.  we will use the sign bit for evil.
 * if (bogus_messages[i]) < 0)
 *    then use up to 80 chars of rfk_fortune_line[-1*i]
 *    else use the usual messages[i].
 * if we don't have my_prefs.replace_foo, then the chance of drawing
 * from the regular list is MESSAGES/(MESSAGES + Memo_Lines).  and similarly.
 * (else use only the memo list.)
 */

DmOpenRef MemoDB;
// this seems weird to me, but what the heck.
typedef struct {
  Char note;           // null terminated
} MemoDBRecordType;
typedef MemoDBRecordType * MemoDBRecordPtr;

// Remember some stuff about fortune memo if we find it.

Boolean fortune_exists = false;
Short fortune_ctr;
Short *fortune_indices;
Boolean *used_fortunes;
Short used_fortune_ctr = 0;
MemoDBRecordPtr fortuneRecP;
Char fortune_buf[81];

Char * my_malloc(Int n);

#define memoDBType 'DATA'
void find_fortune() {
  UInt recordNum;
  VoidHand recH;
  UInt mode = dmModeReadOnly;
  Char magic_title[20];
  Short title_len, i, j, fortune_len;

  StrPrintF(magic_title, "rfk-fortune%c", linefeedChr);
  title_len = StrLen(magic_title);

  if (!(MemoDB = DmOpenDatabaseByTypeCreator(memoDBType, sysFileCMemo, mode))){
    fortune_exists = false;
    fortune_ctr = 0;
    return; // no memos, therefore no fortune memo
  }

  recordNum = 0;
  // get a record (stop if we run out)
  while ((recH = DmQueryNextInCategory(MemoDB, &recordNum, dmAllCategories))) {
    CharPtr memo;
    fortuneRecP = MemHandleLock (recH);
    memo = &(fortuneRecP->note);
    if (0 == StrNCompare(memo, magic_title, title_len)) {
      // the title matches our magic title!
      fortune_exists = true;
      fortune_len = StrLen(memo);
      // Ok now let's count the number of lines in the memo..
      // fortune_ctr will be number of lines EXCLUDING title;
      // the "fortune_len-1" is to ignore newlines that terminate the memo.
      // yeah, I could be using StrChr, but it didn't like me.
      for (i = 0, fortune_ctr = 0 ; i < fortune_len-1 ; i++)
	if (memo[i] == linefeedChr)
	  fortune_ctr++;
      // Ok now let's index the starts of the (non-title) lines.
      // again we have that fortune_len-1 to ignore possible final newline
      fortune_indices = (Short *) my_malloc(sizeof(Short) * fortune_ctr);
      used_fortunes = (Boolean *) my_malloc(sizeof(Boolean) * fortune_ctr);
      for (i = 0, j = 0 ; i < fortune_len-1 && j < fortune_ctr ; i++)
	if (memo[i] == linefeedChr)
	  fortune_indices[j++] = i+1; // line starts right after newline (duh)
      // We are done now.  Do NOT unlock the record.
      return;
    } // else memo does not match
    MemHandleUnlock(recH);
    recordNum++;
  }
  
  if (!fortune_exists)
    DmCloseDatabase(MemoDB);
  // otherwise we'll close it when we exit the app.
}

void reinit_used_fortunes() {
  Short i;
  for (i = 0 ; i < fortune_ctr ; i++)
    used_fortunes[i] = false;
  used_fortune_ctr = 0;
}


// If there are more than num_bogus messages in the memo,
// then we will return 'unique' ones, otherwise we won't try (yes, even
// if we are also drawing from the messages.h pool)
#define OH_STOP_TRYING 1 /* increase for less fairness and less search time */
Short random_fortune(Short num_bogus) {
  Short index = SysRandom(0) % fortune_ctr;
  if (used_fortune_ctr < fortune_ctr - OH_STOP_TRYING)
    while (used_fortunes[index]) // get an unused one
      index = SysRandom(0) % fortune_ctr;
  // else: we've used nearly everything already, so stop caring about it.
  used_fortunes[index] = true;
  used_fortune_ctr++;
  return index;
}
#undef OH_STOP_TRYING

Boolean lookup_fortune(Short i) {
  CharPtr memo;
  Short len, maxlen = 80;
  if (!fortune_exists || (i >= fortune_ctr) || (i < 0))
    return false; // caller is moron
  memo = &(fortuneRecP->note);
  memo += fortune_indices[i]; // jump to the right line
  if (i+1 < fortune_ctr) { // we know it ends where the next one begins
    Short tmp = fortune_indices[i+1] - fortune_indices[i];
    maxlen = (tmp < maxlen) ? tmp : maxlen;
  } // else maxlen is 80
  StrNCopy(fortune_buf, memo, maxlen);
  fortune_buf[maxlen] = '\0';
  len = StrLen(fortune_buf);
  if (fortune_buf[len-1] == linefeedChr) fortune_buf[len-1] = '\0';
  return true;
}

void cleanup_fortune() {
  if (!fortune_exists) return;
  // free fortune_indices?  free used_fortunes?  probably a good idea..

  // release record
  MemPtrUnlock(fortuneRecP);
  // close database
  if (MemoDB)
    DmCloseDatabase(MemoDB);
  fortune_exists = false;
}




/***************************************************************
                   x
 IN:
 n = size to allocate
 OUT:
 pointer to the locked chunk
 PURPOSE:
 Allocate and lock a moveable chunk of memory.
****************************************************************/
Char * my_malloc(Int n) {
  VoidHand h;
  VoidPtr p;

  h = MemHandleNew((ULong) n);
  if (!h) {
    /* the caller might want to check this and die. */
    return NULL;
  }

  p = MemHandleLock(h);
  MemSet(p, n, 0); /* just to make really sure the memory is zeroed */
  return p;
}
