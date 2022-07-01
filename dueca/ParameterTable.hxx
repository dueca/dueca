/* ------------------------------------------------------------------   */
/*      item            : ParameterTable.hh
        made by         : Rene' van Paassen
        date            : 001006
        category        : header file
        description     :
        changes         : 001006 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef ParameterTable_hh
#define ParameterTable_hh

#ifdef ParameterTable_cc
#endif

#include <dueca_ns.h>
DUECA_NS_START
class GenericVarIO;

/** Element of a parameter table for configuration of modules.

    Modules can be extensively configured from the Scheme creation
    script. A parameter table, with ParameterTable objects, defines
    the names of parameters and the probing object to set these
    parameters in the module class.

    Note that the table always has to be closed off with a row of NULL
    pointers. Typical use would be:
    \code
    const ParameterTable* SomeClass::getParameterTable()
    {
      // note the use of static! We don't want this table to disappear
      static const ParameterTable table[] = {
        { "my-variable", new VarProbe<SomeClass,int>
          (REF_MEMBER(&SomeClass::my_variable)),
          "this controls the blah blah blah, enter an integer value\n"
          "between 0 and 10" },
        { "set-timing", new MemberCall<SomeClass,TimeSpec>
          (&SomeClass::setTimeSpec),
          "Control the update rate with a time specification" },
        { NULL, NULL,
          "A descrription for the complete module, purpose etc. " }
      };
      return table;
    }
    \endcode
    Note the use of the REF_MEMBER define, this is needed for compatibility
    with  older versions of gcc compilers (2.95).

    The third element of the ParameterTable class point to a
    description of the variable, and for the very last element in the
    table it points to a string with a description of the complete
    module. Note that older code (where this third element was not
    present), still compiles, only there are no descriptions.

    You can list all descriptions in a DUECA executable by starting it
    with the environment variable DUECA_SCRIPTINSTRUCTIONS set, e.g.:
    \code
    DUECA_SCRIPTINSTRUCTIONS=y ./dueca_run.x
    \endcode
    In this way you can get some documentation for writing the dueca.cnf
    and dueca.mod scripts.
*/
struct ParameterTable
{
  /** Name of parameter, as used from Scheme. */
  const char* name;

  /** Pointer to MemberCall or VarProbe object that gets the data into
      the module. */
  const GenericVarIO* probe;

  /** An additional comment to describe the required use of the
      parameter. If not added to the "table", it will be NULL. */
  const char* description;

  ~ParameterTable();
};

/** Two descriptions that are always used, since they are in the
    automatically generated module code. For efficiency reasonse, they
    are defined here, once. */
extern const char* set_timing_description;
extern const char* check_timing_description;

DUECA_NS_END
#endif
