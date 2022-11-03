/* ------------------------------------------------------------------   */
/*      item            : CommObjectElementReader.hxx
        made by         : Rene van Paassen
        date            : 131220
        category        : header file
        description     :
        api             : DUECA_API
        changes         : 131220 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CommObjectElementReader_hxx
#define CommObjectElementReader_hxx

#include "CommObjectElementReaderBase.hxx"

DUECA_NS_START;

/** ElementReader  an object to read a member of a DCO object

    The ElementReader enables a generic level (based on std::string or
    boost::any) interaction with a member of a DCO object. Generally,
    the ElementReader will be provided by a dueca::CommObjectReader,
    which in turn may be supplied by a channel through its descendant
    dueca::DCOReader.

    If the member is iterable, it may be read multiple times by read
    calls, each read call will set an iterator to point to the next
    element of the member. The isEnd() call can be used to test
    whether the iteration has ended. If a type is not iterable, there
    will be only one read until isEnd() returns true.

    Single members may be read in std::string or boost::any form,
    compound members (themselves DCO objects) may be recursively
    inspected using the recurse() call, producing new ElementReader
    objects. When attempting to read a single member, but the object
    is compound, a dueca::ConversionNotDefined exception will be
    thrown. When attempting to recurse into a compound member, but the
    member is single, a dueca::TypeIsNotNested exception will be
    thrown.

    The isEnd() call indicates whether all elements of a member have
    been read.
*/
class ElementReader
{
public:
  /** Dirty use of memory; reserve empty space here */
  const static size_t RESERVE = 3*sizeof(void*);
private:

  /** room for the child that does the actual access */
  char stretch[RESERVE];

  /** Means to overlay the pointer to the child */
  union {
    char *stretch;
    ReadElementBase *bptr;
  } u;

  /** access the child that does the reading */
  inline ReadElementBase* base() const
  { return u.bptr; }

  /** Make this (templated class) a friend, for constructor and
      placement of the internal ReadElement object. */
  template <typename C, typename T>
  friend class CommObjectMemberAccess;

  /** Constructor. After this, the placement new operator must be used to
      fill the ReadElementBase/ReadElement object. */
  ElementReader() { u.stretch = stretch; }

  /** return the spot for the placement constructor of the child */
  void* placement() { return u.stretch; }

public:
  /** read a value. If the read value is a map, the key k will return
      a non-null value */
  inline void read(std::string& s, std::string& k)
  { base()->read(s, k); }

  /** read a value. If the read value is a map, the key k will return
      a non-null value */
  inline void read(boost::any& a, boost::any& k)
  { base()->read(a, k); }

  /** read a value, no provision for returning a key if it exists. */
  inline void read(std::string& s)
  { static std::string dum; base()->read(s, dum); }

  /** read a value, no provision for returning a key if it exists. */
  inline void read(boost::any& a)
  { static boost::any dum; base()->read(a, dum); }

  /** read a value, but do not step/iterate/invalidate. */
  inline void peek(std::string& s)
  { static std::string dum; base()->peek(s, dum); }

  /** read a value, but do not step/iterate/invalidate. */
  inline void peek(boost::any& a)
  { static boost::any dum; base()->peek(a, dum); }

  /** recursively access a nested object. k returns the key if it exists */
  inline CommObjectReader recurse(std::string& k)
  { return base()->recurse(k); }

  /** recursively access a nested object. k returns the key if it exists */
  inline CommObjectReader recurse(boost::any& k)
  { return base()->recurse(k); }

  /** returns true if the last value has been read. */
  inline bool isEnd() { return base()->isEnd(); }

  /** returns whether the object can be recursed or not */
  inline bool isNested() { return base()->isNested(); }

  /** returns what type of "Arity" the object has; either a single object,
      an iterable object, or a mapped object, i.e. iterable with key */
  inline MemberArity getArity() { return base()->getArity(); }

  /** Returns the current size of an array, map, list, if applicable */
  inline size_t size() const { return base()->size(); }
};

DUECA_NS_END;

#endif
