/* ------------------------------------------------------------------   */
/*      item            : SnapshotInventoryGtk4.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Mon May  2 17:16:41 2022
        category        : body file
        description     :
        changes         : Mon May  2 17:16:41 2022 first version
        template changes: 030401 RvP Added template creation comment
                          060512 RvP Modified token checking code
                          160511 RvP Some comments updated
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "SnapshotInventory.hxx"
#include "gio/gio.h"
#include "gtk/gtk.h"
#include <boost/smart_ptr/shared_ptr.hpp>
#include <memory>
#define SnapshotInventoryGtk4_cxx

// include the definition of the module class
#include "SnapshotInventoryGtk4.hxx"
#include <dueca/DuecaPath.hxx>
#define DEBPRINTLEVEL -1
#include <debprint.h>
#include <dueca/gui/gtk4/GtkDuecaView.hxx>
#include <boost/date_time/posix_time/posix_time.hpp>

// include the debug writing header, by default, write warning and
// error messages
#define W_MOD
#define E_MOD
#include <debug.h>

// include additional files needed for your calculation here

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca/dueca.h>

DUECA_NS_START;

// class/module name
const char *const SnapshotInventoryGtk4::classname = "initials-inventory";

// Parameters to be inserted
const ParameterTable *SnapshotInventoryGtk4::getParameterTable()
{
  static const ParameterTable parameter_table[] = {

    { "glade-file",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::gladefile),
      "Interface description (glade, gtkbuilder) for the channel view window" },

    { "position-size",
      new MemberCall<_ThisModule_, std::vector<int>>(
        &_ThisModule_::setPositionAndSize),
      "Specify the position, and optionally also the size of the interface\n"
      "window." },

    { "reference-file",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::reference_file),
      "File with existing initial states (snapshots). Will be read and\n"
      "used to populate the initial set" },

    { "store-file",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::store_file),
      "When additional snapshots are taken in this simulation, these will\n"
      "be written in this file, together with the existing initial state\n"
      "sets. Uses a template, check check boost time_facet for format\n"
      "strings. Default \"\", suggestion\n"
      "initial-[entity name]-%Y%m%d_%H%M%S.toml" },

    { NULL, NULL, "Manage loading of initial states (snapshots). " }
  };

  return parameter_table;
}

// constructor
SnapshotInventoryGtk4::SnapshotInventoryGtk4(Entity *e, const char *part,
                                             const PrioritySpec &ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  Module(e, classname, part),

  // initialize the data you need in your simulation or process
  gladefile(DuecaPath::prepend("initials_inventory-gtk4.ui")),
  window(),
  snaps_store(NULL),
  menuaction(NULL),
  reference_file(),
  store_file()
{
  // connect the triggers for simulation
  // do_calc.setTrigger(/* fill in your triggering channels,
  //                      or enter the clock here */);
}

static std::string formatTime(const boost::posix_time::ptime &now,
                              const std::string &lft)
{
  using namespace boost::posix_time;
  std::locale loc(std::cout.getloc(), new time_facet(lft.c_str()));

  std::basic_stringstream<char> wss;
  wss.imbue(loc);
  wss << now;
  return wss.str();
}

// define the datatype to be held in the store
struct _DSnapShotSet
{
  GObject parent;
  std::string name;
  std::string date_time;
  struct SnapShotData
  {
    std::string origin;
    std::string coding;
    std::string sample;
    SnapShotData(const std::string &o, const std::string &c,
                 const std::string &s) :
      origin(o),
      coding(c),
      sample(s)
    {}
  };
  std::list<std::shared_ptr<SnapShotData>> snaps;
  std::shared_ptr<SnapShotData> snap;
};

G_DECLARE_FINAL_TYPE(DSnapShotSet, d_snap_shot_set, D, SNAP_SHOT_SET, GObject);
G_DEFINE_TYPE(DSnapShotSet, d_snap_shot_set, G_TYPE_OBJECT);

static void d_snap_shot_set_class_init(DSnapShotSetClass *klass) {}
static void d_snap_shot_set_init(DSnapShotSet *self) {}

static DSnapShotSet *
d_snap_shot_set_new(const std::string name,
                    const SnapshotInventory::SnapshotData *set,
                    std::shared_ptr<DSnapShotSet::SnapShotData> snap)
{
  auto res = D_SNAP_SHOT_SET(g_object_new(d_snap_shot_set_get_type(), NULL));
  res->name = name;
  if (snap) {
    res->snap = snap;
  }
  else {
    res->date_time = set->getTimeLocal();
  }
  return res;
}

static GListModel *add_data_element(gpointer _item, gpointer user_data)
{
  auto item = D_SNAP_SHOT_SET(_item);
  if (item->snaps.size()) {
    auto lm = g_list_store_new(d_snap_shot_set_get_type());
    for (auto &c : item->snaps) {
      auto child = d_snap_shot_set_new("", NULL, c);
      g_list_store_append(lm, gpointer(child));
      g_object_unref(child);
    }
    return G_LIST_MODEL(lm);
  }
  return G_LIST_MODEL(NULL);
}

bool SnapshotInventoryGtk4::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  if (getPart().size() == 0) {
    /* DUSIME replay&initial

       Specify the entity to be managed in the part name. */
    E_XTR("Supply the managed entity to the snapshot inventory");
    return false;
  }

  // find the underlying inventory
  inventory = SnapshotInventory::findSnapshotInventory(getPart());

  // if applicable, open the files
  inventory->setFiles(
    reference_file,
    formatTime(boost::posix_time::second_clock::local_time(), store_file));

  // table with callbacks to be connected to widget actions
  static GladeCallbackTable cb_table[] = {
    { "initials_close", "clicked", gtk_callback(&_ThisModule_::cbClose) },
    { "initials_newentryname", "changed",
      gtk_callback(&_ThisModule_::cbSetName) },
    { "initials_send", "clicked", gtk_callback(&_ThisModule_::cbSendInitial) },
    // { "initials_selection", "changed",
    //   gtk_callback(&_ThisModule_::cbSelection) },
    { "initials_view", "close-request", gtk_callback(&_ThisModule_::cbDelete) },
    { "initials_name_fact", "setup",
      gtk_callback(&_ThisModule_::cbSetupExpander) },
    { "initials_datetime_fact", "setup",
      gtk_callback(&_ThisModule_::cbSetupLabel) },
    { "initials_origin_fact", "setup",
      gtk_callback(&_ThisModule_::cbSetupLabel) },
    { "initials_coding_fact", "setup",
      gtk_callback(&_ThisModule_::cbSetupLabel) },
    { "initials_sample_fact", "setup",
      gtk_callback(&_ThisModule_::cbSetupLabel) },
    { "initials_name_fact", "bind", gtk_callback(&_ThisModule_::cbBindName) },
    { "initials_datetime_fact", "bind",
      gtk_callback(&_ThisModule_::cbBindDateTime) },
    { "initials_origin_fact", "bind",
      gtk_callback(&_ThisModule_::cbBindOrigin) },
    { "initials_coding_fact", "bind",
      gtk_callback(&_ThisModule_::cbBindCoding) },
    { "initials_sample_fact", "bind",
      gtk_callback(&_ThisModule_::cbBindSample) },

    { NULL }
  };

  // create the window
  bool res = window.readGladeFile(gladefile.c_str(), "initials_view",
                                  reinterpret_cast<gpointer>(this), cb_table);
  if (!res) {
    /* DUECA UI.

       Cannot find the glade file defining the initial state view
       GUI. Check DUECA installation and paths.
    */
    E_CNF("failed to open initials overview " << gladefile);
    return res;
  }

  // find the store for the snapshot data, and fill
  GtkColumnView *treeview = GTK_COLUMN_VIEW(window["initials_initiallist"]);
  snaps_store = g_list_store_new(d_snap_shot_set_get_type());
  auto model = gtk_tree_list_model_new(G_LIST_MODEL(snaps_store), FALSE, FALSE,
                                       add_data_element, NULL, NULL);
  auto selection = gtk_single_selection_new(G_LIST_MODEL(model));
  auto cb = gtk_callback(&_ThisModule_::cbSelection, this);
  g_signal_connect(selection, "selection-changed", cb->callback(), cb);
  gtk_column_view_set_model(treeview, GTK_SELECTION_MODEL(selection));

  // it is a tree structure, base element for each snapshot set, with
  // leafs for the different snapshots

  // load the tree with the currently present data
  for (const auto &snapset : inventory->getSnapshotData()) {

    auto nset = d_snap_shot_set_new(snapset.first, &snapset.second, NULL);
    g_list_store_append(snaps_store, nset);
    for (const auto &snap : snapset.second.snaps) {
      nset->snaps.emplace_back(new DSnapShotSet::SnapShotData(
        snap.originator.name, getString(snap.coding), snap.getSample(30)));
    }
  }

  // create a callback for getting any new incoming snapshot sets
  inventory->informOnNewSet(
    [this](const std::string &name,
           const SnapshotInventory::SnapshotData &snapset) {
      auto nset = d_snap_shot_set_new(name, &snapset, NULL);
      g_list_store_append(snaps_store, nset);
    });

  inventory->informOnNewSnap([this](const Snapshot &snap) {
    auto n = g_list_model_get_n_items(G_LIST_MODEL(snaps_store));
    auto lset =
      D_SNAP_SHOT_SET(g_list_model_get_item(G_LIST_MODEL(snaps_store), n - 1));
    lset->snaps.emplace_back(new DSnapShotSet::SnapShotData(
      snap.originator.name, getString(snap.coding), snap.getSample(30)));
  });

  // set a title
  gtk_window_set_title(
    GTK_WINDOW(window["initials_view"]),
    (std::string("Initials control - ") + getPart()).c_str());

  // insert in DUECA's menu
  menuaction = GtkDuecaView::single()->requestViewEntry(
    (std::string("initial_") + getPart()).c_str(),
    (std::string("Initial state - ") + getPart()).c_str(),
    G_OBJECT(window["initials_view"]));

  return res;
}

// destructor
SnapshotInventoryGtk4::~SnapshotInventoryGtk4()
{
  //
}

// tell DUECA you are prepared
bool SnapshotInventoryGtk4::isPrepared()
{
  bool res = true;

  // return result of checks
  return res;
}

// start the module
void SnapshotInventoryGtk4::startModule(const TimeSpec &time)
{
  // do_calc.switchOn(time);
}

// stop the module
void SnapshotInventoryGtk4::stopModule(const TimeSpec &time)
{
  // do_calc.switchOff(time);
}

// callbacks to link to the gui
void SnapshotInventoryGtk4::cbClose(GtkWidget *button, gpointer gp)
{
  GtkDuecaView::toggleView(menuaction);
  //g_signal_emit_by_name(G_OBJECT(menuaction), "activate", NULL);
}

void SnapshotInventoryGtk4::cbSetName(GtkWidget *text, gpointer gp)
{
  inventory->setSnapName(gtk_editable_get_chars(GTK_EDITABLE(text), 0, -1));
}

void SnapshotInventoryGtk4::cbSendInitial(GtkWidget *btn, gpointer gp)
{
  if (inventory->sendSelected()) {
    gtk_label_set_text(GTK_LABEL(window["initials_status"]), "sent");
    gtk_widget_set_sensitive(GTK_WIDGET(window["initials_send"]), FALSE);
    // gtk_tree_selection_unselect_all(GTK_TREE_SELECTION
    //  (window["initials_listselection"]));
  }
  else {
    gtk_label_set_text(GTK_LABEL(window["initials_status"]), "send failed");
  }
}

void SnapshotInventoryGtk4::cbSelection(GtkSelectionModel *sel, guint position,
                                        guint n_items, gpointer gp)
{
  if (gtk_selection_model_is_selected(sel, position)) {
    auto it = D_SNAP_SHOT_SET(
      g_list_model_get_item(G_LIST_MODEL(snaps_store), position));
    gtk_label_set_text(GTK_LABEL(window["initials_selected"]),
                       it->name.c_str());
    gtk_widget_set_sensitive(GTK_WIDGET(window["initials_send"]), TRUE);
    gtk_label_set_text(GTK_LABEL(window["initials_status"]), "selected");
  }
  else {
    gtk_label_set_text(GTK_LABEL(window["initials_selected"]),
                       "<< none selected >>");
    gtk_widget_set_sensitive(GTK_WIDGET(window["initials_send"]), FALSE);
    gtk_label_set_text(GTK_LABEL(window["initials_status"]), "");
  }
}

gboolean SnapshotInventoryGtk4::cbDelete(GtkWidget *window, gpointer user_data)
{
  // fixes the menu check, and closes the window
  //g_signal_emit_by_name(G_OBJECT(menuaction), "activate", NULL);
  GtkDuecaView::toggleView(menuaction);
  // indicate that the event is handled
  return TRUE;
}

bool SnapshotInventoryGtk4::setPositionAndSize(const std::vector<int> &p)
{
  if (p.size() == 2 || p.size() == 4) {
    window.setWindow(p);
  }
  else {
    /* DUECA UI.

       Window setting needs 2 (for size) or 4 (also location)
       arguments. */
    E_CNF(getId() << '/' << classname << " need 2 or 4 arguments");
    return false;
  }
  return true;
}

void SnapshotInventoryGtk4::cbSetupLabel(GtkSignalListItemFactory *fact,
                                         GtkListItem *item, gpointer user_data)
{
  auto label = gtk_label_new("");
  gtk_list_item_set_child(item, label);
}

void SnapshotInventoryGtk4::cbSetupExpander(GtkSignalListItemFactory *fact,
                                            GtkListItem *item,
                                            gpointer user_data)
{
  auto label = gtk_label_new("");
  auto expander = gtk_tree_expander_new();
  gtk_tree_expander_set_child(GTK_TREE_EXPANDER(expander), label);
  gtk_list_item_set_child(item, expander);
}

void SnapshotInventoryGtk4::cbBindName(GtkSignalListItemFactory *fact,
                                       GtkListItem *item, gpointer user_data)
{
  auto expander = gtk_list_item_get_child(item);
  auto row = gtk_list_item_get_item(item);
  auto snap =
    D_SNAP_SHOT_SET(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  auto label = gtk_tree_expander_get_child(GTK_TREE_EXPANDER(expander));
  if (snap->snaps.size()) {
    gtk_tree_expander_set_list_row(GTK_TREE_EXPANDER(expander),
                                   GTK_TREE_LIST_ROW(row));
  }
  gtk_label_set_label(GTK_LABEL(label), snap->name.c_str());
}

void SnapshotInventoryGtk4::cbBindDateTime(GtkSignalListItemFactory *fact,
                                           GtkListItem *item,
                                           gpointer user_data)
{
  auto label = gtk_list_item_get_child(item);
  auto row = gtk_list_item_get_item(item);
  auto snap =
    D_SNAP_SHOT_SET(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  gtk_label_set_label(GTK_LABEL(label), snap->date_time.c_str());
}

void SnapshotInventoryGtk4::cbBindOrigin(GtkSignalListItemFactory *fact,
                                         GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto snap =
    D_SNAP_SHOT_SET(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (snap->snap) {
    auto label = gtk_list_item_get_child(item);
    gtk_label_set_label(GTK_LABEL(label), snap->snap->origin.c_str());
  }
}

void SnapshotInventoryGtk4::cbBindCoding(GtkSignalListItemFactory *fact,
                                         GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto snap =
    D_SNAP_SHOT_SET(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (snap->snap) {
    auto label = gtk_list_item_get_child(item);
    gtk_label_set_label(GTK_LABEL(label), snap->snap->coding.c_str());
  }
}

void SnapshotInventoryGtk4::cbBindSample(GtkSignalListItemFactory *fact,
                                         GtkListItem *item, gpointer user_data)
{
  auto row = gtk_list_item_get_item(item);
  auto snap =
    D_SNAP_SHOT_SET(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  if (snap->snap) {
    auto label = gtk_list_item_get_child(item);
    gtk_label_set_label(GTK_LABEL(label), snap->snap->sample.c_str());
  }
}

DUECA_NS_END;
