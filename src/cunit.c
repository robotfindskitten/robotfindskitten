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

#include "cunit.h"

#define CUNIT_SUITE  1
#define CUNIT_TEST   2

#define CUNIT_ENTITY_IS_SUITE(x) (x->type == CUNIT_SUITE)
#define CUNIT_ENTITY_IS_TEST(x) (!CUNIT_ENTITY_IS_SUITE(x))

#define CUNIT_FUN_SETUP 0
#define CUNIT_FUN_TEST   1
#define CUNIT_FUN_TEARDOWN 2

gint cunit_stdin ;
gint cunit_stdout ;
gint cunit_stderr ;


static GPtrArray * __cunit_strpath2ptrpath( cunit_test_session *sp, const gchar* strpath ) ;
static void __cunit_free_test_suite( cunit_test_session *sp, gchar *strpath ) ;
static void __cunit_free_test_suite_resources( cunit_test_suite *suite ) ;

static void
__cunit_entity_init(cunit_entity *ep, const gchar *name, gint type)
{
  memset( ep, 0, sizeof( cunit_entity ) ) ;
  ep->name = g_strdup( name ) ;
  ep->type = type ;
  
  if( type == CUNIT_SUITE )
    ((cunit_test_suite*)ep)->sub = g_array_new( 0, 1, sizeof( cunit_entity ) ) ;
}

static cunit_entity *
__cunit_array_find_entity( GArray *arr, const gchar *name )
{
  gint i ;
  
  for( i = 0 ; i < arr->len ; i++ )
    if(( g_strcasecmp( (g_array_index( arr, cunit_entity, i )).name, name ) == 0 ) &&
       ( strlen( (g_array_index( arr, cunit_entity, i )).name) == strlen( name )))
      return & g_array_index( arr, cunit_entity, i ) ;
  
  return NULL ;
}

/*
 * Translates a string path to a cunit entity to a poiter array, if create is set 
 * it will create entities as it goes.
 */
static gint
__cunit_new_ptrpath2( GPtrArray *ptrpath, const gchar *strpath, const gint create, const gint type )
{
  cunit_entity *ep ;
  cunit_test_suite *prev_suite ;
  gchar *c, *name, *newpath ;
  gint result ;

  g_assert( ptrpath != NULL ) ;
  g_assert( strpath != NULL ) ;
  g_assert( ! create || (type == CUNIT_TEST || type == CUNIT_SUITE) ) ;

  if( *strpath == '\0' )
    return 0 ;

  ep = (cunit_entity*) g_ptr_array_index( ptrpath, ptrpath->len - 1 ) ;

  g_assert( CUNIT_ENTITY_IS_SUITE( ep ) ) ;
  prev_suite = (cunit_test_suite*)ep ;

  name = g_strdup( strpath ) ;

  if((c = (gchar*)strchr(name, '.')) != NULL) {
    *c = '\0' ;
    newpath = c + 1 ;
  } else
    newpath = & name[ strlen( name ) ] ; /* '\0' */
  
  ep = __cunit_array_find_entity( prev_suite->sub, name ) ;
  
  if( ep == NULL && create ) {
    cunit_entity e ;

    __cunit_entity_init( &e, name, (*newpath == '\0') ? type : CUNIT_SUITE ) ;
    g_array_append_val( prev_suite->sub, e ) ;
    ep = & g_array_index( prev_suite->sub, cunit_entity, prev_suite->sub->len - 1 ) ;
  }
 
  if( ep == NULL ) {
    g_free( name ) ;
    return -1 ;
  }

  g_ptr_array_add( ptrpath, ep ) ;

  result = __cunit_new_ptrpath2( ptrpath, newpath, create, type ) ;

  g_free( name ) ;
  return result ;
}

static GPtrArray *
__cunit_new_ptrpath( cunit_test_suite *suitep, const gchar *strpath, const gint create, const gint type )
{
  GPtrArray *ptrarr ;
  gint result ;
  
  g_assert( suitep != NULL ) ;
  g_assert( CUNIT_ENTITY_IS_SUITE( suitep ) ) ;

  ptrarr = g_ptr_array_new() ;
  g_ptr_array_add( ptrarr, suitep ) ;

  result = __cunit_new_ptrpath2( ptrarr, strpath, create, type ) ;

  if( result != 0 ) {
    g_ptr_array_free( ptrarr, 0 ) ;
    return NULL ;
  }

  return ptrarr ;
}

static void 
__cunit_register( cunit_test_suite *suitep, const char *strpath, void* fptr, gint funtype )
{
  GPtrArray *ptrarr ;
  cunit_entity *ep ;
  gint t ;

  g_assert( suitep != NULL ) ;
  g_assert( fptr != NULL ) ;
  g_assert( CUNIT_ENTITY_IS_SUITE( suitep ) ) ;

  t = (funtype == CUNIT_FUN_TEST) ? CUNIT_TEST : CUNIT_SUITE ;
  ptrarr = __cunit_new_ptrpath( suitep, strpath, 1, t ) ;

  ep = g_ptr_array_index( ptrarr, ptrarr->len - 1 ) ;

  g_ptr_array_free( ptrarr, 0 ) ;

  /* Sanety check, only register run on tests and setup/teardown on suites */
  if( CUNIT_ENTITY_IS_TEST( ep ) && (funtype != CUNIT_FUN_TEST))
    g_error("Can only register a run function on a test ('%s')", strpath ) ;
  
  if( CUNIT_ENTITY_IS_SUITE( ep ) && (funtype == CUNIT_FUN_TEST))
    g_error("Can only register a setup or teardown function on a test suite ('%s')", strpath ) ;

  switch( funtype ) {
  case CUNIT_FUN_TEST:
    ((cunit_test*)ep)->run = fptr ;
    break ;
  case CUNIT_FUN_SETUP:
    ((cunit_test_suite*)ep)->setup = fptr ;
    break ;
  case CUNIT_FUN_TEARDOWN:
    ((cunit_test_suite*)ep)->teardown = fptr ;
    break ;
  }

}

cunit_test_session *
cunit_new_test_session()
{
  cunit_test_session *new_session ;
  
  new_session = g_new0( cunit_test_session, 1 ) ;
  
  __cunit_entity_init( (cunit_entity*)(& new_session->top_suite), "", CUNIT_SUITE ) ;

  return new_session ;
}

void
cunit_reset_test_session( cunit_test_session *sp )
{
  sp->number_of_tests_passed = 0 ;

  sp->number_of_tests_run = 0 ;
}

void
cunit_free_test_session( cunit_test_session *sp )
{
  __cunit_free_test_suite_resources( & sp->top_suite ) ;

  g_free( sp ) ;
}

static void
__cunit_free_test_suite_resources( cunit_test_suite *suite )
{
  gint i ;
  
  g_assert( CUNIT_ENTITY_IS_SUITE( suite ) ) ;

  g_free( suite->name ) ;

  for( i = 0 ; i < suite->sub->len ; ++i ) {
    cunit_entity *ep = & g_array_index( suite->sub, cunit_entity, i ) ;

    if( CUNIT_ENTITY_IS_SUITE( ep ) ) {
      __cunit_free_test_suite_resources( ((cunit_test_suite*)ep) ) ;
    
      g_array_free( ((cunit_test_suite*)ep)->sub, 1 ) ;
    } else
      g_free( ep->name ) ;
    
  }

}

void 
cunit_register_setup( cunit_test_session *sp, const gchar *strpath, cunit_setup_fp fptr )
{
  __cunit_register( & sp->top_suite, strpath, fptr, CUNIT_FUN_SETUP ) ;
}

void
cunit_register_test( cunit_test_session *sp, const gchar *strpath, cunit_test_fp fptr )
{
  __cunit_register( & sp->top_suite, strpath, fptr, CUNIT_FUN_TEST ) ;
}

void 
cunit_register_teardown( cunit_test_session *sp, const gchar *strpath, cunit_teardown_fp fptr )
{
  __cunit_register( & sp->top_suite, strpath, fptr, CUNIT_FUN_TEARDOWN ) ;
} 

typedef void (*cunit_foreach_fp) (GPtrArray*, void*) ;


static void
__cunit_foreach2( GPtrArray *ptrpath, gint which, cunit_foreach_fp function, void* userdata, gint depth )
{
  gint i ;
  cunit_test_suite *prev_suite ;
  cunit_entity *ep ;
  
  ep = (cunit_entity*) g_ptr_array_index( ptrpath, ptrpath->len - 1 ) ;
 
  if( depth == 0 )
    return ;

  if( CUNIT_ENTITY_IS_SUITE( ep ) ) {
    prev_suite = (cunit_test_suite*)ep ;

    for( i = 0 ; i < prev_suite->sub->len ; ++i ) {
      ep = & g_array_index( prev_suite->sub, cunit_entity, i ) ;
      
      g_ptr_array_add( ptrpath, ep ) ;
      
      if( which & CUNIT_SUITE && CUNIT_ENTITY_IS_SUITE( ep ) )
	function( ptrpath, userdata ) ;
      
      if( which & CUNIT_TEST && CUNIT_ENTITY_IS_TEST( ep ) )
	function( ptrpath, userdata ) ;
      
      if( CUNIT_ENTITY_IS_SUITE( ep ) )
	__cunit_foreach2( ptrpath, which, function, userdata, depth - 1 ) ;
      
      g_ptr_array_remove_index( ptrpath, ptrpath->len - 1 ) ;
    }

  } else {
    
    if( which & CUNIT_TEST && CUNIT_ENTITY_IS_TEST( ep ) ) 
      function( ptrpath, userdata ) ;
    
  }

  return ;

}

static void
__cunit_foreach( cunit_test_session *sp, gchar *strpath, gint which, cunit_foreach_fp function, void* userdata, gint depth )
{
  GPtrArray *ptrarr ;
  gint result ;
  
  g_assert( strpath != NULL ) ;
  g_assert( function != NULL ) ;

  ptrarr = __cunit_strpath2ptrpath( sp, strpath ) ;

  if( ptrarr == NULL )
    g_error("can not find suite or test \"%s\"", strpath ) ;

  __cunit_foreach2( ptrarr, which, function, userdata, depth ) ;

  g_ptr_array_free( ptrarr, 0 ) ;
 
  return ;
}


static GPtrArray *
__cunit_strpath2ptrpath( cunit_test_session *sp, const gchar* strpath )
{
  return __cunit_new_ptrpath( & sp->top_suite, strpath, 0, 0 ) ;
}

static gchar *
__cunit_ptrpath2strpath( const GPtrArray *ptrpath )
{

  gint i ;
  GString *str ;
  gchar *res ;

  str = g_string_new("") ;

  for( i = 0 ; i < ptrpath->len ; ++i ) {
    g_string_append( str, ((cunit_entity*)g_ptr_array_index( ptrpath, i ))->name ) ;
    if((i > 0) && (i < ptrpath->len - 1))
      g_string_append( str, "." ) ;
  }

  res = str->str ;

  g_string_free(str, 0 ) ;

  return res ;
}

static void
__cunit_print_entity( GPtrArray *ptrpath, void* data )
{
  cunit_entity *ep ;
  gchar *strpath ;
  
  strpath = __cunit_ptrpath2strpath( ptrpath ) ;

  ep = (cunit_entity*) g_ptr_array_index( ptrpath, ptrpath->len - 1 ) ;
  
  if( CUNIT_ENTITY_IS_SUITE( ep ) ) {

    if( ((cunit_test_suite*)ep)->setup != NULL ) 
      g_print( "%s.setup\n", strpath ) ;
  
    if( ((cunit_test_suite*)ep)->teardown != NULL ) 
      g_print( "%s.teardown\n", strpath ) ;

  } else if( ((cunit_test*)ep)->run != NULL ) 
    
    g_print( "%s\n", strpath ) ;
  
}

void
cunit_list_all( cunit_test_session *sp, gchar *strpath )
{
  __cunit_foreach( sp, strpath, (CUNIT_SUITE|CUNIT_TEST), __cunit_print_entity, NULL, -1 ) ;
}

void
cunit_list_test( cunit_test_session *sp, gchar *strpath )
{
  __cunit_foreach( sp, strpath, CUNIT_TEST, __cunit_print_entity, NULL, -1 ) ;
}

static void
__cunit_setup_context( GPtrArray *ptrpath, cunit_contextp *context )
{
  gint i, res = 0 ;
  cunit_test_suite *sp ;

  for( i = 0 ; i < ptrpath->len ; ++i ) {
    sp = ((cunit_test_suite*)g_ptr_array_index( ptrpath, i )) ;
    
    if( CUNIT_ENTITY_IS_TEST( sp ))
      if (i == ptrpath->len - 1) continue ;
      else g_error("__cunit_setup_context(): Found a test in ptrpath ('%s')", __cunit_ptrpath2strpath( ptrpath ) ) ;
    
    if((sp->setup != NULL) && (sp->setup( context ) != 0 )) {
      g_ptr_array_set_size( ptrpath, i + 1 ) ;
      g_error("setup failed ('%s')", __cunit_ptrpath2strpath( ptrpath ) ) ;
    }

  }
  
}


static void
__cunit_teardown_context( GPtrArray *ptrpath, cunit_contextp *context )
{
  gint i ;
  cunit_test_suite *sp ;

  for( i = ptrpath->len - 1 ; i > 0 ; --i ) {
    sp = ((cunit_test_suite*)g_ptr_array_index( ptrpath, i )) ;
    
    if( CUNIT_ENTITY_IS_TEST( sp ) )
      if (i == ptrpath->len - 1) continue ;
      else g_error("__cunit_teardown_context(): Found a test in ptrpath ('%s')", __cunit_ptrpath2strpath( ptrpath ) ) ;
    
    if((sp->teardown != NULL) && (sp->teardown( context ) != 0 )) {
      g_ptr_array_set_size( ptrpath, i + 1 ) ;
      g_error("teardown failed ('%s')", __cunit_ptrpath2strpath( ptrpath ) ) ;
    }
    
  }
  
}

static void
__cunit_run_test( GPtrArray *ptrpath, void* datap ) 
{
  gchar *strpath = NULL, *cp = NULL ;
  cunit_test_session *session ;
  cunit_contextp context ;
  cunit_test *tp ;
  
  session = (cunit_test_session*)datap ;

  session->number_of_tests_run = session->number_of_tests_run + 1 ;

  tp = (cunit_test*)g_ptr_array_index( ptrpath, ptrpath->len - 1 ) ;

  if( session->report_level != 0)
    strpath = __cunit_ptrpath2strpath( ptrpath ) ;

  if( session->report_level & CUNIT_REPORT_ALL )
    g_print("cunit: %s ", strpath ) ;

  __cunit_setup_context( ptrpath, & context ) ;
  cp = tp->run( context ) ;
  __cunit_teardown_context( ptrpath, & context ) ;

  if((session->report_level & CUNIT_REPORT_ALL) && (cp == NULL))
    g_print("PASS\n", strpath ) ;

  if((session->report_level & (CUNIT_REPORT_ALL|CUNIT_REPORT_FAIL)) && (cp != NULL)) {
    
    if(! (session->report_level & CUNIT_REPORT_ALL))
      g_print("cunit: %s ", strpath ) ;
  
    g_print("FAIL :\ncunit:\t%s\n", cp ) ;
  }

  session->number_of_tests_passed = session->number_of_tests_passed + (cp==NULL?1:0) ;

  if( session->report_level != 0)
    g_free( strpath ) ;

  g_free( cp ) ;
}

void
cunit_run_session( cunit_test_session *session, gchar *strpath )
{
  __cunit_foreach( session, strpath, CUNIT_TEST, __cunit_run_test, session, -1 ) ;
  
  if( session->report_level & CUNIT_REPORT_SUMMARY )
    g_print("cunit: %d tests of %d passed.\n",
	    (session->number_of_tests_passed),
	    session->number_of_tests_run ) ;
}

gint
cunit_TestRunner( gint argc, gchar *argv[], cunit_build_session_fp build_session )
{
  gint i, list = 0, exit_code = -100, verbose = 1 ;
  gint report_level ;
  cunit_test_session *session ;
  gchar *strpath ;

  void ( *action )(cunit_test_session *, gchar* ) ;

  extern int optind, opterr, optopt;

  g_assert( build_session != NULL ) ;

  /* Check for a '--TestRunner' option */
  if( (argc < 2) || ((argc > 1) && (strcmp(argv[1], "--TestRunner") != 0)))
    return 0 ;

  optind = optind + 1 ;
  
  while (((i = getopt(argc, argv, "vqlL")) != -1) && (exit_code == -100))
    switch (i) {
    case 'v':
      verbose = verbose + 1 ;
      break ;
    case 'q':
      verbose = 0 ;
      break ;
    case 'l':
      list = 1 ; 
      break ;
    case 'L':
      list = 2 ; 
      break ;
    case '?': 
      exit_code = 0 ;
      break ;
    default :
      exit_code = -1 ;
    }
  
  if( exit_code != -100 ) {

    g_print("TestRunner usage: %s --TestRunner [-vvqlL] [SUITE|TEST]\n", argv[0] ) ;

    exit( exit_code ) ;

  } else
    exit_code = 0 ;
  
  switch ( verbose ) {
  case 0: report_level = CUNIT_REPORT_LEVEL_0 ; break ;
  case 1: report_level = CUNIT_REPORT_LEVEL_1 ; break ;
  case 2: report_level = CUNIT_REPORT_LEVEL_2 ; break ;
  default : report_level = CUNIT_REPORT_LEVEL_3 ; break ;
  }

  session = build_session() ;

  session->report_level = report_level ;

  
  if( list )
    if( list == 1 ) action = cunit_list_test ;
    else action = cunit_list_all ;
  else
    action = cunit_run_session ;

  if( optind < argc ) {

    for( i = optind ; i < argc ; ++i ) {
      action( session, argv[i] ) ;
      exit_code = exit_code - ((session->number_of_tests_run - session->number_of_tests_passed) != 0)  ;
      cunit_reset_test_session( session ) ;
    }
    
  } else {
    
    action( session, "" ) ;
    exit_code = exit_code - ((session->number_of_tests_run - session->number_of_tests_passed) != 0)  ;

  }
    
  cunit_free_test_session( session ) ;

  exit( exit_code ) ;

}











