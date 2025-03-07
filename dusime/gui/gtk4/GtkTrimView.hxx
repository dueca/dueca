/* ------------------------------------------------------------------   */
/*      item            : GtkTrimView.hxx
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

#ifndef GtkTrimView_hxx
#define GtkTrimView_hxx

#ifdef GtkTrimView_cxx
#endif

#include <dueca-conf.h>

class IncoCalculator;
#include <TrimView.hxx>
#include <IncoMode.hxx>
#include <GtkGladeWindow.hxx>
#include <TrimId.hxx>
#include <TrimLink.hxx>
#include <Summary.hxx>
#include <dueca_ns.h>
DUECA_NS_START
class IncoCalculator;
struct IncoVariable;

// @note currently U/S

/** Presentation of trim controls and results on an interface
    window. */
class GtkTrimView: public TrimView
{
  GtkGladeWindow window;

  /** ctree widget with the tree of entities/trim values */
  GtkTreeStore* trim_tree;

  /** window with the trim calculation tree and controls. */
  GtkWidget* trim_view;

  /** feedback text label, with short messages. */
  GtkWidget* trim_status;

  /** widget in which the used can enter changes. */
  GtkSpinButton* entry_widget;

  /** menu item in main view menu. */
  GAction* menuitem;

  /** Remember whether the window is open. */
  bool window_open;

  /** Is there a calculation active, if yes, which client? */
  int calculation_active;

  /** GtkTrimView root object. */
  Summary<TrimId, TrimLink, TrimView> *root;

  /** List of calculators that checked in. */
  vector<IncoCalculator*> calculators;

  /** Present mode. */
  TrimMode mode;

  /** Singleton class */
  static GtkTrimView* singleton;

  /** Constructor. */
  GtkTrimView();

  /** Returns true if this is the root, base class. All derived
      classes must return false. */
  virtual bool isRootClass();

public:

  /** Singleton accessor. */
  static inline GtkTrimView& single() {
    if (singleton == NULL) singleton = new GtkTrimView();
    return *singleton;
  }

  /** Destructor. */
  ~GtkTrimView();

  /// \group handling callbacks from the interface

  /** Open or close the trim view window. */
  void toggleWindow(GtkButton *button, gpointer user_data);

  /** Select a new trim mode. */
  void setMode(GtkButton *button, gpointer user_data);

  /** Return the currently selected trim mode. */
  inline TrimMode getMode() const {return mode;}

  /** Calculate a trim, with the currently selected entity. */
  void calculate(GtkButton *button, gpointer user_data);

  /** Cancel the calculation, e.g. because it takes too long. */
  void cancelCalculation(GtkButton *button, gpointer user_data);

  /** Select a row. */
  void selectRow(GtkTreeView *ctree, GList *node,
                 gint column, gpointer user_data);

  /** Unselect a row. */
  void unSelectRow(GtkTreeView *ctree, GList *node,
                   gint column, gpointer user_data);

  /// \endgroup

  /// \group Handling communication from the clients

  /** Add an entity IncoCalculator to this view. This will make a main
      tree in the tree view, under which the entity can add trim
      variables.
      \param ename        (Main) name for the entity
      \returns            An integer handle for this entity. This
                          handle must be used in subsequent calls. */
  int addEntity(const std::string& ename, IncoCalculator *calculator);

  /** The reverse, remove an entity from the view. */
  void removeEntity(const std::string& name);

  /** Add a single variable to this view. */
  bool addVariable(const vector<vstring>& names,
                   int cal, int tvar,
                   const IncoVariableWork& ivar);
  /// \endgroup

  /** Update the external view. */
  void refreshView();

  /** Insert a new entity node.
      \param  name      name for the node
      \param  parent    pointer to the node's parent, to be re-cast to
                        the toolkit's objects that represent a parent
      \param  dueca_node dueca node number where the object lives.
      \param  obj       pointer to the object on the Dueca side.
      \returns          A pointer to the node on the toolkit side. */
  virtual void* insertEntityNode(const char* name, void* parent,
                                 int dueca_node, TrimLink* obj);

  /** Return a reference to an inco variable, given the calculator it
      comes from and the variable no. */
  IncoVariableWork& getIncoVariable(unsigned int calculator,
                                unsigned int variable);

  /** Return a pointer to the ctree widget. */
  inline void* getTree() { return trim_tree;}

  /** Return a pointer to the entry widget. */
  inline GtkSpinButton* getEntryWidget() { return entry_widget; }
};
DUECA_NS_END

#endif
