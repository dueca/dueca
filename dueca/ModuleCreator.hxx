/* ------------------------------------------------------------------   */
/*      item            : ModuleCreator.hh
        made by         : Rene' van Paassen
        date            : 990723
        category        : header file
        description     :
        changes         : 990723 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_INTERNAL
*/

#ifndef ModuleCreator_hh
#define ModuleCreator_hh

//#include <list>
#include <SharedPtrTemplates.hxx>
#include <stringoptions.h>
#include <ArgElement.hxx>
#include <dueca_ns.h>
#include <PrioritySpec.hxx>

DUECA_NS_START

class GenericTypeCreator;
class Entity;
class Module;
class NamedObject;
class PrioritySpec;
class ModuleCreator;

/** Class used in the two-stage process of module creation.

    Modules are created in a two-stage process. First for each module
    a ModuleCreator is made, this ModuleCreator keeps a reference to
    all parameters needed by the module. Then the Entity is created,
    and when ready, the Entity kickstarts the module creation, by
    calling the ModuleCreators. */
class ModuleCreator INHERIT_REFCOUNT(ModuleCreator)
{
  INCLASS_REFCOUNT(ModuleCreator);

  /** Copy constructor, not implemented */
  ModuleCreator(const ModuleCreator&);

  /** assignment, not implemented */
  ModuleCreator& operator = (const ModuleCreator&);
protected:

  /// processed arguments
  ArgElement::arglist_t processed_arguments;

  /// Name of the part.
  string32 part;

  /** Enumeration value to check state of creation */
  enum CreationState {
    Initial,         /** Creating and setting parameters */
    ArgumentError,   /** Arguments not all OK */
    CompletionError, /** Error in completion phase */
    Completed        /** Complete function called */
  };

  /** Arguments ok? */
  CreationState cstate;

  /** The father of the module, the TypeCreator that can call the
      constructor. */
  GenericTypeCreator* father;

  /** The Entity that the module belongs to. */
  Entity* entity;

  /** A pointer to the module itself, once it has been created. */
  Module* object;

  /** The priority specification to be used. */
  PrioritySpec prio_spec;

public:
  /** Constructor.

      \param part    Part name of the module, may become obsolete
      \param father  Pointer to the type creator for this class of objects.
      \param prio_spec Priority at which this object should run.
  */
  ModuleCreator(const std::string& part,
                GenericTypeCreator* father, const PrioritySpec& prio_spec);

  /// Destructor
  ~ModuleCreator();

  /** Enter module parameters.

      @param idx     Index into parameter table.
      @param value   Value to be set.
   */
  static void addParam(unsigned idx, boost::any value);

  /** Work to be done, creation of the module. */
  Module* createModule(Entity* e);

  /** Completion step */
  void completeModule();

  /** Print module name if available */
  const std::string& getName();

  /** Print entity name */
  std::string getEntityName();

  /** Get type name */
  const std::string& getType();

  /** Process prepared arguments, to be called after an update of the
      module. */
  void injectAndCheckComplete();

  /** Indicate parameter failure */
  inline void argumentError() { cstate = ArgumentError; }

  /** Get the father */
  inline const GenericTypeCreator* getFather() { return father; }

  /** Access the arguments */
  inline ArgElement::arglist_t& processed() { return processed_arguments; }
};




DUECA_NS_END
#endif
