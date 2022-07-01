/* ------------------------------------------------------------------   */
/*      item            : TimedServicer.cxx
        made by         : Rene' van Paassen
        date            : 140404
        category        : body file
        description     :
        changes         : 140404 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define TimedServicer_cxx
#include "TimedServicer.hxx"
#include "Ticker.hxx"
#include "Callback.ixx"
#include <math.h>

DUECA_NS_START;

static void do_nothing() { }

TimedServicer::Client::Client(const callback_t &fcn,
                              Client* next,
                              unsigned client_id) :
  fcallback(fcn),
  client_id(client_id),
  next(next)
{ }

TimedServicer::TimedServicer() :
  NamedObject(NameSet("TimedServicer://dueca")),
  clientlist(new Client(do_nothing, 0)),
  currentclient(clientlist),
  nclients(1U),
  nticks(0),
  cb1(this, &TimedServicer::checkservice),
  do_service(getId(), "general dueca service", &cb1, PrioritySpec(0, 0)),
  alarm(NULL)
{
  //
}

void TimedServicer::_startService()
{
  nticks =
    std::max(1, int(round(1.0/(Ticker::single()->getCompatibleIncrement()*
                               Ticker::single()->getTimeGranule())) ));
  alarm = new PeriodicAlarm
    (TimeSpec(0, Ticker::single()->getCompatibleIncrement()));
  do_service.setTrigger(*(alarm));
  do_service.switchOn(TimeSpec(0, 0));
}

TimedServicer::~TimedServicer()
{
  do_service.switchOff(TimeSpec(0, 0));
  delete alarm;
}

void TimedServicer::checkservice(const TimeSpec& ts)
{
  for (int ii = std::max(1U, nclients/nticks); ii--; ) {
    /* if (currentclient->callback) {
      currentclient->callback->service();
      } */
    currentclient->fcallback();

    // cleanup
    while (currentclient->next != NULL &&
           currentclient->next->client_id == 0 &&
           currentclient->next->next != NULL) {
      auto tmp = currentclient->next;
      currentclient->next = currentclient->next->next;
      delete(tmp);
    }
    currentclient = currentclient->next;
    if (!currentclient) currentclient = clientlist;
  }
}


TimedServicer& TimedServicer::single()
{
  static TimedServicer thesingle;
  return thesingle;
}

unsigned TimedServicer::_requestService(const callback_t& callback)
{
  Client* newclient = new Client(callback, clientlist, nclients);
  while (!__sync_bool_compare_and_swap(&(clientlist),
                                       newclient->next,
                                       newclient)) {
    newclient->next = clientlist;
  }
  return nclients++;
}

void TimedServicer::_releaseService(unsigned svcid)
{
  auto client = clientlist;
  while (client != NULL && client->client_id != svcid) {
    client = client->next;
  }
  if (client != NULL) {
    client->fcallback = do_nothing;
    client->client_id = 0;
  }
}

DUECA_NS_END;
