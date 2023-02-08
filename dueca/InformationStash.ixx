//  -*-C++-*-
/* ------------------------------------------------------------------   */
/*      item            : InformationStash.ixx
        made by         : Rene' van Paassen
        date            : 061221
        category        : body file
        description     :
        changes         : 061221 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define InformationStash_cxx
#include "InformationStash.hxx"
#include "NodeManager.hxx"
#include "Callback.ixx"
#include "TimedServicer.hxx"
#include <dueca/ChannelWriteToken.hxx>
#include <dueca/WrapSendEvent.hxx>

#define DEBPRINTLEVEL 0
#include <debprint.h>

DUECA_NS_START

static void do_nothing() {}

template<class T>
InformationStash<T>::InformationStash(const char* name) :
  StateGuard(name, false),
  id(NULL),
  w_info(NULL),
  _stash(3, name),
  name(name),
  sequence(0),
  direct(false),
  service_id(0U),
  work2(do_nothing)
  //srvc(NULL)
{

}

template<class T>
InformationStash<T>::~InformationStash()
{
  if (_stash.size()) {
    DEB("InformationStash \"" << name << "\" has " << _stash.size()
        << " items unsent");
  }
  if (service_id != 0U) {
    TimedServicer::releaseService(service_id);
  }
}

template<class T>
void InformationStash<T>::initialise(unsigned nreserved, bool install_service)
{
  // very first time, initialise stuff.
  if (!id) {
    DEB1("InformationStash initialise, channel " << name << "://dueca");
    id = new EasyId("dueca", name, static_node_id);
    w_info = new ChannelWriteToken
      (id->getId(), NameSet("dueca", name, ""), T::classname, "",
       Channel::Events, Channel::OneOrMoreEntries,
       Channel::OnlyFullPacking, Channel::Bulk, NULL, nreserved);
  }

  if (!service_id && install_service) {
    DEB1("InformationStash " << name << " service callback");

    service_id = TimedServicer::requestService
      ([this] () { this->service(); });
  }
}

template<class T>
template<class U>
void InformationStash<T>::initialise(InformationStash<U>* dependent,
                                     unsigned nreserved,
                                     bool install_service)
{
  work2 = [dependent]() { dependent->service(); };

  // very first time, initialise stuff.
  if (!id) {
    DEB1("InformationStash initialise, channel " << name << "://dueca");
    id = new EasyId("dueca", name, static_node_id);
    w_info = new ChannelWriteToken
      (id->getId(), NameSet("dueca", name, ""), T::classname, "",
       Channel::Events, Channel::OneOrMoreEntries,
       Channel::OnlyFullPacking, Bulk, NULL, nreserved);
  }

  if (!service_id && install_service) {
    DEB1("InformationStash " << name << " service callback");

    service_id = TimedServicer::requestService
      ([this] () { this->service(); });
  }
}

template<class T>
void InformationStash<T>::service()
{
  work2();
  if (w_info == NULL || !w_info->isValid()) return;

  while (_stash.notEmpty()) {
    AsyncQueueReader<const T*> r(_stash);
    DEB2("InformationStash idx " << sequence << " send " << *r.data());
    wrapSendEvent(*w_info, r.data(), sequence++);
  }
}

template<class T>
unsigned long InformationStash<T>::stash(const T* i)
{
  AsyncQueueWriter<const T*> w(_stash);
  w.data() = i;

  DEB1("InformationStash adding " << *i);

  // note, that as the w destruction and actual storage takes place
  // after the return, this size check returns the size *without* the
  // present addition
  return _stash.size();
}


#include <undebprint.h>

DUECA_NS_END
