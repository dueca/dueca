/* ------------------------------------------------------------------   */
/*      item            : IPTwoWay.hxx
        made by         : Rene' van Paassen
        date            : 001102
        category        : header file
        description     :
        changes         : 001102 first version
                          061006 changed to use CoreCreator
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef IPTwoWay_hxx
#define IPTwoWay_hxx

#include "IPAccessor.hxx"

#include <dueca_ns.h>
DUECA_NS_START
/** Two-way (send and receive) or one-way (only send) communication
    over IP.

    The IPTwoWay class accesses ethernet hardware for communication
    with one other host. It is of general, but of course limited
    use. It can also do one-way communication, only sending. This
    class is often used for special-purpose communication, e.g. to
    FlightGear for visual generation. */
class IPTwoWay:
  public IPAccessor
{
  /** Address of own interface. */
  vstring ip_address;

  /** Address of other host. */
  vstring if_address;

  /** Base port number. */
  int base_port;

public:
  SCM_FEATURES_DEF;

  /** Constructor., normally called from a scheme script. */
  IPTwoWay();

  /** Table with tunable parameters. */
  static const ParameterTable* getParameterTable();

  /** Check and complete construction. */
  bool complete();

  /** Destructor. */
  virtual ~IPTwoWay();

  /** Send first option. */
  bool selectSendFirst(const bool& first);

  /** One way sending. */
  bool selectOneWay(const bool& oneway);

private:
  /** Copy constructor, these objects should not be copied. */
  IPTwoWay(const IPTwoWay&);
};

DUECA_NS_END
#endif
