/* ------------------------------------------------------------------   */
/*      item            : TrimView.hxx
        made by         : Rene van Paassen
        date            : 010826
        category        : header file
        description     :
        changes         : 010826 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef TrimView_hxx
#define TrimView_hxx

#include <dueca_ns.h>
#include <stringoptions.h>
#include <vector>
#include <IncoMode.hxx>
#include <TrimLink.hxx>
#include "IncoVariableWork.hxx"

DUECA_NS_START

class IncoCalculator;
struct IncoVariable;

/** Presentation of trim controls and results on an interface
    window. */
class TrimView
{
protected:
  /** Singleton pointer. The singleton is not created automatically,
      so users, beware. */
  static TrimView* singleton;

  /** Returns true if this is the root, base class. All derived
      classes must return false. */
  virtual bool isRootClass();

public:

  /** Constructor. */
  TrimView();

  /** Destructor. */
  virtual ~TrimView();

  /** Obtain the single possible instance of this class. */
  static TrimView* single();

public:

  /** Return the currently selected trim mode. */
  virtual TrimMode getMode() const;

  /** Add an entity IncoCalculator to this view. This will make a main
      tree in the tree view, under which the entity can add trim
      variables.
      \param ename        (Main) name for the entity
      \returns            An integer handle for this entity. This
                          handle must be used in subsequent calls. */
  virtual int addEntity(const std::string& s, IncoCalculator *calculator);

  /** The reverse, remove an entity from the view. */
  virtual void removeEntity(const std::string& s);

  /** Add a single variable to this view. */
  virtual bool addVariable(const vector<vstring>& names,
                           int cal, int tvar,
                           const IncoVariableWork& ivar);

  /** Insert a new entity node.
      \param  name      name for the node
      \param  parent    pointer to the node's parent, to be re-cast to
                        the toolkit's objects that represent a parent
      \param  dueca_node dueca node number where the object lives.
      \param  obj       pointer to the object on the Dueca side.
      \returns          A pointer to the node on the toolkit side. */
  virtual void* insertEntityNode(const char* name, void* parent,
                                 int dueca_node, TrimLink* obj);

  /** Update the external view. */
  virtual void refreshView();

  /** Return a reference to an inco variable, given the calculator it
      comes from and the variable no. */
  virtual IncoVariableWork& getIncoVariable(unsigned int calculator,
                                            unsigned int variable);

};
DUECA_NS_END

#endif
