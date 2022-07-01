/* ------------------------------------------------------------------   */
/*      item            : ReflectoryViewBase.hxx
        made by         : Rene van Paassen
        date            : 160928
        category        : header file
        description     :
        changes         : 160928 first version
        language        : C++
        copyright       : (c) 16 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ReflectoryViewBase_hxx
#define ReflectoryViewBase_hxx

#include <boost/function.hpp>

#include "SharedPtrTemplates.hxx"
#include "ReflectoryBase.hxx"

#include <dueca_ns.h>

DUECA_NS_START;

/** Creates an observation of a reflectory entry.

    Access the data for reading. A client is responsible for keeping a supplied
    callback function alive for the lifetime of this observation. */
template <typename TICK>
class ReflectoryViewBase: public ReflectoryParent
{

public:
  typedef TICK ticktype;
  typedef typename boost::intrusive_ptr<ReflectoryViewBase<TICK> > ref_pointer;
  typedef typename boost::intrusive_ptr<const ReflectoryViewBase<TICK> > const_ref_pointer;
  typedef ReflectoryViewBase<TICK>* pointer;

public:
  virtual void notify(const TICK& tick) = 0;

protected:

  typedef typename boost::intrusive_ptr<const ReflectoryBase<TICK> > backend_pointer;

  /** reference to the back-end with data and config */
  backend_pointer backend;

protected:
  /** Constructor

      @param root  root node of reflectory
      @param path  relative path to the node */
  ReflectoryViewBase(typename ReflectoryBase<TICK>::ref_pointer root,
                     const std::string& path) :
    backend(root->operator[] (path))
  {
    // if the backend does not exist, add this node as a waiter
    if (!backend) {
      root->addWaitingClient(ref_pointer(this), path);
    }
    else {
      backend->addView(this);
    }
  }

  /** Destructor. Notifies backend that there is one less view */
  virtual ~ReflectoryViewBase()
  {
    if (backend) {
      backend->removeView(this);
    }
  }
};

DUECA_NS_END;

#endif
