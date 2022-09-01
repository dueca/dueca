/* ------------------------------------------------------------------   */
/*      item            : AssociateObject.hxx
        made by         : Rene van Paassen
        date            : 220707
        category        : header file
        description     :
        changes         : 220707 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2022 René van Paassen
*/

#ifndef AssociateObject_hxx
#define AssociateObject_hxx

#include <dueca/dueca_ns.h>

DUECA_NS_START

/** Proxy identity for helper classes associated with an object
    derived from dueca::NamedObject.

    In DUECA, objects that can use DUECA services (accessing channels,
    creating and using activities) need to have a name and an
    GlobalId, commonly by deriving from NamedObject. In addition, the
    convenience macros for checking channels and the like normally use
    the dueca::getclassname<>() templated function to access the
    "classname" member defined in modules.

    This template class enables helper classes/objects that don't have
    their own GlobalId to assume the GlobalId and name of their
    "owner", and sets the proper class name.

    Use this to delegate capabilities in a manner like (note,
    incomplete code!):
    @code
    MyModule: public Module
    {

      // defining a helper reading a channel
      struct MyHelper: public AssociateObject<MyModule>
      {
        ChannelReadToken token;

        MyHelper(const MyModule& m);
      };

      // e.g. a list of helpers
      typedef std::list<MyHelper> helperlist_t;

    };
    @endcode

    In your c++ file, ensure that the helpers have the proper "classname":
    @code
    template<>
    const char* const AssociateObject<MyModule>::classname =
      MyModule::classname;
    @endcode

    The MyHelper struct will now be able to open channels, create
    activities, etc. Its name and id are equal to that of the host
    class.
*/
template<class Host>
class AssociateObject: public NamedObject {
  ObjectType host_type;
public:
  static const char* const classname;

public:
  /** Constructor

      @param host   Host class for name and ID.
  */
  AssociateObject(const Host& host) :
    host_type(host.getObjectType()),
    // classname(getclassname<Host>()),
    NamedObject(host.getId())
  { }

  /** Destructor */
  ~AssociateObject() { }

  /** Object type for DUECA's sanity */
  ObjectType getObjectType() const final { return host_type; }
};

DUECA_NS_END

#endif
