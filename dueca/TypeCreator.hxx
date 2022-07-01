/* ------------------------------------------------------------------   */
/*      item            : TypeCreator.hh
        made by         : Rene' van Paassen
        date            : 990723
        category        : header file
        description     : This defines a module type and gives an interface
                          to ModuleCreator proxy objects.
        changes         : 990723 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2

https://stackoverflow.com/questions/33877467/set-a-python-variable-to-a-c-object-pointer-with-boost-python
*/

#ifndef TypeCreator_hh
#define TypeCreator_hh

#include "dueca/scriptinterface.h"
#include <dueca/GenericTypeCreator.hxx>
#include <stringoptions.h>
#include <vector>
#include "VarProbe.hxx"
#include "ArgListProcessor.hxx"
#include <dueca_ns.h>

DUECA_NS_START

/** Script-dependent type creator, still type-independent, fuses the
    ArgListProcessor with the type creator */
class ScriptTypeCreator: public GenericTypeCreator, public ArgListProcessor
{
public:
  /** Constructor */
  ScriptTypeCreator(const std::string& type_name,
                    const ParameterTable* table);

  /** Destructor */
  virtual ~ScriptTypeCreator();

  /** Process parameter values into an object */
  bool injectValues(ArgElement::arglist_t& vals, void* object);
};


/** Templated type creator, borrows most of its functionality from the
    GenericTypeCreator. For each module class to be used in DUECA,
    one, and only one, type creator should be made. The creation of
    the type creator makes the module available to Scheme. */
template<class T>
class TypeCreator: public ScriptTypeCreator
{
  /** Pointer to the single instance of this type creator. */
  static TypeCreator* singleton;

public:
  /** Constructor. With the template parameter and a pointer to the
      table, this enables access to module creation from Scheme.
      \param table Pointer to the parameter table. May be NULL, in
                   this case there are no parameters to be given in
                   the module creation. */
  TypeCreator(const ParameterTable* table);

  /** Destructor. */
  ~TypeCreator();

  /** Call to actually create a module.
      \param entity   Entity that this module will become a part of.
      \param part     Part name for the module.
      \param ps       Default priority specification for the module's
                      activities.
      \returns        A pointer to the newly created module. */
  Module* createModule(Entity* entity, const std::string& part, const
                       PrioritySpec& ps);
};

DUECA_NS_END

#endif

//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------

#if defined(DO_INSTANTIATE)
#ifndef TypeCreator_ii
#define TypeCreator_ii

#include <dueca_ns.h>
#include <TypeCreator.hxx>

DUECA_NS_START

template<class T>
TypeCreator<T>* TypeCreator<T>::singleton = NULL;

template<class T>
TypeCreator<T>::TypeCreator(const ParameterTable* table) :
  ScriptTypeCreator(T::classname, table)
{
  if (singleton != NULL) {
    std::cerr << "extra type creator created in file:" << __FILE__
              << std::endl;
  }
  singleton = this;
}

template<class T>
TypeCreator<T>::~TypeCreator()
{
  //
}

template<class T>
Module* TypeCreator<T>::createModule(Entity* entity, const vstring&
                                     part, const PrioritySpec& ps)
{
  return new T(entity, part.c_str(), ps);
}

DUECA_NS_END

#endif
#endif


