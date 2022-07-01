/* ------------------------------------------------------------------   */
/*      item            : IPBroadcastAccessor.hxx
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

#ifndef IPBroadcastAccessor_hxx
#define IPBroadcastAccessor_hxx

#include "IPAccessor.hxx"

#define MAXHOSTNAME 256

#include <dueca_ns.h>
DUECA_NS_START

/** Broadcast-based communication over IP.

    The IPBroadcastAccessor accesses ethernet hardware for
    communication with one or more other hosts. It uses the broadcast
    protocol.

    These objects are normally created from a scheme script:
    \code
    (make-ip-broadcast-accessor
     packer  unpacker
     store-size n-stores
     input-size n-inputs  ... etc.
    \endcode
*/
class IPBroadcastAccessor :
  public IPAccessor
{
  /** Address of own interface. */
  vstring if_address;

  /** Address of multicast target. */
  vstring bc_address;

  /** Base port number. */
  int base_port;

public:
  SCM_FEATURES_DEF;

  /** Constructor, normally called from a scheme script. */
  IPBroadcastAccessor();

  /** Table with tunable parameters. */
  static const ParameterTable* getParameterTable();

  /** Check and complete construction. */
  bool complete();

  /** Destructor. */
  virtual ~IPBroadcastAccessor();

private:
  /** Copy constructor is not implemented. */
  IPBroadcastAccessor(const IPBroadcastAccessor&);
};

DUECA_NS_END
#endif
