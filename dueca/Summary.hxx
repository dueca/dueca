/* ------------------------------------------------------------------   */
/*      item            : Summary.hxx
        made by         : Rene van Paassen
        date            : 010819
        category        : header file
        description     :
        changes         : 010819 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Summary_hxx
#define Summary_hxx

#include <list>
#include <iostream>
using namespace std;
#include <dueca_ns.h>
#include <dueca/visibility.h>

DUECA_NS_START

template<class LI, class S, class V> class Summary;
template<class LI, class S, class V>
ostream& operator << (ostream& os, const Summary<LI,S,V>& o);

/** This is a nested tree of objects that hold zero, one or more
    Status reports. Each object either has status reports, then it is
    a leaf, or it has no status reports but branches; then it is a
    node in the tree. One can use this to maintain and calculate
    status/state of a distributed, tree-based system. */
template<class LI, class S, class V>
class Summary
{
  /** A link id, an object through which this node can be found. */
  LI*                       link_id;

  /** The status we want to calculate. */
  S*                        status;

  /** A link to the GTK world. */
  mutable void*     cnode;

  /** Defines a pointer type to this instantiated summary. */
  typedef Summary<LI,S,V>*    SummaryPtr;

  /** Shorthand for traversing over lists of summaries. */
  typename list<SummaryPtr>::iterator SPI;

  /** A list of branches, subsidiary to this node. If the size of the
      list is zero, then this is a leaf node. */
  list <SummaryPtr>         branches;

  /** A flag to determine whether status is correct. */
  bool dirty;

  /** My parent, if I have one. */
  Summary<LI,S,V>*            parent;

 public:

  /** Constructor. */
  Summary(LI* id, S* status, Summary<LI,S,V>* parent = NULL);

  /// Destructor
  ~Summary();

 public:

  /** Returns true if this is a leaf node in the tree. */
  inline bool isLeaf() { return branches.size() == 0; }

  /** Returns true if this status, or a status of descendants, has
      been changed. */
  inline bool isDirty() { return dirty; }

  /** This returns true if the node should be a descendant of the current
      one. All knowledge about "names" is contained in the LI
      object. */
  bool isDescendant(const LI& ls);

  /** This returns true if the node should be a child of the current
      one. All knowledge about "names" is contained in the LI
      object. */
  bool isChild(const LI& ls);

  /** This obtains the status. This status may be simply the
      LI status part if this is a leaf node, or it may be a combined
      status from all of the children. In that case the status is
      updated and a reference returned. */
  const S& getOrCalculateStatus();

  /** This obtains the status. It does no calculation. */
  inline S& getStatus() {return *status;}

  /** This updates the status of one of my leafs. As a side effect
      this set the dirty flag, if the present or a child's status has
      changed.
      \param ls        A link object, that however only is used to get
                       the match.
      \param newstatus new status.
      \returns         False if the ls name or id dont match this node nor a
                       descendant, otherwise true. */
  bool updateStatus(const LI& ls, const S& newstatus);

  /** Insert a node at the proper level in the tree. Depends on the LI
      object to tell whether related, child or at sibling.
      \returns  false If the status could not be inserted. */
  bool insertLinkAndStatus(const LI& l, const S& s);

  /** Check that a summary exists. */
  bool existsSummary(const LI& id);

  /** Obtain a reference to a summary, based on an id that must match
      the summary's. */
  Summary<LI,S,V>& findSummary(const LI& id);

  /** To indicate that the data has been changed. */
  void setDirty();

  /** Obtain the parent, if possible, throws if not. */
  const Summary<LI,S,V>& getParent() const;

  /** Return a pointer to the Gtk node linked to this summary. */
  void* getNode() const {return cnode;}

  /** Attach a pointer to the graphical representation. */
  void setNode(void* node) const {cnode = node;}

  /** Return a reference to the link. */
  LI& getLink() { return *link_id; }

  /** print to stream. */
  ostream& print(ostream& os) const;
  //friend ostream& operator << <> (ostream& os, const Summary<LI,S,V> & o);
};

/** An exception to be thrown when the tree is traversed. */
class LNK_PUBLIC NotFound: public exception
{
  /** Message. */
  static const char* msg;
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return msg;}
};

DUECA_NS_END
#endif
