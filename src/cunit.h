/*
    $Header$

    cunit - A C Unit testing framework
    Copyright (C) 2000 Christian Holmboe <spotty@codefactory.se>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef CUNIT_H
#define CUNIT_H
#include <glib.h>

#define CUNIT_REPORT_SUMMARY 1
#define CUNIT_REPORT_ALL     2
#define CUNIT_REPORT_FAIL    4

#define CUNIT_REPORT_LEVEL_0 (0)
#define CUNIT_REPORT_LEVEL_1 (CUNIT_REPORT_SUMMARY)
#define CUNIT_REPORT_LEVEL_2 (CUNIT_REPORT_SUMMARY|CUNIT_REPORT_FAIL)
#define CUNIT_REPORT_LEVEL_3 (CUNIT_REPORT_SUMMARY|CUNIT_REPORT_ALL)

/* Use cunit_assert in the unit tests */
#define cunit_assert(expr) \
     if (!(expr))						 \
       return g_strdup_printf (					 \
	      "%s:%d in %s(): failed assertion cunit_assert(%s)",\
	      __FILE__,						 \
	      __LINE__,						 \
	      __PRETTY_FUNCTION__,				 \
	      #expr)


typedef void * cunit_contextp ;

typedef gint (*cunit_setup_fp) ( cunit_contextp ) ;

typedef gchar* (*cunit_test_fp) ( cunit_contextp ) ;

typedef gint (*cunit_teardown_fp) ( cunit_contextp ) ;

/* Common struct for cunit_test_suit and cunit_test */
typedef struct {
  gint type ;
  gchar* name ;

  void *buf[3] ; /* Place holder */
} cunit_entity ;

typedef struct cunit_test_suite {
  gint type ;
  gchar* name ;
  
  cunit_setup_fp setup ;
  cunit_teardown_fp teardown ; 

  GArray *sub ;
} cunit_test_suite ;

typedef struct cunit_test {
  gint type ;
  gchar* name ;

  cunit_test_fp run ;
} cunit_test ;

typedef struct {
  gint report_level ;
  gint number_of_tests_run ;
  gint number_of_tests_passed ;

  cunit_test_suite top_suite ;
} cunit_test_session ;

typedef cunit_test_session* (*cunit_build_session_fp)(void) ;

/* cunit_new_test_session: return a pointer to an empty cunit_test_suite */
cunit_test_session * cunit_new_test_session() ;

/* cunit_reset_test_session: reset a test session */
void cunit_reset_test_session( cunit_test_session *sp ) ;

/* cunit_free_test_session: free up all system resources used by a test session */
void cunit_free_test_session( cunit_test_session *sp ) ;


/* cunit_register_setup: register a setup function on a test suite */
void cunit_register_setup( cunit_test_session *sp, const gchar *strpath, cunit_setup_fp fptr ) ;

/* cunit_register_test: register a unit test */
void cunit_register_test( cunit_test_session *sp, const gchar *strpath, cunit_test_fp fptr ) ;

/* cunit_register_teardown: register a teardown function on a test suite */
void cunit_register_teardown( cunit_test_session *sp, const gchar *strpath, cunit_teardown_fp fptr ) ;


/* cunit_list_test: print unit test names */
void cunit_list_test( cunit_test_session *sp, gchar *strpath ) ;

/* cunit_list_all: print unit test names, setups and teardowns */
void cunit_list_all( cunit_test_session *sp, gchar *strpath ) ;


/* cunit_run_suite: run a test suite */
void cunit_run_suite (gchar * suite_path) ;


/* cunit_TestRunner: If argv[1] is '--TestRunner' cunit_TestRunner
   will invoke the TestRunner otherwise it will return without doing
   anything, allowing the program to execute as usual.

   TestRunner usage:
   
   $ <prog_name> --TestRunner [-vvqlL] [SUITE|TEST]

   If no suite or test is specified all registered tests are run.

   Flags :

   -q  quiet, only report by exit status
   -v  verbose
   -vv more verbose
   -l  list unit tests
   -L  list setup, teardowns and, unit tests.   

 */
gint cunit_TestRunner( gint argc, gchar *argv[], cunit_build_session_fp build_session ) ;

#endif












