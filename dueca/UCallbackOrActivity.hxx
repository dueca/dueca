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

#include "DataTimeSpec.hxx"
#include <Trigger.hxx>
#include <cstddef>
#include <dueca_ns.h>

DUECA_NS_START;

class Activity;
class GenericCallback;
class TimeSpec;

/** Small helper class, accepts either a callback or activity, and will do
    only one call or activation.

    This is used to signal read or write channel access token validity.

    Using a GenericCallback* there, gives an immediate callback on validity
    in the current thread, or a later callback with priority 0.

    By using an "Activity", you can control the priority where the callback
    will be done.
  */
class UCallbackOrActivity : private TriggerPuller
{
  /** If with activity */
  Activity *act;

  /** if with callback */
  GenericCallback *cb;

public:
  /** Constructor with callback */
  UCallbackOrActivity(Activity *act);

  /** Constructor activity */
  UCallbackOrActivity(GenericCallback *cb);

  /** Constructor from a null pointer */
  UCallbackOrActivity(std::nullptr_t ncb);

  /** Empty */
  UCallbackOrActivity();

  /** Copy constructor */
  UCallbackOrActivity(const UCallbackOrActivity &o);

  /** Run the thing - once only */
  void operator()(const TimeSpec &ts);

  /** Reset-null */
  void reset()
  {
    act = NULL;
    cb = NULL;
  }

  /** Destructor */
  ~UCallbackOrActivity();

  /** Boolean value */
  inline operator bool() const { return act != NULL || cb != NULL; }
};

DUECA_NS_END;
