/* ------------------------------------------------------------------   */
/*      item            : GtkDuecaView.cxx
        made by         : Rene' van Paassen
        date            : 000721
        category        : body file
        description     :
        changes         : 000721 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define GtkDuecaView_cc
#include <dueca-conf.h>

// this module is only feasible if libglade is available.

#include "GtkDuecaButtons.hxx"
#include "GtkDuecaView.hxx"
#include <ClockTime.hxx>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <StatusT1.hxx>

#define W_CNF
#define E_CNF
#include <debug.h>

#include "Environment.hxx"
#include "EntityManager.hxx"
#include "NodeManager.hxx"
#include "ParameterTable.hxx"
#include <DuecaPath.hxx>
#include "Ticker.hxx"

#define DO_INSTANTIATE
#include "MemberCall.hxx"
#include "Callback.hxx"
#include "VarProbe.hxx"

enum {
  ENT_COL_NAME = 0,
  ENT_COL_MSTATE,
  ENT_COL_SSTATE,
  ENT_COL_NODE,
  ENT_COL_OBJ,
  ENT_NUMCOL
};

extern "C" {
void read_md2_file(GtkWidget *widget, gpointer user_data);
}

// forward declaration for a function that hides or shows windows
// in the "View" menu of the dueca main control window.
static void hide_or_show_view(GtkWidget *widget, gpointer data)
{
  GtkCheckMenuItem *menuitem = GTK_CHECK_MENU_ITEM(widget);
  GtkWidget *window = GTK_WIDGET(data);
  if (menuitem != NULL && window != NULL) {
    if (gtk_check_menu_item_get_active(menuitem)) {
      gtk_widget_show(window);
    }
    else {
      gtk_widget_hide(window);
    }
  }
}

DUECA_NS_START

const char *const GtkDuecaView::classname = "dueca-view";

GtkDuecaView *GtkDuecaView::singleton = NULL;

GtkDuecaView::GtkDuecaView(Entity *e, const char *part,
                           const PrioritySpec &ps) :
  Module(e, classname, part),
  DuecaView(true),
#ifdef BUILD_DMODULES
  gladefile(DuecaPath::prepend("dusime.glade3")),
#else
  gladefile(DuecaPath::prepend("dueca.glade3")),
#endif
  shutdownscript(""),
  followup(None),
  second(int(1.0 / Ticker::single()->getTimeGranule() + 0.5)),
  simple_io(false),
  hw_off(NULL),
  hw_safe(NULL),
  hw_on(NULL),
  emergency(NULL),
  entities_list(NULL),
  entities_store(NULL),
  nodes_list(NULL),
  nodes_store(NULL),
  safe_armed(false),
  cb(this, &GtkDuecaView::updateInterface),
  update_interface(getId(), "update dueca interface", &cb,
                   PrioritySpec(0, -20)),
  waker()
{
  // update the singleton pointer. Note that checking is done in the
  // base class.
  singleton = this;
}

bool GtkDuecaView::complete()
{
  /* Switch activity on. */
  update_interface.setTrigger(waker);
  update_interface.switchOn(0);

  bool res = window.readGladeFile(gladefile.c_str(), "dueca_if");
  if (!res) {
    /* DUECA UI.

       Cannot find the glade file defining the base GUI. Check DUECA
       installation and paths.
    */
    E_CNF(" failed to open DUECA interface");
    return res;
  }

  // realize and show window
  window.show();

  // check for interface variant
  simple_io = window["button_run"] != NULL;

  if (simple_io) {
    // connect callbacks for simple variant, all buttons route through
    // GtkDuecaView
    GladeCallbackTable cb_links[] = {
      // buttons for the minimal interface variant
      { "button_quit", "released", gtk_callback(&GtkDuecaView::cbQuit) },
      { "button_stop", "released", gtk_callback(&GtkDuecaView::cbStop) },
      { "button_safe", "released", gtk_callback(&GtkDuecaView::cbSafe) },
      { "button_run", "released", gtk_callback(&GtkDuecaView::cbRun) },
      { "button_shutdown", "released",
        gtk_callback(&GtkDuecaView::cbShutDown) },
      { "button_confirm", "released", gtk_callback(&GtkDuecaView::cbConfirm) },
      { NULL, NULL, NULL }
    };
    window.connectCallbacks(reinterpret_cast<gpointer>(this), cb_links);

    // initial button state
    gtk_widget_set_sensitive(window["button_run"], FALSE);
    gtk_widget_set_sensitive(window["button_confirm"], FALSE);
  }
  else {

    // obtain some commonly used widgets
    entities_list = window["entity_status"];
    entities_store = GTK_TREE_STORE(window.getObject("model_entities"));

    nodes_list = window["nodes_list"];
    nodes_store = GTK_LIST_STORE(window.getObject("model_nodes"));

    hw_off = window["hw_off"];
    hw_safe = window["hw_safe"];
    hw_on = window["hw_on"];
    emergency = window["emergency"];

    // stuff some buttons with status feedback pixmaps
    const char *blist[] = { "hw_off",       "hw_safe",  "hw_on",
                            "replay", // also does any DUSIME buttons
                            "hw_calibrate", "inactive", "holdcurrent",
                            "advance",      NULL };
    for (const char **pw = blist; *pw; pw++) {
      GtkContainer *wg = GTK_CONTAINER(window[*pw]);
      if (wg) {
        // clean the current child (empty label), and replace with a
        // container full of buttons
        GList *child = gtk_container_get_children(wg);
        if (child) {
          gtk_container_remove(wg, GTK_WIDGET(child->data));
          g_list_free(child);
        }
        gtk_container_add(wg, NewGtkDuecaButton_pixmaps());
        gtk_dueca_button_set_image(GTK_WIDGET(wg), 0);

        // allso add button release as event
        gtk_widget_add_events(window[*pw],
                              GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK);
      }
      else {
        /* DUECA UI.

           Cannot find images for the default buttons. */
        W_CNF("Cannot attach image list to button" << pw);
      }
    }

    // stuff the emergency button
    GtkContainer *wg = GTK_CONTAINER(window["emergency"]);
    if (wg) {
      GList *child = gtk_container_get_children(wg);
      if (child) {
        gtk_container_remove(wg, GTK_WIDGET(child->data));
        g_list_free(child);
      }
      gtk_container_add(wg, NewGtkDuecaAbortButton_pixmaps());
      gtk_dueca_button_set_image(GTK_WIDGET(wg), 0);
    }
    gtk_widget_add_events(window["emergency"],
                          GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK);

    // switch on the stop button
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hw_off), TRUE);

    // connect callbacks entity control widgets
    GladeCallbackTable cb_links[] = {
      { "hw_on", "button-release-event", gtk_callback(&GtkDuecaView::cbOn2) },
      { "hw_safe", "button-release-event",
        gtk_callback(&GtkDuecaView::cbSafe2) },
      { "hw_off", "button-release-event", gtk_callback(&GtkDuecaView::cbOff2) },
      { "emergency", "button-release-event",
        gtk_callback(&GtkDuecaView::cbEmerg2) },
      { "quit", "activate", gtk_callback(&GtkDuecaView::cbWantToQuit) },
      { "dueca_if", "delete_event", gtk_callback(&GtkDuecaView::deleteView) },
      { "about", "activate", gtk_callback(&GtkDuecaView::cbShowAbout) },
      { "read_extra_mod", "activate",
        gtk_callback(&GtkDuecaView::cbExtraModDialog) },
      { NULL, NULL, NULL, NULL }
    };
    window.connectCallbacks(reinterpret_cast<gpointer>(this), cb_links);

    // additional windows that might be opened
    string commonglade = DuecaPath::prepend("common_if.glade3");

    {
      GladeCallbackTable cb_links[] = {
        { "button_really_quit", "clicked",
          gtk_callback(&GtkDuecaView::cbQuit2) },
        { "ok_button1", "clicked", gtk_callback(&GtkDuecaView::cbReadMod),
          gpointer(1) },
        { "obnoxious", "clicked", gtk_callback(&GtkDuecaView::cbQuit2) },
        { "button_continue", "clicked",
          gtk_callback(&GtkDuecaView::cbCloseQuit) },
        { "close_dontstop", "clicked",
          gtk_callback(&GtkDuecaView::cbCloseQuit) },
        { NULL, NULL, NULL, NULL }
      };

      res = gw_common.readGladeFile(commonglade.c_str(), "about2",
                                    reinterpret_cast<gpointer>(this), cb_links,
                                    true);
      if (!res) {
        /* DUECA UI.

           Could not create the "about" dialog. Check DUECA
           installation and paths.
        */
        E_CNF(" failed to create about dialog");
        return res;
      }
    }

    // check that the other top-level widgets are there
    if (!gw_common["really_quit"]) {
      /* DUECA UI.

         Could not create the "quit" dialog. Check DUECA
         installation and paths.
      */
      E_CNF(" failed to create quit dialog");
      return false;
    }
    gtk_window_set_transient_for(GTK_WINDOW(gw_common["really_quit"]),
                                 GTK_WINDOW(window["dueca_if"]));

    if (!gw_common["dont_stop_warning"]) {
      /* DUECA UI.

         Could not create the stop warning dialog. Check DUECA
         installation and paths.
      */
      E_CNF(" failed to create stop warning dialog");
      return false;
    }
    gtk_window_set_transient_for(GTK_WINDOW(gw_common["dont_stop_warning"]),
                                 GTK_WINDOW(window["dueca_if"]));

    if (!gw_common["dont_change_a_running"]) {
      /* DUECA UI.

         Could not create the "change" warning dialog. Check DUECA
         installation and paths.
      */
      E_CNF(" failed to create change warning dialog");
      return false;
    }
    gtk_window_set_transient_for(GTK_WINDOW(gw_common["dont_change_a_running"]),
                                 GTK_WINDOW(window["dueca_if"]));


    if (!gw_common["select_md2"]) {
      /* DUECA UI.

         Could not create the "additional model" dialog. Check DUECA
         installation and paths.
      */
      E_CNF(" failed to create addition model dialog");
      return false;
    }
    gtk_window_set_transient_for(GTK_WINDOW(gw_common["select_md2"]),
                                 GTK_WINDOW(window["dueca_if"]));



  }

  return res;
}

bool GtkDuecaView::isPrepared() { return true; }

GtkDuecaView::~GtkDuecaView()
{
  // g_object_unref(rc_altstyle);
  // g_object_unref(rc_quitaltstyle);
  // g_object_unref(rc_cfaltstyle);
  // g_object_unref(G_OBJECT(entities_store));
  // g_object_unref(G_OBJECT(nodes_store));
}

bool GtkDuecaView::setPositionAndSize(const vector<int> &p)
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

void GtkDuecaView::startModule(const TimeSpec &time)
{
  // does nothing
}

void GtkDuecaView::stopModule(const TimeSpec &time)
{
  // does nothing again
}

const ParameterTable *GtkDuecaView::getParameterTable()
{
  static ParameterTable table[] = {
    { "glade-file",
      new VarProbe<GtkDuecaView, vstring>(REF_MEMBER(&GtkDuecaView::gladefile)),
      "Supply the filename for the glade interface definition. If not\n"
      "supplied, it uses a default window" },
    { "shutdown-script",
      new VarProbe<GtkDuecaView, vstring>(
        REF_MEMBER(&GtkDuecaView::shutdownscript)),
      "Script called to shut down the DUECA nodes" },
    { "position-size",
      new MemberCall<GtkDuecaView, vector<int>>(
        &GtkDuecaView::setPositionAndSize),
      "Specify the position, and optionally also the size of the interface\n"
      "window." },
    { NULL, NULL, "Creates a main window for control of DUECA/DUSIME" }
  };
  return table;
}

void *GtkDuecaView::requestViewEntry(const char *name, void *object)
{
  GtkWidget *viewmenu, *dueca_control;

  viewmenu = window["view_menu"];
  dueca_control = window["dueca_if"];

  GtkWidget *newitem = gtk_check_menu_item_new_with_label(name);
  gtk_widget_set_name(newitem, name);
  g_object_ref(newitem);
  g_object_set_data_full(G_OBJECT(dueca_control), name, newitem,
                         (GDestroyNotify)g_object_unref);
  gtk_widget_show(newitem);
  gtk_container_add(GTK_CONTAINER(viewmenu), newitem);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(newitem), FALSE);

  g_signal_connect(G_OBJECT(newitem), "activate", G_CALLBACK(hide_or_show_view),
                   G_OBJECT(object));

  return newitem;
}

void GtkDuecaView::cbQuit(GtkButton *button, gpointer gp)
{
  // quit button needs confirmation
  followup = Quit;
  waker.requestAlarm(SimTime::getTimeTick() + 5 * second);

  // put yellow on the quit button
  // gtk_widget_modify_style (window["button_quit"], rc_altstyle);
  gtk_style_context_add_class(
    gtk_widget_get_style_context(window["button_quit"]), "quit-yellow");

  // enable confirm button + make it green
  gtk_widget_set_sensitive(window["button_confirm"], 1);
  // gtk_widget_modify_style (window["button_confirm"], rc_cfaltstyle);
  gtk_style_context_add_class(
    gtk_widget_get_style_context(window["button_confirm"]), "confirm-green");
}

void GtkDuecaView::cbStop(GtkButton *button, gpointer gp)
{
  EntityManager::single()->controlEntities(0);
  followup = Stop;
  waker.requestAlarm(SimTime::getTimeTick() + 2 * second);

  // unplug safe button, set colour amber?
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(window["button_safe"]), FALSE);
  gtk_widget_set_sensitive(window["button_run"], 0);

  // gtk_widget_modify_style (window["button_stop"], rc_altstyle);
  gtk_style_context_add_class(
    gtk_widget_get_style_context(window["button_stop"]), "button-amber");
}

void GtkDuecaView::cbSafe(GtkButton *button, gpointer gp)
{
  EntityManager::single()->controlEntities(1);
  followup = Safe;
  waker.requestAlarm(SimTime::getTimeTick() + 2 * second);

  // unplug other two buttons
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(window["button_stop"]), FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(window["button_run"]), FALSE);
  gtk_widget_set_sensitive(window["button_stop"], 1);
  // gtk_widget_modify_style (window["button_safe"], rc_altstyle);
  gtk_style_context_add_class(
    gtk_widget_get_style_context(window["button_safe"]), "quit-yellow");
}

void GtkDuecaView::cbRun(GtkButton *button, gpointer gp)
{
  EntityManager::single()->controlEntities(2);
  followup = Run;
  waker.requestAlarm(SimTime::getTimeTick() + 2 * second);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(window["button_safe"]), FALSE);
  gtk_widget_set_sensitive(window["button_stop"], 0);
  // gtk_widget_modify_style (window["button_run"], rc_altstyle);
  gtk_style_context_add_class(
    gtk_widget_get_style_context(window["button_run"]), rc_altstyle);
}

void GtkDuecaView::cbShutDown(GtkButton *button, gpointer gp)
{
  // quit button needs confirmation
  followup = Shutdown;
  waker.requestAlarm(SimTime::getTimeTick() + 5 * second);

  // put yellow on the shutdown button
  // gtk_widget_modify_style (window["button_shutdown"], rc_altstyle);
  gtk_style_context_add_class(
    gtk_widget_get_style_context(window["button_shutdown"]), rc_altstyle);

  // enable confirm button, make it green
  gtk_widget_set_sensitive(window["button_confirm"], 1);
  // gtk_widget_modify_style (window["button_confirm"], rc_cfaltstyle);
  gtk_style_context_add_class(
    gtk_widget_get_style_context(window["button_confirm"]), rc_cfaltstyle);
}

void GtkDuecaView::cbConfirm(GtkButton *button, gpointer gp)
{
  switch (followup) {
  case Quit:
    NodeManager::single()->breakUp();
    // gtk_widget_modify_style (window["button_quit"], rc_style);
    clean_style(window["button_quit"]);
    break;
  case Shutdown:
    NodeManager::single()->breakUp();
    if (shutdownscript.size()) {

      // check the script is executable
      struct stat statres;
      int r = stat(shutdownscript.c_str(), &statres);
      if (r || !S_ISREG(statres.st_mode) ||
          (!(statres.st_uid == geteuid() &&
             (statres.st_mode & (S_IXUSR | S_IRUSR)) == (S_IXUSR | S_IRUSR)) &&
           !(statres.st_gid == getegid() &&
             (statres.st_mode & (S_IXGRP | S_IRGRP)) == (S_IXGRP | S_IRGRP)) &&
           !((statres.st_mode & (S_IXOTH | S_IROTH)) == (S_IXOTH | S_IROTH)))) {
        /* DUECA UI.

           Failure in executing the specified shutdown script. Check
           the script, settings, and permissions.
        */
        W_SYS(getId() << '/' << classname << " cannot execute "
                      << shutdownscript);
      }

      pid_t pid = fork();
      if (!pid) {
        // in child,
        execlp(shutdownscript.c_str(), shutdownscript.c_str(), NULL);
      }
    }
    // gtk_widget_modify_style (window["button_shutdown"], rc_style);
    clean_style(window["button_shutdown"]);
    break;
  default:
    break;
  }
  followup = None;

  // gtk_widget_modify_style (window["button_confirm"], rc_style);
  clean_style(window["button_confirm"]);
  gtk_widget_set_sensitive(window["button_confirm"], 0);
}

inline void GtkDuecaView::clean_style(GtkWidget *w)
{
  GtkStyleContext *cnt = gtk_widget_get_style_context(w);
  if (gtk_style_context_has_class(cnt, rc_altstyle))
    gtk_style_context_remove_class(cnt, rc_altstyle);
  if (gtk_style_context_has_class(cnt, rc_quitaltstyle))
    gtk_style_context_remove_class(cnt, rc_quitaltstyle);
  if (gtk_style_context_has_class(cnt, rc_cfaltstyle))
    gtk_style_context_remove_class(cnt, rc_cfaltstyle);
}

void GtkDuecaView::updateInterface(const TimeSpec &time)
{
  switch (followup) {
  case Quit:
  case Shutdown:
    gtk_widget_set_sensitive(window["button_confirm"], 0);
    clean_style(window["button_confirm"]);
    clean_style(window["button_shutdown"]);
    clean_style(window["button_quit"]);
    followup = None;
    break;

  case Stop:
    if (EntityManager::single()->getConfirmedState() ==
        ModuleState::InitialPrep) {
      clean_style(window["button_stop"]);
    }
    else {
      waker.requestAlarm(SimTime::getTimeTick() + second);
    }
    break;

  case Safe:
    if (EntityManager::single()->getConfirmedState() == ModuleState::Prepared &&
        NodeManager::single()->isDuecaSynced()) {
      clean_style(window["button_safe"]);
      gtk_widget_set_sensitive(window["button_run"], 1);
    }
    else {
      waker.requestAlarm(SimTime::getTimeTick() + second);
    }
    break;

  case Run:
    if (EntityManager::single()->getConfirmedState() == ModuleState::On) {
      clean_style(window["button_run"]);
    }
    else {
      waker.requestAlarm(SimTime::getTimeTick() + second);
    }
    break;

  default:
    break;
  }
}

void GtkDuecaView::cbShowAbout(GtkMenuItem *menuitem, gpointer user_data)
{
  gtk_widget_show(gw_common["about2"]);
}

void GtkDuecaView::cbCloseAbout(GtkDialog *menuitem, gpointer user_data)
{
  gtk_widget_hide(gw_common["about2"]);
}

void GtkDuecaView::cbExtraModDialog(GtkMenuItem *menuitem, gpointer user_data)
{
  if (DUECA_NS::EntityManager::single()->stopIsOK()) {
    gtk_widget_show(gw_common["select_md2"]);
  }
  else {
    gtk_widget_show(gw_common["dont_change_a_running"]);
  }
}

void GtkDuecaView::cbReadMod(GtkWidget *widget, gpointer user_data)
{
  // close the window
  gtk_widget_hide(gw_common["select_md2"]);

  if (user_data) {
    const gchar *filename =
      gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(gw_common["select_md2"]));

    if (filename == NULL) {
      return;
    }

    // obtain the filename
    vstring fname(filename);

    // ask environment to add this filename to the configuration
    if (fname.size()) {
      Environment::getInstance()->readMod(fname);
    }
  }
}

void GtkDuecaView::cbWantToQuit(GtkWidget *button, gpointer gp)
{
  if (EntityManager::single()->stopIsOK()) {
    gtk_widget_show(gw_common["really_quit"]);
  }
  else {
    gtk_widget_show(gw_common["dont_stop_warning"]);
  }
}

gboolean GtkDuecaView::deleteView(GtkWidget *window, GdkEvent *event,
                                  gpointer user_data)
{
  cbWantToQuit(NULL, NULL);
  return TRUE;
}

void GtkDuecaView::cbQuit2(GtkWidget *button, gpointer gp)
{
  NodeManager::single()->breakUp();
  gtk_widget_hide(gw_common["really_quit"]);
  gtk_widget_hide(gw_common["dont_stop_warning"]);
}

void GtkDuecaView::updateEntityButtons(const ModuleState &confirmed_state,
                                       const ModuleState &command_state,
                                       bool emergency_flag)
{
  if (!simple_io) {
    switch (confirmed_state.get()) {
    case ModuleState::InitialPrep:
      if (command_state == confirmed_state) {
        gtk_dueca_button_set_image(hw_off, 2);
      }
      gtk_widget_set_sensitive(hw_off, TRUE);
      gtk_widget_set_sensitive(hw_safe, TRUE);
      gtk_widget_set_sensitive(hw_on, FALSE);
      break;
    case ModuleState::Safe:
      gtk_dueca_button_set_image(hw_safe, 2);
      gtk_widget_set_sensitive(hw_off, TRUE);
      if (!emergency_flag) {
        gtk_widget_set_sensitive(hw_safe, TRUE);
      }
      gtk_widget_set_sensitive(hw_on, FALSE);
      break;
    case ModuleState::Prepared:
      gtk_dueca_button_set_image(hw_safe, 2);
      gtk_widget_set_sensitive(hw_off, TRUE);
      if (!emergency_flag) {
        gtk_widget_set_sensitive(hw_safe, TRUE);
        gtk_widget_set_sensitive(hw_on, TRUE);
      }
      break;
    case ModuleState::On:
      if (command_state == confirmed_state) {
        gtk_dueca_button_set_image(hw_on, 2);
      }
      gtk_widget_set_sensitive(hw_off, FALSE);
      if (please_keep_running) {
        gtk_widget_set_sensitive(hw_safe, FALSE);
      }
      else {
        gtk_widget_set_sensitive(hw_safe, TRUE);
      }
      gtk_widget_set_sensitive(hw_on, TRUE);
      break;
    default:

      // Unprepared Neutral and Undefined
      break;
    }
  }
}

static void deep_refresh(GtkTreeStore *tree, GtkTreeIter *iter)
{
  GtkTreeIter child;
  gboolean has_child =
    gtk_tree_model_iter_children(GTK_TREE_MODEL(tree), &child, iter);
  while (has_child) {
    deep_refresh(tree, &child);
    has_child = gtk_tree_model_iter_next(GTK_TREE_MODEL(tree), &child);
  }

  // refresh the displayed values
  GValue gobj = { 0 };
  gtk_tree_model_get_value(GTK_TREE_MODEL(tree), iter, 4, &gobj);
  StatusT1 *obj = reinterpret_cast<StatusT1 *>(g_value_peek_pointer(&gobj));
  gtk_tree_store_set(tree, iter, ENT_COL_MSTATE,
                     obj->getModuleState().getString(), -1);
  if (obj->haveSState()) {
    gtk_tree_store_set(tree, iter, ENT_COL_SSTATE,
                       obj->getSimulationState().getString(), -1);
  }
}

void GtkDuecaView::refreshEntitiesView()
{
  if (!entities_store)
    return;

  GtkTreeIter iter;
  gboolean f =
    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(entities_store), &iter);
  while (f) {
    deep_refresh(entities_store, &iter);
    f = gtk_tree_model_iter_next(GTK_TREE_MODEL(entities_store), &iter);
  }
  gtk_widget_queue_draw(entities_list);
}

void *GtkDuecaView::insertEntityNode(const char *name, void *vparent,
                                     int dueca_node, StatusT1 *obj)
{
  if (!entities_store)
    return NULL;

  GtkTreeIter iter, iparent;

  if (!vparent) {
    gtk_tree_store_append(entities_store, &iter, NULL);
  }
  else {
    GtkTreeRowReference *ref = reinterpret_cast<GtkTreeRowReference *>(vparent);
    gboolean res =
      gtk_tree_model_get_iter(GTK_TREE_MODEL(entities_store), &iparent,
                              gtk_tree_row_reference_get_path(ref));
    if (res) {
      gtk_tree_store_append(entities_store, &iter, &iparent);
    }
  }

  // add the data
  gtk_tree_store_set(entities_store, &iter, ENT_COL_NAME, name, // node name
                     ENT_COL_MSTATE, "unknown", // module status
                     ENT_COL_SSTATE, "", // sim status
                     ENT_COL_NODE, dueca_node, // dueca node no
                     ENT_COL_OBJ, obj, // pointer to status object
                     -1);

  return gtk_tree_row_reference_new(
    GTK_TREE_MODEL(entities_store),
    gtk_tree_model_get_path(GTK_TREE_MODEL(entities_store), &iter));
}

void GtkDuecaView::refreshNodesView()
{
  if (!nodes_store)
    return;

  // clear the listgtk_tree_gtk_tree_
  gtk_list_store_clear(nodes_store);
  GtkTreeIter iter;

  // fill it with the proper data
  for (NodeManager::query_iterator ii = NodeManager::single()->startQuery();
       !NodeManager::single()->isQueryComplete(ii);
       NodeManager::single()->goQueryNext(ii)) {
    gtk_list_store_append(nodes_store, &iter);
    gtk_list_store_set(nodes_store, &iter, 0, ii, 1,
                       NodeManager::single()->getNodeStatus(ii), -1);
  }
  gtk_widget_queue_draw(nodes_list);
}

gboolean GtkDuecaView::cbOn2(GtkWidget *widget, GdkEventButton *event,
                             gpointer user_data)
{
  if (event->type != GDK_BUTTON_RELEASE || event->button != 1) {
    // wrong button, and only act on presses
    return TRUE;
  }

  if (EntityManager::single()->controlEntities(2)) {
    gtk_dueca_button_set_image(widget, 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hw_safe), FALSE);
    gtk_widget_set_sensitive(hw_off, FALSE);
    gtk_dueca_button_set_image(hw_safe, 0);
  }
  else {
    // transition not possible, reset to unpressed
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
  }
  return TRUE;
}

gboolean GtkDuecaView::cbEmerg2(GtkWidget *widget, GdkEventButton *event,
                                gpointer user_data)
{
  if (event->type != GDK_BUTTON_RELEASE || event->button != 1) {
    // wrong button, and only act on presses
    return TRUE;
  }

  if (safe_armed) {
    // pressed for the second time. Exciting. Does the event indicate
    // that the lower portion was pressed?
    if (event->y > 42) {

      EntityManager::single()->emergency();
      NodeManager::single()->emergency();
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
      gtk_dueca_button_set_image(hw_on, 0);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hw_on), FALSE);
      gtk_dueca_button_set_image(hw_safe, 1);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hw_on), TRUE);

      // set all buttons to insensitive, everything should be off,
      gtk_widget_set_sensitive(hw_safe, FALSE);
      gtk_widget_set_sensitive(hw_on, FALSE);
      gtk_widget_set_sensitive(hw_off, FALSE);
    }
    else {
      safe_armed = false;
      gtk_dueca_button_set_image(widget, 0);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
    }
  }
  else {
    safe_armed = true;
    // set the yes/return face
    gtk_dueca_button_set_image(widget, 1);
    // but leave the toggle button out
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
  }
  return TRUE;
}

gboolean GtkDuecaView::cbOff2(GtkWidget *widget, GdkEventButton *event,
                              gpointer user_data)
{
  if (event->type != GDK_BUTTON_RELEASE || event->button != 1) {
    // wrong button, and only act on presses
    return TRUE;
  }
  if (EntityManager::single()->controlEntities(0)) {
    gtk_dueca_button_set_image(widget, 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hw_safe), FALSE);
    gtk_widget_set_sensitive(hw_on, FALSE);
    gtk_dueca_button_set_image(hw_safe, 0);
  }
  else {
    // transition not possible, reset to unpressed
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
  }
  return TRUE;
}

gboolean GtkDuecaView::cbSafe2(GtkWidget *widget, GdkEventButton *event,
                               gpointer user_data)
{
  if (event->type != GDK_BUTTON_RELEASE || event->button != 1) {
    // wrong button, and only act on presses
    return TRUE;
  }

  if (EntityManager::single()->controlEntities(1)) {
    gtk_dueca_button_set_image(widget, 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hw_on), FALSE);
    gtk_dueca_button_set_image(hw_on, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hw_off), FALSE);
    gtk_dueca_button_set_image(hw_off, 0);
  }
  else {
    // transition not possible, reset to unpressed
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
  }
  return TRUE;
}

void GtkDuecaView::cbCloseQuit(GtkWidget *btn, gpointer user_data)
{
  auto rq = gw_common["really_quit"];
  if (rq) {
    gtk_widget_set_visible(rq, FALSE);
  }
  auto ds = gw_common["dont_stop_warning"];
  if (ds) {
    gtk_widget_set_visible(ds, FALSE);
  }
}

void GtkDuecaView::requestToKeepRunning(bool keep_running)
{
  this->DuecaView::requestToKeepRunning(keep_running);
  if (!simple_io) {
    if (please_keep_running) {
      gtk_widget_set_sensitive(hw_safe, FALSE);
    }
    else {
      gtk_widget_set_sensitive(hw_safe, TRUE);
    }
  }
}

DUECA_NS_END;
