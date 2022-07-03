/* ------------------------------------------------------------------   */
/*      item            : CommObjectElementWriter.hxx
        made by         : Rene van Paassen
        date            : 131220
        category        : header file
        description     :
        changes         : 131220 first version
        language        : C++
        api             : DUECA_API
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CommObjectElementWriter_hxx
#define CommObjectElementWriter_hxx

#include "CommObjectElementWriterBase.hxx"


DUECA_NS_START;


/** ElementWriter. An object to write members of DCO objects

    The ElementWriter enables a generic level (based on
    boost::any) interaction with a member of a DCO object. Generally,
    the ElementWriter will be generated by a CommObjectWriter, which
    in turn may be the parent of a DCOWriter, that takes a channel
    access token to obtain access to a channel entry.

    This is for generic access. In most cases, it is more efficient,
    easier and safer to reference the DCO in the comm-objects.lst for
    your modules, and use a DataWriter. However, if you want to create
    for example a generic logger that can handle any data type, this
    might be useful.

    If the member is iterable, it may be written to multiple times;
    each write enters the next value. The member arity/multiplicity
    can be tested with getArity(). Note that a member that returns
    FixedIterable has a fixed size, and will fail with an
    IndexExceeded exception when you exceed the vector size.

    Mapped containers (e.g. std::map), require a key to be defined for
    writing. This interface is used for simply filling the container,
    not using it.

    In addition, a fixed length array, identified as a FixedIterable
    type, can be written to with an integer index.

    If the member is nested, the recurse method can be used to get a
    new CommObjectWriter that can be used to write elements of the
    nested member. For iterables (arrays, lists) of nested members
    this can be done repeatedly.
*/
class ElementWriter
{
public:
  /** To keep the required data light-weight and on the stack, this
      defines space for the parent's pointers/indices */
  const static size_t RESERVE = 3*sizeof(void*);
private:

  /** room for the child that does the actual access */
  char stretch[RESERVE];

  /** Means to overlay the pointer to the child */
  union {
    /** The stretch points to the extra space in this object */
    char *stretch;

    /** This pointer overlays the elements's base class */
    WriteElementBase *bptr;
  } u;

  /** access the parent that does the writing */
  inline const WriteElementBase* base() const
  { return u.bptr; }

  /** access the parent that does the writing */
  inline WriteElementBase* base()
  { return u.bptr; }

  /** Make this (templated class) a friend, for constructor and
      placement of the internal WriteElement object. */
  template <typename C, typename T>
  friend class CommObjectMemberAccess;

  /** return the spot for the placement constructor of the child */
  void* placement() { return u.stretch; }
public:

  /** Constructor. After this, the placement new operator must be used to
      initialize the WriteElementBase/WriteElement object. */
  ElementWriter() { u.stretch = stretch; }

  /** write data. Only for fixed length arrays.

      @param a     Data to be written
      @param idx   Index into the array */
  inline void write(const boost::any& a, unsigned idx)
  { base()->write(a, idx); }

  /** write data. If the entry is of a "mapped" type, the key must also be
      provided.

      @param a     Data to be written
      @param key   Key, only used for mapped data (e.g. std::map)
  */
  inline void write(const boost::any& a, const boost::any& key=boost::any())
  { base()->write(a, key); }

  /** Recursively access a nested object. k provides the key if
      needed.  If the object is iterable (list, vector-like), repeated
      calls write the next element.

      @param key   Key, if applicable, only for mapped data.
      @returns     A new CommObjectWriter, which can be recursively
                   written.
   */
  inline CommObjectWriter recurse(const boost::any& key=boost::any())
  { return base()->recurse(key); }

  /** Recursively access a fixed length iterable object (fixed-length
      vector.

      @param idx   Index into the fixed-length array
      @returns     A new CommObjectWriter, which can be recursively
                   written.
   */
  inline CommObjectWriter recurse(unsigned idx)
  { return base()->recurse(idx); }

  /** Whether the object can be recursed or not.

      @returns    true if nested, in that case the recurse method can be
                  used.
  */
  inline bool isNested() const { return base()->isNested(); }

  /** returns what type of "Arity" the object has; either a single object,
      an iterable object, or a mapped object, i.e. iterable with key

      @returns    A MemberArity enum indicating indexing
  */
  inline MemberArity getArity() const { return base()->getArity(); }

  /** Returns the typeindex_t of the class type */
  inline typeindex_t getTypeIndex() const
  { return base()->getTypeIndex(); }

  /** Returns the typeindex_t of the key type, if applicable */
  inline typeindex_t getKeyTypeIndex() const
  { return base()->getKeyTypeIndex(); }

  /** Returns true if a complete fixed-length array has been written */
  inline bool arrayIsComplete() const
  { return base()->arrayIsComplete(); }

  /** Assignment. Copy the hidden object in stretch */
  ElementWriter& operator = (const ElementWriter& o)
  {
    memcpy(this->stretch, o.stretch, RESERVE);
    return *this;
  }
};

DUECA_NS_END;
#endif