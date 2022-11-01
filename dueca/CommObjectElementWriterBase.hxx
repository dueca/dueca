/* ------------------------------------------------------------------   */
/*      item            : CommObjectElementWriterBase.hxx
        made by         : Rene van Paassen
        date            : 191021
        category        : header file
        description     :
        changes         : 191021 first version
        language        : C++
        copyright       : (c) 19 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CommObjectElementWriterBase_hxx
#define CommObjectElementWriterBase_hxx

#include <boost/any.hpp>
#include <boost/lexical_cast.hpp>
#include <dueca_ns.h>
#include <CommObjectTraits.hxx>
#include <CommObjectWriter.hxx>
#include <CommObjectExceptions.hxx>
#include <CommObjectMemberArity.hxx>
#include <DCOTypeIndex.hxx>

DUECA_NS_START;

/** Base class defining writing interaction with a member of a DCO object

    This is for generic access. In most cases, it is more efficient,
    easier and safer to reference the DCO in the comm-objects.lst for your
    modules, and use a DataWriter. */
class WriteElementBase
{
public:

  WriteElementBase() {}

  virtual void write(const boost::any& res,
                     const boost::any& key)
  { throw ConversionNotDefined(); }

  virtual void write(const boost::any& res,
                     unsigned idx)
  { throw ConversionNotDefined(); }

  virtual void setFirstValue()
  { throw ConversionNotDefined(); }

  virtual bool setNextValue()
  { throw ConversionNotDefined(); }

  virtual void skip()
  { throw ConversionNotDefined(); }

  virtual CommObjectWriter recurse(const boost::any& key)
  { throw TypeIsNotNested(); }

  virtual CommObjectWriter recurse(unsigned idx)
  { throw TypeIsNotNested(); }

  virtual MemberArity getArity() const { return Single; }

  virtual bool isEnd() const { return true; }

  virtual bool isNested() const { return false; }

  virtual typeindex_t getTypeIndex() const { return TYPEID(void); }

  virtual typeindex_t getKeyTypeIndex() const { return TYPEID(void); }

  virtual bool arrayIsComplete() const { return false; }

  virtual ~WriteElementBase() {}
};

/** Generic template for access to a single data element */
template<class Type, typename T>
struct elementdataw
{
  T* object;
  bool have_written;
  typedef T value_type;
  typedef T elt_value_type;
  typedef void elt_key_type;
  constexpr static const MemberArity arity = Single;
};

/** Specialization for access to an iterable container, optionally
    indexed and with variable size (list, vector) */
template<typename T>
struct elementdataw<dco_write_iterable,T>
{
  T* object;
  typedef T value_type;
  typedef typename T::value_type elt_value_type;
  typedef void elt_key_type;
  constexpr static const MemberArity arity = Iterable;
};

/** Specialization for access to a key-value pair container */
template<typename T>
struct elementdataw<dco_write_map,T>
{
  T* object;
  typedef T value_type;
  typedef typename T::mapped_type elt_value_type;
  typedef typename T::key_type    elt_key_type;
  constexpr static const MemberArity arity = Mapped;
};

/** Specialization for access to an iterable, indexed, fixed-length
    container. */
template<typename T>
struct elementdataw<dco_write_fixed_it,T>
{
  T* object;
  typename T::iterator ii;
  typedef T value_type;
  typedef typename T::value_type elt_value_type;
  typedef uint32_t elt_key_type;
  constexpr static const MemberArity arity = FixedIterable;
};

/** Templated implementation of the WriteElementBase functionalities */
template<class T>
class WriteElement: public elementdataw< typename dco_traits<T>::wtype, T>,
  public WriteElementBase
{
  /** needed data for accessing either the single element or a container
      of elements, using the above-defined data types */
  typedef elementdataw<typename dco_traits<T>::wtype, T> par;
public:

  /** constructor, calls one of the implementations, depending on
      container/direct type traits */
  WriteElement(T& data)
  { constructor(data, typename dco_traits<T>::wtype()); }

private:
  void constructor(T& data, const dco_write_single&)
  { par::object = &data; }
  void constructor(T& data, const dco_write_iterable&)
  { par::object = &data; }
  void constructor(T& data, const dco_write_map&)
  { par::object = &data; }
  void constructor(T& data, const dco_write_fixed_it&)
  { par::ii = data.begin(); par::object = &data; }

public:

  /** Return information about the multiplicity of the entry */
  MemberArity getArity() const
  { return par::arity; }

public:
  /** Return information about nesting of the member. */
  bool isNested() const
  { return isNested(dco_nested<typename par::elt_value_type>()); }

  /** Type information */
  typeindex_t getTypeIndex() const
  { return TYPEID(typename par::elt_value_type); }

  /** Type information */
  typeindex_t getKeyTypeIndex() const
  { return TYPEID(typename par::elt_key_type); }

private:
  inline bool isNested(const dco_isdirect&) const
  { return false; }
  inline bool isNested(const dco_isnested&) const
  { return true; }

public:
  /** Array completely written (for fixed) */
  bool arrayIsComplete() const
  { return arrayIsComplete(typename dco_traits<T>::wtype()); }

private:
  bool arrayIsComplete(const dco_write_fixed_it&) const
  { return par::ii == par::object->end(); }

  template <typename X>
  bool arrayIsComplete(const X&) const {return false;}

public:
  /** Access a member as a composite object, access the next element
      in an array of members or create the next element in an
      extensible array. */
  CommObjectWriter recurse()
  { return recurse(dco_nested<typename par::elt_value_type>()); }

  /** Create an element in a map and access it. */
  CommObjectWriter recurse(const boost::any& key)
  { return _recurse(dco_nested<typename par::elt_value_type>(), key); }

  /** Access an element in a fixed-length vector. */
  CommObjectWriter recurse(unsigned idx)
  { return _recurse(dco_nested<typename par::elt_value_type>(), idx); }

private:
  inline CommObjectWriter _recurse(const dco_isnested&)
  { return CommObjectWriter(par::elt_value_type::classname,
                            get_object(typename dco_traits<T>::wtype())); }

  inline CommObjectWriter _recurse(const dco_isnested&, const boost::any& key)
  { return CommObjectWriter(par::elt_value_type::classname,
                            get_object(typename dco_traits<T>::wtype(), key)); }

  inline CommObjectWriter _recurse(const dco_isnested&, unsigned idx)
  { return CommObjectWriter(par::elt_value_type::classname,
                            get_object(typename dco_traits<T>::wtype(), idx)); }

  inline CommObjectWriter _recurse(const dco_isdirect&)
  { throw TypeIsNotNested(); }

  inline CommObjectWriter _recurse(const dco_isdirect&, const boost::any& key)
  { throw TypeIsNotNested(); }

  inline CommObjectWriter _recurse(const dco_isdirect&, unsigned idx)
  { throw TypeIsNotNested(); }

private:
  inline void* get_object(const dco_write_single&,
                          const boost::any& key=boost::any())
  { return reinterpret_cast<void*>(par::object); }

  inline void* get_object(const dco_write_iterable&,
                          const boost::any& key=boost::any())
  { par::object->push_back(typename par::elt_value_type());
    return reinterpret_cast<void*>(&(par::object->back())); }

  inline void* get_object(const dco_write_map&,
                          const boost::any& key=boost::any())
  { const typename par::elt_key_type *keycast =
      boost::any_cast<const typename par::elt_key_type>(&key);
    if (keycast == NULL) { throw(ConversionNotDefined()); }
    return reinterpret_cast<void*>
      (&(*(par::object->emplace
           (*keycast, typename par::elt_value_type()).first))); }

  inline void* get_object(const dco_write_fixed_it&,
                          const boost::any& key=boost::any())
  { if (par::ii == par::object->end()) throw IndexExceeded();
    return reinterpret_cast<void*>(&(*par::ii++)); }

  inline void* get_object(const dco_write_single&,
                          unsigned idx)
  { if (idx != 0) throw IndexExceeded();
    return reinterpret_cast<void*>(par::object); }

  inline void* get_object(const dco_write_fixed_it&,
                          unsigned idx)
  { if (idx > par::object->size()) throw IndexExceeded();
    return reinterpret_cast<void*>(&(*par::object[idx])); }

public:
  inline void write(const boost::any& val, const boost::any& key=boost::any())
  { write(dco_nested<typename par::elt_value_type>(),
	  typename dco_traits<T>::wtype(), val, key); }

  inline void write(const boost::any& val, unsigned idx)
  { write(dco_nested<typename par::elt_value_type>(),
	  typename dco_traits<T>::wtype(), val, idx); }

private:
  void write(const dco_isenum&, const dco_write_single&,
             const boost::any& val, const boost::any& key)
  { const std::string valcast =
      boost::any_cast<const std::string>(val);
    readFromString(*par::object, valcast); }

  void write(const dco_isdirect&, const dco_write_single&,
             const boost::any& val, const boost::any& key)
  { typename par::elt_value_type const *valcast =
      boost::any_cast<const typename par::elt_value_type>(&val);
    if (valcast == NULL) { throw(ConversionNotDefined()); }
    *par::object = *valcast; }

  void write(const dco_isenum&, const dco_write_iterable&,
             const boost::any& val, const boost::any& key)
  { const std::string valcast = boost::any_cast<const std::string>(val);
    const typename par::elt_value_type newval;
    readFromString(newval, val);
    par::object->push_back(newval);
  }

  void write(const dco_isdirect&, const dco_write_iterable&,
             const boost::any& val, const boost::any& key)
  { typename par::elt_value_type const *valcast =
      boost::any_cast<const typename par::elt_value_type>(&val);
    if (valcast == NULL) { throw(ConversionNotDefined()); }
    par::object->push_back(*valcast); }

  void write(const dco_isenum&, const dco_write_fixed_it&,
             const boost::any& val, const boost::any& key)
  { if (par::ii == par::object->end()) throw IndexExceeded();
    const std::string valcast = boost::any_cast<const std::string>(val);
    const typename par::elt_value_type newval;
    readFromString(newval, val);
    *par::ii++ = newval; }

  void write(const dco_isdirect&, const dco_write_fixed_it&,
             const boost::any& val, const boost::any& key)
  { if (par::ii == par::object->end()) throw IndexExceeded();
    typename par::elt_value_type const *valcast =
      boost::any_cast<const typename par::elt_value_type>(&val);
    if (valcast == NULL) { throw(ConversionNotDefined()); }
    *par::ii++ = *valcast; }

  void write(const dco_isenum&, const dco_write_fixed_it&,
             const boost::any& val, unsigned idx)
  { if (idx >= par::object->size()) throw IndexExceeded();
    const std::string valcast = boost::any_cast<const std::string>(val);
    const typename par::elt_value_type newval;
    readFromString(newval, val);
    *(par::object->ptr() + idx) = newval; }

  void write(const dco_isdirect&, const dco_write_fixed_it&,
             const boost::any& val, unsigned idx)
  { if (idx >= par::object->size()) throw IndexExceeded();
    typename par::elt_value_type const *valcast =
      boost::any_cast<const typename par::elt_value_type>(&val);
    if (valcast == NULL) { throw(ConversionNotDefined()); }
    *(par::object->ptr() + idx) = *valcast; }


  void write(const dco_isenum&, const dco_write_single&,
             const boost::any& val, unsigned idx)
  { if (idx != 0) throw IndexExceeded();
    const std::string valcast = boost::any_cast<const std::string>(val);
    readFromString(*par::object, valcast); }

  void write(const dco_isdirect&, const dco_write_single&,
             const boost::any& val, unsigned idx)
  { if (idx != 0) throw IndexExceeded();
    typename par::elt_value_type const *valcast =
      boost::any_cast<const typename par::value_type>(&val);
    if (valcast == NULL) { throw(ConversionNotDefined()); }
    *par::object = *valcast; }

  void write(const dco_isdirect&, const dco_write_map&,
             const boost::any& key, const boost::any& val)
  { typename par::elt_key_type const *keycast =
      boost::any_cast<const typename par::elt_key_type>(&key);
    typename par::elt_value_type const *valcast =
      boost::any_cast<const typename par::elt_value_type>(&val);
    if (keycast == NULL || valcast == NULL) { throw(ConversionNotDefined()); }
    (*par::object)[*keycast] = *valcast; }

  template<typename Dum>
  void write(const dco_isnested&, const Dum&,
             const boost::any& val, const boost::any& key)
  { throw(ConversionNotDefined()); }

public:
  inline void setFirstValue() final
  { setFirstValue(dco_nested<typename par::elt_value_type>(),
	     typename dco_traits<T>::wtype()); }

  inline bool setNextValue() final
  { return setNextValue(dco_nested<typename par::elt_value_type>(),
		   typename dco_traits<T>::wtype()); }

private:
  void setFirstValue(const dco_isenum&, const dco_write_single&)
  { getFirst(*par::object); }

  void setFirstValue(const dco_isenum&, const dco_write_iterable&)
  { const typename par::elt_value_type newval; getFirst(newval);
    par::object->push_back(newval); }

  void setFirstValue(const dco_isenum&, const dco_write_fixed_it&)
  { if (par::ii == par::object->end()) throw IndexExceeded();
    getFirst(*par::ii++); }

  template<typename D1, typename D2>
  void setFirstValue(const D1&, const D2&)
  { throw(ConversionNotDefined()); }

  bool setNextValue(const dco_isenum&, const dco_write_single&)
  { return getNext(*par::object); }

  bool setNextValue(const dco_isenum&, const dco_write_iterable&)
  { return getNext(par::object->back()); }

  bool setNextValue(const dco_isenum&, const dco_write_fixed_it&)
  { return getNext(*par::ii); }

  template<typename D1, typename D2>
  bool setNextValue(const D1&, const D2&)
  { throw(ConversionNotDefined()); }

public:
  inline void skip()
  { skip(typename dco_traits<T>::wtype()); }

private:
  void skip(const dco_write_fixed_it&)
  { if (par::ii == par::object->end()) throw IndexExceeded();
    par::ii++; }
  template<typename Dum>
  void skip(const Dum&)
  { throw(ConversionNotDefined()); }

public:
  inline bool isEnd()
  { return isEnd(typename dco_traits<T>::wtype()); }
private:
  bool isEnd(const dco_write_fixed_it&)
  { return par::ii == par::object->end(); }
  bool isEnd(const dco_write_single&)
  { throw(ConversionNotDefined()); }
  template<typename Dum>
  bool isEnd(const Dum&)
  { return false; }
};

DUECA_NS_END;
#endif
