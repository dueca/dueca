/* ------------------------------------------------------------------   */
/*      item            : ScriptInterpret.hh
        made by         : Rene' van Paassen
        date            : 990701
        category        : header file
        description     :
        changes         : 990701 first version
        language        : C++
        api             : DUECA_API
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ScriptInterpret_hh
#define ScriptInterpret_hh

#ifdef ScriptInterpret_cc
#endif

#include "StateGuard.hxx"
#include "NamedObject.hxx"
#include "Activity.hxx"
#include "Callback.hxx"
#include <list>
#include <stringoptions.h>
#include <fstream>
#include <dueca_ns.h>
#include <dueca/visibility.h>

int main(int argc, char* argv[]);

DUECA_NS_START
class GenericCallback;
struct ScriptLine;
struct ScriptConfirm;
class TimeSpec;
struct ScriptHelper;
struct PythonScripting;
struct SchemeScripting;
void init_module_dueca();
class ChannelReadToken;
class ChannelWriteToken;

/** Type of a function returning void. */
typedef void (*voidfunc)(void);

/** Init function class */
struct InitFunction
{
  /** Name of the associated class */
  const char* name;

  /** Name of a parent, if applicable */
  const char* parent;

  /** Function to call */
  voidfunc    func;

  /** Constructor */
  InitFunction(const char* name, const char* parent, voidfunc func);

  /** Copy constructor */
  InitFunction(const InitFunction& o);

  /** Call operator */
  void operator () (void) const;
};

/** Flag problems with script reading */
class LNK_PUBLIC scriptexception: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return "script exception";}
};

/** Interaction with the scripting language.

    Start and handle scripting in DUECA, either Scheme or Python
    currently. The ScriptInterpret singleton iss created by the main dueca
    program. It takes care of appending the configuration information
    (from a local file) and model information, sent by dueca, or from a
    local file on node 0, onto a file which is read and interpreted.

    This class is used mostly internally by DUECA, however client
    modules can add init functions (for python scripting) and have the
    interpreter run pieces of code.
 */
class ScriptInterpret: public NamedObject
{
  /// Can only use one of these objects.
  static ScriptInterpret* singleton;

  /** Helper to process specific script language */
  ScriptHelper           *helper;

  /** A list of all the functions that have to be added to scheme. The
      method addInitFunct can be used to add a function to this list. */
  list<const InitFunction*>     init_functions;

  /** A list of all the functions that have to be added to scheme. The
      method addInitFunct can be used to add a function to this list. */
  list<const InitFunction*>     unsorted_functions;

  /** Script init function */
  voidfunc       scriptinit;

  /** Flag to indicate that scheme has been entered. In principle we
      never get out again. */
  bool in_script;

  /** Flag to indicate that the model data has been completely read
      and copied. */
  bool model_copied;

  /** Vector that contains all confirmation status. */
  vector<int> confirmation_count;

  /** My number of received sets of scheme lines for model */
  uint16_t   received_sets;

  /** Number of sets for which the read go-ahead has been issued. */
  uint16_t   sent_sets;

  /** Callback on token completion */
  Callback<ScriptInterpret>                        token_valid;

  /** Function on token completion. */
  void tokenValid(const TimeSpec& ts);

  /** Flag to remember token completion. */
  bool token_action;

  /** Pointer to an access token for a channel to write the model
      script onto. This is only used by node 0. */
  ChannelWriteToken  *w_creation;

  /** Access token for the channel from which the model script is
      received. */
  ChannelReadToken   *t_creation;

  /** Confirmation for the received scheme commands */
  ChannelWriteToken  *w_confirm;

  /** Confirmation for the received scheme commands */
  ChannelReadToken   *t_confirm;

  /** Final command to go ahead and read the added configuration. */
  ChannelWriteToken  *w_goahead;

  /** \{ Callback function for reading model data. */
  Callback<ScriptInterpret>           cb1, cb2; /// \}

  /** Activity that reads the model data. */
  ActivityCallback*                   handle_lines;

  /** Activity for checking arrival confirms, only used on node 0. */
  ActivityCallback*                   process_confirm;

private:

  /** Constructor. Private, since an object of this class is
      automatically made after access to the single() method. */
  ScriptInterpret();

  /// Copy constructor, not implemented.
  ScriptInterpret(const ScriptInterpret&);

  /// Assignment operator, not implemented either
  ScriptInterpret& operator = (const ScriptInterpret&);

  /// Destructor
  ~ScriptInterpret();

  /// Method that performs the activity.
  void handleConfigurationLines(const TimeSpec& time);

  /// Method to check the confirmations
  void checkConfirms(const TimeSpec& time);

  /// Initialize scripting language and core modules
  static void initializeScriptLang();

  /// sort the init functions
  void sortInitFunctions();

private:
  friend void scheme_inner_main (void *closure, int argc, char **argv);
  friend class Environment;
  friend class NodeManager;
  friend int ::main(int argc, char* argv[]);
  friend struct PythonScripting;
  friend void init_module_dueca();
  friend struct SetScriptInitFunction;

  /** Opens the scratch file. */
  void initiate();

  /** Called when DUECA communication is operational, creates the
      channel access tokens. */
  void completeCreation();

  /** Read modification of modules, or create new modules. */
  bool readAdditionalModuleConf(const vstring& fname);

  /** Write (quit) onto the scheme file, for ovbious reasons. */
  void writeQuit();

  /** Start up the scheme interpreter. */
  void startScript();

  /** A flag to indicate that the complete model has been
      received. Only then we can return to scheme again. */
  inline bool modelCopied() { return model_copied;}

  /** Get the next init function */
  const InitFunction* getNextInitFunction();

  /** phase 2 */
  void createObjects();

public:

  /** Return the singleton. */
  static ScriptInterpret* single(ScriptHelper* helper = NULL);

  /// @cond DO_NOT_DOCUMENT
  /// @endcond DO_NOT_DOCUMENT
  /** @deprecated Add a new function to the script language. Anyone can add one,
      this is the way to make the script extensible with new
      commands.

      @param ifunct   A void function
  */
  DUECA_DEPRECATED("Use the variant with a specified name!")
  static void addInitFunction(voidfunc ifunct);

  /** Add a new function to the script language. Anyone can add one,
      this is the way to make the script extensible with new
      commands.

      @param name   A descriptive name for the function, used in error
                    messages.
      @param parent Parent class, if name is a class, NULL, if no parent.
      @param ifunct A void function
  */
  static void addInitFunction(const char* name, const char* parent,
                              voidfunc ifunct);

  /** Add a new init function to the script language */
  static void addInitFunction(const InitFunction* ifunct);

  /** Get the number of current init functions */
  inline size_t getNumInitFunctions() const { return init_functions.size(); }

  /** Run a piece of code in the interpreter. Only do this from the
      priority 0 thread!

      @param code   Code to run
      @throws       boost::python::error_already_set if a problem occurs. */
  void runCode(const char* code);

  /** Tell that we are a part of the DUECA core. */
  ObjectType getObjectType() const {return O_Dueca;}
};


/** Helper struct to add an init function from initialization code

    Use by creating an init function, and a static object to run the
    initialisation. When the script language is initialized, the init
    functions are called to make classes and functions available.

    \code
    void myinit()
    {
      // script init code
    }

    static AddInitFunction addmyinit(myinit);
    \endcode
 */
struct AddInitFunction
{
  /** Constructor that remembers and adds the init function.

      @param name    Name of the class or function, as known to the
                     script language.
      @param ifunct  Initialisation function.
      @param parent  Parent class, if applicable. Information on the
                     parent class is used to initialize classes in the
                     right order.
  */
  AddInitFunction(const char* name, voidfunc ifunct,
                  const char* parent = NULL);
};

/** Helper struct to add script init function from initialization code

    Use by creating an init function, and a static object to run the
    initialisation

    \code
    void myinit()
    {
      // script init code
    }

    static SetScriptInitFunction addmyinit(myinit);
    \endcode
 */
struct SetScriptInitFunction
{
  /** Constructor that defines the script init function */
  SetScriptInitFunction(voidfunc ifunct);
};



DUECA_NS_END
#endif
