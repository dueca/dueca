/* ------------------------------------------------------------------   */
/*      item            : ReflectivePacker.hxx
        made by         : Rene van Paassen
        date            : 010404
        category        : header file
        description     :
        changes         : 010404 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ReflectivePacker_hxx
#define ReflectivePacker_hxx

#ifdef ReflectivePacker_cxx
#endif

#include <GenericPacker.hxx>
#include <AmorphStore.hxx>
#include <CyclicInt.hxx>
#include <inttypes.h>
#include <Callback.hxx>
#include <Activity.hxx>
#include <EasyId.hxx>
using namespace std;
#include <dueca_ns.h>
DUECA_NS_START
class ReflectiveAccessor;
struct ParameterTable;

/** Objects that assemble event data and send it over a reflective
    memory system, and send notices of stream data changes over a
    reflective system. */
class ReflectivePacker : public GenericPacker, public TriggerPuller
{
  /** The accessor for our piece of shared memory. */
  ReflectiveAccessor* accessor;

  /** Pointer to the portion of that buffer for control data.  Each
      node, including the current one, has a field in this control
      buffer.

      The field of this ReflectivePacker,
      stream_area_cb[reflect_node_id], gives the offset in the stream
      area where the next write will take place. Initially this offset
      will be one, i.e. the writing starts almost at the beginning of
      the buffer (there is one sentinel word).

      The fields of the other nodes, managed by the reflective
      accessor, give the offset in the stream area that has been most
      recently read by the respective node. Initially they will
      contain the last element in the stream area. To avoid
      ambiguities, these will never be written by the writer
      (i.e. there is a sentinel word, otherwise having to read an
      entire buffer would equal having to read 0 words). */
  volatile uint32_t* area_cb;

  /** Array of pointers to the communication areas. */
  volatile uint32_t* area;

  /** Element of the circular buffer that is written to. */
  unsigned int area_write_idx;

  /** Size of the stream notification buffer. */
  unsigned int  area_size;

  /** Number of nodes in the reflective memory usage. */
  int no_reflect_nodes;

  /** Id of this node in the reflective memory usage. */
  int reflect_node_id;

  /** temporary buffer for packing object data */
  uint32_t* object_buffer;

  /** Size of the temporary buffer */
  int object_buffer_size;

  /** Store object. Here the channel data is packed, checksums are
      added, and the stuff in the store is copied word-by-word into
      the reflective or shared memory area.

      The store contents are kept after a copy action. For the next
      packing action, these contents are checked. If the check is
      successfull, the store is recycled. */
  AmorphStore store;

  /** Callback object */
  Callback<ReflectivePacker>      cb1;

  /** An ID for the following activity */
  EasyId                          id;

  /** Activity, ensures that queued work is performed in a specified
      thread */
  ActivityCallback                pack_activity;

  volatile bool                   scheduled;

public:
  /** Objects of this class can be constructed from scheme */
  SCM_FEATURES_DEF;

  /** Obtain a pointer to the table with tunable parameters. */
  static const ParameterTable* getParameterTable();
public:
  /** Constructor. Normally called from Scheme initialisation script. */
  ReflectivePacker();

  /** Completion call, called after all parameters are set. */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Destructor. */
  ~ReflectivePacker();

  /** Initialise the communication buffers. This works differently
      from IP-based packers, but we are using the same interface.
      \param i   A StoreInformation object that contains the sizes and
                 locations of the stores, and the number of nodes
                 using the shared memory, and the node id of this node
                 in that number. */
  void initialiseStoresR(StoreInformation& i);

  /** Report (by a channel) that some data has to be
      packed. Overridden from the GenericPacker version, since here
      each notification triggers activity.
      \param entry    Channel entry that requested the pack
      \param ts       Time for which the transport has to be done,
      \param idx      Index of the notification, so that the entry
                      can differentiate between packing clients. */
  void notification(UChannelEntry* entry,
                    TimeTickType ts,
                    unsigned idx);

  /** Start the packing activities. */
  void start(ReflectiveAccessor* acc);

  /** Stop packing, used at destruction time. */
  void stopPacking();

  /** Pack the notification */
  void packWorkR(const TimeSpec& ts);

  /** Not used in this case. */
  int changeCurrentStore(int& store_no);

  /** Print the packer, for debugging purposes. */
  friend ostream& operator << (ostream& os, const ReflectivePacker& p);

private:
  /** Check the status of all reading nodes, and calculate how much
      space is again available for writing. */
  int roundUpFree(volatile uint32_t *area, int size);

  /** Copy the stuff that was packed by the channel into the
      buffer. This also considers wrapping-around in the circular
      buffer.
      \param nwords   Number of wordt from the object_buffer to
                      copy.*/
  void copyBuffer(int nwords);

  /** Set a new packing priority */
  bool setPrioritySpec(const PrioritySpec& ps);
};

DUECA_NS_END
#endif
