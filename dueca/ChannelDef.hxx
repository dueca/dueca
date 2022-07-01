/* ------------------------------------------------------------------   */
/*      item            : ChannelDef.hxx
        made by         : Rene van Paassen
        date            : 141015
        category        : header file
        description     :
        api             : DUECA_API
        changes         : 141015 first version
                          160425 extended doc strings
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ChannelDef_hxx
#define ChannelDef_hxx

#include <dueca_ns.h>
#include <iostream>
#include <inttypes.h>

DUECA_NS_START;

/** @file ChannelDef.hxx

    Definitions for specifying channel properties */

/** Collection of definitions for modifying channel behaviour */
struct Channel
{
  /** Enumeration type defining the entry data type */
  enum EntryTimeAspect {
    Continuous,  /**< Stream-like data, time is contiguous */
    Events,      /**< Event-like data, time is a point stamp, multiple
                      events may share the same time */
    AnyTimeAspect/**< For reading, indicate that both variants are
                      acceptable. */
  };

  /** Enumeration type defining the arity of the entry */
  enum EntryArity {
    OnlyOneEntry,     /**< For writing, only one entry may be present
                           in the channel, for reading, attach only a
                           single entry with the token. */
    ZeroOrOneEntries, /**< When opening the channel for reading, it is
                           acceptable that there is no entry yet. The
                           token will be valid, but data reading will
                           result in an exception. */
    ZeroOrMoreEntries,/**< When opening the channel for reading, it is
                           acceptable that there is no entry yet. The
                           token will be valid, but data reading will
                           result in an exception. */
    OneOrMoreEntries  /**< Accept one or multiple entries in the
                           channel, relevant option for both reading
                           and writing tokens. */
  };

  /** Reading mode, all data, step-by-step or time based */
  enum ReadingMode {
    ReadAllData,     /**< Sequentially read all data, from oldest to
                          newest, all data is read once, and reading
                          fails when no more new data is available for
                          the requested time. Note that if you select
                          this reading mode, data will be kept in the
                          channel until it is read by your module;
                          thus there is an obligation to read or flush
                          data, otherwise excessive memory use
                          follows. */
    ReadReservation, /**< Sequentially read all data, from oldest to
                          newest, like ReadAllData. In addition, this
                          uses a reservation that needs to have been
                          made by the write token(s) that are
                          accessed, ensuring that data written before
                          the creation of this read token remains
                          available. For most user-created channels
                          this is seldom needed, since creation of
                          both reading and writing tokens takes place
                          first, and writing and reading only starts
                          after all tokens have been created. Some of
                          DUECA's own channels do use this, since
                          these channels are created at start-up, and
                          almost immediately used. */
    JumpToMatchTime, /**< Read data that matches the requested time,
                          possibly skipping data. To get a workable
                          simulation, specify a suitable time span,
                          otherwise data has been cleaned before you
                          have an opportunity to read it. */
    AdaptEventStream /**< Adapt to event or stream data, by using
                          ReadAllData for event type data, and
                          JumpToMatchTime for stream data */
  };

  /** Allow differential packing, or always use full packing */
  enum PackingMode {
    MixedPacking,    /**< Use full and differential packing in a
                          mix. Select this when you have a mix of
                          static data and changing elements in large
                          objects. With differential packing, an
                          object in a channel is compared to the
                          preceding object, and only elements that
                          differ are packed. */
    OnlyFullPacking  /**< Only use full packing. This is more
                          appropriate for channel entries with small
                          object, or when you expect that all or most
                          data changes from timestep to timestep. */
  };

  /** Different priorities of transporting data. */
  enum TransportClass {
    UndefinedTransport, /**< Transport class not (yet) specified. */
    Bulk,         /**< Bulk priority is lowest priority in DUECA
                       transport. Very large objects can be sent by
                       bulk, because in principle the objects do not
                       need to fit in a single buffer used by the
                       transport device. Send and arrival order of
                       Bulk data among its class is preserved, bulk
                       data may arrive later than non-bulk data sent
                       at a later time.*/
    Regular,      /**< Regular priority data is sent at the earliest
                       possible moment. Triggering of arrival at the
                       next node is no later than the first following
                       communication clock tick. Send/receive ordering
                       among high priority and regular data is
                       preserved. */
    HighPriority /**<  High priority data is sent at earliest possible
                       moment, (like regular data). However,
                       triggering in the next node is immediately
                       after arrival in that node. The difference
                       between regular and high priority is currently
                       only possible in set-ups with reflective memory
                       communications. */
  };
};

/** Convenience function, returns true if an event is acceptable or
    requested. */
inline bool isTimeAspectEvent(const Channel::EntryTimeAspect &t)
{ return t != Channel::Continuous; }

/** Convenience function, returns true if a events need to be saved up
    while there is no reader. */
inline bool isReadingModeReservation(const Channel::ReadingMode &m)
{ return m == Channel::ReadReservation; }

/** Convenience function, returns true if sequential read specified. */
inline bool isSequentialRead(const Channel::ReadingMode& m,
                             bool eventtype)
{ if (m == Channel::AdaptEventStream) return eventtype;
  return m == Channel::ReadAllData || m == Channel::ReadReservation; }

/** Convenience function, returns true if the arity option is valid
    for read tokens. */
inline bool isValidReadTokenOption(const Channel::EntryArity& a)
{ return true; }

/** Convenience function, returns true if the arity option is valid
    for write tokens. */
inline bool isValidWriteTokenOption(const Channel::EntryArity& a)
{ return a == Channel::OnlyOneEntry || a == Channel::OneOrMoreEntries; }

/** Convenience function, returns true if the arity option indicates a
    single entry. */
inline bool isSingleEntryOption(const Channel::EntryArity& a)
{ return a == Channel::OnlyOneEntry || a == Channel::ZeroOrOneEntries; }

/** Convenience function, returns true if the arity option indicates
    no entries is acceptable for validity. */
inline bool isZeroEntriesAcceptable(const Channel::EntryArity& a)
{ return a == Channel::ZeroOrOneEntries || a == Channel::ZeroOrMoreEntries; }

/** Convenience function, returns true if the packing method is full */
inline bool isFullPacking(const Channel::PackingMode& p)
{ return p == Channel::OnlyFullPacking; }

/** advance definition, packing store */
class AmorphReStore;
/** advance definition, packing store */
class AmorphStore;

/** Definition of the entry id type; distinguishes between different
    entries in a channel. */
typedef uint16_t entryid_type;

/** Constant indicating last entry found */
const entryid_type entry_end = 0xffff;

/** Constant indicating any entry match is acceptable */
const entryid_type entry_any = 0xffff;

/** Constant indicating that entry should be selected by label */
const entryid_type entry_bylabel = 0xfffe;

/** classname function, default for DCO objects */
template <typename T>
const char* getclassname();

/** classname function, needed in case the enum must be sent over */
template<>
const char* getclassname<dueca::Channel::EntryTimeAspect>();

/** classname function, needed in case the enum must be sent over */
template<>
const char* getclassname<dueca::Channel::EntryArity>();

/** classname function, needed in case the enum must be sent over */
template<>
const char* getclassname<dueca::Channel::PackingMode>();

/** classname function, needed in case the enum must be sent over */
template<>
const char* getclassname<dueca::Channel::TransportClass>();

DUECA_NS_END;


PRINT_NS_START
/** Printing of the EntryTimeAspect. */
std::ostream& operator << (std::ostream& os,
                           const DUECA_NS::Channel::EntryTimeAspect& tc);
/** Printing of the EntryArity. */
std::ostream& operator << (std::ostream& os,
                           const DUECA_NS::Channel::EntryArity& tc);
/** Printing of the ReadingMode. */
std::ostream& operator << (std::ostream& os,
                           const DUECA_NS::Channel::ReadingMode& tc);
/** Printing of the PackingMode. */
std::ostream& operator << (std::ostream& os,
                           const DUECA_NS::Channel::PackingMode& tc);
/** Printing of the TransportClass. */
std::ostream& operator << (std::ostream& os,
                           const DUECA_NS::Channel::TransportClass& tc);
PRINT_NS_END;
#endif
