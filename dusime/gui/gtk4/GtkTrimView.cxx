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

GtkTrimView *GtkTrimView::singleton = NULL;

static vstring getMyString(double d)
{
#ifdef HAVE_SSTREAM
  stringstream s;
  s << d << std::ends;
  return s.str();
#else
  char cbuf[32];
  ostrstream s(cbuf, 32);
  s << d << std::ends;
  return vstring(s.str());
#endif
}

static GladeCallbackTable cb_links[] = {
  { "trim_mode", "changed", gtk_callback(&GtkTrimView::setMode) },
  { "trim_calculate", "clicked", gtk_callback(&GtkTrimView::calculate) },
  { "trim_cancel", "clicked", gtk_callback(&GtkTrimView::cancelCalculation) },
  { "trim_closewindow", "clicked", gtk_callback(&GtkTrimView::toggleWindow) },
  { "trim_tree", "tree_select_row", gtk_callback(&GtkTrimView::selectRow) },
  { "trim_tree", "tree_unselect_row", gtk_callback(&GtkTrimView::unSelectRow) },
  { NULL, NULL, NULL, NULL },
};

bool GtkTrimView::isRootClass() { return false; }

GtkTrimView::GtkTrimView() :
  trim_tree(NULL),
  trim_view(NULL),
  trim_status(NULL),
  entry_widget(NULL),
  window_open(false),
  calculation_active(false),
  root(new Summary<TrimId, TrimLink, TrimView>(
    &TrimId::create(vector<vstring>(), -1, -1), new TrimLink(0.0, 0.0, 0.0))),
  mode(FlightPath)
{
  // check the presence of a DuecaView object, for getting initial
  // access to the interface
  if (DuecaView::single() == NULL) {
    /* DUSIME UI.

       The trim view module for DUECA requires access to the generic
       DUECA view. Modify your configuration. */
    W_TRM("GtkTrimView needs DuecaView!");
    return;
  }

  // make the view, realize it so further work can be done
  window.readGladeFile(DuecaPath::prepend("trimcalc_window.glade3").c_str(),
                       "trimcalc_window", reinterpret_cast<gpointer>(this),
                       cb_links);
  trim_view = GTK_WIDGET(window["trimcalc_window"]);
  gtk_widget_realize(trim_view);

  // get the tree, the status feedback, the entry widget
  trim_tree = GTK_TREE_STORE(window.getObject("trim_tree"));
  trim_status = GTK_WIDGET(window["trim_status"]);
  entry_widget = GTK_SPIN_BUTTON(window["trim_entry_field"]);

  // get the trim mode selector, and feed it with possible modes
  //  GtkWidget* trim_mode = window["trim_mode"];

#if 0
  GtkMenu* mode_menu = GTK_COMBO_BOX
    (gtk_option_menu_get_menu(GTK_OPTION_MENU(trim_mode)));
  for (IncoMode ii = NoIncoModes; ii < Ground; ii = IncoMode(int(ii)+1)) {
    GtkWidget* menuitem =
      gtk_menu_item_new_with_label((getMyString(ii)).c_str());
    gtk_widget_realize(menuitem);
    gtk_menu_append(mode_menu, menuitem);
    gtk_widget_show(menuitem);
    g_object_set_data(G_OBJECT(menuitem), "user_data",
                      reinterpret_cast<gpointer>(ii));
  }
#endif
  // request the DuecaView object to make an entry for my window,
  // opening it on activation
  menuitem = GtkDuecaView::single()->requestViewEntry("trim", "Trim Window",
                                                      GTK_WIDGET(trim_view));

  // some assertions about the stuff
  //  assert(GTK_CTREE(trim_tree) != NULL);
  assert(GTK_LABEL(trim_status) != NULL);
}

GtkTrimView::~GtkTrimView()
{
  // should delete the view?
}

void GtkTrimView::toggleWindow(GtkButton *button, gpointer user_data)
{
  g_signal_emit_by_name(G_OBJECT(menuitem), "activate", NULL);

  if (!window_open) {
    refreshView();
  }
  window_open = !window_open;
}

void GtkTrimView::setMode(GtkButton *button, gpointer user_data)
{
#if 0
  GtkWidget* active = gtk_menu_get_active(GTK_MENU(button));
  mode = TrimMode
    (reinterpret_cast<long>(g_object_get_data(G_OBJECT(active), "user_data")));
  // \todo update the view
#endif
}

void GtkTrimView::calculate(GtkButton *button, gpointer user_data)
{
  calculation_active = true;
  for (vector<IncoCalculator *>::iterator ii = calculators.begin();
       ii != calculators.end(); ii++) {
    (*ii)->initiate(mode);
  }
}

void GtkTrimView::cancelCalculation(GtkButton *button, gpointer user_data)
{
  calculation_active = false;
  // to complete
}

static void deep_refresh(GtkTreeStore *tree, GtkTreeIter *iter)
{
#if 0
  GtkTreeIter child;
  gboolean has_child =
    gtk_tree_model_iter_children(GTK_TREE_MODEL(tree), &child, iter);
  while (has_child) {
    deep_refresh(tree, &child);
    has_child = gtk_tree_model_iter_next(GTK_TREE_MODEL(tree), &child);
  }

  GValue gobj = { 0 };
  gtk_tree_model_get_value(GTK_TREE_MODEL(tree), iter, 4, &gobj);

  // data should point to the summary
  Summary<TrimId, TrimLink, TrimView> *sum =
    reinterpret_cast<Summary<TrimId, TrimLink, TrimView> *>(
      g_value_peek_pointer(&gobj));

  // first column contains the role of the variable, for this mode
  gtk_tree_store_set(
    tree, iter, 1,
    sum->getLink().getRoleString(GtkTrimView::single().getMode()));

  // second column contains the actual value
  gtk_tree_store_set(
    tree, iter, 2,
    getMyString(sum->getLink().getIncoVariable().getValue()).c_str());

  // third column contains the user input. Depending on the mode, this
  // input may be given (for targets and constraints) or it may not
  // (for controls). Because the current gtkctree has no widgets, an
  // "external" widget is used to modify the single selectable
  // control. So this just gives feedback.
  if (sum->getLink().getIncoVariable().isUserControllable(
        GtkTrimView::single().getMode())) {
    gtk_tree_store_set(
      tree, iter, 3,
      getMyString(sum->getLink().getIncoVariable().getTarget()).c_str());
  }
  else {
    gtk_tree_store_set(tree, iter, 3, "");
  }
#endif
}
#if 1
void GtkTrimView::selectRow(GtkTreeView *ctree, GList *node, gint column,
                            gpointer user_data)
{
#if 0
  GtkTreeModel *model = gtk_tree_view_get_model(ctree);



  // data should point to the summary
  Summary<TrimId,TrimLink,TrimView> *sum =
    reinterpret_cast<Summary<TrimId,TrimLink,TrimView>*>
    (gtk_treeview_node_get_row_data(ctree, GTK_CTREE_NODE(node)));

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
#endif
}

void GtkTrimView::unSelectRow(GtkTreeView *ctree, GList *node, gint column,
                              gpointer user_data)
{
  // reads the data in the entry window and transfers it to the
  // ctree/trimid
#if 0
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
    //update_all(ctree, GTK_CTREE_NODE(node), NULL);
  }
#endif
}
#endif

int GtkTrimView::addEntity(const std::string &ename, IncoCalculator *calculator)
{
  vector<vstring> names;
  names.push_back(ename);
  if (root->insertLinkAndStatus(TrimId::create(names, calculators.size(), -1),
                                TrimLink(0.0, 0.0, 0.0))) {
    calculators.push_back(calculator);
    return calculators.size() - 1;
  }
  return -1;
}

void GtkTrimView::removeEntity(const std::string &name)
{
  /**
     \todo implement
  */
}

bool GtkTrimView::addVariable(const vector<vstring> &names, int cal, int tvar,
                              const IncoVariableWork &ivar)
{
  bool result = root->insertLinkAndStatus(
    TrimId::create(names, cal, tvar),
    TrimLink(ivar.getValue(), ivar.getMin(), ivar.getMax()));
  return result;
}

IncoVariableWork &GtkTrimView::getIncoVariable(unsigned int calculator,
                                               unsigned int variable)
{
  assert(calculator < calculators.size());
  return calculators[calculator]->getIncoVariable(variable);
}

void GtkTrimView::refreshView()
{
  GtkTreeIter iter;
  gboolean f = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(trim_tree), &iter);
  while (f) {
    deep_refresh(trim_tree, &iter);
    f = gtk_tree_model_iter_next(GTK_TREE_MODEL(trim_tree), &iter);
  }
  //  DEB1("Refreshing the view");
  // gtk_ctree_post_recursive(GTK_CTREE(trim_tree), NULL,
  //                       update_all, 0);
}

void *GtkTrimView::insertEntityNode(const char *name, void *vparent,
                                    int dueca_node, TrimLink *obj)
{
  // GtkCTreeNode* parent = GTK_CTREE_NODE(vparent);
  GtkTreeIter iter, iparent;

  if (!vparent) {
    gtk_tree_store_append(GTK_TREE_STORE(trim_tree), &iter, NULL);
  }
  else {
    GtkTreeRowReference *ref = reinterpret_cast<GtkTreeRowReference *>(vparent);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(trim_tree), &iparent,
                            gtk_tree_row_reference_get_path(ref));
    gtk_tree_store_append(trim_tree, &iter, &iparent);
  }
  gtk_tree_store_set(trim_tree, &iter, 0, name,       // node name
                     1, "",         // module status
                     2, getMyString(obj->getValue()).c_str(),    // sim status
                     3, "", // dueca node no
                     4, obj,        // pointer to status object
                     -1);

  return gtk_tree_row_reference_new(
    GTK_TREE_MODEL(trim_tree),
    gtk_tree_model_get_path(GTK_TREE_MODEL(trim_tree), &iter));
}

DUECA_NS_END
