/* ------------------------------------------------------------------   */
/*      item            : SchemeObject.hh
        made by         : Rene' van Paassen
        date            : 990707
        category        : header file
        description     :
        changes         : 990707 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef SchemeObject_hh
#define SchemeObject_hh

#ifdef SchemeObject_cc
#endif

#include <scriptinterface.h>
#include <list>
#include <dueca/visibility.h>
#include <dueca_ns.h>
#include <boost/intrusive_ptr.hpp>

DUECA_NS_START

/** Pre-define */
class ScriptCreatable;
class ModuleCreator;

/** Attachment to scheme, for an object -- i.e. instance -- of a
    class. */
class SchemeObject
{
  /** The scheme representation of this object. */
  SCM self;

  /** A list with all scheme stuff that depends on me. At the time
      Scheme tries to perform garbage collection, all referred stuff
      must be tagged by me to keep it from being collected out of
      the way. */
  //std::list<SCM> referred;
  SCM referred;

  /** Pointer to the object wrapped */
  boost::intrusive_ptr<ScriptCreatable> object;

  /** Or pointer to a wrapped ModuleCreator */
  boost::intrusive_ptr<ModuleCreator> module;

  /** Copy constructor. */
  SchemeObject(const SchemeObject& o);

  /** Assignment */
  SchemeObject& operator = (const SchemeObject& o);

public:

  /** Constructor. */
  SchemeObject(ScriptCreatable* r);

  /** Constructor. */
  SchemeObject(ModuleCreator* r);

  /** Destructor. */
  ~SchemeObject();

  /** Add an object that I depend on.

      @param r     Pointer to the SchemeCreatable to keep alive.

      For Scheme, the object can be added by giving the Scheme ID as
      argument, using the getSCM() function. However, note that this
      is obsolete. This call does nothing for Python scripting.
 */
  // void addReferred(ScriptCreatable* r);

  /** Add an object that I depend on.

      @param r     SCM object.

      For Scheme, the object can be added by giving the Scheme ID as
      argument, using the getSCM() function. However, note that this
      is obsolete.
  */
  void addReferred(SCM r);

  /** Mark all referred objects as being in use, and therefore not
      candidate for garbage collection. */
  void markReferred();

  /** Return the scheme object. */
  SCM getSCM() const;

  /** Set the scheme object. */
  void setSCM(SCM obj);

  /** Get the object pointer */
  inline ScriptCreatable* getObject() { return object.get(); }

  /** Get the object pointer */
  inline ModuleCreator* getModuleCreator() { return module.get(); }
};

DUECA_NS_END
#endif


