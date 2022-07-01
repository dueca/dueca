/* ------------------------------------------------------------------   */
/*      item            : EntryWriter.hxx
        made by         : Rene van Paassen
        date            : 170201
        category        : header file
        description     :
        changes         : 170201 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef EntryWriter_hxx
#define EntryWriter_hxx

#include "EntryHandler.hxx"

STARTNSREPLICATOR;
// advance declaration
class PeerTiming;

/** Write entries of a specific channel for the replicator. */
class EntryWriter: public EntryHandler
{
  /** Remember when entry valid */
  bool                           valid;

  /** Sending peer id */
  unsigned                       originator_id;

  /** Callback object validity. */
  Callback<EntryWriter>          cbvalid;

  /** Writing token */
  dueca::ChannelWriteToken       w_entry;

public:
  /** Constructor */
  EntryWriter(const GlobalId& master_id, unsigned originator_id, uint16_t rid,
              const std::string& channelname, const std::string& dataclass,
              uint32_t data_magic, const std::string& entrylabel,
              Channel::EntryTimeAspect time_aspect, Channel::EntryArity arity,
              Channel::PackingMode, Channel::TransportClass tclass,
	      const GlobalId& origin);

  /** Destructor */
  ~EntryWriter();

  /** token valid */
  void tokenIsValid(const TimeSpec& ts);

  /** Originating node within replicator system */
  unsigned getOrigin() const { return originator_id; }

  /** Decode and write an entry
      @param s         Store from which the data is retrieved, first
                       size of data (2 bytes), time tick *or* time
                       spec, then data itself
      @param timeshift shift in time, added to tick or time spec;
                       to correct, must be local time - remote time
      @param spanskip  If true, data contains time spec, otherwise
                       assume only time tick.
  */
  void writeChannel(AmorphReStore& s, const PeerTiming& timeshift,
                    bool spanskip);
};

ENDNSREPLICATOR;

#endif
