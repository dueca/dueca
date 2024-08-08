/* ------------------------------------------------------------------   */
/*      item            : ConglomerateFactory.hxx
        made by         : Rene van Paassen
        date            : 100201
        category        : header file/only templates
        description     : This implements a factory pattern, where the
                          factory may be extended at will with subsidiaries
        changes         : 100201 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ConglomerateFactory_hxx
#define ConglomerateFactory_hxx
#include <map>
#include <exception>
#include <iostream>
#include <iomanip>

/** An exception to be thrown when things cannot be made */
class CFCannotMake: public std::exception
{
public:
  /** Print exception reason */
  const char* what() const throw() {return "cannot create this type";}
};

/** An exception to be thrown when creation results in an error */
class CFErrorConstruction: public std::exception
{
  /** Reason */
  const char* reason;
public:
  /** Constructor

      @param error   Text describing the error
  */
  CFErrorConstruction(const char* error) { reason = error; }

  /** Print exception reason */
  const char* what() const throw() {return reason;}
};


/** A simple factory, uses subcontractors to create objects. A key
    defines the class of the desired object, and the subcontractor who
    will create the object.

    What is a "Factory" in software engineering? It has become known as
    a pattern, a typical solution.

    The problem this factory was originally written for may serve as
    an illustration. I needed a way to flexibly add graphical objects
    to a scene graph visualization. The objects all behave similar in
    some respects (they are visible, have a location, can be moved
    around, stuff like that), but they are all slightly
    different. Some examples:

    <ul>
    <li> A skydome, that keeps itself centered over the "observer"
    viewpoint
    <li> Decorations in the world that simply stay put
    <li> Aircraft flying around that need to be controlled
    </ul>

    And I might want to extend the repertoire later. If you solve that
    "the classical" way, you have to add a case to your graphics code
    for each new type of object; e.g. add a model for the Citation
    with moving gear and control surfaces. That is something I wanted
    to avoid.

    The factory solves it in a different way. This factory has a
    "map", basically an array indexed with, for example, a string. The
    elements in the array point to "subsidiaries", pieces of code that
    are able to create an object of a desired type.

    If you want to add a model, now you simply create the code for the
    model (as before), and then you extend the factory by adding a
    "subsidiary", a supplier for your factory. There is a template to
    do that. Now, by simply adding your code to a project, the factory
    is automatically extended to include the new type of (graphical)
    object.

    There are a number of steps in creating a factory of your own:

    Step 1, define a specification of the types in your base
    product/spec class:

    \code
    struct MyKey
    {
      // the key type that differentiates the products/subsidiaries
      typedef std::string Key;
      // base type of the product (may be pointer, may be reference,
      // may be smart pointer)
      typedef MyProduct ProductBase;
      // base type / only type for specification (what product is made of)
      // must be flexible enough to produce all wanted products
      typedef MySpecification SpecBase;
    };
    \endcode

    This definition is typically used by a client of the factory. The
    exact type of product created is defined by the Key; usually a
    string-like type. Of course, replace My... etc by the thing you
    are actually interested in.

    Step 2, instantiate a factory with the base type as template
    parameter. You can use the singleton provided here, and the factory
    template

    \code
    typedef CFSingletonWrapper<
      ConglomerateFactory<
        MyKey,
        SubcontractorBase<MyKey>* > >
        MyNewFactory;
    \endcode

    It can be seen that here the subcontractors are stored as a pointer.

    Step 3, create a base class for your products. This class defines
    the interaction with the simulation, in the above example, with
    the state updating and object drawing code. In this example the
    base class has virtual functions to access the simulation data and
    interact with the scene graph code.

    Step 4, For each (family of) products, create a class derived from
    your base class, implementing the desired behaviour.

    The derived class should implement the following constructor:
    \code
    class MySpecificObject: public MyGenericObject
    {
      MySpecificObject(const MyKey::SpecBase& argument);
      ...
    };

    Note that the MyKey::SpecBase (or MySpecification) must be generic
    enough to be able create all derived objects. In some cases you might
    have to resort to strings with JSON or XML.

    Step 5, instantiate a subcontractor to the factory, using the
    CFSubcontractor template, e.g.

    \code
    static CFSubcontractor<MyKey, MySpecificObject, MyNewFactory>
      MySpecificObject_maker
        ("MySpecificObject",
         "Some illustrative information about this object");
    \endcode

*/
template <typename X, typename SubConPtr>
class ConglomerateFactory
{
  /** Helper, type of the map with subcontractors. */
  typedef std::map<const typename X::Key, SubConPtr> SubcontractorMap;

  /** Map with subcontractors who can create a specific product */
  SubcontractorMap subcontractors;

public:
  /** Constructor */
  ConglomerateFactory() :
    subcontractors()
  { }

  /** Destructor */
  ~ConglomerateFactory() { }

  /** Create one exemplar. First finds the subcontractor, based on
      the key, and then invokes the create function of that
      subcontractor. Note that this can return a default object
      when there is no factory for this type!
      \param key    Type of object that is going to be created.
      \param spec   Specifications/arguments for the creation. */
  typename X::ProductBase create
  (const typename X::Key& key, const typename X::SpecBase& spec) const
  {
    typename SubcontractorMap::const_iterator jj = subcontractors.find(key);
    if (jj == subcontractors.end()) {
      throw(CFCannotMake());
    }
    return jj->second->create(key, spec);
  }

  /** Check for presence of a specific key */
  bool haveSubcontractor(const typename X::Key& key)
  { return subcontractors.find(key) != subcontractors.end(); }

  /** Add a subcontractor, who can make a specific type of
      object. The subcontractor needs to be derived from
      SubcontractorBase, and implement the create function.
      \param key        Type of object this subcontractor can create
      \param contractor Pointer or reference to the subcontractor
      \returns Double contracting is silently ignored, returns false. */
  bool addSubcontractor(const typename X::Key& key,
                        const SubConPtr& contractor)
  {
    typename SubcontractorMap::iterator jj = subcontractors.find(key);
    if (jj == subcontractors.end()) {
      subcontractors.insert
        (typename SubcontractorMap::value_type(key, contractor));
      return true;
    }
    return false;
  }

  /** Remove a subcontractor from the pool.
      \param key        Type of object this subcontractor can create
      \param contractor Pointer or reference to the subcontractor
      \returns          false if the subcontractor/type is not found,
                        true otherwise */
  bool removeSubcontractor(const typename X::Key& key,
                           const SubConPtr& contractor)
  {
    typename SubcontractorMap::iterator jj = subcontractors.find(key);
    if (jj != subcontractors.end() && jj->second == contractor) {
      subcontractors.erase(jj);
      return true;
    }
    return false;
  }

  /** Print a listing of all on offer */
  void catalogue(std::ostream& os)
  {
    for (const auto sub: subcontractors) { sub.second->print(os); }
  }
};

/** Wrapper to create a singleton subsidiary class. */
template <class W>
class CFSingletonWrapper: public W
{
public:
  /** Return a reference to the single instance. */
  static CFSingletonWrapper<W> &instance()
  {
    static CFSingletonWrapper<W> *_instance = new CFSingletonWrapper<W>;
    return *_instance;
  }
};

/** Base subcontractor for the ConglomerateFactory */
template <typename Xbase>
struct SubcontractorBase
{
  /** Only function to implement. Return a pointer to an object
      \param key        Type of object (useful for cascading
      \param spec       Specification
      \returns          The product */
  virtual typename Xbase::ProductBase
  create(const typename Xbase::Key& key, const typename Xbase::SpecBase& spec)
  = 0;

  /** Print debug information to the given stream
      @param os         Output stream
  */
  virtual void print(std::ostream& os) { }
};

/** Specific subcontractor template

    Create a single instance of this subcontractor, which will be able to
    use the definition of the derived class (Derived) to create the desired
    objects of type XBase::ProductBase, for instance bare pointers, shared or
    intrusive pointers, etc.

    The constructor of the CFSubcontractor template will register and
    add the subcontractor to the conglomerate.

    @tparam XBase       Definition of the key type, base product type
                        and specification type
    @tparam Derived     Class defining the subcontractor's product. Needs
                        a constructor of the type
                        Derived(const XBase::SpecBase&);
    @tparam Conglomerat Class definition of the conglomerate, needs an
                        instance() singleton function.
 */
template <typename Xbase, typename Derived, typename Conglomerate>
  class CFSubcontractor: public SubcontractorBase<Xbase>
{
  /** The class name or label of the type of objects created here */
  typename Xbase::Key key;

  /** Description of the object */
  std::string         description;

public:
  /** Constructor, only uses the key as argument */
  CFSubcontractor(const typename Xbase::Key& key,
                  const std::string& description = std::string()) :
    key(key),
    description(description)
  {
    // access the factory singleton
    if (!Conglomerate::instance().addSubcontractor(this->key, this)) {
      std::cerr << "Could not register subcontractor for '"
                << key << "'" << std::endl;
    }
  }

  /** Destructor */
  ~CFSubcontractor()
  {
    if (!Conglomerate::instance().removeSubcontractor(this->key, this)) {
      std::cerr << "Could not unregister subcontractor for '"
                << key << "'" << std::endl;
    }
  }

  /** Only function, create a new object */
  typename Xbase::ProductBase create(const typename Xbase::Key& key,
                                     const typename Xbase::SpecBase& spec)
  {
    return typename Xbase::ProductBase(new Derived(spec));
  }

  /** Print name and description */
  void print(std::ostream& os)
  { os << std::setw(17) << key << " : " << description << std::endl; }
};

#endif
