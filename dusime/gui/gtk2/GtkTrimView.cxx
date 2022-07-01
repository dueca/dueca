/* ------------------------------------------------------------------   */
/*      item            : GtkTrimView.cxx
        made by         : Rene' van Paassen
        date            : 010826
        category        : body file
        description     :
        changes         : 010826 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define GtkTrimView_cxx

#include <dueca-conf.h>

// the whole class is conditional on the availability of gtk
#include "GtkTrimView.hxx"
#include <IncoCalculator.hxx>
#include <DuecaPath.hxx>
#include <GtkDuecaView.hxx>
#include <IncoVariable.hxx>

#define D_TRM
#define I_TRM
#define W_TRM
#define E_TRM
#include <debug.h>
#include <cmath>
#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif
#include <dassert.h>
#include <debprint.h>
DUECA_NS_START

GtkTrimView* GtkTrimView::singleton = NULL;

static vstring getMyString(double d)
{
#ifdef HAVE_SSTREAM
  stringstream s;
  s << d << std::ends;
  return s.str();
#else
  char cbuf[32]; ostrstream s(cbuf, 32);
  s << d << std::ends;
  return vstring(s.str());
#endif
}

static GladeCallbackTable cb_links[] = {
  { "trim_mode", "clicked",  gtk_callback(&GtkTrimView::setMode) },
  { "trim_calculate", "clicked", gtk_callback(&GtkTrimView::calculate) },
  { "trim_cancel", "clicked", gtk_callback(&GtkTrimView::cancelCalculation) },
  { "trim_closewindow", "clicked", gtk_callback(&GtkTrimView::toggleWindow) },
  { "trim_tree", "tree_select_row", gtk_callback(&GtkTrimView::selectRow) },
  { "trim_tree", "tree_unselect_row", gtk_callback(&GtkTrimView::unSelectRow) },
  { NULL, NULL, NULL, NULL},
};

bool GtkTrimView::isRootClass()
{
  return false;
}

GtkTrimView::GtkTrimView() :
  trim_tree(NULL),
  trim_view(NULL),
  trim_status(NULL),
  entry_widget(NULL),
  window_open(false),
  root(new Summary<TrimId, TrimLink, TrimView>
       (&TrimId::create(vector<vstring>(), -1, -1),
        new TrimLink(0.0, 0.0, 0.0))),
  mode(FlightPath)
{
  // check the presence of a DuecaView object, for getting initial
  // access to the interface
  if (DuecaView::single() == NULL) {
    /* DUSIME UI.

       The trim view module for DUECA requires access to the generic
       DUECA view. Modify your configuration. */
    W_CNF("GtkTrimView needs DuecaView!");
    return;
  }

  // make the view, realize it so further work can be done
#if GTK_MAJOR_VERSION > 1
  window.readGladeFile(DuecaPath::prepend("trimcalc_window.glade2").c_str(),
                       "trimcalc_window",
                       reinterpret_cast<gpointer>(this), cb_links);
#else
  window.readGladeFile(DuecaPath::prepend("trimcalc_window.glade").c_str(),
                       "trimcalc_window",
                       reinterpret_cast<gpointer>(this), cb_links);
#endif

  trim_view = window["trimcalc_window"];
  gtk_widget_realize(trim_view);

  // get the tree, the status feedback, the entry widget
  trim_tree = window["trim_tree"];
  trim_status = window["trim_status"];
  entry_widget = GTK_SPIN_BUTTON(window["trim_entry_field"]);

  // get the trim mode selector, and feed it with possible modes
  GtkWidget* trim_mode = window["trim_mode"];
  GtkMenu* mode_menu = GTK_MENU
    (gtk_option_menu_get_menu(GTK_OPTION_MENU(trim_mode)));
  for (IncoMode ii = NoIncoModes; ii < Ground; ii = IncoMode(int(ii)+1)) {
    GtkWidget* menuitem =
      gtk_menu_item_new_with_label((getMyString(ii)).c_str());
    gtk_widget_realize(menuitem);
    gtk_menu_append(mode_menu, menuitem);
    gtk_widget_show(menuitem);
    gtk_object_set_user_data(GTK_OBJECT(menuitem),
                             reinterpret_cast<gpointer>(ii));
  }

  // request the DuecaView object to make an entry for my window,
  // opening it on activation
  menuitem = GTK_WIDGET
    (GtkDuecaView::single()->requestViewEntry
     ("Trim Window", GTK_OBJECT(trim_view)));

  // some assertions about the stuff
  assert(GTK_CTREE(trim_tree) != NULL);
  assert(GTK_LABEL(trim_status) != NULL);
}

GtkTrimView::~GtkTrimView()
{
  // should delete the view?
}

void GtkTrimView::toggleWindow(GtkButton *button, gpointer user_data)
{
  gtk_signal_emit_by_name(GTK_OBJECT(menuitem), "activate", NULL);

  if (!window_open) {
    refreshView();
  }
  window_open = !window_open;
}

void GtkTrimView::setMode(GtkButton *button, gpointer user_data)
{
  GtkWidget* active = gtk_menu_get_active(GTK_MENU(button));
  mode = TrimMode
    (reinterpret_cast<long>(gtk_object_get_user_data(GTK_OBJECT(active))));
  // \todo update the view
}

void GtkTrimView::calculate(GtkButton *button, gpointer user_data)
{
  for (vector<IncoCalculator*>::iterator ii = calculators.begin();
       ii != calculators.end(); ii++) {
    (*ii)->initiate(mode);
  }
}

void GtkTrimView::cancelCalculation(GtkButton *button, gpointer user_data)
{
  // to complete
}

static void update_all(GtkCTree* tree, GtkCTreeNode* node,
                       gpointer data)
{
  // data should point to the summary
  Summary<TrimId,TrimLink,TrimView> *sum =
    reinterpret_cast<Summary<TrimId,TrimLink,TrimView>*>
    (gtk_ctree_node_get_row_data(tree, node));

  // do not set anything for the branch nodes, only do for leaves
  gboolean is_leaf;
  gtk_ctree_get_node_info(tree, node, NULL, NULL, NULL, NULL, NULL, NULL,
                          &is_leaf, NULL);
  if (!is_leaf) return;

  // first column contains the role of the variable, for this mode
  gtk_ctree_node_set_text(tree, node, 1, sum->getLink().
                          getRoleString(GtkTrimView::single().getMode()));

  // second column contains the actual value
  gtk_ctree_node_set_text(tree, node, 2,
                          getMyString(sum->getLink().
                                      getIncoVariable().getValue()).c_str());

  // third column contains the user input. Depending on the mode, this
  // input may be given (for targets and constraints) or it may not
  // (for controls). Because the current gtkctree has no widgets, an
  // "external" widget is used to modify the single selectable
  // control. So this just gives feedback.
  if (sum->getLink().getIncoVariable().
      isUserControllable(GtkTrimView::single().getMode())) {
    gtk_ctree_node_set_text(tree, node, 3,
                            getMyString(sum->getLink().getIncoVariable().
                                      getTarget()).c_str());
  }
  else {
    gtk_ctree_node_set_text(tree, node, 3, "");
  }
}

void GtkTrimView::selectRow(GtkCTree *ctree, GList *node,
                            gint column, gpointer user_data)
{
  // data should point to the summary
  Summary<TrimId,TrimLink,TrimView> *sum =
    reinterpret_cast<Summary<TrimId,TrimLink,TrimView>*>
    (gtk_ctree_node_get_row_data(ctree, GTK_CTREE_NODE(node)));

  // is this a leaf node?
  gboolean is_leaf;
  gtk_ctree_get_node_info(ctree, GTK_CTREE_NODE(node), NULL,
                          NULL, NULL, NULL, NULL, NULL, &is_leaf, NULL);

  DEB1("select, leaf=" << is_leaf << " (c,v)=" <<
        sum->getLink().getCalculator() << ',' <<
        sum->getLink().getVariable() << " ucontrol=" <<
        sum->getLink().getIncoVariable().
        isUserControllable(GtkTrimView::single().getMode()) <<
        " max=" << sum->getLink().getIncoVariable().getMax() <<
        " min=" << sum->getLink().getIncoVariable().getMin());

  if (is_leaf && sum->getLink().getCalculator() >= 0 &&
      sum->getLink().getVariable() >= 0 &&
      sum->getLink().getIncoVariable().
      isUserControllable(GtkTrimView::single().getMode()) &&
      fabs(sum->getLink().getIncoVariable().getMax() -
           sum->getLink().getIncoVariable().getMin()) > 1.0e-5) {

    // the widget can work
    gtk_widget_set_sensitive
      (GTK_WIDGET(GtkTrimView::single().getEntryWidget()), TRUE);

    // get the value from the row and feed it to the spin button
    gtk_spin_button_set_value
      (GtkTrimView::single().getEntryWidget(),
       sum->getLink().getIncoVariable().getTarget());

    // and set the spin button limits, update interval
    GtkAdjustment *adjust = gtk_spin_button_get_adjustment
      (GtkTrimView::single().getEntryWidget());

    gtk_adjustment_set_lower(adjust, sum->getLink().getIncoVariable().getMin());
    gtk_adjustment_set_upper(adjust, sum->getLink().getIncoVariable().getMax());
    if (sum->getLink().getIncoVariable().isInteger()) {
      DEB1("integer spin");
      gtk_adjustment_set_step_increment(adjust, 1);
      gtk_adjustment_set_page_increment(adjust, 10);
      gtk_spin_button_set_digits
        (GtkTrimView::single().getEntryWidget(), 0);
      gtk_adjustment_changed(adjust);
    }
    else {
      float baseincr = fabs(sum->getLink().getIncoVariable().getMax() -
                            sum->getLink().getIncoVariable().getMin());
      int digits = -int(rint(log10(baseincr))) + 2;  // 1 becomes 1.00
      baseincr = pow(10.0, double(-digits));         // basic incr 0.01
      gtk_adjustment_set_step_increment(adjust, baseincr);
      gtk_adjustment_set_page_increment(adjust, baseincr*10);
      if (digits < 0) digits = 0;
      DEB1("float spin, dig=" << digits << " step="
            << gtk_adjustment_get_step_increment(adjust) << " page="
            << gtk_adjustment_get_page_increment(adjust));
      gtk_spin_button_set_digits
        (GtkTrimView::single().getEntryWidget(), digits);
      gtk_adjustment_changed(adjust);
    }
  }
  else {
    gtk_widget_set_sensitive
      (GTK_WIDGET(GtkTrimView::single().getEntryWidget()), FALSE);
  }
}

void GtkTrimView::unSelectRow(GtkCTree *ctree, GList *node,
                           gint column, gpointer user_data)
{
  // reads the data in the entry window and transfers it to the
  // ctree/trimid

  // data should point to the summary
  Summary<TrimId,TrimLink,TrimView> *sum =
    reinterpret_cast<Summary<TrimId,TrimLink,TrimView>*>
    (gtk_ctree_node_get_row_data(ctree, GTK_CTREE_NODE(node)));

  // if this row is not a leaf, exit
  gboolean is_leaf;
  gtk_ctree_get_node_info(ctree, GTK_CTREE_NODE(node), NULL,
                          NULL, NULL, NULL, NULL, NULL, &is_leaf, NULL);
  if (!is_leaf) return;

  // get value from the spin button
  float v = gtk_spin_button_get_value_as_float
    (GtkTrimView::single().getEntryWidget());

  // insert this in the link
  if (sum->getLink().getCalculator() >= 0 &&
      sum->getLink().getVariable() >= 0 &&
      sum->getLink().getIncoVariable().
      isUserControllable(GtkTrimView::single().getMode())) {
    sum->getLink().getIncoVariable().setTarget(v);
    // update the row
    update_all(ctree, GTK_CTREE_NODE(node), NULL);
  }
}


int GtkTrimView::addEntity(const std::string& ename,
                           IncoCalculator *calculator)
{
  vector<vstring> names;
  names.push_back(ename);
  if (root->insertLinkAndStatus
      (TrimId::create(names, calculators.size(), -1),
       TrimLink(0.0, 0.0, 0.0))) {
    calculators.push_back(calculator);
    return calculators.size() - 1;
  }
  return -1;
}

void GtkTrimView::removeEntity(const std::string& name)
{
  /**
     \todo implement
  */
}

bool GtkTrimView::addVariable(const vector<vstring>& names,
                              int cal, int tvar,
                              const IncoVariableWork& ivar)
{
  bool result = root->insertLinkAndStatus
    (TrimId::create(names, cal, tvar),
     TrimLink(ivar.getValue(), ivar.getMin(), ivar.getMax()));
  return result;
}

IncoVariableWork& GtkTrimView::getIncoVariable(unsigned int calculator,
                                               unsigned int variable)
{
  assert (calculator < calculators.size());
  return calculators[calculator]->getIncoVariable(variable);
}

void GtkTrimView::refreshView()
{
  DEB1("Refreshing the view");
  gtk_ctree_post_recursive(GTK_CTREE(trim_tree), NULL,
                           update_all, 0);
}

void* GtkTrimView::insertEntityNode(const char* name, void* vparent,
                                    int dueca_node, TrimLink* obj)
{
  GtkCTreeNode* parent = GTK_CTREE_NODE(vparent);

  // view text \todo Really do something useful with it
  const gchar* text[4] = {name, "", getMyString(obj->getValue()).c_str(), ""};

  // insert the new child
  GtkCTreeNode* newnode = gtk_ctree_insert_node
    (GTK_CTREE(trim_tree), parent, NULL, const_cast<gchar**>(text), 0,
     NULL, NULL, NULL, NULL, TRUE, TRUE);

  // keep a pointer to the DUECA representation in the row data
  gtk_ctree_node_set_row_data(GTK_CTREE(trim_tree), newnode,
                              reinterpret_cast<gpointer>(obj));

  return reinterpret_cast<void*>(newnode);
}

DUECA_NS_END
