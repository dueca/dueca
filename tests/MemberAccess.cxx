#include <exception>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <string>
#include <iostream>
#include <cassert>
#include <list>


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

class ElementAccessor;

class CommObjectReader
{
  void* obj;

public:
  CommObjectReader(const char* classname, void* obj = NULL) :
    // do something with classname
    obj(obj)
  { }

  void accessObject(void *obj)
  { this->obj = obj; }

  inline ElementAccessor operator [] (const char* ename);

  inline ElementAccessor operator [] (unsigned i);

  inline const char* getMemberName(unsigned i);
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

class ElementAccessorBase
{
public:

  ElementAccessorBase() {}

  virtual void read(std::string& r) { throw ConversionNotDefined(); };

  virtual void write(const std::string& i) { throw ConversionNotDefined(); };

  virtual bool isEnd() {return false;}

  virtual CommObjectReader recurse() { throw TypeIsNotNested(); }

  virtual void step() { throw TypeCannotBeIterated(); }

  virtual ~ElementAccessorBase() {}
};

template<class T>
class SingleElement: public ElementAccessorBase
{
  T*                          object;
public:
  typedef T                   value_type;

  SingleElement(T& data): object(&data) {}

  void read(std::string& r) { r = boost::lexical_cast<std::string>(*object); }

  void write(const std::string& i) { *object = boost::lexical_cast<T>(i); }

  virtual ~SingleElement() {}
};

template<class T>
class SingleNested: public ElementAccessorBase
{
  T*                          object;

public:
  typedef T                   value_type;
  typedef T                   elt_value_type;

  SingleNested(T& data): object(&data) {}

  CommObjectReader recurse()
  {
    return CommObjectReader(T::classname, object);
  }
};

template<class T>
class IterableElement: public ElementAccessorBase
{

  typename T::iterator         ii, end;
public:
  typedef T                   value_type;
  typedef typename T::value_type        elt_value_type;

  IterableElement(T& data): ii(data.begin()), end(data.end()) {}

  void read(std::string& r)
  { r = boost::lexical_cast<std::string>(*ii); }

  void write(const std::string& i)
  { *ii = boost::lexical_cast<elt_value_type>(i); }

  bool isEnd() {return ii == end;}

  void step() {++ii;}

  ~IterableElement() {}
};

template<class T>
class IterableNested: public ElementAccessorBase
{
  typename T::iterator         ii, end;

public:
  typedef T                    value_type;
  typedef typename T::value_type        elt_value_type;

  IterableNested(T& data): ii(data.begin()), end(data.end()) {}

  CommObjectReader recurse()
  {
    return CommObjectReader(T::value_type::classname, &(*ii));
  }

  bool isEnd() {return ii == end;}

  void step() {++ii;}

  ~IterableNested() {}
};

class ElementAccessor
{
  // room for the child
  void* stretch[5];
  // pointer to the child
  ElementAccessorBase *base;

public:
  ElementAccessor(): base(NULL) {}
  void setBase(ElementAccessorBase *b) { base = b;}
  void* getPlace() { return stretch; }
  inline void read(std::string& s) { base->read(s); }
  inline void write(const std::string& s) { base->write(s); }
  inline bool isEnd() { return base->isEnd(); }
  inline ElementAccessor& operator++ ()
  { base->step(); return *this; }
  inline CommObjectReader recurse()
  { return base->recurse(); }
};


class CommObjectMemberAccessBase
{
public:
  /** Constructor */
  CommObjectMemberAccessBase() {}

  /** Destructor */
  virtual ~CommObjectMemberAccessBase() {}

  /** Given a void pointer to an object, return a helper to access the
      element data */
  virtual ElementAccessor getAccessor(void* obj) = 0;
};

/** Access object to a unitary member of class C, with type of the
    member T

    @tparam C        DUECA CommObject (compatbile) class with members
    @tparam Access   Either derived from Access or DirectAccess,
                     to indicate whether the accessed member is actually
                     again a class that can be inspected, or a "simple type"
                     directly readable/writeable
    @tparam Arity    Either derived from IterableElement or SingleElement,
                     to indicate whether the accessed member is iterable
                     over, or a single element.
*/
template<typename C, typename Access>
class CommObjectMemberAccess : public CommObjectMemberAccessBase
{
  /** Class member pointer, to be combined with a class instance for
      accessing actual data */
  typename Access::value_type C:: *data_ptr;
public:
  /** Constructor, accepts a pointer to a class member */
  CommObjectMemberAccess(typename Access::value_type C:: *dp) :
    data_ptr(dp) { }

  /** Destructor */
  ~CommObjectMemberAccess() {}

  /** Return an accessor object that can access the actual data. If
      the data member is iterable (list, vector, etc.), the accessor
      objects's methods allow iterating over the elements. */
  ElementAccessor getAccessor(void* obj)
  {
    // create a new element accessor
    ElementAccessor a;
    // insert the inner accessor with placement new
    a.setBase(new(a.getPlace()) Access
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

class Testobject
{
public:
  int i;
  double x;
  Test2 n;
  std::list<int>  l;
  std::list<Test2>  l2;

};



static CommObjectMemberAccess
<Testobject,SingleElement<int> >Testobject_member_i(&Testobject::i);
static CommObjectMemberAccess
<Testobject,SingleElement<double> >Testobject_member_x(&Testobject::x);
static CommObjectMemberAccess
<Testobject,SingleNested<Test2> >Testobject_member_n(&Testobject::n);
static CommObjectMemberAccess
<Testobject,IterableElement<std::list<int> > >Testobject_member_l(&Testobject::l);
static CommObjectMemberAccess
<Testobject,IterableNested<std::list<Test2> > >Testobject_member_l2(&Testobject::l2);
int main()
{
  Testobject to;

  assert(sizeof(SingleElement<int>) == 16);

  to.x = 5.0; to.i = 2;

  ElementAccessor acc_i = Testobject_member_i.getAccessor
    (reinterpret_cast<void*>(&to));

  std::string test; acc_i.read(test);
  std::cout << test << std::endl;
};
