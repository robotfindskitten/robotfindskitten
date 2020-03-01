#ifndef __RFK_MEMO__
#define __RFK_MEMO__
extern Boolean fortune_exists; // Does the memo exist (if not don't use, duh).
extern Short fortune_ctr; // How many messages does it contain.
extern Char fortune_buf[81]; // messages from the memo will be NCopy'd to here
void find_fortune(); // app calls this initially
Short random_fortune(Short num_bogus); // may call when generating nki
Boolean lookup_fortune(Short i); // may call before printing a nki message
void reinit_used_fortunes();
void cleanup_fortune(); // app calls this finally
#endif
