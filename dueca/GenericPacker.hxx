/* ------------------------------------------------------------------   */
/*      item            : GenericPacker.hh
        made by         : Rene' van Paassen
        date            : 980611
        category        : header file
        description     : The classes that send data from one host to
                          one or more others
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   :
*/

#ifndef GenericPacker_hh
#define GenericPacker_hh

#include "StateGuard.hxx"
#include "NamedObject.hxx"
#include "ScriptCreatable.hxx"
#include "AsyncQueueMT.hxx"
#include "Activity.hxx"

#include <dueca_ns.h>
DUECA_NS_START
class StoreInformation;
class GenericChannel;
class TransportNotification;
class TimeSpec;
class Accessor;
class UChannelEntry;

/** Packs data written to channels for transport.

    Transport of data between nodes is done via a two-stage
    process. The writing of data in a channel triggers the packing of
    the data for transport. The Packer actively packs the data (or
    does almost nothing if you have reflective memory hardware, and
    so that the accessor of the media (e.g. IPAccessor) can do the
    physical transport. */
class GenericPacker: public ScriptCreatable,
                     public NamedObject
{
private:
  /** A unique number for identification. */
  static int unique;

  /** No copy constructor */
  GenericPacker(const GenericPacker&);

protected:
  /** A pointer to the accessor I am serving. */
  Accessor* accessor;

  /** Set of information needed to pack data from a channel */
  struct PackUnit
  {
    UChannelEntry*   entry;
    unsigned         idx;
    TimeTickType     tick;
  };

  /** A queue of items to be obtained from channel entries and packed
      for transport. */
  AsyncQueueMT<PackUnit> work_queue;

  /** Constructor.
      \param tname  Name for the packer type. */
  GenericPacker(const char* tname);


public:
  /** Report (by a channel) that some data has to be packed.
      \param entry    Channel entry that requested the pack
      \param ts       Time for which the transport has to be done,
      \param idx      Index of the notification, so that the entry
                      can differentiate between packing clients. */
  virtual void notification(UChannelEntry* entry,
                            TimeTickType ts,
                            unsigned idx);

  /** Type name information */
  virtual const char* getTypeName();

  /** Indicate who is the accessor. */
  inline void setAccessor(Accessor* acc) {accessor = acc;}

  /** Run built-up notifications */
  virtual void packWork();

  /// Destructor.
  virtual ~GenericPacker();

public:
  /** Initialise the stores in which data will be packed.
      It is the responsibility of the transport media accessor to
      offer storage space for the packer. */
  virtual void initialiseStores(char** area, int* store_status,
                                int n_stores, int store_size);

  /** Tell the packer to change to a different store.
      This is done by the transport media accessor. */
  virtual int changeCurrentStore(int & store);

  /** Stop packing, used at destruction time. */
  virtual void stopPacking();

  /** Return the object classification. */
  ObjectType getObjectType () const {return O_Dueca;}
};

DUECA_NS_END
#endif





