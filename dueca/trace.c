/* ------------------------------------------------------------------   */
/*      item            : trace.c
        made by         : Rene' van Paassen
        date            : 151016
        category        : body file
        description     :
        changes         : 151016 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
*/

#define trace_c

#include <stdio.h>
#include <time.h>

static FILE *fp_trace = NULL;

void
__attribute__ ((no_instrument_function))
trace_begin (void)
{
 fp_trace = fopen("duecatrace.out", "w");
}

void
__attribute__ ((destructor,no_instrument_function))
trace_end (void)
{
 if(fp_trace != NULL) {
 fclose(fp_trace);
 }
}

void
__attribute__ ((no_instrument_function))
__cyg_profile_func_enter (void *func,  void *caller)
{
  if (!fp_trace) {
    trace_begin();
  }
  if(fp_trace != NULL) {
    fprintf(fp_trace, "e %p %p %lu\n", func, caller, time(NULL) );
  }
}

void
__attribute__ ((no_instrument_function))
__cyg_profile_func_exit (void *func, void *caller)
{
 if(fp_trace != NULL) {
 fprintf(fp_trace, "x %p %p %lu\n", func, caller, time(NULL));
 }
}
