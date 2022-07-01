/* ------------------------------------------------------------------   */
/*      item            : ListElementAllocator.hxx
        made by         : Rene van Paassen
        date            : 211217
        category        : header file
        description     :
        api             : DUECA_API
        changes         : 211217 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ListElementAllocator_hxx
#define ListElementAllocator_hxx

#include <dueca_ns.h>
DUECA_NS_START

/** Default allocator list objects.

    This allocates objects for dueca::AsyncList and
    dueca::AsyncQueueMT containers. The data objects of class T are
    created with a default constructor. If you have different needs,
    you can use this allocator as a template, and create custom
    allocators for these containers.
*/
template<class T>
struct ListElementAllocator
{
  /** Single elements of data, with link to the next. */
  template<class X>
  class Element
  {
  public:
    /** Pointer to the next element in the list. */
    Element<X> * volatile next;

    /** Data of the element. */
    X data;

    /** Constructor. */
    inline Element(Element<T>* next) : next(next), data() { }

    /** Destructor. */
    inline ~Element() { }
  };

  /** Shorthand type definition */
  typedef Element<T> element_type;

  /** Shorthand pointer definition */
  typedef Element<T>* element_ptr;

  /** Create a list element with data, for allocating new elements */
  inline Element<T>* operator () (element_ptr next=NULL)
  { return new Element<T>(next); }

  /** Create a list element with data on recycled memory, with
      placement new, for re-initialsing elements */
  inline Element<T>* operator () (void* mem, element_ptr next=NULL)
  { return ::new (mem) Element<T>(next); }
};

DUECA_NS_END

#endif
