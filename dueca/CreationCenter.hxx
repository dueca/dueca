/* ------------------------------------------------------------------   */
/*      item            : CreationCenter.hh
        made by         : Rene' van Paassen
        date            : 990723
        category        : header file
        description     :
        changes         : 990723 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef CreationCenter_hh
#define CreationCenter_hh

#include <stringoptions.h>
#include <map>

#include <dueca_ns.h>

DUECA_NS_START

class GenericTypeCreator;
class ModuleCreator;
class PrioritySpec;

/** Central point for creation of modules from a Scheme script,
    factory with distributed work, i.e. sub-contractors. */
class CreationCenter
{
  /** Map of module names to the (pointers of) the objects that can
      actually create these modules. */
  map<vstring, GenericTypeCreator*> type_map;

  /** Only one factory of this kind may exist. */
  static CreationCenter* singleton;

  friend class HaveNoFriends;

  /** Constructor. */
  CreationCenter();

  /** Destructor. */
  virtual ~CreationCenter();

public:
  /** Access to the single instance of this center. */
  static CreationCenter* single();

  /** This is called to extend the factory with a new module type. The
      type creator calls this at construction time.
      \param name  Name for the new type.
      \param c     Type creator that can create the new type. */
  void addObjectType(const std::string& name, GenericTypeCreator* c);

  /** Make a module creator, a one-time object that creates the
      module, when the name of the module and the further parameters,
      as scheme specifications, have come in. */
  ModuleCreator* createModuleCreator(const std::string& type_name,
                                     const std::string& part,
                                     const PrioritySpec& ps_ptr1);

  /** Returns true if a module with name c exists. */
  bool hasType(const std::string& c);

};

DUECA_NS_END

#endif
