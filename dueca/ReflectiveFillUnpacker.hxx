/* ------------------------------------------------------------------   */
/*      item            : ReflectiveFillUnpacker.hxx
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

#ifndef ReflectiveFillUnpacker_hxx
#define ReflectiveFillUnpacker_hxx

#ifdef ReflectiveFillUnpacker_cxx
#endif

#include <NamedObject.hxx>
#include <FillSet.hxx>
#include <EventAccessToken.hxx>
#include <Callback.hxx>
#include <Activity.hxx>
#include <AmorphStore.hxx>
#include <dueca_ns.h>
#include "ScriptCreatable.hxx"
DUECA_NS_START
class ChannelManager;
class ReflectiveStoreInformation;

/** Unpacks low-priority (bulk) messages.

    A ReflectiveFillUnpacker unpacks the low-priority bulk messages
    that are sent in the space left over by the normal messaging. This
    space is filled at the sending end by a FillPacker. A FillPacker
    message block may be divided over several normal messages, the
    ReflectiveFillUnpacker is called with the function
    changeCurrentStore when a message comes in with this data. It then
    schedules itself to be run in the requested priority (usually
    level 0), where it accesses the store and re-assembles the pieces
    found in the stores. It then proceeds to unpack data. */

class ReflectiveFillUnpacker:
  public ScriptCreatable,
  public NamedObject
{
  /** storage areas for unpacking the objects we receive. */
  char **buffer;

  /** Size of the storage areas. */
  int buffer_size;

  /** Storage objects with which the data is unpacked. */
  AmorphReStore* store;

  /** Pointer to the channel manager, quick access. */
  ChannelManager* channel_manager;

  /** A counter, needed to make the name unique. */
  static int unique;

  /** Callback on token completion */
  Callback<ReflectiveFillUnpacker>                        token_valid;

  /** Function on token completion. */
  void tokenValid(const TimeSpec& ts);

  /** For convenience, a pair of store no and pointer to a channel
      acces token. */
  typedef pair<int, EventChannelReadToken<FillSet>*> SenderInfo;

  /** Priority of unpacking, default */
  PrioritySpec unpack_prio;

  /** Method that does the data reception and re-distribution. */
  void receiveAPiece(const TimeSpec& ts, SenderInfo& info);

  /** Activities that checks whether there is stuff to send, and sends
      it. */
  list<ActivityCallback*>               all_receive_stuff;

  list<EventChannelReadToken<FillSet> *> all_tokens;

public:
  /** This class can be created from scheme */
  SCM_FEATURES_DEF;

  /** Return parameter table. */
  static const ParameterTable* getParameterTable();

public:
  /** Constructor. */
  ReflectiveFillUnpacker();

  /** complete method. */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Destructor. */
  ~ReflectiveFillUnpacker();

  /** initialisation, called from the Accessor, to tell us who we are
      in this context (send order). This also tells us how many
      channels to subscribe to. */
  void initialise(const ReflectiveStoreInformation& i);

  /** part of dueca. */
  ObjectType getObjectType() const {return O_Dueca; }

  /** Print to a stream. */
  //friend ostream& operator << (ostream& os, const ReflectiveFillUnpacker& p);
};

DUECA_NS_END
#endif
