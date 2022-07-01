/* ------------------------------------------------------------------   */
/*      item            : TimedServicer.hxx
        made by         : Rene van Paassen
        date            : 140404
        category        : header file
        description     :
        changes         : 140404 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef TimedServicer_hxx
#define TimedServicer_hxx

#include "NamedObject.hxx"
#include "NameSet.hxx"
#include "Callback.hxx"
#include "PrioritySpec.hxx"
#include "PeriodicAlarm.hxx"
#include "Activity.hxx"
#include "dueca_ns.h"
#include <algorithm>

DUECA_NS_START;

/** Service class for triggering cleanup jobs */
class TimedServicer: public NamedObject
{
public:
  /** Callback function type */
  typedef std::function<void (void)> callback_t;

private:
  /** Definition of a client */
  struct Client
  {
    // callback function
    callback_t fcallback;

    // function number
    unsigned client_id;

    // Next service callback
    Client *next;

    // Constructor
    Client(const callback_t &fcn, Client* next=NULL, unsigned client_id=0U);
  };

  /** The services */
  Client* clientlist;

  /** Current client */
  Client* currentclient;

  /** Number of clients */
  unsigned nclients;

  /** Ticks per second */
  unsigned nticks;

  /** Constructor */
  TimedServicer();

  /** Destructor */
  ~TimedServicer();

  /** Callback object */
  Callback<TimedServicer> cb1;

  /** The activity */
  ActivityCallback do_service;

  /** Clock to provide the timing */
  PeriodicAlarm* alarm;

  /** Function that does the work */
  void checkservice(const TimeSpec& ts);

  /** Tell that this is a basic object of dueca. */
  ObjectType getObjectType() const {return O_Dueca;}

  friend class ServiceCallbackBase;

  /** Request a new service */
  unsigned _requestService(const callback_t &callback);

  /** Release the service */
  void _releaseService(unsigned svcid);

  /** Access the TimedServicer singleton */
  static TimedServicer& single();

  /** Start the servicer */
  void _startService();

public:
  /** Start the servicer */
  inline static void startService()
  { single()._startService(); }

  /** Add or remove service request */
  inline static unsigned requestService(const callback_t &callback)
  { return single()._requestService(callback); }

  inline static void releaseService(unsigned svcid)
  { single()._releaseService(svcid); }
};

DUECA_NS_END;

#endif
