/* ------------------------------------------------------------------   */
/*      item            : Su.cxx
        made by         : Rene' van Paassen
        date            : 010620
        category        : body file
        description     :
        changes         : 010620 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define Su_cxx
#include "Su.hxx"

#include <debug.h>
#include <dueca-conf.h>
#include <unistd.h>
#include <iostream>
using namespace std;
DUECA_NS_START

Su* Su::singleton = NULL;

Su::Su() :
#ifdef HAVE_SETUID
  orig_user(geteuid()),
  have_tried(orig_user == 0),
  is_capable(orig_user == 0)
#else
  orig_user(1),
  have_tried(false),
  is_capable(false)
#endif
{
  //
}

Su& Su::single()
{
  if (singleton == NULL) {
    singleton = new Su();
  }
  return *singleton;
}

Su::~Su()
{
  //
}

bool Su::isCapable()
{
  if (!have_tried) {
#ifdef HAVE_SETUID
    if (seteuid(0)) {
      /* DUECA timing.

         Failing to acquire superuser priviledges. Note that superuser
         priviledges are normally not needed, it is better to specify
         real-time permissions (memory lock and priority) with PAM.
      */
      W_SYS("Su: no root/super user capabilities");
      is_capable = false;
    }
    else {
      is_capable = true;

      if (seteuid(orig_user) != 0) {
      /* DUECA timing.

         Failing to revert superuser priviledges. Note that superuser
         priviledges are normally not needed, it is better to specify
         real-time permissions (memory lock and priority) with PAM.
      */
        W_SYS("Problem reverting to original user id");
      }
    }
#else
    /* DUECA timing.

       No configuration for superuser priviledges. Note that superuser
       priviledges are normally not needed, it is better to specify
       real-time permissions (memory lock and priority) with PAM.
    */
    W_SYS("Su: no setuid configured, no setuid capabilities");
#endif
    have_tried = true;
  }
  return is_capable;
}

bool Su::acquire()
{
#ifdef HAVE_SETUID
  if (seteuid(0) != 0) {
    is_capable = false;
  }
  else {
    is_capable = true;
  }
  have_tried = true;
#endif
  return is_capable;
}

bool Su::revert()
{
#ifdef HAVE_SETUID
  if (orig_user != 0 && geteuid() == 0) {
    if (seteuid(orig_user) != 0) {
      /* DUECA timing.

         Failing to revert superuser priviledges. Note that superuser
         priviledges are normally not needed, it is better to specify
         real-time permissions (memory lock and priority) with PAM.
      */
      W_SYS("Problem reverting to original user ID");
    }
    return true;
  }
#endif
  return true;
}
DUECA_NS_END
