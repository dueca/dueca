/* ------------------------------------------------------------------   */
/*      item            : GenericTypeCreator.hxx
        made by         : Rene van Paassen
        date            : 180322
        category        : header file
        description     :
        changes         : 180322 first version
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef GenericTypeCreator_hxx
#define GenericTypeCreator_hxx

#include <dueca_ns.h>
#include <ArgElement.hxx>
#include <string>

DUECA_NS_START

struct ParameterTable;
class PrioritySpec;
class ModuleCreator;
class Module;
class Entity;

/**  A class that connects module creation code (constructor) to the
     Scheme command make-module.

    Creation of one object of this class for a module (using the
    template) and a minimum of interfacing requirements, namely having
    a "const char* const classname" static data member and a parameter
    table enables creation of the module from scheme. Typical use is:
    \code
    static TypeCreator<MyModule> a(MyModule::getParameterTable());
    \endcode */
class GenericTypeCreator
{
  /** the classname of the module to be created. */
  std::string type_name;

protected:
  /** Constructor.
      @param type_name  Name for the type, as known in script.
      @param table      Points to table with parameter values. */
  GenericTypeCreator(const std::string& type_name, const char* vhash);

public:
  /** Destructor. */
  virtual ~GenericTypeCreator();

  /** Return a module creator, for modules of the type given in the
      type creator.
      \param part   Part name for the module.
      \param ps_ptr Pointer to the priority specification with
                    activity priority. */
  ModuleCreator* createModuleCreator(const std::string& part,
                                     const PrioritySpec& ps);

  /** Return the type of module this creator can create. */
  inline const std::string& getType() {return type_name;}

  /** Return a descriptive name, used by ArgListProcessor parent. */
  const char* callName() const { return type_name.c_str(); }

  /** Create a module. Module creators do just this, invoke this call.
      \param entity  Entity that the module will be a part of.
      \param part    Part name for the module.
      \param ps      Priority specification for the module's
                     activities. */
  virtual Module* createModule(Entity* entity, const std::string& part,
                               const PrioritySpec& ps) = 0;

  /** Process parameter values into an object */
  virtual bool injectValues(ArgElement::arglist_t& vals, void* object) = 0;
};

DUECA_NS_END

#endif
