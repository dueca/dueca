/* ------------------------------------------------------------------   */
/*      item            : GenericDataReader.hxx
        made by         : Rene van Paassen
        date            : 130122
        category        : header file
        description     :
        changes         : 130122 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef GenericDataReader_hxx
#define GenericDataReader_hxx

/** Short-lived object that can extract data for a specific time
    written to a channel.

    This object is for generic extraction, i.e., without knowledge of
    a class one can print or extract the data. If you know the type of
    data you are handling, by all means use StreamReader or
    EventReader, these are way more efficient.

    This object acts as a combined referrer and iterator. Using the ++
    and -- operators, one can iterate over the data members (note:
    prefix operators are more efficient, so use these if you have the
    choice).

    Use rewind(), ++ and, atbegin() and atend() to run through the
    data members and test for where you are.

    Use print to access atomic (single) data members. Before
    using accessObject, use getType to determine which variants are
    valid.
*/
class GenericDataReader {

  /** object that can access the data. It communicates its lifetime to the
      token that grants the access. */
  const DataObjectReadAccess data;

  /** Pointer to the table with data member names and translators;
      each translator corresponds to one data member. */
  const DataTable *translators;

  /** Index pointing to the current data member */
  int index;

public:

  typedef SubIterator iterator;

  /** Constructor. Requires acces to a channel specifying a time for
      reading the data.

      \param token  Access token that grants access to the channel.
      \param ts     Time for which data is requested. */
  GenericDataReader(const GenericToken& token, const DataTimeSpec& ts) :
    data(token, ts),
    translators(token.getTranslator()),
    index(0)
  {
    // empty
  }

  /** Constructor. Requests acces to the latest data point in a
      channel.

      \param token  Access token that grants access to the channel. */
  GenericDataReader(const GenericToken& token) :
    data(token),
    translators(token.getTranslator()),
    index(0)
  {
    // empty
  }

  /** Copy constructor */
  GenericDataReader(const GenericDataReader& o) :
    data(o.data),
    translators(o.translators),
    index(o.index)
  {
    // empty
  }

  /** Destructor */
  ~GenericDataReader()
  {
    //
  }

  /** Print a representation of the data to the specified data stream. */
  void print(std::ostream& os)
  {
    translators[index].ab->print(std::ostream& os, dataobject);
  }

  /** Return an enum indicating a compatible data type */
  DataCompatType& getType()
  { return translators[index].ab->getType(); }

  /** */
  inline void rewind() { index = 0;}

  inline bool atend() { translators[index].name = NULL;}

  inline GenericDataReader& operator ++ ()
  {
    if (translators[ii].name != NULL) ++ii;
    return *this;
  }
  inline GenericDataReader& operator -- ()
  {
    if (ii) --ii;
    return *this;
  }
  inline GenericDataReader& operator ++ (int dummyindicatespostfix)
  {
    GenericDataReader copy(*this);
    ++(*this);
    return copy;
  }
  inline GenericDataReader& operator -- (int dummyindicatespostfix)
  {
    GenericDataReader copy(*this);
    --(*this);
    return copy;
  }

  inline const char* getName() {return translators[index].name; }

  void indexTo(const char* name)
  {
    // not yet efficient for large arrays
    for (index = 0; translators[index].name &&
           strcmp(name, translators[index].name); index++);
    if (!translators[index].name) {
      throw(MemberNotFound("name"));
    }
  }

  inline GenericDataReader operator[] (const char* name)
  {
    GenericDataReader res(*this);
    res.indexTo(name);
    return res;
  }

  iterator begin()
  {
    return translators[index].ab->begin();
  }
};

std::ostream& operator << (std::ostream& os, const GenericDataReader& rdr)
{
  rdr.print(os);
  return os;
}

#endif
