/* ------------------------------------------------------------------   */
/*      item            : SharedPtrTemplates.hxx
        made by         : Rene van Paassen
        date            : 180307
        category        : header file
        description     :
        changes         : 180307 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef SharedPtrTemplates_hxx
#define SharedPtrTemplates_hxx

#include <dueca/dueca_ns.h>
#include <dueca/dueca-conf-intrusive.h>
#include <boost/intrusive_ptr.hpp>
#ifdef HAVE_BOOST_SMART_PTR_INTRUSIVE_REF_COUNTER_HPP
#define USING_BOOST_INHERIT 1
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#else
#define USING_BOOST_INHERIT 1
#endif


#if USING_BOOST_INHERIT == 0

#define INCLASS_REFCOUNT(A)	       \
  mutable unsigned intrusive_refcount; \
  friend void intrusive_ptr_add_ref(const A *); \
  friend void intrusive_ptr_release(const A *)

#define INIT_REFCOUNT			\
  intrusive_refcount(0U)

#define INIT_REFCOUNT_COMMA			\
  intrusive_refcount(0U),

#define CODE_REFCOUNT(A) \
void intrusive_ptr_add_ref(const A *t) \
{ t->intrusive_refcount++; } \
void intrusive_ptr_release(const A *t) \
{ if (--(t->intrusive_refcount) == 0) { delete t; } }

#define INHERIT_REFCOUNT(A)
#define INHERIT_REFCOUNT_COMMA(A)

#else

#define INCLASS_REFCOUNT(A)
#define INIT_REFCOUNT
#define INIT_REFCOUNT_COMMA
#define CODE_REFCOUNT(A)

#define INHERIT_REFCOUNT(A)			\
  : public boost::intrusive_ref_counter< A >
#define INHERIT_REFCOUNT_COMMA(A) \
  public boost::intrusive_ref_counter< A >,

#endif

#endif
