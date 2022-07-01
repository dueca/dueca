#include <exception>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/any.hpp>
#include <typeinfo>
#include <fixvector.hxx>
#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <list>
#include <map>
#include <inttypes.h>

/*
  Renew the approach?

  http://www.boost.org/doc/libs/1_55_0/libs/type_traits/doc/html/boost_typetraits/background.html

  Type traits.

  Different types of data, to:
  - read, write, pack, unpack
  - iterate over/add/remove/replace/extend
    * iteration can use index?
    * iteration is list-like, linear
    * iteration is map-like, ordered and with pairs

  - For reading:
    * fixed size vector; iterator, start to end, array
    * variable size vector, dequeue; same
    * list; same
    * map, multimap; iterator, return key and value, or pair?
    * set; just iterate?

  Can use to distinguish between nesting or not:
  template <typename T>
  struct is_nested: public false_type{};

  In each dco header specialise
  template <>
  struct is_nested<MyDCO>: public true_type{};


  Can get names:
  template <typename T>
  const char* getclassname() { return T::classname; }

  template <>
  const char* getclassname<double>() { return "double"; }

  template <typename T>
  struct dco_type: public dco_type_single {};

  template <typename D>
  struct dco_type<list<D> >: public dco_type_list{};

  template <typename D>
  struct dco_type<std::vector<D> >: public dco_type_vector{};


  Get difference between vectorlike (overwrite and append possible)
  single element
  fixedsize (no append possible)
  maplike (key and value)
  listlike (insert, append)

  Distinctions for writing:
  - fixed size, with an iterator, insert from 0 to end, throw after end
  - list-like, no end, use push_back
  - map-like, insert
  - single, only one setting

*/


/* traits, capturing an element for reading */
struct dco_read_single {};
struct dco_read_iterable {};
struct dco_read_map {};

template <typename T>
struct dco_store { typedef dco_read_single type; };
template <typename D>
struct dco_store<std::list<D> > { typedef dco_read_iterable type; };
template <typename D>
struct dco_store<std::vector<D> > {typedef dco_read_iterable type; };
template <typename K, typename D>
struct dco_store<std::map<K,D> > {typedef dco_read_map type; };

/** Nested objects (themselves DCO) or not */
struct dco_isnested {};
struct dco_isdirect {};

template <typename T>
struct dco_nested: public dco_isdirect { };

/* traits, capturing an element for writing */
struct dco_write_single {};
struct dco_write_iterable {};
struct dco_write_fixedit {};
struct dco_write_map {};

template <typename T>
struct dco_write { typedef dco_write_single type; };
template <typename D>
struct dco_write<std::list<D> > { typedef dco_write_iterable type; };
template <typename D>
struct dco_write<std::vector<D> > {typedef dco_write_iterable type; };
template <int N, typename D>
struct dco_write<fixvector<N,D> > {typedef dco_write_fixedit type; };
template <typename K, typename D>
struct dco_write<std::map<K,D> > {typedef dco_write_map type; };


template <typename T>
const char* getclassname() { return T::classname; }

template <>
const char* getclassname<double>() { return "double"; }
template <>
const char* getclassname<int32_t>() { return "int32_t"; }
template <>
const char* getclassname<std::string>() { return "std::string"; }

class TypeCannotBeIterated: public std::exception
{
  /** Message. */
  static const char* msg;
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return msg;}
};

class TypeIsNotNested: public std::exception
{
  /** Message. */
  static const char* msg;
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return msg;}
};

class ConversionNotDefined: public std::exception
{
  /** Message. */
  static const char* msg;
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return msg;}
};

const char* TypeCannotBeIterated::msg = "this type cannot be iterated";
const char* TypeIsNotNested::msg = "this type is not nested";
const char* ConversionNotDefined::msg = "conversion is not defined";

class ElementReader;

class CommObjectReader
{
  const void* obj;

public:
  CommObjectReader(const char* classname, const void* obj = NULL) :
    // do something with classname
    obj(obj)
  { }

  void accessObject(const void *obj)
  { this->obj = obj; }

  inline ElementReader operator [] (const char* ename);

  inline ElementReader operator [] (unsigned i);

  inline const char* getMemberName(unsigned i);
};

class ElementWriter;

class CommObjectWriter
{
  void* obj;

public:
  CommObjectWriter(const char* classname, void* obj = NULL) :
    // do something with classname
    obj(obj)
  { }

  void accessObject(void *obj)
  { this->obj = obj; }

  inline ElementWriter operator [] (const char* ename);

  inline ElementWriter operator [] (unsigned i);

  inline const char* getMemberName(unsigned i);

  inline unsigned size();
};


// getting an int value out

// getting a string from a dueca string type
// template<int N>
// struct elementconverter<std::string, DUECA_NS ::Dstring<N> >
// {
//   inline string to(const DUECA_NS ::Dstring<N>& f)
//   {return string(f);}
//   inline dueca::Dstring<N> from(const std::string& t)
//   {return dueca::Dstring<N>(t);}
// };

class ReadElementBase
{
public:

  virtual ~ReadElementBase() {}
  ReadElementBase() {}

  virtual void read(std::string& res,
                    std::string& key)
  { throw ConversionNotDefined(); };
  virtual void read(boost::any& res,
                    boost::any& key)
  { throw ConversionNotDefined(); };

  virtual CommObjectReader recurse(std::string& key)
  { throw TypeIsNotNested(); }
  virtual CommObjectReader recurse(boost::any& key)
  { throw TypeIsNotNested(); }

  virtual bool isEnd() {return false;}
};

class WriteElementBase
{
public:

  WriteElementBase() {}

  virtual void write(const std::string& res,
                     const std::string& key)
  { throw ConversionNotDefined(); };
  virtual void write(const boost::any& res,
                     const boost::any& key)
  { throw ConversionNotDefined(); };
  virtual CommObjectWriter recurse(const std::string& key)
  { throw TypeIsNotNested(); }
  virtual CommObjectWriter recurse(const boost::any& key)
  { throw TypeIsNotNested(); }

  virtual ~WriteElementBase() {}
};

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
class ReadElement: public elementdata< typename dco_store<T>::type, T>,
  public ReadElementBase
{
  typedef elementdata<typename dco_store<T>::type, T> par;
public:
  ReadElement(const T& data)
  { constructor(data, typename dco_store<T>::type()); }

private:
  void constructor(const T& data, const dco_read_single&)
  { par::object = &data; par::have_read = false; }
  void constructor(const T& data, const dco_read_iterable&)
  { par::ii = data.begin(); par::object = &data; }
  void constructor(const T& data, const dco_read_map&)
  { par::ii = data.begin(); par::object = &data; }

public:
  bool isEnd() { return isEnd(typename dco_store<T>::type()); }

private:
  inline bool isEnd(const dco_read_single&)
  { return par::have_read; }
  inline bool isEnd(const dco_read_iterable&)
  { return par::ii == par::object->end(); }
  inline bool isEnd(const dco_read_map&)
  { return par::ii == par::object->end(); }

private:
  inline void step(const dco_read_single&) { par::have_read = true; }
  inline void step(const dco_read_iterable&) { ++par::ii; }
  inline void step(const dco_read_map&) { ++par::ii; }

public:
  CommObjectReader recurse(std::string& key)
  { return recurse(key, dco_nested<T>()); }
  CommObjectReader recurse(boost::any& key)
  { return recurse(key, dco_nested<T>()); }
private:
  inline CommObjectReader recurse(std::string& key, const dco_isnested&)
  { get_key(key,typename dco_store<T>::type());
    return CommObjectReader(T::classname,
                            &get_object(typename dco_store<T>::type())); }
  inline CommObjectReader recurse(std::string& key, const dco_isdirect&)
  { throw TypeIsNotNested(); }

  inline CommObjectReader recurse(boost::any& key, const dco_isnested&)
  { get_key(key, typename dco_store<T>::type());
    return CommObjectReader(T::classname,
                            &get_object(typename dco_store<T>::type())); }
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
  { get_key(key, typename dco_store<T>::type());
    val = boost::lexical_cast<std::string>
      (get_object(typename dco_store<T>::type())); }
  void read_any(boost::any& val, boost::any& key, const dco_isnested&)
  { throw ConversionNotDefined() ;}
  void read_any(boost::any& val, boost::any& key, const dco_isdirect&)
  { get_key(key, typename dco_store<T>::type());
    val = boost::any(get_object(typename dco_store<T>::type()));}
};


/** ElementReader  an object to read a member of a DCO object

    The ElementReader enables a generic level (string or boost::any)
    interaction with a member of a DCO object. Generally, the
    ElementReader will be provided by a CommObjectReader, which in
    turn may be supplied by a channel.

    If the member is iterable, it may be read multiple times by read
    calls, each read call will set an iterator to point to the next
    element of the member.

    Single members may be read in string or boost::any form, compound
    members (themselves DCO objects) may be recursively inspected
    using the recurse() call.

    The isEnd() call indicates whether all elements of a member have
    been read.
*/
class ElementReader
{
  /** room for the child that does the actual access */
  void* stretch[3];

  /** access the child that does the reading */
  inline ReadElementBase* base()
  { return reinterpret_cast<ReadElementBase*>(&stretch[0]);}

public:
  /** Constructor, do not use directly */
  ElementReader() {}

  /** return the spot for the placement constructor of the child */
  void* placement() { return stretch; }

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

  /** recursively access a nested object. k returns the key if it exists */
  inline CommObjectReader recurse(std::string& k)
  { return base()->recurse(k); }

  /** recursively access a nested object. k returns the key if it exists */
  inline CommObjectReader recurse(boost::any& k)
  { return base()->recurse(k); }

  /** returns true if the last value has been read. */
  inline bool isEnd() { return base()->isEnd(); }
};



template<class Type, typename T>
struct elementdataw
{
  T* object;
  bool have_written;
  typedef T value_type;
  typedef T elt_value_type;
};

template<typename T>
struct elementdataw<dco_write_iterable,T>
{
  T* object;
  typedef T value_type;
  typedef typename T::value_type elt_value_type;
};

template<typename T>
struct elementdataw<dco_write_map,T>
{
  T* object;
  typedef T value_type;
  typedef typename T::mapped_type elt_value_type;
  typedef typename T::key_type    elt_key_type;
};

template<typename T>
struct elementdataw<dco_write_fixedit,T>
{
  T* object;
  typename T::iterator ii;
  typedef T value_type;
  typedef typename T::value_type elt_value_type;
};


template<class T>
class WriteElement: public elementdataw< typename dco_write<T>::type, T>,
  public WriteElementBase
{
  typedef elementdataw<typename dco_write<T>::type, T> par;
public:
  WriteElement(T& data)
  { constructor(data, typename dco_write<T>::type()); }

private:
  void constructor(T& data, const dco_write_single&)
  { par::object = &data; }
  void constructor(T& data, const dco_write_iterable&)
  { par::object = &data; }
  void constructor(T& data, const dco_write_map&)
  { par::object = &data; }
  void constructor(T& data, const dco_write_fixedit&)
  { par::ii = data.begin(); par::object = &data; }

public:
  /** Access a member as a composite object, access the next element
      in an array of members or create the next element in an
      extensible array. */
  CommObjectWriter recurse()
  { return recurse(dco_nested<T>()); }

  /** Create an element in a map and access it. */
  CommObjectWriter recurse(const boost::any& key)
  { return recurse(key, dco_nested<T>()); }

private:
  inline CommObjectWriter recurse(const dco_isnested&)
  { return CommObjectWriter(par::elt_value_type::classname,
                            get_object(typename dco_write<T>::type())); }
  inline CommObjectWriter recurse(const dco_isdirect&)
  { throw TypeIsNotNested(); }

  inline CommObjectWriter recurse(const boost::any& key, const dco_isnested&)
  { return CommObjectWriter(par::elt_value_type::classname,
                            get_object(typename dco_write<T>::type(), key)); }
  inline CommObjectWriter recurse(const boost::any& key, const dco_isdirect&)
  { throw TypeIsNotNested(); }

private:
  inline void* get_object(const dco_write_single&,
                          const boost::any& key=boost::any())
  { return reinterpret_cast<void*>(par::object); }
  inline void* get_object(const dco_write_iterable&,
                          const boost::any& key=boost::any())
  { par::data->push_back(par::elt_value_type());
    return reinterpret_cast<void*>(&(par::object->back())); }
  inline void* get_object(const dco_write_map&,
                          const boost::any& key=boost::any())
  { return reinterpret_cast<void*>
      (&(*par::data->insert
         (std::pair<const typename par::elt_key_type,
          typename par::elt_value_type>
              (boost::any_cast<par::elt_key_type>(key),
               par::elt_value_type())).second)); }
  inline void* get_object(const dco_write_fixedit&,
                          const boost::any& key=boost::any())
  { if (par::ii == par::data->end()) throw indexexception();
    return reinterpret_cast<void*>(&(*par::ii++)); }

public:
  inline void write(const boost::any& val, const boost::any& key=boost::any())
  { write(dco_nested<T>(), typename dco_write<T>::type(), val, key); }

private:
  void write(const dco_isdirect&, const dco_write_single&,
             const boost::any& val, const boost::any& key)
  { *par::object = *boost::any_cast<typename par::elt_value_type>(&val); }
  void write(const dco_isdirect&, const dco_write_iterable&,
             const boost::any& val, const boost::any& key)
  { par::object->push_back
      (*boost::any_cast<typename par::elt_value_type>(&val)); }
  void write(const dco_isdirect&, const dco_write_fixedit&,
             const boost::any& val, const boost::any& key)
  { if (par::ii == par::data->end()) throw indexexception();
    *par::ii++ = *boost::any_cast<typename par::elt_value_type>(&val);  }
  void write(const dco_isdirect&, const dco_write_map&,
             const boost::any& key, const boost::any& val)
  { (*par::object)[*boost::any_cast<typename par::elt_key_type>(&key)] =
      *boost::any_cast<typename par::elt_value_type>(&val); }

  template<typename Dum>
  void write(const dco_isnested&, const Dum&,
             const boost::any& val, const boost::any& key)
  { throw(ConversionNotDefined()); }
};

class ElementWriter
{
  // room for the child
  void* stretch[3];
  // pointer to the child
  WriteElementBase *base;

public:
  ElementWriter(): base(NULL) {}
  void setBase(WriteElementBase *b) { base = b;}
  void* getPlace() { return stretch; }

  inline void write(const boost::any& a, const boost::any& key=boost::any())
  { base->write(a, key); }
  inline CommObjectWriter recurse(const boost::any& key=boost::any())
  { return base->recurse(key); }
};


/** Base class for CommObjectMemberAccess */
class CommObjectMemberAccessBase
{
public:
  /** Constructor */
  CommObjectMemberAccessBase() {}

  /** Destructor */
  virtual ~CommObjectMemberAccessBase() {}

  /** Given a void pointer to an object, return a helper to access the
      element data */
  virtual ElementReader getReader(const void* obj) = 0;

  /** Given a void pointer to an object, return a helper to access the
      element data */
  virtual ElementWriter getWriter(void* obj) = 0;

  /** Get the class name of the element */
  virtual const char* getClassname() = 0;
};

/** Access object to a unitary member of class C, with type of the
    member T

    @tparam C        DUECA CommObject (compatbile) class with members
    @tparam T        Type/class of the accessed member. Traits for different
                     types are used to distinguish between different treatments;
                     i.e. single values versus lists/vectors/maps etc.
*/
template<typename C, typename T>
class CommObjectMemberAccess : public CommObjectMemberAccessBase
{
  /** Class member pointer, to be combined with a class instance for
      accessing actual data */
  T C:: *data_ptr;
public:
  /** Constructor, accepts a pointer to a class member */
  CommObjectMemberAccess(T C:: *dp) :
    data_ptr(dp) { }

  /** Destructor */
  ~CommObjectMemberAccess() {}

  /** Return a string representation of the data member's class */
  const char* getClassname()
  { return getclassname<typename elementdata
                        <typename dco_store<T>::type,T>::elt_value_type>(); }

  /** Return an accessor object that can read the actual data. If
      the data member is iterable (list, vector, etc.), the accessor
      objects's methods allow repeated reading of the element. */
  ElementReader getReader(const void* obj)
  {
    // create a new element accessor
    ElementReader a;
    // insert the inner accessor with placement new
    new(a.placement()) ReadElement<T>
      ( (* reinterpret_cast<const C*>(obj)) .* data_ptr);
    // and produce the result
    return a;
  }

  /** Return an accessor object that can write the actual data. If
      the data member is iterable (list, vector, etc.), the accessor
      objects's methods allow repeated writing until the object is
      full (or unlimited, if the object can be extended without limit). */
  ElementWriter getWriter(void* obj)
  {
    // create a new element accessor
    ElementWriter a;
    // insert the inner accessor with placement new
    a.setBase(new(a.getPlace()) WriteElement<T>
              ( (* reinterpret_cast<C*>(obj)) .* data_ptr) );
    // and produce the result
    return a;
  }
};

class Test2
{
public:
  static const char* classname;
  int j;
};

const char* Test2::classname = "Test2";
template <>
struct dco_nested<Test2>: public dco_isnested { };

class Testobject
{
public:
  int i;
  double x;
  Test2 n;
  std::list<int>  l;
  std::list<Test2>  l2;
  std::map<std::string,double> m;
  std::string s;
};



static CommObjectMemberAccess
<Testobject,int >Testobject_member_i(&Testobject::i);
static CommObjectMemberAccess
<Testobject,double >Testobject_member_x(&Testobject::x);
static CommObjectMemberAccess
<Testobject,Test2 >Testobject_member_n(&Testobject::n);
static CommObjectMemberAccess
<Testobject,std::list<int> >Testobject_member_l(&Testobject::l);
static CommObjectMemberAccess
<Testobject,std::list<Test2> >Testobject_member_l2(&Testobject::l2);
static CommObjectMemberAccess
<Testobject,std::map<std::string,double> >Testobject_member_m(&Testobject::m);
static CommObjectMemberAccess
<Testobject,std::string>Testobject_member_s(&Testobject::s);

int main()
{
  Testobject to;

  assert(sizeof(ReadElement<int>) == 24);

  to.x = 5.0; to.i = 2;
  to.m[std::string("test")] = 6.7;
  to.s = "noval";

  ElementReader acc_i = Testobject_member_i.getReader
    (reinterpret_cast<void*>(&to));

  std::string test; acc_i.read(test);
  std::cout << test << std::endl;

  ElementReader acc_m = Testobject_member_m.getReader
    (reinterpret_cast<void*>(&to));
  acc_m.read(test);
  std::cout << Testobject_member_m.getClassname() << test << std::endl;

  ElementReader acc_s = Testobject_member_s.getReader
    (reinterpret_cast<void*>(&to));
  acc_s.read(test);
  std::cout << Testobject_member_s.getClassname() << test << std::endl;

  boost::any ta;
  acc_s.read(ta);
  std::cout << *boost::any_cast<std::string>(&ta) << std::endl;

  ElementWriter acc_w = Testobject_member_s.getWriter
    (reinterpret_cast<void*>(&to));

  *boost::any_cast<std::string>(&ta) = std::string("new string");
  acc_w.write(ta);

  acc_s.read(ta);
  std::cout << *boost::any_cast<std::string>(&ta) << std::endl;
};
