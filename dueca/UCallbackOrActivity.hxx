/* ------------------------------------------------------------------   */
/*      item            : UCallbackOrActivity.hxx
        made by         : Rene van Paassen
        date            : 240910
        category        : header file
        description     : Small helper class for handling both direct
                          callback (mgr 0) and activities in channel
                          validation.
        changes         : 240910 first version
        language        : C++
        copyright       : (c) 2024 Rene van Paassen
*/

#pragma once

#include <Trigger.hxx>
#include "DataTimeSpec.hxx"
#include <dueca_ns.h>

DUECA_NS_START;

class Activity;
class GenericCallback;
class TimeSpec;

/** Small helper class, accepts either a callback or activity, and will do 
    only one call or activation. */
class UCallbackOrActivity: private TriggerPuller
{
  /** If with activity */
  Activity* act;

  /** if with callback */
  GenericCallback* cb;

public:
  /** Constructor with callback */
  UCallbackOrActivity(Activity* act);

  /** Constructor activity */
  UCallbackOrActivity(GenericCallback* cb);
  
  /** Copy constructor */
  UCallbackOrActivity(const UCallbackOrActivity& o);

  /** Run the thing - once only */
  void operator() (const TimeSpec& ts);

  /** Reset-null */
  void reset() { act = NULL; cb = NULL; }

  /** Destructor */
  ~UCallbackOrActivity();

  /** Boolean value */
  inline operator bool() const { return act != NULL || cb != NULL; } 
};

DUECA_NS_END;
