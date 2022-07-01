#include <vector>
#include <iostream>

#include <GenericToken.hxx>



// simulation of comm class
class Some {
public:
  int i;
  std::vector<double> v;
  Some() :
    i(2),
    v(4, 0.5)
  {}
};

DUECA_NS_START;

// companion class to contain print and size functions

// static table, with name, print and size function -- class specific

// iterator object, that points to parentclass of companion, and the begin() and end () are the static table pointers? ++ and -- moves through table?

// dereference essentially gives c? can be used in ostream?

// how to get a specific variable? Use parent class and

enum DataMasterType {
  Float,
  Integer,
  Unhandled,
  Iterable
};

typedef int32_t ChannelEntryIndex;

/** Encapsulate the access to a data set in a channel.

    Access to data in channels is by reference, and the channels are
    responsible for accounting that access is returned at the end of a
    session. This class encapsulates a pointer to the object, a
    pointer to the intermediary (GenericToken) that grants the access
    and an id of the access itself. Upon destruction, access is
    returned.

    Note that access is lazy, i.e. only requested when needed.
*/
class DataObjectReadAccess
{
private:
  void* object;
  GenericToken* granter;
  DataResourceId rid;
  const DataTimeSpec ts;
public:
  /** Create access to data previously  written to a channel. The time
      specification selects  the data for  the matching time,  and the
      iterator selects which entry is to be returned.
  */
  DataObjectReadAccess(GenericToken &t, DataTimeSpec& ts,
                       const ChannelEntryIndex it = 0) :
    object(NULL),
    granter(&t),
    rid(it),
    ts(ts)
  {
    //
  }

  void *operator () ()
  {
    if (!object) {
      granter->getDataAccess(object, rid, ts);
    }
    return object;
  }

  DataObjectReadAccess(const DataObjectReadAccess& other) :
    object(NULL),
    granter(other.granter),
    rid(other.rid)
  {
    //
  }

  DataObjectReadAccess(GenericToken &t, const ChannelEntryIndex it = 0) :
    object(NULL),
    granter(&t),
    rid(it)
  {
    //
  }

  /** Destructor. Releases the access again. */
  ~DataObjectReadAccess()
  {
    if (object) {
      granter->returnDataAccess(rid);
    }
  }
};

/** Short-lived object that can extract data for a specific time
    written to a channel.

    This object is for generic extraction, i.e., without knowledge of
    a class one can print or extract the data.

    This object acts as a combined referrer and iterator. Using the ++
    and -- operators, one can iterate over the data members (note:
    prefix operators are more efficient).

    Use rewind(), ++ and, atbegin() and atend() to run through the
    data members.

    Use print and accessObject to access the data members. Before
    using accessObject, use getType to determine which variants are
    valid.
*/
class MultiDataReader {
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
  MultiDataReader(const GenericToken& token, const DataTimeSpec& ts) :
    data(t, ts),
    translators(t.getTranslator()),
    index(0)
  {
    // empty
  }

  /** Constructor. Requests acces to the latest data point in a
      channel.

      \param token  Access token that grants access to the channel. */
  MultiDataReader(const GenericToken& token) :
    data(t),
    translators(t.getTranslator()),
    index(0)
  {
    // empty
  }

  /** Copy constructor */
  MultiDataReader(const MultiDataReader& o) :
    data(o.data),
    translators(o.translators),
    index(o.index)
  {
    // empty
  }

  /** Destructor */
  ~MultiDataReader()
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

  inline MultiDataReader& operator ++ ()
  {
    if (translators[ii].name != NULL) ++ii;
    return *this;
  }
  inline MultiDataReader& operator -- ()
  {
    if (ii) --ii;
    return *this;
  }
  inline MultiDataReader& operator ++ (int dummyindicatespostfix)
  {
    MultiDataReader copy(*this);
    ++(*this);
    return copy;
  }
  inline MultiDataReader& operator -- (int dummyindicatespostfix)
  {
    MultiDataReader copy(*this);
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

  inline MultiDataReader operator[] (const char* name)
  {
    MultiDataReader res(*this);
    res.indexTo(name);
    return res;
  }

  iterator begin()
  {
    return translators[index].ab->begin();
  }
};

/** Iterate over an iterable member of a channel object. */
class SubIterator
{
  virtual void print(std::ostream& os) = 0;

  virtual SubIterator& operator ++ () = 0;

  virtual SubIterator& operator ++ (int dummyindicatespostfix) = 0;

  virtual DataMasterType getType() = 0;

  virtual SubIterator() {}

  virtual ~SubIterator() {}
};

template<class T, DataMasterType DTYPE>
class SubIteratorChild: public SubIterator
{
  T::iterator ii;
public:
  SubIteratorChild(const T::iterator &it) :
    SubIterator(),
    ii(it)
  { }

  void print(std::ostream& os)
  { os << (*ii); }

  SubIteratorChild& operator ++ ()
  { ++ii; }

  SubIteratorChild operator ++ (int dummyindicatespostfix)
  { tmp = *this; ++i; return tmp; }

  DataMasterType getType() { return DTYPE; }
};



template<typename DATATYPE, DataMasterType DATATYPELABEL>
struct typeconvert
{
  typedef type DATATYPE;
  const DataMasterType DATATYPELABEL;
};

template<>
struct typeconvert<double, Float>;

struct AccessBase
{
  AccessBase() {}
  virtual ~AccessBase() {}

  virtual void print(std::ostream& os, void* obj)
  { throw(TypeCannotBePrinted()); }
  virtual CompatibleDataType getType() = 0;
  virtual SubIterator begin() { throw(TypeCannotBeIterated()); }
  virtual SubIterator end() { throw(TypeCannotBeIterated()); }
};

/** Class to encapsulate print access to a member of a data object.
    \tparam C    Data object class
    \tparam T    Type of the data member
    \tparam TC   typeconverter */
template<typename C, typename T, typename TC >
struct AnyAccess : public AccessBase
{
  T C:: *data_ptr;

  AnyAccess(T C:: *dp) :
    data_ptr(dp) { }

  void print(std::ostream& os, void* obj)
  {
    union {const void* v; const C* obj;} conv; conv.v = v;
    os << (*(conv.obj)) .* data_ptr;
  }

  DataMasterType getType() { return TC::DATATYPELABEL; }
};

/** \tparam C    Class for which member is accessed
    \tparam T    Type of the data member
    \tparam DT   */template<typename C, typename T, typename DT, DataMasterType MT>
struct AnyAccess<C, T, typeconvert<DT,Iterable> > : public AccessBase
{
  T C:: *data_ptr;

  AnyAccess(T C:: *dp) :
    data_ptr(dp) { }

  DataMasterType getType() { return TC:DATATYPELABEL; }

  SubIterator begin()
  { return SubIteratorChild<T,IM>( ((*(conv.obj)) .* data_ptr).begin() ); }

  SubIterator end()
  { return SubIteratorChild<T,IM>( ((*(conv.obj)) .* data_ptr).end() ); }
};



struct DataTable {
  const char* name;
  AccessBase* ab;
};
DUECA_NS_END;
USING_DUECA_NS;
DataTable Some_table[] = {
  { "i", new AnyAccess<Some,int,double>(&Some::i) },
  { "v", new AnyAccess<Some,stdvector<
  { NULL, NULL }
};

int main()
{
  Some s;

  Some_table[0].ab -> print(std::cout, &s);
  std::cout << std::endl;
  int i = Some_table[0].ab -> operator(&s);
  return 0;
}



#if 0
  /** The intention is to also provide conversion to a limited set of type
      (double, int, char pointer), but don't know all ramifications yet,
      so waiting a bit with that possibility */
  virtual TC operator() (void* obj)
  {
    union {const void* v; const C* obj;} conv; conv.v = v;
    return TC( (*(conv.obj)) .* data_ptr );
  }
#endif
