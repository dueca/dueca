/* ------------------------------------------------------------------   */
/*      item            : DuecaEnv.hxx
        made by         : Rene van Paassen
        date            : 010817
        category        : header file
        description     :
        changes         : 010817 first version
                          040220 Modified, to be able to generate
                                 specific instructions for one object
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DuecaEnv_hxx
#define DuecaEnv_hxx

#include <stringoptions.h>

#include <dueca_ns.h>
DUECA_NS_START

class ModuleCreator;

/** This is a singleton-like class (the only public functions are
    static) that reads variables from the environment and uses there
    to give generic instructions to the DUECA executuable.
    Currently implemented are:
    <ul>
    <li> DUECA_SCRIPTINSTRUCTIONS : With this variable set to a null
    string, all script-creatable objects and modules spit out their
    script instructions.

    With this variable set to a module name, only that module spits
    out its instructions, all other script addition notices are
    suppressed.
    </ul>
*/
class DuecaEnv
{
  /** Env to the dueca installation directory. */
  bool script_instructions;

  /** Object in question. */
  vstring object;

  /** Remember all OK */
  bool specific_to_be_done;

  /** Constructor. */
  DuecaEnv();

  /** Destructor. */
  ~DuecaEnv();

  /** singleton pointer. */
  static DuecaEnv* singleton;

  /** to prevent complaints about not being able to use a
      constructor. */
  friend class Nobody;
public:

  /** Return true if the user needs instructions on scripting options
      for this module or class. */
  static bool scriptInstructions(const std::string& modname);

  /** Returns true if the script instructions are for a specific
      module or class. */
  static bool scriptSpecific();

  /** Exit value and check on specific instructions */
  static int handledSpecific();

  /** File a module for completion */
  static void queueComplete(ModuleCreator* mod);

  /** Process completion calls */
  static void callComplete();
};

DUECA_NS_END
#endif
