/* ------------------------------------------------------------------   */
/*      item            : Entity.hh
        made by         : Rene' van Paassen
        date            : 990726
        category        : header file
        description     :
        changes         : 990726 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Entity_hh
#define Entity_hh

#include <list>
#include <NamedObject.hxx>
#include <PrioritySpec.hxx>
#include "ScriptCreatable.hxx"
#include <boost/intrusive_ptr.hpp>

using namespace std;

#include <dueca_ns.h>
DUECA_NS_START
class Module;
class ModuleCreator;
struct ModuleState;
class TimeSpec;

/** An encapsulating object for a set of modules. An Entity can be
    distributed over several nodes, and then each node will have an
    Entity object. */
class Entity:
  public ScriptCreatable,
  public NamedObject
{
protected:

  /** Flag to determine whether the entity is prepared. */
  bool is_prepared;

  /** Flag to determine whether the entity can be prepared. */
  bool is_initial_prepared;

  /** List of module creators. These can be used to create the actual
      modules. */
  list<boost::intrusive_ptr<ModuleCreator> > foetae;

  /** A list of all models locally to this node. */
  list<Module*> local_modules;

public:
  /** Objects of this class can be created from scheme. */
  SCM_FEATURES_DEF;

  /** Constructor. */
  Entity(const char* ename, const list<ModuleCreator*>& foetae);

  /** Constructor. */
  Entity(const std::string& ename);

  /** Destructor. */
  virtual ~Entity();

  /** Type name information */
  const char* getTypeName();

  /** An additional module */
  void addModule(ModuleCreator* module);

  /** An additional module */
  void addModule(boost::intrusive_ptr<ModuleCreator> module);

  /** Command the Entity . */
  void controlEntity(const TimeSpec &time, const ModuleState& state);

  /** Report status of all modules. */
  void reportStatus();

  /** Call to create the modules; right after construction of the
      entity itself. */
  void createModules();

  /** Deletion of a module */
  void completeModule(Module* mod);

  /** Tell dueca what kind of fish we are. */
  ObjectType getObjectType() const {return O_Entity;}

  /** Print to stream, for debugging purposes. */
  ostream& print (ostream& os) const;
};

DUECA_NS_END

PRINT_NS_START
inline ostream& operator << (ostream& os, const DUECA_NS::Entity& e)
{ return e.print(os); }
PRINT_NS_END
#endif
