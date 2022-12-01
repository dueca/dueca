/* ------------------------------------------------------------------   */
/*      item            : Reflectory.hxx
        made by         : Rene van Paassen
        date            : 151024
        category        : header file
        description     :
        changes         : 151024 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Reflectory_hxx
#define Reflectory_hxx

#include <boost/function.hpp>
#include <boost/intrusive_ptr.hpp>
#include <AsyncList.hxx>
#include <map>
#include <vector>
#include <dueca_ns.h>

#include "ReflectoryData.hxx"
#include "ReflectoryLocal.hxx"

DUECA_NS_START;


/** \file Reflectory.hxx

    The principles of the Reflectory trees:

    - A hierarchical collection of objects
    - Objects carry data
    - Objects have one owner-writer, and may have multiple readers
    - Only the owner writes the data
    - Data writing is time-released; time release is controlled by the
      clock, at every read it is checked
      * is there a following set of data
      * if so, is it time to release it
      * if so, then release it
    - Likewise, configuration changes are time-released.
    - All objects have a slot (hierarchy)
    - A reader may request an entry/object in the slot, which then
      becomes its object to manage and own

    Reflectory trees are replicated across different computing
    nodes. Each tree node has a master in only one of the nodes.

    For the base organisation, the following classes are used:

    - ReflectoryBase<TICK> This is a common base class, replicated in
                            all nodes. This contains a timed list of
                            configuration changes, the data (in void*)
                            common format, and a list of accessing views
                            It gets notified of data changes and
                            configuration changes from local or
                            remote points alike.

    - ReflectoryView<DATA,TICK> Reading of a reflectory node, each
                                piece of reading code (module etc.)
                                can create a view. The view links back
                                to a ReflectoryBase. Through a view
                                a child node can be created. On the
                                view, one can create a callback that
                                receives data and configuration
                                changes for the associated backend.

    - Reflectory<DATA,TICK> Master/writing end of a reflectory
                            node. The client must provide a callback
                            to respond to configuration changes.  The
                            callback gets called for:
                            * top-down imposed changes (basically node deletion)
                            * child data changes
                            * child induced changes, and child
                              responses.

    Reflectory nodes are all numbered, and a position in the
    reflectory is defined by a tuple of these numbers, e.g. (0,1)
    would mean a 3-deep node (since the root is implicit), so the
    first child of the root (e.g., nodemanager), second child of that
    one (node 1).

    When data comes in from elsewhere, it is marked by the tuple, so
    it can be sent to the proper node.

    Strategy for reflecting data.
    <ul>

    <li> Creation of a new local node is done with a reflectory
    constructor. The constructor uses the reference to the local root
    to find its way in the reflectory hierarchy. When the branch is
    found where the new node will be located, an addChild call is
    made. When that branch is local, the id number for the node is
    determined, and the data on creation is passed through a
    passConfigChange call with a ReflectoryData object. When the branch is
    remote, the id number will be issued after, and the node will
    initially not be valid. </li>

    <li> Update of the data is done through the newData call. This
    inserts data for a time that is at least later than the latest
    cycle of the Reflectory. The newData call will insert the data
    locally and call passDataChange to pass the data to copies of the
    reflectory. </li>

    </ul>

    Strategy for creating a copy of the reflectory.  When a new copy
    of the reflectory is requested, the reflectory tree is traversed
    with the date/cycle of the request, coding all configuration and
    data. At the same time the new changes for the new copy are also
    passed and saved up at the remote end. After the reflectory copy
    has been created, the new changes are fed in to catch up.
*/



/** Base class for distributed data

 */

// forward declaration
template<class DATA, typename TICK> class ReflectoryView;


/** */
template<class DATA, typename TICK>
class Reflectory: public ReflectoryLocal<TICK>
{
protected:
  //void registerWithParent(typename ReflectoryBase<TICK>::ref_pointer root);
public:
  typedef TICK ticktype;
  typedef DATA datatype;
  typedef typename boost::intrusive_ptr<ReflectoryBase<TICK> > ref_pointer;
  typedef Reflectory<DATA,TICK>* pointer;
  typedef Reflectory<DATA,TICK> parenttype;
public:

  /** Constructor only for root node */
  Reflectory();

  /** Constructor of the reflectory. */
  Reflectory(typename ReflectoryBase<TICK>::ref_pointer root, const std::string path,
             typename ReflectoryBase<TICK>::child_change& childhandler,
             typename ReflectoryBase<TICK>::node_change& nodehandler);

  virtual ~Reflectory()
  {
    //
  }
  void notify(const TICK& tick)
  {
    std::cout << "notified" << std::endl;
  }

  const char* getDataClass() const
  {
    return DATA::classname;
  }

};



DUECA_NS_END;

#endif
