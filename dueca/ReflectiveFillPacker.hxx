/* ------------------------------------------------------------------   */
/*      item            : ReflectiveFillPacker.hxx
        made by         : Rene van Paassen
        date            : 010814
        category        : header file
        description     :
        changes         : 010814 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ReflectiveFillPacker_hxx
#define ReflectiveFillPacker_hxx

#ifdef ReflectiveFillPacker_cxx
#endif

#include <TimeSpec.hxx>
#include <Callback.hxx>
#include <Activity.hxx>
#include <FillSet.hxx>
#include <GenericPacker.hxx>
#include <AsyncList.hxx>
#include <iostream>

#include <dueca_ns.h>
DUECA_NS_START
class ReflectiveStoreInformation;
class AmorphStore;
struct ParameterTable;
class ChannelWriteToken;

/** Objects that uses a reflective/shared memory communications area
    to transport -- slowly -- large bulk data objects. */
class ReflectiveFillPacker: public GenericPacker
{
  /// Pointer to a vector of (2) stores in which to pack data.
  AmorphStore* store;

  /// Keeps a tab on the store currently used for packing.
  int store_to_fill;

  /// and a tab on the store currently being sent
  int store_to_send;

  /** Counter for the number of bytes still to be sent from the store
      that is currently being sent. */
  int bytes_to_send;

  /** counter for the number of bytes that are sent in the current
      sending store. */
  int index_to_send;

  /// Total number of stores.
  static const int no_of_stores;

  /** Size of the packets (maximum) I will send over */
  int packet_size;

  /** Time specification of the sending process. */
  PeriodicTimeSpec time_spec;

  /** Size of the stores. */
  int store_size;

  /// Flag to remember to start the sending
  int wait_period;

  /** Helper function, which packs one dataset. If exceptions are
      enabled, this throws an exception if the store is
      full. Otherwise it returns false if filling was not successful. */
  bool packOneSet(AmorphStore& s, const PackUnit& c);

  /** access token for sending data. This indirectly uses the normal
      transmission channels. */
  ChannelWriteToken *out;

  /** Method that does the data sending. */
  void sendAPiece(const TimeSpec& ts);

  /** Callback function object. */
  Callback<ReflectiveFillPacker> cb1;

  /** Activity that checks whether there is stuff to send, and sends
      it. */
  ActivityCallback               send_stuff;

public:
  /// This class can be created from scheme
  SCM_FEATURES_DEF;

  /** Return table with tunable parameters. */
  static const ParameterTable* getParameterTable();
public:
  /** Constructor. */
  ReflectiveFillPacker();

  /** Continue after construction and tunable parameter setting. */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /// Destructor.
  ~ReflectiveFillPacker();

  /** initialisation, called from the Accessor, to tell us who we are
      in this context (send order). This determines the channel name
      we will send over. At this point also, the channel can be made. */
  void initialise(const ReflectiveStoreInformation& i);

  /** Stop packing, used at destruction time. */
  void stopPacking();

private:
  /** Internal routine, pack all the stuff left. */
  void packWork();
};

DUECA_NS_END
#endif
