/* ------------------------------------------------------------------   */
/*      item            : @Module@.hxx
        made by         : @author@
        from template   : DuecaHelperTemplate.hxx (2022.06)
        date            : @date@
        category        : header file
        description     :
        changes         : @date@ first version
        language        : C++
        copyright       : (c)
*/

#ifndef @Module@_hxx
#define @Module@_hxx

// include the dueca header
#include <ScriptCreatable.hxx>
#include <stringoptions.h>
#include <ParameterTable.hxx>
#include <dueca_ns.h>

USING_DUECA_NS;

/** A class definition for a DUECA helper class

    This class has been derived from the ScriptCreatable base class,
    and has a (scheme) script command to create it and optionally add
    parameters.

    The instructions to create an object of this class from the Scheme
    script are:

    \verbinclude @smodule@.scm
 */
class @Module@: public ScriptCreatable
{
private: // simulation data

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  @Module@();

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. */
  bool complete();

  /** Destructor. */
  ~@Module@();

  /** Obtain a pointer to the parameter table. */
  static const ParameterTable* getParameterTable();

public:
  /** Default script linkage. */
  SCM_FEATURES_DEF;

};

#endif
