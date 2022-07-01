/* ------------------------------------------------------------------   */
/*      item            : IPMulticastAccessor.hxx
        made by         : Rene' van Paassen
        date            : 990611
        category        : header file
        description     :
        changes         : 990611 first version
                          061006 changed to use CoreCreator
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef IPMulticastAccessor_hxx
#define IPMulticastAccessor_hxx

#include "IPAccessor.hxx"

#define MAXHOSTNAME 256

#include <dueca_ns.h>
DUECA_NS_START

/** Multicast-based communication over IP.

    The IPMulticastAccessor accesses ethernet hardware for
    communication with one or more other hosts. It uses the multicast
    protocol.

    These objects are normally created from a scheme script:
    \code
    (make-ip-multicast-accessor
     packer  unpacker
     store-size n-stores
     input-size n-inputs  ... etc.
    \endcode
*/
class IPMulticastAccessor :
  public IPAccessor
{
  /** Address of own interface. */
  vstring if_address;

  /** Address of multicast target. */
  vstring mc_address;

  /** Base port number. */
  int base_port;

  /** Enable port re-use. */
  bool port_re_use;

  //using IPAccessor::log_communications;
public:
  SCM_FEATURES_DEF;

  /** Constructor, normally called from a scheme script. */
  IPMulticastAccessor();

  /** Table with tunable parameters. */
  static const ParameterTable* getParameterTable();

  /** Check and complete construction. */
  bool complete();

  /** Destructor. */
  virtual ~IPMulticastAccessor();

private:
  /** Copy constructor is not implemented. */
  IPMulticastAccessor(const IPMulticastAccessor&);
};

DUECA_NS_END
#endif
