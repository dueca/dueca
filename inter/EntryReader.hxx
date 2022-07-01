/* ------------------------------------------------------------------   */
/*      item            : EntryReader.hxx
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

#ifndef EntryReader_hxx
#define EntryReader_hxx

#include "EntryHandler.hxx"

STARTNSREPLICATOR;


class EntryReader: public EntryHandler
{
  /** Remember token validity */
  bool                               tokenvalid;

  /** Callbacks for validity */
  dueca::Callback<EntryReader>       cbv;

  /** Access to the entry */
  dueca::ChannelReadToken            r_entry;

  /** To remember, for the first read flush channel history */
  bool                               firstread;

public:
  /** Constructor */
  EntryReader(const dueca::GlobalId& master_id,
              const dueca::ChannelEntryInfo& i,
              const std::string& channelname);

  /** Destructor */
  ~EntryReader();

  /** Reading and storing data */
  bool readChannel(AmorphStore& s, uint16_t channelid);

  /** Validity callback */
  void tokenIsValid(const TimeSpec& ts);

  /** flush any data. */
  void flushEntry();

};


ENDNSREPLICATOR;

#endif
