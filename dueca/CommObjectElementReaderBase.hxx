/* ------------------------------------------------------------------   */
/*      item            : CommObjectElementReaderBase.hxx
        made by         : Rene van Paassen
        date            : 131220
        category        : header file
        description     :
        api             :
        changes         : 131220 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CommObjectElementReaderBase_hxx
#define CommObjectElementReaderBase_hxx

#include <boost/any.hpp>
#include <boost/lexical_cast.hpp>
#include <dueca_ns.h>
#include <CommObjectTraits.hxx>
#include <CommObjectReader.hxx>
#include <CommObjectExceptions.hxx>
#include <CommObjectMemberArity.hxx>


DUECA_NS_START;

/** Base class for introspection reading a single element from a DCO
    object.

    This is used "internally" by the ElementReader class.
 */
class ReadElementBase
{
public:

  virtual ~ReadElementBase() {}
  ReadElementBase() {}

  virtual void read(std::string& res,
                    std::string& key) = 0;
  virtual void read(boost::any& res,
                    boost::any& key) = 0;

  virtual void peek(std::string& res,
                    std::string& key) = 0;
  virtual void peek(boost::any& res,
                    boost::any& key) = 0;

  virtual CommObjectReader recurse(std::string& key) = 0;
  virtual CommObjectReader recurse(boost::any& key) = 0;

  virtual bool isEnd() const = 0;
  virtual MemberArity getArity() const = 0;
  virtual bool isNested() const = 0;
  virtual size_t size() const = 0;
};

/** elementdata struct. Has two partial specialisations, for iterable
    and for map */
template<class Type, typename T>
struct elementdata
{
  const T* object;
  bool have_read;
  typedef T value_type;
  typedef T elt_value_type;
  typedef void* elt_key_type;
};

template<typename T>
struct elementdata<dco_read_iterable,T>
{
  typename T::const_iterator         ii;
  const T* object;
  typedef T value_type;
  typedef typename T::value_type elt_value_type;
  typedef void* elt_key_type;
};

template<typename T>
struct elementdata<dco_read_map,T>
{
  typename T::const_iterator         ii;
  const T* object;
  typedef T value_type;
  typedef typename T::mapped_type elt_value_type;
  typedef typename T::key_type    elt_key_type;
};


template<class T>
class ReadElement: public elementdata< typename dco_traits<T>::rtype, T>,
  public ReadElementBase
{
  typedef elementdata<typename dco_traits<T>::rtype, T> par;
public:
  ReadElement(const T& data)
  { constructor(data, typename dco_traits<T>::rtype()); }

private:
  void constructor(const T& data, const dco_read_single&)
  { par::object = &data; par::have_read = false; }
  void constructor(const T& data, const dco_read_iterable&)
  { par::ii = data.begin(); par::object = &data; }
  void constructor(const T& data, const dco_read_map&)
  { par::ii = data.begin(); par::object = &data; }

public:
  bool isEnd() const final
  { return _isEnd(typename dco_traits<T>::rtype()); }

private:
  inline bool _isEnd(const dco_read_single&) const
  { return par::have_read; }
  inline bool _isEnd(const dco_read_iterable&) const
  { return par::ii == par::object->end(); }
  inline bool _isEnd(const dco_read_map&) const
  { return par::ii == par::object->end(); }

public:
  MemberArity getArity() const
  { return getArity(typename dco_traits<T>::rtype()); }

private:
  inline MemberArity getArity(const dco_read_single&) const
  { return Single; }
  inline MemberArity getArity(const dco_read_iterable&) const
  { return Iterable; }
  inline MemberArity getArity(const dco_read_map&) const
  { return Mapped; }

public:
  size_t size() const
  { return size(typename dco_traits<T>::rtype()); }

private:
  inline size_t size(const dco_read_single&) const
  { return size_t(1); }
  inline size_t size(const dco_read_iterable&) const
  { return par::object->size(); }
  inline size_t size(const dco_read_map&) const
  { return par::object->size(); }

public:
  bool isNested() const
  { return isNested(dco_nested<typename par::elt_value_type>()); }

private:
  inline bool isNested(const dco_isdirect&) const
  { return false; }
  inline bool isNested(const dco_isnested&) const
  { return true; }

private:
  inline void step(const dco_read_single&) { par::have_read = true; }
  inline void step(const dco_read_iterable&) { ++par::ii; }
  inline void step(const dco_read_map&) { ++par::ii; }

public:
  CommObjectReader recurse(std::string& key)
  { return recurse(key, dco_nested<typename par::elt_value_type>()); }
  CommObjectReader recurse(boost::any& key)
  { return recurse(key, dco_nested<typename par::elt_value_type>()); }
private:
  inline CommObjectReader recurse(std::string& key, const dco_isnested&)
  { get_key(key,typename dco_traits<T>::rtype());
    return CommObjectReader(par::elt_value_type::classname,
                            &get_object(typename dco_traits<T>::rtype())); }

  inline CommObjectReader recurse(std::string& key, const dco_isdirect&)
  { throw TypeIsNotNested(); }

  inline CommObjectReader recurse(boost::any& key, const dco_isnested&)
  { get_key(key, typename dco_traits<T>::rtype());
    return CommObjectReader(par::elt_value_type::classname,
                            &get_object(typename dco_traits<T>::rtype())); }
  inline CommObjectReader recurse(boost::any& key, const dco_isdirect&)
  { throw TypeIsNotNested(); }

private:
  const typename par::elt_value_type& get_object(const dco_read_single& x)
  { const typename par::elt_value_type *tmp = par::object;
    step(x); return *tmp; }
  const typename par::elt_value_type& get_object(const dco_read_iterable& x)
  { const typename par::elt_value_type *tmp = &(*par::ii);
    step(x); return *tmp; }
  const typename par::elt_value_type& get_object(const dco_read_map& x)
  { const typename par::elt_value_type *tmp = &(par::ii->second);
    step(x); return *tmp; }

  const typename par::elt_value_type& peek_object(const dco_read_single& x)
  { const typename par::elt_value_type *tmp = par::object;
    return *tmp; }
  const typename par::elt_value_type& peek_object(const dco_read_iterable& x)
  { const typename par::elt_value_type *tmp = &(*par::ii);
    return *tmp; }
  const typename par::elt_value_type& peek_object(const dco_read_map& x)
  { const typename par::elt_value_type *tmp = &(par::ii->second);
    return *tmp; }

  void get_key(std::string&, const dco_read_single&) { }
  void get_key(std::string&, const dco_read_iterable&) { }
  void get_key(std::string& key, const dco_read_map&)
  { key = boost::any_cast<std::string>(par::ii->first); }
  void get_key(boost::any&, const dco_read_single&) { }
  void get_key(boost::any&, const dco_read_iterable&) { }
  void get_key(boost::any& key, const dco_read_map&)
  { key = par::ii->first; }

public:
  void read(std::string& res, std::string& key)
  { read_as(res, key, dco_nested<typename par::elt_value_type>()); }
  void read(boost::any& res, boost::any& key)
  { read_any(res, key, dco_nested<typename par::elt_value_type>()); }

private:
  void read_as(std::string& val, std::string& key, const dco_isnested&)
  { throw ConversionNotDefined() ;}
  void read_as(std::string&val, std::string&key, const dco_isdirect&)
  { get_key(key, typename dco_traits<T>::rtype());
    val = boost::lexical_cast<std::string>
      (get_object(typename dco_traits<T>::rtype())); }
  void read_any(boost::any& val, boost::any& key, const dco_isnested&)
  { throw ConversionNotDefined() ;}
  void read_any(boost::any& val, boost::any& key, const dco_isdirect&)
  { get_key(key, typename dco_traits<T>::rtype());
    val = boost::any(get_object(typename dco_traits<T>::rtype()));}
  void read_any(boost::any& val, boost::any& key, const dco_isenum&)
  { get_key(key, typename dco_traits<T>::rtype());
    val = boost::any
      (std::string(getString(get_object(typename dco_traits<T>::rtype()))));}

public:
  void peek(std::string& res, std::string& key)
  { peek_as(res, key, dco_nested<typename par::elt_value_type>()); }
  void peek(boost::any& res, boost::any& key)
  { peek_any(res, key, dco_nested<typename par::elt_value_type>()); }

private:
  void peek_as(std::string& val, std::string& key, const dco_isnested&)
  { throw ConversionNotDefined() ;}
  void peek_as(std::string&val, std::string&key, const dco_isdirect&)
  { get_key(key, typename dco_traits<T>::rtype());
    val = boost::lexical_cast<std::string>
      (peek_object(typename dco_traits<T>::rtype())); }
  void peek_any(boost::any& val, boost::any& key, const dco_isnested&)
  { throw ConversionNotDefined() ;}
  void peek_any(boost::any& val, boost::any& key, const dco_isdirect&)
  { get_key(key, typename dco_traits<T>::rtype());
    val = boost::any(peek_object(typename dco_traits<T>::rtype()));}
  void peek_any(boost::any& val, boost::any& key, const dco_isenum&)
  { get_key(key, typename dco_traits<T>::rtype());
    val = boost::any
      (std::string(getString(peek_object(typename dco_traits<T>::rtype()))));}
};

DUECA_NS_END;

#endif
