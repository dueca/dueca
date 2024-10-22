/* ------------------------------------------------------------------   */
/*      item            : UCallbackOrActivity.hxx
        made by         : Rene van Paassen
        date            : 240910
        category        : header file
        description     : Small helper class for handling both direct
                          callback (mgr 0) and activities in channel
                          validation.
        changes         : 240910 first version
        api             : DUECA_API
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

/** Small helper class to provide flexibility in the creation of 
    ReadAccessToken and WriteAccessToken, accepts either a callback 
    or activity, and will do only one call or activation.

    This class features constructors that enable implicit conversion from
    a GenericCallback pointer (GenericCallback*) or from an Activity/
    ActivityCallback pointer, when creating a ReadAccessToken or a
    WriteAccessToken. You can thus use a pointer to either of these objects,
    instead of a UCallbackOrActivity in your constructor call.

    When the token becomes valid, the callback or activity is called.

    When using a GenericCallback*, the callback is immediate in some cases
    when the token is implicitly vallid. If the callback comes later, it
    will be handled by the administration thread ActivityManager (priority 0).

    When using an Activity, you must supply the priority where the callback
    will be done. By taking the same priority as the process that uses the
    token to write or read the data, you can avoid threading problems. 
    Validity is in this case always signalled later, as a separate activity.
  */
class UCallbackOrActivity : private TriggerPuller
{
  /** If with activity */
  Activity *act;

  /** if with callback */
  GenericCallback *cb;

public:
  /** Constructor from an Activity pointer. Note that the pointer must
      remain valid for the lifetime of the token where this is used. */
  UCallbackOrActivity(Activity *act);

  /** Constructor from a GenericCallback pointer. Note that the pointer must
      remain valid for the lifetime of the token where this is used. */
  UCallbackOrActivity(GenericCallback *cb);

  /** Constructor from a null pointer. Will not perform a callback. */
  UCallbackOrActivity(std::nullptr_t ncb);

  /** Empty constructor. Will also not perform a callback. */
  UCallbackOrActivity();

  /** Copy constructor */
  UCallbackOrActivity(const UCallbackOrActivity &o);

  /** Run the thing - is effective once only */
  void operator()(const TimeSpec &ts);

  /** Reset to null */
  void reset()
  {
    act = NULL;
    cb = NULL;
  }

  /** Destructor */
  ~UCallbackOrActivity();

  /** Test whether any callback can be run. */
  inline operator bool() const { return act != NULL || cb != NULL; }
};

DUECA_NS_END;
