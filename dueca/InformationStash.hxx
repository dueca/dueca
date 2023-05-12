/* ------------------------------------------------------------------   */
/*      item            : InformationStash.hxx
        made by         : Rene van Paassen
        date            : 061221
        category        : header file
        description     :
        changes         : 061221 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef InformationStash_hxx
#define InformationStash_hxx

#include <dueca_ns.h>
#include <list>
#include <EasyId.hxx>
#include <TimeSpec.hxx>
#include <StateGuard.hxx>
#include <AsyncQueueMT.hxx>
#include <ChannelWriteToken.hxx>
#include <boost/scoped_ptr.hpp>
#include "TimedServicer.hxx"
#include <Callback.hxx>

DUECA_NS_START

/** Placeholder function */
void InformationStash_do_nothing();

/** This is a helper object for dueca components, for sending
    information to central logging/information points. It keeps data
    while not initialised, and sends stuff after a call to the
    initialisation. Since oct 2014, this class is multi-threading
    safe, so you can call the .stash() method from any thread.

    Simple example:
    \code
    class MyClass
    {
       static InformationStash<MyInfo>  stash;

       ....
    };
    \endcode

    Declare the object:
    \code
    InformationStash<MyInfo> MyClass::stash("channel-class");
    \endcode

    To start sending the data:
    \code
    stash.initialise();
    \endcode
    Note that you can use the .stash() method before initialisation;
    the initialise() method simply starts the sending, and empties the
    stash.

    And in some method
    \code
    stash.stash(new MyInfo(....));
    \endcode

    If you have a class that may be called at global initialisation,
    be a little more careful, because the stash object might not have
    been initialised:
    \code
    class MyClass
    {
       ....
       InformationStash<MyInfo>& stash();
    };

    // in c file
    InformationStash<MyInfo>& MyClass::stash()
    {
      static InformationStash<MyInfo> _stash("channel-class");
      return _stash;
    }

    // and then to use:
    this->stash().stash(new MyInfo(.....));
    \endcode

    The InformationStash object by default auto-empties itself. If you
    select not to auto-empty the stash, then the stash can only be
    emptied by (regularly) calling the service() method.
*/
template<class T>
class InformationStash: private StateGuard
{
protected:

  /** Id. */
  EasyId                       *id;

  /** Pointer to the token for writing the information. */
  ChannelWriteToken            *w_info;

  /** Lists of temporary information items, remembered while channel
      not initialised/valid. */
  AsyncQueueMT<const T*>        _stash;

  /** Name of the channel, one level only, entity always "dueca". */
  const char*                   name;

  /** Timespec mis-used as sequence. */
  TimeTickType                  sequence;

  /** Boolean to remember whether doing direct or using stash. */
  bool                          direct;

  /** ID for the service. */
  unsigned                      service_id;

  /** service request for regular invocation to process sending */
  //boost::scoped_ptr<ServiceCallback<InformationStash<T> > > srvc;
  std::function<void ()>        work2;

public:
  /** Constructor.
      \param   name     Distinguishing name for channel.
      \param   auto_run Will start sending stashed information once
                        the channel is valid, and afterwards directly
                        send to the channel. Otherwise, the
                        feedStashed function needs to be called
                        "manually" to empty the stash. */
  InformationStash(const char* name);

  /** Destructor. Checks for an empty stash at the end, and warns */
  ~InformationStash();

  /** Initialisation.  */
  void initialise(unsigned nreservations = 1, bool install_service = true);

  /** Initialisation. */
  template<class U>
  void initialise(InformationStash<U>* dependent, unsigned nreservations = 1,
                  bool install_service = true);

  /** Feed all stashed stuff, called by the service callback. */
  virtual void service();

  /** Send or save information.
      @param   i        Data to be sent; the object will be
                        de-allocated by the stash/channel
      @returns          The number of items that were already in the
                        stash
  */
  unsigned long stash(const T* i);

  /** Return an appropriate object id, is required for activities. */
  inline const GlobalId& getId() const
  {
    static GlobalId no_id;
    if (id) return id->getId();
    return no_id;
  }
};

DUECA_NS_END

#endif
