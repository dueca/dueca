/* ------------------------------------------------------------------   */
/*      item            : debug-direct.cxx
        made by         : Rene' van Paassen
        date            : 20070413
        category        : implementation file
        description     : Implements a time writing function
        changes         :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#include <dueca-conf.h>
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <iostream>
#include <iomanip>
#include <dueca_ns.h>

DUECA_NS_START

void writeclock()
{
#if defined(HAVE_SYS_TIME_H)
  timeval tv;
  gettimeofday(&tv, NULL);
  char buf[26]; ctime_r(&tv.tv_sec, buf); buf[19]='\000';
  std::cerr << &buf[11] << '.' << std::setfill('0') << std::setw(6)
            << tv.tv_usec << std::setfill(' ');
#else
#error "No way to measure time"
#endif
}

DUECA_NS_END
