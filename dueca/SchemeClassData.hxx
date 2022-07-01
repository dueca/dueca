/* ------------------------------------------------------------------   */
/*      item            : SchemeClassData.hh
        made by         : Rene' van Paassen
        date            : 990709
        category        : header file
        description     :
        changes         : 990709 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef SchemeClassData_hh
#define SchemeClassData_hh

#include <list>
#include "scriptinterface.h"
#include <sstream>


using namespace std;
#include <dueca_ns.h>
DUECA_NS_START


class ScriptCreatable;

/** \file
    Header with classes and macros to make a class accessible from Scheme. */

/** Objects of this class contain all data necessary to interface an
    object in DUECA with scheme. This is normally used for the key
    components of DUECA, e.g. the Ticker, that are created from within
    the scheme script. It is also used for SchemeClassCreatable-derived
    helper classes. */
class GenericSchemeClassData
{
protected:

  /** The name for the class, as called from scheme. */
  char* scheme_name;

  /** A list of tags for the descendants that can also be accepted as
      valid for this class. */
  list<scm_t_bits> scheme_tags;

  /** Identifying tag, given by Scheme, at creation of the type/class
      in Scheme. */
  scm_t_bits scheme_tag;

  /** A pointer to the parent's class information. */
  GenericSchemeClassData *parent;

protected:
  /** At creation of a descendant type, this call from the parent is
      used to validate the descendant type as being a substitute for
      the parent. */
  void addTagValue(scm_t_bits t);

public:
  /** Constructor */
  GenericSchemeClassData(const char* scheme_name,
                         GenericSchemeClassData* parent);
  /** Destructor */
  ~GenericSchemeClassData();

  /** returns non-zero if the tag t is valid, i.e. part of the list of
      tags for descendants or the own tag. */
  int validTag(SCM object);

  /** Return the tag for this type. */
  inline scm_t_bits getTag() {return scheme_tag;}

  /** Set the tag value, called at creation of the type in scheme. */
  void setTag(scm_t_bits t);

  /** Return the name for this type. */
  char* getName();

  /** Return the "make-name" for this type, this is equal to the
      command in scheme used to create the type/call in scheme. */
  char* getMakeName();
};


/** Objects of this class contain all data necessary to interface an
    object in DUECA with scheme. This is normally used for the key
    components of DUECA, e.g. the Ticker, that are created from within
    the scheme script. It is also used for SchemeClassCreatable-derived
    helper classes. */
template<class T>
class SchemeClassData: public GenericSchemeClassData
{
  /** Class type definition */
  // typedef typename T class_t;

  /** Constructor. */
  SchemeClassData(const char* scheme_name,
                  GenericSchemeClassData *parent=NULL) :
    GenericSchemeClassData(scheme_name, parent)
  { }

public:
  /** Singleton */
  static SchemeClassData* single();
};

#define SCHEME_CLASS_DEC(C) \
  template<> SchemeClassData<C>* SchemeClassData<C>::single();

// ScriptCreatable has no parent, and is therefore special in instantiation
SCHEME_CLASS_DEC(ScriptCreatable);

// define for all other classes
#define SCHEME_CLASS_SINGLE(C,P,N)              \
template<> \
 SchemeClassData< C >* SchemeClassData< C >::single()  \
{ \
  static SchemeClassData< C > singleton \
    ( N, SchemeClassData< P >::single()); \
  return &singleton; \
}



DUECA_NS_END


#endif
