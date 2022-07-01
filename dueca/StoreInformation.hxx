/* ------------------------------------------------------------------   */
/*      item            : StoreInformation.hxx
        made by         : Rene van Paassen
        date            : 010412
        category        : header file
        description     :
        changes         : 010412 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef StoreInformation_hxx
#define StoreInformation_hxx

#ifdef StoreInformation_cxx
#endif

#include <inttypes.h>
#include <iostream>
using namespace std;
#include <dueca_ns.h>

DUECA_NS_START
class ReflectiveAccessor;

/** Set of information for initialisation of packers by their media
    accessors. */
class StoreInformation
{
public:
  /** Constructor. */
  StoreInformation() { }

  /** Destructor. */
  virtual ~StoreInformation() { }
};

/** Information about the layout of reflective memory for sending
    stream and event data. */
class ReflectiveStoreInformation: public StoreInformation
{
public:
  /// Shared memory accessor
  ReflectiveAccessor* accessor;

  /// location of the communications control buffers
  volatile uint32_t** area_cb;

  /// location of the communications areas
  volatile uint32_t** area;

  /// Size of each of the communications buffers
  int area_size;

  /// Number of nodes using reflective memory (and thus reading this,
  /// == no of communications buffers)
  int no_parties;

  /// Id of this node.
  int node_id;

  /** Constructor.
      \param accessor       pointer to the reflective accessor.
      \param area_cb        array, of length reflect_nodes, with
                            pointers to the control buffers of the
                            communication areas
      \param area           array, of length reflect_nodes, with
                            pointers to the communication areas
      \param area_size      size of each of the communication areas
      \param reflect_nodes  no of participants in this communication
      \param node_id        identification number of this node */
  ReflectiveStoreInformation(ReflectiveAccessor* accessor,
                             volatile uint32_t** area_cb,
                             volatile uint32_t** area,
                             int area_size,
                             int reflect_nodes,
                             int node_id);

  /** Print to a stream. */
  friend ostream& operator << (ostream& os, const
                               ReflectiveStoreInformation& o);
};

#ifdef DELETED_BY_WEST
/** A specialisation of the above for transportation that is performed
    by copying stuff over a link. */

class TransmittingStoreInformation: public StoreInformation
{
public:
  /// Array with areas for communication (buffers)
  int** areas;

  /// Status of each area
  int* store_status;

  /// Number of areas/store used
  int n_stores;

  /// Size of each individual store
  int store_size;

  /// Constructor
  TransmittingStoreInformation(int** areas, int* store_status,
                               int n_stores, int stone_size);
};
#endif /* DELTED_BY_WEST */

DUECA_NS_END
#endif
