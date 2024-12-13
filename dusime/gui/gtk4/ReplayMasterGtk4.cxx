/* ------------------------------------------------------------------   */
/*      item            : ReplayMasterGtk4.cxx
        made by         : Rene' van Paassen
        date            : 220418
        category        : body file
        description     :
        changes         : 220418 first version
        language        : C++
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "gtk/gtk.h"
#include "gtk/gtkdropdown.h"
#define ReplayMasterGtk4_cxx
#include "ReplayMasterGtk4.hxx"

// include the debug writing header, by default, write warning and
// error messages
#include <debug.h>
#include <dueca/ObjectManager.hxx>
#include <dusime/DusimeController.hxx>
#include <debprint.h>
#include <dueca/DuecaPath.hxx>
#include "GtkDuecaView.hxx"
#include <boost/format.hpp>
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca/dueca.h>
#include <boost/date_time/posix_time/posix_time.hpp>

DUECA_NS_START

// class/module name
const char *const ReplayMasterGtk4::classname = "replay-master";

// Parameters to be inserted
const ParameterTable *ReplayMasterGtk4::getParameterTable()
{
  static const ParameterTable parameter_table[] = {

    { "gui-file",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::gladefile),
      "Interface description (glade, gtkbuilder) for the channel view window" },

    { "position-size",
      new MemberCall<_ThisModule_, std::vector<int>>(
        &_ThisModule_::setPositionAndSize),
      "Specify the position, and optionally also the size of the interface\n"
      "window." },

    { "reference-files",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::reference_file),
      "Files with existing initial states (snapshots), one in each node. Will\n"
      "be read and used to populate the initial set" },

    { "store-files",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::store_file),
      "When additional snapshots are taken in this simulation, these will\n"
      "be written in these files, one per node, together with the existing\n"
      "initial state sets. Uses a template, check boost time_facet for format\n"
      "strings. Default \"recordings-%Y%m%d_%H%M%S.ddff\"" },

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL,
      "A module that presents an overview of recordings for replay." }
  };

  return parameter_table;
}

ReplayMasterGtk4::ReplayMasterGtk4(Entity *e, const char *part,
                                   const PrioritySpec &ps) :
  Module(e, classname, part),

  // initialize the data you need in your simulation or process
  inco_inventory(SnapshotInventory::findSnapshotInventory(getPart())),
  replays(ReplayMaster::findReplayMaster(getPart())),
  gladefile(DuecaPath::prepend("replay_master-gtk4.ui")),
  window(),
  replay_store(NULL),
  menuaction(NULL),
  reference_file(""),
  store_file("recordings-%Y%m%d_%H%M%S.ddff"),
  files_initialized(false)
{
  //
}

ReplayMasterGtk4::~ReplayMasterGtk4() {}

// organize some structure for initializing the tree
namespace {

std::string formatTime(const boost::posix_time::ptime &now,
                       const std::string &lft)
{
  using namespace boost::posix_time;
  std::locale loc(std::cout.getloc(), new time_facet(lft.c_str()));
  std::basic_stringstream<char> wss;
  wss.imbue(loc);
  wss << now;
  return wss.str();
}
}; // namespace

struct _DReplayRun
{
  GObject parent;
  ReplayMaster::ReplayInfo rr;
};

G_DECLARE_FINAL_TYPE(DReplayRun, d_replay_run, D, REPLAY_RUN, GObject);
G_DEFINE_TYPE(DReplayRun, d_replay_run, G_TYPE_OBJECT);

static void d_replay_run_class_init(DReplayRunClass *klass) {}
static void d_replay_run_init(DReplayRun *self) {}

static DReplayRun *d_replay_run_new(const ReplayMaster::ReplayInfo &rr)
{
  auto res = D_REPLAY_RUN(g_object_new(d_replay_run_get_type(), NULL));
  res->rr = rr;
  return res;
}

bool ReplayMasterGtk4::complete()
{
  // the "part" indicate for which entity the replay is controlled
  if (getPart().size() == 0) {
    /* DUSIME replay&initial

       You did not specify an entity in the part label */
    E_XTR("Supply the managed entity to the snapshot inventory");
    return false;
  }

  // install a callback to update the state of the interface, depending on
  // initial condition changes
  inco_inventory->informOnNewMode([this](
                                    SnapshotInventory::IncoInventoryMode mode,
                                    const std::string &name) {
    DEB("New inco mode " << mode << " inconame " << name);
    switch (mode) {
    case SnapshotInventory::IncoLoaded:

      // if the inco name matches the one for the currently selected replay
      // enable the replay control
      gtk_widget_set_sensitive(GTK_WIDGET(this->window["replay_sendrecording"]),
                               (this->replays->initialStateMatches()) ? TRUE
                                                                      : FALSE);

      // if the name matches the currently selected replay record,
      // note that its inco is loaded
      if (this->replays->initialStateMatches()) {
        DEB("Loaded inco name matches");
        gtk_label_set_text(GTK_LABEL(this->window["replay_inco_status"]),
                           "loaded");
      }

      // anyhow, set the name of the recorded inco on the record tab
      gtk_label_set_text(GTK_LABEL(this->window["record_inco_status"]),
                         name.c_str());

      // and enable entering a recording name
      gtk_widget_set_sensitive(GTK_WIDGET(this->window["record_name"]), TRUE);
      break;

    case SnapshotInventory::IncoRecorded:

      // this can not be sent
      gtk_widget_set_sensitive(GTK_WIDGET(this->window["replay_sendrecording"]),
                               FALSE);

      // set the name of the recorded inco
      gtk_label_set_text(GTK_LABEL(this->window["record_inco_status"]),
                         name.c_str());

      // enable entering a recording name
      gtk_widget_set_sensitive(GTK_WIDGET(this->window["record_name"]), TRUE);
      break;

    default: // (UnSet, StartFiles)

      // replay tab
      gtk_widget_set_sensitive(GTK_WIDGET(this->window["replay_sendrecording"]),
                               FALSE);
      gtk_widget_set_sensitive(GTK_WIDGET(this->window["replay_sendinitial"]),
                               FALSE);

      // record tab
      gtk_label_set_text(GTK_LABEL(this->window["record_inco_status"]), "--");
      gtk_widget_set_sensitive(GTK_WIDGET(this->window["record_name"]), FALSE);
      gtk_widget_set_sensitive(GTK_WIDGET(this->window["record_prepare"]),
                               FALSE);
      gtk_label_set_text(GTK_LABEL(this->window["record_status"]),
                         "not prepared");
    }
  });

  // install a callback to update the state of the interface
  replays->informOnNewMode([this](ReplayMaster::ReplayMasterMode mode) {
    DEB("New replay mode " << mode);
    switch (mode) {
    case ReplayMaster::Idle:
      gtk_label_set_text(GTK_LABEL(this->window["replay_rec_status"]), "--");

      // sending a new recording only after re-loading the matching initial
      gtk_widget_set_sensitive(GTK_WIDGET(this->window["replay_sendrecording"]),
                               FALSE);

      // entering a new recording name is only possible after inco known
      gtk_widget_set_sensitive(GTK_WIDGET(this->window["record_name"]), FALSE);
      break;

    case ReplayMaster::RecordingPrepared:
      gtk_label_set_text(GTK_LABEL(this->window["record_status"]), "prepared");
      gtk_widget_set_sensitive(GTK_WIDGET(window["record_prepare"]), FALSE);
      break;

    case ReplayMaster::ReplayPrepared:
      gtk_label_set_text(GTK_LABEL(window["replay_rec_status"]), "prepared");
      gtk_widget_set_sensitive(GTK_WIDGET(window["replay_sendinitial"]), FALSE);
      gtk_widget_set_sensitive(GTK_WIDGET(window["replay_sendrecording"]),
                               FALSE);
      break;

    case ReplayMaster::ReplayingThenHold:
    case ReplayMaster::ReplayingThenAdvance:
      // replay buttons should still be insensitive
      gtk_label_set_text(GTK_LABEL(window["replay_rec_status"]), "replaying");

      break;

    case ReplayMaster::Recording:

      // status recording
      gtk_label_set_text(GTK_LABEL(window["record_status"]), "recording");

      // inco is now broken, do not change record name, cannot prepare
      gtk_label_set_text(GTK_LABEL(this->window["record_inco_status"]), "--");
      gtk_widget_set_sensitive(GTK_WIDGET(this->window["record_name"]), FALSE);

      // no break; intentional fall-through!

    case ReplayMaster::UnSet:
      gtk_label_set_text(GTK_LABEL(window["replay_rec_status"]), "--");
      gtk_widget_set_sensitive(GTK_WIDGET(window["replay_sendinitial"]), FALSE);
      gtk_widget_set_sensitive(GTK_WIDGET(window["replay_sendrecording"]),
                               FALSE);
      break;

    default:
      break;
    };
  });
  static GladeCallbackTable cb_table[] = {
    { "replay_close", "clicked", gtk_callback(&_ThisModule_::cbClose) },
    { "replay_sendinitial", "clicked",
      gtk_callback(&_ThisModule_::cbSendInitial) },
    { "replay_sendrecording", "clicked",
      gtk_callback(&_ThisModule_::cbSendReplay) },
    { "replay_todoafter", "notify::selected",
      gtk_callback(&_ThisModule_::cbSelectTodoAfter) },
    { "record_name", "changed", gtk_callback(&_ThisModule_::cbRecordName) },
    { "record_prepare", "clicked",
      gtk_callback(&_ThisModule_::cbRecordPrepare) },
    { "replay_select_view", "close-request",
      gtk_callback(&_ThisModule_::cbDelete) },
    { NULL }
  };

  bool res = window.readGladeFile(gladefile.c_str(), "replay_select_view",
                                  reinterpret_cast<gpointer>(this), cb_table);
  if (!res) {
    /* DUSIME replay&initial

       Cannot find the glade file defining the replay control
       GUI. Check DUECA installation and paths.
    */
    E_CNF("failed to open replay overview " << gladefile);
    return res;
  }

  // check whether advance is actually allowed in the DUSIME configuration
  if (!replays->canAdvanceAfterReplay()) {
    /* DUSIME replay&initial

       The advance mode cannot be controlled programmatically. Look in
       the options for your DUSIME module to enable this. */
    W_MOD("ReplayMaster cannot set DUSIME to advance, disabling option to "
          "continue.");
    auto model = GTK_STRING_LIST(
      gtk_drop_down_get_model(GTK_DROP_DOWN(window["replay_todoafter"])));
    gtk_string_list_remove(model, 1);
  }

  // set up the model behind the column view
  GtkColumnView *replaytree =
    GTK_COLUMN_VIEW(window["replay_recording_overview"]);
  replay_store = g_list_store_new(d_replay_run_get_type());
  auto selection = gtk_single_selection_new(G_LIST_MODEL(replay_store));
  auto cb = gtk_callback(&_ThisModule_::cbSelectReplay, this);
  g_signal_connect(selection, "selection-changed", cb->callback(), cb);
  gtk_column_view_set_model(replaytree, GTK_SELECTION_MODEL(selection));

  // callback that adds new replays
  auto fcn = [this](const ReplayMaster::ReplayInfo &rep) {
    auto item = d_replay_run_new(rep);
    g_list_store_append(this->replay_store, item);
  };

  // install callback for initial and incremental
  replays->runRecords(fcn);
  replays->informOnNewRecord(fcn);

  // set a title
  gtk_window_set_title(
    GTK_WINDOW(window["replay_select_view"]),
    (std::string("Record&Replay control - ") + getPart()).c_str());

  // insert in DUECA's menu
  menuaction = GtkDuecaView::single()->requestViewEntry(
    (std::string("replay_") + getPart()).c_str(),
    (std::string("Replay Control - ") + getPart()).c_str(),
    window.getObject("replay_select_view"));

  return res;
}

bool ReplayMasterGtk4::isPrepared()
{
  bool res = true;
  CHECK_CONDITION(replays->channelsValid());
  CHECK_CONDITION(inco_inventory->channelsValid());

  if (res && !files_initialized) {
    std::string file_marked =
      formatTime(boost::posix_time::second_clock::universal_time(), store_file);
    replays->initWork(reference_file, file_marked);

    /* DUSIME replay&initial

       Information on the suffix used for the recording files. Note that
       multiple nodes will write these files */
    I_XTR("Writing recording files " << store_file);
    files_initialized = true;
  }

  return res;
}

void ReplayMasterGtk4::startModule(const TimeSpec &ts)
{
  //
}

void ReplayMasterGtk4::stopModule(const TimeSpec &ts)
{
  //
}

void ReplayMasterGtk4::cbClose(GtkButton *button, gpointer gp)
{
  // g_signal_emit_by_name(G_OBJECT(menuaction), "activate", NULL);
  GtkDuecaView::toggleView(menuaction);
}

void ReplayMasterGtk4::cbSendReplay(GtkButton *btn, gpointer gp)
{
  // the button is only active when the correct inco has been sent,
  // and a replay selected
  replays->sendSelected();
}

void ReplayMasterGtk4::cbSelectTodoAfter(GObject *widget, GParamSpec *pspec,
                                         gpointer gp)
{
  auto sel = gtk_drop_down_get_selected(GTK_DROP_DOWN(widget));
  replays->setAdvanceAfterReplay(bool(sel));
}

void ReplayMasterGtk4::cbSelectReplay(GtkSelectionModel *sel, guint position,
                                      guint nsel, gpointer gp)
{
  if (gtk_selection_model_is_selected(sel, position)) {
    auto it =
      D_REPLAY_RUN(g_list_model_get_item(G_LIST_MODEL(replay_store), position));
    gtk_editable_set_text(GTK_EDITABLE(window["replay_inco_selected"]),
                          it->rr.inco_name.c_str());
    gtk_widget_set_sensitive(GTK_WIDGET(window["replay_sendinitial"]), TRUE);

    if (it->rr.label.size()) {
      gtk_editable_set_text(GTK_EDITABLE(window["replay_recording_selected"]),
                            it->rr.label.c_str());
      gtk_widget_set_sensitive(GTK_WIDGET(window["replay_sendrecording"]),
                               FALSE);
    }
    replays->changeSelection(it->rr.cycle);
    DEB("cbSelectReplay, changing to replay "
        << it->rr.cycle << "/" << it->rr.label << " inco " << it->rr.inco_name);
  }
  else {
    gtk_editable_set_text(GTK_EDITABLE(window["replay_inco_selected"]), "");
    gtk_widget_set_sensitive(GTK_WIDGET(window["replay_sendinitial"]), FALSE);
  }
}

void ReplayMasterGtk4::cbSendInitial(GtkButton *button, gpointer gp)
{
  // this button should only be sensitive when the correct inco
  // has been selected
  bool success = inco_inventory->sendSelected();

  DEB("cbSendInitial, result=" << success << " sending "
                               << inco_inventory->getSelected());

  // whatever, block further sending
  gtk_widget_set_sensitive(GTK_WIDGET(window["replay_sendinitial"]), FALSE);

  if (success) {
    // can now send the recording
    gtk_widget_set_sensitive(GTK_WIDGET(window["replay_sendrecording"]), TRUE);
    gtk_label_set_text(GTK_LABEL(window["replay_inco_status"]), "loaded");
  }
  else {
    gtk_label_set_text(GTK_LABEL(window["replay_inco_status"]), "failed");
  }
}

gboolean ReplayMasterGtk4::cbDelete(GtkWidget *window, gpointer user_data)
{
  // fixes the menu check, and closes the window
  // g_signal_emit_by_name(G_OBJECT(menuaction), "activate", NULL);
  GtkDuecaView::toggleView(menuaction);

  // indicate that the event is handled
  return TRUE;
}

void ReplayMasterGtk4::cbRecordPrepare(GtkButton *button, gpointer gp)
{
  std::string recording{ gtk_editable_get_text(
    GTK_EDITABLE(window["record_name"])) };
  DEB("cbRecordPrepare, with record name " << recording);
  replays->prepareRecording(recording);
  gtk_widget_set_sensitive(GTK_WIDGET(window["record_prepare"]), FALSE);
}

void ReplayMasterGtk4::cbRecordName(GtkWidget *text, gpointer gp)
{
  std::string newtext{ gtk_editable_get_text(GTK_EDITABLE(text)) };
  DEB("cbRecordName, name " << newtext << " existing? "
                            << replays->haveReplaySet(newtext));
  gtk_widget_set_sensitive(GTK_WIDGET(window["record_prepare"]),
                           replays->haveReplaySet(newtext) ? FALSE : TRUE);
}

void ReplayMasterGtk4::cbSetupLabel(GtkSignalListItemFactory *fact,
                                    GtkListItem *object, gpointer user_data)
{
  auto label = gtk_label_new("");
  gtk_list_item_set_child(object, label);
  g_object_unref(label);
}

void ReplayMasterGtk4::cbBindReplayName(GtkSignalListItemFactory *fact,
                                        GtkListItem *item, gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  auto entry = D_REPLAY_RUN(gtk_list_item_get_item(item));
  gtk_label_set_text(label, entry->rr.label.c_str());
}

void ReplayMasterGtk4::cbBindReplayDate(GtkSignalListItemFactory *fact,
                                        GtkListItem *item, gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  auto entry = D_REPLAY_RUN(gtk_list_item_get_item(item));
  gtk_label_set_text(label, entry->rr.getTimeLocal().c_str());
}

void ReplayMasterGtk4::cbBindReplayDuration(GtkSignalListItemFactory *fact,
                                            GtkListItem *item,
                                            gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  auto entry = D_REPLAY_RUN(gtk_list_item_get_item(item));
  gtk_label_set_text(
    label,
    boost::str(boost::format("%4d s") % entry->rr.getSpanInSeconds()).c_str());
}

void ReplayMasterGtk4::cbBindReplayInitial(GtkSignalListItemFactory *fact,
                                           GtkListItem *item,
                                           gpointer user_data)
{
  auto label = GTK_LABEL(gtk_list_item_get_child(item));
  auto entry = D_REPLAY_RUN(gtk_list_item_get_item(item));
  gtk_label_set_text(label, entry->rr.label.c_str());
}

bool ReplayMasterGtk4::setPositionAndSize(const std::vector<int> &p)
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

DUECA_NS_END;
