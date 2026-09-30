/* ------------------------------------------------------------------   */
/*      item            : SnapshotInventoryGtk3.cxx
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

#define SnapshotInventoryGtk3_cxx

// include the definition of the module class
#include "SnapshotInventoryGtk3.hxx"
#include <dueca/DuecaPath.hxx>
#define DEBPRINTLEVEL -1
#include <debprint.h>
#include <dueca/gui/gtk3/GtkDuecaView.hxx>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iterator>
#include <cassert>

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

namespace dueca {

// class/module name
const char *const SnapshotInventoryGtk3::classname = "initials-inventory";

// Parameters to be inserted
const ParameterTable *SnapshotInventoryGtk3::getParameterTable()
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

    { "path",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::store_path),
      "Path for storing and retrieving initial state files." },

    { NULL, NULL, "Manage loading of initial states (snapshots). " }
  };

  return parameter_table;
}

// constructor
SnapshotInventoryGtk3::SnapshotInventoryGtk3(Entity *e, const char *part,
                                             const PrioritySpec &ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  Module(e, classname, part),

  // initialize the data you need in your simulation or process
  inventory(nullptr),
  gladefile(DuecaPath::prepend("initials_inventory_gtk3.ui")),
  window(),
  editwin(NULL),
  snaps_store(NULL),
  menuitem(NULL),
  set_iterator(),
  reference_file(),
  store_file(),
  store_path(),
  editmap(),
  sel_origin(editmap.end()),
  editing_snap()
{
  //
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

// organize construction of the tree model
namespace {
  // attributes, name, and attachment to column in the model
struct attributedata
{
  const char *name;
  const gint column;
};

  // column data. Columns are created in glade, this attaches the proper
  // renderer, indicate if expanded, and gives attributes
struct columndata
{
    // cell renderer
  GtkCellRenderer *renderer;
    // expand/extra space
  gboolean expand;
    // list of attributes
  const attributedata attribs[4];
};
}; // namespace

bool SnapshotInventoryGtk3::complete()
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

  // check and if needed correct the name of store_path
  if (store_path.size() && store_path.back() != '/') {
    store_path = store_path + std::string("/");
  }

  // if applicable, open the files
  inventory->setFiles(
    reference_file,
    formatTime(boost::posix_time::second_clock::local_time(), store_file),
    store_path);

  // table with callbacks to be connected to widget actions
  static GladeCallbackTable cb_table[] = {
    { "initials_close", "clicked", gtk_callback(&_ThisModule_::cbClose) },
    { "initials_newentryname", "changed",
      gtk_callback(&_ThisModule_::cbSetName) },
    { "initials_send", "clicked", gtk_callback(&_ThisModule_::cbSendInitial) },
    { "initials_edit", "clicked", gtk_callback(&_ThisModule_::cbEditInitial) },
    { "editwin_revert", "clicked", gtk_callback(&_ThisModule_::cbEditRevert) },
    { "editwin_update", "clicked", gtk_callback(&_ThisModule_::cbEditUpdate) },
    { "editwin_module_selection", "changed",
      gtk_callback(&_ThisModule_::cbEditSelection) },
    { "initials_listselection", "changed",
      gtk_callback(&_ThisModule_::cbSelection) },
    { "initials_view", "delete_event", gtk_callback(&_ThisModule_::cbDelete) },
    { "editwin", "delete_event", gtk_callback(&_ThisModule_::cbEditDelete) },
    { "editwin_close", "clicked", gtk_callback(&_ThisModule_::cbEditClose) },
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

  // figure out the edit window
  editwin = window["editwin"];

  // find the store for the snapshot data, and fill
  GtkTreeView *treeview = GTK_TREE_VIEW(window["initials_initiallist"]);
  snaps_store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));

  // it is a tree structure, base element for each snapshot set, with
  // leafs for the different snapshots
  GtkTreeIter itset, itsnap;
  gtk_tree_model_get_iter_first(GTK_TREE_MODEL(snaps_store), &itsnap);

  // get the toggle for viewing the snapshot details
  static GtkCellRenderer *txtrenderer = gtk_cell_renderer_text_new();

  // create the connections between the tree view and the data table
  static columndata cdata[] = {
    { txtrenderer,
      FALSE,
      { { "text", S_name }, { "visible", S_isset }, { NULL, 0 } } },
    { txtrenderer,
      FALSE,
      { { "text", S_time }, { "visible", S_isset }, { NULL, 0 } } },
    { txtrenderer,
      FALSE,
      { { "text", S_origin }, { "visible", S_isinitial }, { NULL, 0 } } },
    { txtrenderer,
      FALSE,
      { { "text", S_coding }, { "visible", S_isinitial }, { NULL, 0 } } },
    { txtrenderer,
      TRUE,
      { { "text", S_example }, { "visible", S_isinitial }, { NULL, 0 } } },
    { NULL, FALSE, { { NULL, 0 } } }
  };

  // this sets the renderer(s) on the columns
  int icol = 0;
  for (const struct columndata *cd = cdata; cd->renderer != NULL; cd++) {
    GtkTreeViewColumn *col = gtk_tree_view_get_column(treeview, icol++);
    gtk_tree_view_column_pack_start(col, cd->renderer, cd->expand);
    for (const struct attributedata *at = cd->attribs; at->name != NULL; at++) {
      gtk_tree_view_column_add_attribute(col, cd->renderer, at->name,
                                         at->column);
    }
  }

  // load the tree with the currently present data
  for (const auto &snapset : inventory->getSnapshotData()) {
    gtk_tree_store_append(snaps_store, &itset, NULL);
    gtk_tree_store_set(snaps_store, &itset, S_name, snapset.first.c_str(),
                       S_time, snapset.second.getTimeLocal().c_str(), S_isset,
                       TRUE, -1);
    for (const auto &snap : snapset.second.snaps) {
      gint snap_position = 0;
      gtk_tree_store_insert(snaps_store, &itsnap, &itset, snap_position++);
      gtk_tree_store_set(snaps_store, &itsnap, S_origin,
                         snap.originator.name.c_str(), S_coding,
                         getString(snap.coding), S_example,
                         snap.getSample(30).c_str(), S_isinitial, TRUE, -1);
    }
  }

  // create a callback for getting any new incoming snapshot sets
  inventory->informOnNewSet(
    [this](const std::string &name,
           const SnapshotInventory::SnapshotData &snapset) {
      gtk_tree_store_append(this->snaps_store, &(this->set_iterator), NULL);
      gtk_tree_store_set(this->snaps_store, &(this->set_iterator), S_name,
                         name.c_str(), S_time, snapset.getTimeLocal().c_str(),
                         S_isset, TRUE, -1);
    });

  inventory->informOnNewSnap([this](const Snapshot &snap) {
      // insert a new row/iterator
    GtkTreeIter snapit;
    gtk_tree_store_append(this->snaps_store, &snapit, &(this->set_iterator));
    gtk_tree_store_set(this->snaps_store, &snapit, S_origin,
                       snap.originator.name.c_str(), S_coding,
                       getString(snap.coding), S_example,
                       snap.getSample(30).c_str(), S_isinitial, TRUE, -1);

      // deselect from list
    gtk_label_set_text(GTK_LABEL(window["initials_status"]), "snapshot taken");
    gtk_tree_selection_unselect_all(
      GTK_TREE_SELECTION(window.getObject("initials_listselection")));
  });

  // set a title
  gtk_window_set_title(
    GTK_WINDOW(window["initials_view"]),
    (std::string("Initials control - ") + getPart()).c_str());

  gtk_window_set_title(GTK_WINDOW(window["editwin"]),
                       (std::string("Initials edit - ") + getPart()).c_str());

  // insert in DUECA's menu
  menuitem = GTK_WIDGET(GtkDuecaView::single()->requestViewEntry(
    (std::string("Initial state - ") + getPart()).c_str(),
    G_OBJECT(window["initials_view"])));

  return res;
}

// destructor
SnapshotInventoryGtk3::~SnapshotInventoryGtk3()
{
  //
}

// tell DUECA you are prepared
bool SnapshotInventoryGtk3::isPrepared()
{
  bool res = true;

  // return result of checks
  return res;
}

// start the module
void SnapshotInventoryGtk3::startModule(const TimeSpec &time)
{
  //do_calc.switchOn(time);
}

// stop the module
void SnapshotInventoryGtk3::stopModule(const TimeSpec &time)
{
  //do_calc.switchOff(time);
}

// callbacks to link to the gui
void SnapshotInventoryGtk3::cbClose(GtkWidget *button, gpointer gp)
{
  g_signal_emit_by_name(G_OBJECT(menuitem), "activate", NULL);
}

void SnapshotInventoryGtk3::cbSetName(GtkWidget *text, gpointer gp)
{
  inventory->setSnapName(gtk_editable_get_chars(GTK_EDITABLE(text), 0, -1));
}

void SnapshotInventoryGtk3::cbSendInitial(GtkWidget *btn, gpointer gp)
{
  if (inventory->sendSelected()) {
    gtk_label_set_text(GTK_LABEL(window["initials_status"]), "sent");
    // gtk_widget_set_sensitive(GTK_WIDGET(window["initials_send"]), FALSE);
    // gtk_tree_selection_unselect_all(GTK_TREE_SELECTION
    //  (window["initials_listselection"]));
  }
  else {
    gtk_label_set_text(GTK_LABEL(window["initials_status"]), "send failed");
  }
}

static void cbBufferChanged(GtkTextBuffer *buf, gpointer gp)
{
  reinterpret_cast<SnapshotInventoryGtk3 *>(gp)->cbEditChange(buf);
}

void SnapshotInventoryGtk3::prepareEditingMap(bool init)
{
  if (init) {
    editing_snap = inventory->getSelected();
  }

  assert(inventory->haveIncoSet(editing_snap));

  // list of tabs/labels
  std::list<std::string> origin_list;

  // has the list been expanded or changed
  bool changes = false;

  auto &snapset = inventory->editSnapshot(editing_snap);
  for (auto &ed : editmap) {
    ed.second.todelete = true;
  }

  for (const auto &sn : snapset.snaps) {

    // add an edit page if the snapshot is of the editable type
    if (sn.coding != Snapshot::UnSpecified && sn.coding != Snapshot::Base64 &&
        sn.coding != Snapshot::Base64File &&
        sn.coding != Snapshot::BinaryFile) {

      auto edit = editmap.find(sn.originator.name);
      origin_list.push_back(sn.originator.name);

      if (edit == editmap.end()) {

        changes = true;
        auto ne = editmap.emplace(
          std::piecewise_construct, std::forward_as_tuple(sn.originator.name),
          std::forward_as_tuple(gtk_text_buffer_new(NULL),
                                gtk_text_buffer_new(NULL)));
        g_signal_connect(G_OBJECT(ne.first->second.edit_text), "changed",
                         G_CALLBACK(cbBufferChanged), this);
        DEB("New buf snapshot " << reinterpret_cast<const void *>(&sn) << " "
                                << sn.originator.name);

        std::string etext(sn.getEdit());
        gtk_text_buffer_set_text(ne.first->second.edit_text, etext.c_str(),
                                 etext.size());
        // edit->second.dirty = false;

        // labels?
        std::ifstream f(store_path + sn.originator.getClass() +
                        std::string("-labels.txt"));
        if (f.good()) {
          std::string labels(std::istreambuf_iterator<char>{ f }, {});
          gtk_text_buffer_set_text(ne.first->second.edit_labels, labels.c_str(),
                                   labels.size());
        }
        else {
          std::stringstream labels;
          for (auto n = 0U; n < std::count(etext.begin(), etext.end(), '\n');
               n++) {
            labels << (n + 1) << std::endl;
          }
          gtk_text_buffer_set_text(ne.first->second.edit_labels,
                                   labels.str().c_str(), labels.str().size());
        }
      }
      else {
        // re-use, refresh this text buffer
        edit->second.todelete = false;
        if (init || edit->second.dirty) {
          DEB("Update snapshot " << reinterpret_cast<const void *>(&sn) << " "
                                 << sn.originator.name);

          std::string etext(sn.getEdit());
          gtk_text_buffer_set_text(edit->second.edit_text, etext.c_str(),
                                   etext.size());
          edit->second.dirty = false;
        }
      }
    }
  }

  // check if any buffers need deleting
  for (auto emit = editmap.begin(); emit != editmap.end();) {
    if (emit->second.todelete) {
      emit = editmap.erase(emit);
      changes = true;
    }
    else {
      emit++;
    }
  }

  // update and revert no longer sensitive
  gtk_widget_set_sensitive(GTK_WIDGET(window["editwin_update"]), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(window["editwin_revert"]), FALSE);

  // when the buffer list changed, or a new inco set loaded, reload the combo,
  // and select the first set
  if (init || changes) {
    // reload the combobox options
    window.loadComboText("editwin_module_selection", origin_list);
    window.setValue(origin_list.front(), "editwin_module_selection");
  }
}

void SnapshotInventoryGtk3::readEditingMap()
{
  auto &snapset = inventory->editSnapshot(editing_snap);

  for (auto &sn : snapset.snaps) {

    auto mapit = editmap.find(sn.originator.name);
    if (mapit != editmap.end() && mapit->second.dirty) {
      DEB("Data snapshot " << reinterpret_cast<void *>(&sn) << " "
                           << sn.originator.name);
      GtkTextIter tstart, tend;
      gtk_text_buffer_get_start_iter(mapit->second.edit_text, &tstart);
      gtk_text_buffer_get_end_iter(mapit->second.edit_text, &tend);
      sn.setEdit(gtk_text_buffer_get_text(mapit->second.edit_text, &tstart,
                                          &tend, FALSE));
      mapit->second.dirty = false;
    }
  }

  // update and revert no longer sensitive
  gtk_widget_set_sensitive(GTK_WIDGET(window["editwin_update"]), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(window["editwin_revert"]), FALSE);
}

void SnapshotInventoryGtk3::cbEditInitial(GtkWidget *btn, gpointer gp)
{
  prepareEditingMap(true);
  gtk_widget_show(editwin);
}

void SnapshotInventoryGtk3::cbEditRevert(GtkWidget *btn, gpointer gp)
{
  // reload all original text buffers
  prepareEditingMap(false);
}

void SnapshotInventoryGtk3::cbEditUpdate(GtkWidget *btn, gpointer gp)
{
  readEditingMap();
}

void SnapshotInventoryGtk3::cbEditChange(GtkTextBuffer *tb)
{
  if (sel_origin != editmap.end()) {
    if (!sel_origin->second.dirty) {
      gtk_widget_set_sensitive(GTK_WIDGET(window["editwin_update"]), TRUE);
      gtk_widget_set_sensitive(GTK_WIDGET(window["editwin_revert"]), TRUE);

      sel_origin->second.dirty = true;
    }
  }
}

SnapshotInventoryGtk3::EditData::EditData(GtkTextBuffer *edit_text,
                                          GtkTextBuffer *edit_labels) :
  edit_text(edit_text),
  edit_labels(edit_labels),
  dirty(false),
  todelete(false)
{
  if (edit_text)
    g_object_ref(edit_text);
  if (edit_labels)
    g_object_ref(edit_labels);
}

SnapshotInventoryGtk3::EditData::~EditData()
{
  if (edit_text)
    g_object_unref(edit_text);
  if (edit_labels)
    g_object_unref(edit_labels);
}

void SnapshotInventoryGtk3::cbEditSelection(GtkComboBoxText *box, gpointer gp)
{
  std::string selorigin;
  window.getValue(selorigin, "editwin_module_selection");
  sel_origin = editmap.find(selorigin);
  if (sel_origin == editmap.end()) {
    /* DUSIME replay&initial

       Error in linking initial state edit window to the selection. Please
       report what you did to get here.
    */
    W_XTR("Somehow cannot find edit selection");
    return;
  }
  gtk_text_view_set_buffer(GTK_TEXT_VIEW(window["editwin_inco"]),
                           sel_origin->second.edit_text);
  gtk_text_view_set_buffer(GTK_TEXT_VIEW(window["editwin_linenumbers"]),
                           sel_origin->second.edit_labels);
}

gboolean SnapshotInventoryGtk3::cbEditDelete(GtkWidget *window, GdkEvent *event,
                                             gpointer user_data)
{
  // fixes the menu check, and closes the window
  gtk_widget_set_visible(editwin, FALSE);

  // indicate that the event is handled
  return TRUE;
}

void SnapshotInventoryGtk3::cbEditClose(GtkWidget *button, gpointer gp)
{
  gtk_widget_set_visible(editwin, FALSE);
}

void SnapshotInventoryGtk3::cbSelection(GtkTreeSelection *sel, gpointer gp)
{
  // figure out which row is selected
  GtkTreeIter iter;
  gchararray name = NULL;
  GtkTreeModel *treemodel = GTK_TREE_MODEL(snaps_store);
  if (gtk_tree_selection_get_selected(sel, &treemodel, &iter)) {
    // retrieve the name of the initials:
    gtk_tree_model_get(treemodel, &iter, S_name, &name, -1);
  }

  if (name != NULL && inventory->changeSelection(name)) {
    gtk_label_set_text(GTK_LABEL(window["initials_selected"]), name);
    gtk_widget_set_sensitive(GTK_WIDGET(window["initials_send"]), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(window["initials_edit"]), TRUE);
    gtk_label_set_text(GTK_LABEL(window["initials_status"]), "selected");
  }
  else {
    gtk_label_set_text(GTK_LABEL(window["initials_selected"]),
                       "<< none selected >>");
    gtk_widget_set_sensitive(GTK_WIDGET(window["initials_send"]), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(window["initials_edit"]), FALSE);
    gtk_label_set_text(GTK_LABEL(window["initials_status"]), "");
    gtk_widget_set_visible(editwin, FALSE);
  }
}

gboolean SnapshotInventoryGtk3::cbDelete(GtkWidget *window, GdkEvent *event,
                                         gpointer user_data)
{
  // fixes the menu check, and closes the window
  g_signal_emit_by_name(G_OBJECT(menuitem), "activate", NULL);

  // indicate that the event is handled
  return TRUE;
}

bool SnapshotInventoryGtk3::setPositionAndSize(const std::vector<int> &p)
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

} // namespace dueca
