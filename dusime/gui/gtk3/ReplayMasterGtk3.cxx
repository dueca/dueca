/* ------------------------------------------------------------------   */
/*      item            : ReplayMasterGtk3.cxx
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

#define ReplayMasterGtk3_cxx
#include "ReplayMasterGtk3.hxx"

// include the debug writing header, by default, write warning and
// error messages
#include <debug.h>
#include <dueca/ObjectManager.hxx>
#include <dusime/DusimeController.hxx>
#include <debprint.h>
#include <dueca/DuecaPath.hxx>
#include "GtkDuecaView.hxx"
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca/dueca.h>
#include <boost/date_time/posix_time/posix_time.hpp>

DUECA_NS_START

// class/module name
const char* const ReplayMasterGtk3::classname = "replay-master";

// Parameters to be inserted
const ParameterTable* ReplayMasterGtk3::getParameterTable()
{
  static const ParameterTable parameter_table[] = {

    { "glade-file",
      new VarProbe<_ThisModule_,std::string>
      (&_ThisModule_::gladefile),
      "Interface description (glade, gtkbuilder) for the channel view window" },

    { "reference-files",
      new VarProbe<_ThisModule_,std::string>
        (&_ThisModule_::reference_file),
      "Files with existing initial states (snapshots), one in each node. Will\n"
      "be read and used to populate the initial set" },

    { "store-files",
      new VarProbe<_ThisModule_,std::string>
        (&_ThisModule_::store_file),
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

ReplayMasterGtk3::ReplayMasterGtk3(Entity* e, const char* part, const
                                   PrioritySpec& ps) :
  Module(e, classname, part),

  // initialize the data you need in your simulation or process
  inco_inventory(SnapshotInventory::findSnapshotInventory(getPart())),
  replays(ReplayMaster::findReplayMaster(getPart())),
  gladefile(DuecaPath::prepend("replay_master_gtk3.ui")),
  window(),
  replay_store(NULL),
  replay_set_iter(),
  menuitem(NULL),
  reference_file(""),
  store_file("recordings-%Y%m%d_%H%M%S.ddff"),
  files_initialized(false)
{
  //
}

ReplayMasterGtk3::~ReplayMasterGtk3()
{

}

// organize some structure for initializing the tree
namespace {

  // attributes, name, and attachment to column in the model
  struct attributedata {
    const char* name;
    const gint column;
  };


  // column data. Columns are created in glade, this attaches the proper
  // renderer, indicate if expanded, and gives attributes
  struct columndata {
    // cell renderer
    GtkCellRenderer *renderer;
    // expand/extra space
    gboolean expand;
    // list of attributes, max 3 + sentinel
    const attributedata attribs[4];
  };

  std::string formatTime(const boost::posix_time::ptime& now,
                         const std::string& lft)
  {
    using namespace boost::posix_time;
    std::locale loc(std::cout.getloc(),
                    new time_facet(lft.c_str()));
    std::basic_stringstream<char> wss;
    wss.imbue(loc);
    wss << now;
    return wss.str();
  }
};

bool ReplayMasterGtk3::complete()
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
  inco_inventory->informOnNewMode
    ([this](SnapshotInventory::IncoInventoryMode mode, const std::string& name)
    {
      DEB("New inco mode " << mode << " inconame " << name);
      switch(mode) {
      case SnapshotInventory::IncoLoaded:

        // if the inco name matches the one for the currently selected replay
        // enable the replay control
        gtk_widget_set_sensitive
          (GTK_WIDGET(this->window["replay_sendrecording"]),
           (this->replays->initialStateMatches()) ? TRUE: FALSE);

        // if the name matches the currently selected replay record,
        // note that its inco is loaded
        if (this->replays->initialStateMatches()) {
          DEB("Loaded inco name matches");
          gtk_label_set_text
            (GTK_LABEL(this->window["replay_inco_status"]), "loaded");
        }

        // anyhow, set the name of the recorded inco on the record tab
        gtk_label_set_text
          (GTK_LABEL(this->window["record_inco_status"]), name.c_str());

        // and enable entering a recording name
        gtk_widget_set_sensitive
          (GTK_WIDGET(this->window["record_name"]), TRUE);
        break;

      case SnapshotInventory::IncoRecorded:

        // this can not be sent
        gtk_widget_set_sensitive
          (GTK_WIDGET(this->window["replay_sendrecording"]), FALSE);

        // set the name of the recorded inco
        gtk_label_set_text
          (GTK_LABEL(this->window["record_inco_status"]), name.c_str());

        // enable entering a recording name
        gtk_widget_set_sensitive
          (GTK_WIDGET(this->window["record_name"]), TRUE);
        break;

      default: // (UnSet, StartFiles)

        // replay tab
        gtk_widget_set_sensitive
          (GTK_WIDGET(this->window["replay_sendrecording"]), FALSE);
        gtk_widget_set_sensitive
          (GTK_WIDGET(this->window["replay_sendinitial"]), FALSE);

        // record tab
        gtk_label_set_text
          (GTK_LABEL(this->window["record_inco_status"]), "--");
        gtk_widget_set_sensitive
          (GTK_WIDGET(this->window["record_name"]), FALSE);
        gtk_widget_set_sensitive
          (GTK_WIDGET(this->window["record_prepare"]), FALSE);
        gtk_label_set_text
          (GTK_LABEL(this->window["record_status"]), "not prepared");
      }
    });

  // install a callback to update the state of the interface
  replays->informOnNewMode
    ([this](ReplayMaster::ReplayMasterMode mode) {
      DEB("New replay mode " << mode);
      switch(mode) {
      case ReplayMaster::Idle:
        gtk_label_set_text(GTK_LABEL(this->window["replay_rec_status"]), "--");

        // sending a new recording only after re-loading the matching initial
        gtk_widget_set_sensitive
          (GTK_WIDGET(this->window["replay_sendrecording"]), FALSE);

        // entering a new recording name is only possible after inco known
        gtk_widget_set_sensitive
          (GTK_WIDGET(this->window["record_name"]), FALSE);
        break;

      case ReplayMaster::RecordingPrepared:
        gtk_label_set_text(GTK_LABEL(this->window["record_status"]), "prepared");
        gtk_widget_set_sensitive
          (GTK_WIDGET(window["record_prepare"]), FALSE);
        break;

      case ReplayMaster::ReplayPrepared:
        gtk_label_set_text(GTK_LABEL(window["replay_rec_status"]), "prepared");
        gtk_widget_set_sensitive
          (GTK_WIDGET(window["replay_sendinitial"]), FALSE);
        gtk_widget_set_sensitive
          (GTK_WIDGET(window["replay_sendrecording"]), FALSE);
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
        gtk_label_set_text
          (GTK_LABEL(this->window["record_inco_status"]), "--");
        gtk_widget_set_sensitive
          (GTK_WIDGET(this->window["record_name"]), FALSE);

        // no break; intentional fall-through!

      case ReplayMaster::UnSet:
        gtk_label_set_text(GTK_LABEL(window["replay_rec_status"]), "--");
        gtk_widget_set_sensitive
          (GTK_WIDGET(window["replay_sendinitial"]), FALSE);
        gtk_widget_set_sensitive
          (GTK_WIDGET(window["replay_sendrecording"]), FALSE);
        break;

      default:
        break;
      };
    });
  static GladeCallbackTable cb_table[] = {
    { "replay_close", "clicked",
      gtk_callback(&_ThisModule_::cbClose) },
    { "replay_sendinitial", "clicked",
      gtk_callback(&_ThisModule_::cbSendInitial) },
    { "replay_sendrecording", "clicked",
      gtk_callback(&_ThisModule_::cbSendReplay) },
    { "replay_holdafter", "toggled",
      gtk_callback(&_ThisModule_::cbSelectHoldAfter) },
    { "replay_advanceafter", "toggled",
      gtk_callback(&_ThisModule_::cbSelectAdvanceAfter) },
    { "replay_recording_selection", "changed",
      gtk_callback(&_ThisModule_::cbSelectReplay) },
    { "record_name", "changed",
      gtk_callback(&_ThisModule_::cbRecordName) },
    { "record_prepare", "clicked",
      gtk_callback(&_ThisModule_::cbRecordPrepare) },
    { "replay_select_view", "delete_event",
      gtk_callback(&_ThisModule_::cbDelete) },
    { NULL }
  };

  bool res = window.readGladeFile
    (gladefile.c_str(), "replay_select_view",
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
    W_MOD("Replay cannot continue with advance, disabling");
    gtk_widget_set_sensitive(GTK_WIDGET(window["replay_advanceafter"]), FALSE);
  }

  GtkTreeView *replaytree = GTK_TREE_VIEW(window["replay_recording_overview"]);
  replay_store = GTK_LIST_STORE(gtk_tree_view_get_model(replaytree));

  static GtkCellRenderer *txtrenderer = gtk_cell_renderer_text_new();

  // name, title, renderer, sort, expand
  // (attribute, column) x n
  static columndata cdata_rec[] = {
    { txtrenderer, FALSE,
      { { "text", S_rec_name}, { NULL, 0 } } },
    { txtrenderer, FALSE,
      { { "text", S_rec_date }, { NULL, 0 } } },
    { txtrenderer, FALSE,
      { { "text", S_rec_span }, { NULL, 0 } } },
    { txtrenderer, TRUE,
      { { "text", S_rec_inco_name }, { NULL, 0 } } },
    { NULL, FALSE, { { NULL, 0} } }
  };

  // this sets the renderer(s) on the columns
  int icol = 0;
  for (const struct columndata* cd = cdata_rec; cd->renderer != NULL; cd++) {
    GtkTreeViewColumn *col = gtk_tree_view_get_column(replaytree, icol++);
    gtk_tree_view_column_pack_start(col, cd->renderer, cd->expand);
    for (const struct attributedata* at = cd->attribs; at->name != NULL; at++) {
      gtk_tree_view_column_add_attribute
        (col, cd->renderer, at->name, at->column);
    }
  }

  auto fcn = [this](const ReplayMaster::ReplayInfo& rep) {
    gtk_list_store_append
      (this->replay_store, &(this->replay_set_iter));
    gtk_list_store_set
      (replay_store, &(this->replay_set_iter),
       S_rec_id, rep.cycle,
       S_rec_name, rep.label.c_str(),
       S_rec_date, rep.getTimeLocal().c_str(),
       S_rec_span, rep.getSpanInSeconds(),
       S_rec_inco_name, rep.inco_name.c_str(),
       -1);
  };
  replays->runRecords(fcn);
  replays->informOnNewRecord(fcn);

  // set a title
  gtk_window_set_title
    (GTK_WINDOW(window["replay_select_view"]),
     (std::string("Record&Replay control - ") + getPart()).c_str());

  // insert in DUECA's menu
  menuitem = GTK_WIDGET(GtkDuecaView::single()->requestViewEntry
                        ((std::string("Replay Control - ") + getPart()).c_str(),
                         window.getObject("replay_select_view")));



  return res;
}

bool ReplayMasterGtk3::isPrepared()
{
  bool res = true;
  CHECK_CONDITION(replays->channelsValid());
  CHECK_CONDITION(inco_inventory->channelsValid());

  if (res && !files_initialized) {
    std::string file_marked = formatTime
      (boost::posix_time::second_clock::universal_time(), store_file);
    replays->initWork(reference_file, file_marked);

    /* DUSIME replay&initial

       Information on the suffix used for the recording files. Note that
       multiple nodes will write these files */
    I_XTR("Writing recording files " << store_file);
    files_initialized = true;
  }

  return res;
}

void ReplayMasterGtk3::startModule(const TimeSpec& ts)
{
  //
}

void ReplayMasterGtk3::stopModule(const TimeSpec& ts)
{
  //
}

void ReplayMasterGtk3::cbClose(GtkButton* button, gpointer gp)
{
  g_signal_emit_by_name(G_OBJECT(menuitem), "activate", NULL);
}

void ReplayMasterGtk3::cbSendReplay(GtkButton* btn, gpointer gp)
{
  // the button is only active when the correct inco has been sent,
  // and a replay selected
  replays->sendSelected();
}

void ReplayMasterGtk3::cbSelectHoldAfter(GtkWidget* widget, gpointer gp)
{
  if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))) {
    replays->setAdvanceAfterReplay(false);
  }
}

void ReplayMasterGtk3::cbSelectAdvanceAfter(GtkWidget* widget, gpointer gp)
{
  if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))) {
    replays->setAdvanceAfterReplay(true);
  }
}

void ReplayMasterGtk3::cbSelectReplay(GtkTreeSelection *sel,
                                      gpointer gp)
{
  GtkTreeIter  iter;
  gint id_rec = -1;
  gchararray rec_name = NULL;
  gchararray inco_name = NULL;
  GtkTreeModel *treemodel = GTK_TREE_MODEL(replay_store);

  // get the currently selected
  if (gtk_tree_selection_get_selected
      (sel, &treemodel, &iter)) {
    gtk_tree_model_get(treemodel, &iter, S_rec_id, &id_rec,
                       S_rec_name, &rec_name,
                       S_rec_inco_name, &inco_name, -1);
  }

  if (inco_name != NULL && inco_inventory->changeSelection(inco_name)) {
    gtk_entry_set_text(GTK_ENTRY(window["replay_inco_selected"]), inco_name);
    gtk_widget_set_sensitive(GTK_WIDGET(window["replay_sendinitial"]), TRUE);
  }
  else {
    gtk_entry_set_text(GTK_ENTRY(window["replay_inco_selected"]), "");
    gtk_widget_set_sensitive(GTK_WIDGET(window["replay_sendinitial"]), FALSE);
  }
  DEB("cbSelectReplay, changing to replay " << id_rec << "/" << rec_name
      << " inco " << inco_name);

  if (rec_name != NULL) {
    gtk_entry_set_text
      (GTK_ENTRY(window["replay_recording_selected"]), rec_name);
    gtk_widget_set_sensitive
      (GTK_WIDGET(window["replay_sendrecording"]), FALSE);
  }
  replays->changeSelection(id_rec);
}

void ReplayMasterGtk3::cbSendInitial(GtkButton* button, gpointer gp)
{
  // this button should only be sensitive when the correct inco
  // has been selected
  bool success = inco_inventory->sendSelected();

  DEB("cbSendInitial, result=" << success << " sending " <<
      inco_inventory->getSelected());

  // whatever, block further sending
  gtk_widget_set_sensitive
    (GTK_WIDGET(window["replay_sendinitial"]), FALSE);

  if (success) {
    // can now send the recording
    gtk_widget_set_sensitive
      (GTK_WIDGET(window["replay_sendrecording"]), TRUE);
    gtk_label_set_text
      (GTK_LABEL(window["replay_inco_status"]), "loaded");
  }
  else {
    gtk_label_set_text
      (GTK_LABEL(window["replay_inco_status"]), "failed");
  }
}

gboolean ReplayMasterGtk3::
cbDelete(GtkWidget *window, GdkEvent *event, gpointer user_data)
{
  // fixes the menu check, and closes the window
  g_signal_emit_by_name(G_OBJECT(menuitem), "activate", NULL);

  // indicate that the event is handled
  return TRUE;
}

void ReplayMasterGtk3::cbRecordPrepare(GtkButton* button, gpointer gp)
{
  std::string recording{gtk_entry_get_text(GTK_ENTRY(window["record_name"]))};
  DEB("cbRecordPrepare, with record name " << recording);
  replays->prepareRecording(recording);
  gtk_widget_set_sensitive
    (GTK_WIDGET(window["record_prepare"]), FALSE);
}

void ReplayMasterGtk3::cbRecordName(GtkWidget* text, gpointer gp)
{
  std::string newtext{gtk_entry_get_text(GTK_ENTRY(text))};
  DEB("cbRecordName, name " << newtext << " existing? " <<
      replays->haveReplaySet(newtext));
  gtk_widget_set_sensitive
    (GTK_WIDGET(window["record_prepare"]),
     replays->haveReplaySet(newtext) ? FALSE : TRUE);
}


DUECA_NS_END;
