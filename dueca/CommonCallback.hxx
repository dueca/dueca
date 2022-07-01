/* ------------------------------------------------------------------   */
/*      item            : CommonCallback.hxx
        made by         : Rene van Paassen
        date            : 200411
        category        : header file
        description     :
        changes         : 200411 first version
        language        : C++
        api             : DUECA_API
        copyright       : (c) 20 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CommonCallback_hxx
#define CommonCallback_hxx

#include <dueca/SharedPtrTemplates.hxx>
#include <dueca_ns.h>
#include <boost/intrusive_ptr.hpp>

DUECA_NS_START;

/** Base class for callback mechanism, 1 parameter

    CommonCallbackBase objects can be used to create callback functions on
    class instances.

    Use the "smart_ptr_type" to create a variable that holds the callback

    @tparam RET   Return type
    @tparam A1    Argument type
 */
template<typename RET, typename A1>
class CommonCallbackBase
#if USING_BOOST_INHERIT == 1
  : public boost::intrusive_ref_counter<CommonCallbackBase<RET,A1> >
#endif
{
#if USING_BOOST_INHERIT == 0
  mutable unsigned intrusive_refcount;
  template<typename RETx, typename A1x>
  friend void intrusive_ptr_add_ref(const CommonCallbackBase<RETx,A1x> *);
  template<typename RETx, typename A1x>
  friend void intrusive_ptr_release(const CommonCallbackBase<RETx,A1x> *);
#endif

public:
  /** Type definition of the callback object */
  typedef boost::intrusive_ptr<CommonCallbackBase<RET, A1> > smart_ptr_type;

  /** Callback function

      @param arg1   Argument 1 */
  virtual RET operator() (A1 &arg1) = 0;

protected:
  /** Constructor */
  CommonCallbackBase()
#if USING_BOOST_INHERIT == 0
    : intrusive_refcount(0)
#endif
  {}

public:
  /** Destructor */
  virtual ~CommonCallbackBase() {}
};

#if USING_BOOST_INHERIT == 0
template<typename RET, typename A1>
void intrusive_ptr_add_ref(const CommonCallbackBase<RET,A1> *t) \
{ t->intrusive_refcount++; } \
template<typename RET, typename A1>
void intrusive_ptr_release(const CommonCallbackBase<RET,A1> *t) \
{ if (--(t->intrusive_refcount) == 0) { delete t; } }
#endif

/** Specific class for callback mechanism, 1 parameter

    It is advisable to use the common_callback function to generate
    these objects.
 */
template<typename RET, typename A1, class M>
class CommonCallback: public  CommonCallbackBase<RET,A1>
{
public:
  /** Type for a pointer to the callback function */
  typedef RET (M:: *call_type) (A1&);
private:
  /** Object receiving the callback */
  M* obj;

  /** Pointer to the callback function */
  call_type call1;

public:

  /** Callback function

      @param a1      Argument 1 */
  RET operator() (A1 &a1) final { return ((*obj) .* call1) (a1); }

  /** Constructor */
  CommonCallback(M* obj, RET (M:: *call1) (A1&)) :
    CommonCallbackBase<RET, A1>(),
    obj(obj),
    call1(call1)
  { }

private:

  /** Destructor */
  ~CommonCallback() { }
};

/** Generate a callback object

    Creates a CommonCallback with the proper template instantiation.

    From within a class, this is easily used as:
    @code
    callback = common_callback(this, &MyClass::myFunction);
    @endcode

    The myFunction object may have only one argument. The callback variable
    must match the definition of argument and return value, for example, if
    myfunction is declared as follows in the class:

    @code
    bool myFunction(const std::string& testthis);
    @endcode

    The callback variable is defined as:

    @code
    CommonCallback<bool, const std::string> callback;
    @endcode
 */
template<typename RET, typename A1, class M>
typename CommonCallbackBase<RET,A1>::smart_ptr_type common_callback
(M* obj, RET (M:: *call) (A1&))
{
  return typename CommonCallbackBase<RET,A1>::smart_ptr_type
    (new CommonCallback<RET,A1,M>(obj, call));
}

DUECA_NS_END;

#endif
