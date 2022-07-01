/* ------------------------------------------------------------------   */
/*      item            : NamedStruct.hxx
        made by         : Joost Ellerbroek
        date            : 080128
        category        : header file
        description     :
        changes         : 080129 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
*/

#ifndef NAMED_STRUCT_HXX
#define NAMED_STRUCT_HXX
#include <list>
#include <string>
#include "TypeNameDef.hxx"
using namespace std;


class NamedStruct: public BaseObject
{
  public:
    /** Constructor.
        \param name       Name of the struct
        \param proj_name  Name of the rtw object this parametertable belongs to
        \param vars       List of type-parameter pairs to be put in parametertable
        \param comm       Optional comment for this struct */
    NamedStruct(const char* name, ObjectList* objs, const char* comm = "");

    /// Destructor.
    ~NamedStruct();

    /// Return the list of children
    const ObjectList* children()    {return child_list;}

    /// For a struct, getSize() returns the number of elements in the struct
    int    getSize() const          {return child_list->size();}

  private:
    /// A struct can contain any combination of other structs and variables
    ObjectList* child_list;
};

#endif
