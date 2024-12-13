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

// for linking gtkapplication and main window
// https://github.com/ToshioCP/Gtk4-tutorial/blob/main/gfm/sec9.md
//
// menu stuff
// https://github.com/ToshioCP/Gtk4-tutorial/blob/main/gfm/sec17.md
//
// actions
// https://developer.gnome.org/documentation/tutorials/actions.html

#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "gtk/gtksingleselection.h"
#include "gui/gtk4/GtkGladeWindow.hxx"
#include <cstdint>
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
#include <boost/format.hpp>
#include "GtkHandler.hxx"

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

extern "C" {
void read_md2_file(GtkWidget *widget, gpointer user_data);
}

// forward declaration for a function that hides or shows windows
// in the "View" menu of the dueca main control window.
static void hide_or_show_view(GSimpleAction *action, GVariant *variant)
{
  auto window = GTK_WIDGET(g_object_get_data(G_OBJECT(action), "window"));
  auto state = g_action_get_state(G_ACTION(action));
  auto newstate = g_variant_new_boolean(!g_variant_get_boolean(state));

  // toggle the window visibility and the action state
  gtk_widget_set_visible(window, g_variant_get_boolean(newstate));
  g_simple_action_set_state(action, newstate);
}

DUECA_NS_START

const char *const GtkDuecaView::classname = "dueca-view";

GtkDuecaView *GtkDuecaView::singleton = NULL;

GtkDuecaView::GtkDuecaView(Entity *e, const char *part,
                           const PrioritySpec &ps) :
  Module(e, classname, part),
  DuecaView(true),
#ifdef BUILD_DMODULES
  gladefile(DuecaPath::prepend("dusime-gtk4.ui")),
#else
  gladefile(DuecaPath::prepend("dueca-gtk4.ui")),
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

// data type to hold node status
struct _DNodeStatus
{
  GObject parent;
  unsigned nodeno;
  // status is pulled from the node manager with nodeno
};

G_DECLARE_FINAL_TYPE(DNodeStatus, d_node_status, D, NODE_STATUS, GObject);
G_DEFINE_TYPE(DNodeStatus, d_node_status, G_TYPE_OBJECT);
DNodeStatus *d_node_status_new(unsigned node)
{
  auto res = D_NODE_STATUS(g_object_new(d_node_status_get_type(), NULL));
  res->nodeno = node;
  return res;
}

static void d_node_status_class_init(DNodeStatusClass *klass) {}
static void d_node_status_init(DNodeStatus *self) {}

struct CoreEntityStatus
{
  std::string name;
  StatusT1 *status;
  unsigned nodeno;
  unsigned ident;
  std::list<std::shared_ptr<CoreEntityStatus>> children;
  CoreEntityStatus() :
    name(""),
    status(NULL),
    nodeno(0U),
    ident(0U)
  {}
  CoreEntityStatus(const char *name, StatusT1 *status, unsigned nodeno,
                   unsigned ident) :
    name(name),
    status(status),
    nodeno(nodeno),
    ident(ident)
  {}
  CoreEntityStatus &operator=(const CoreEntityStatus &o)
  {
    this->name = o.name;
    this->status = o.status;
    this->nodeno = o.nodeno;
    this->ident = o.ident;
    children.clear();
    for (const auto &x : o.children) {
      children.push_back(x);
    }
    return *this;
  }
};

struct _DEntityStatus
{
  GObject parent;
  CoreEntityStatus s;
};

G_DECLARE_FINAL_TYPE(DEntityStatus, d_entity_status, D, ENTITY_STATUS, GObject);
G_DEFINE_TYPE(DEntityStatus, d_entity_status, G_TYPE_OBJECT);
DEntityStatus *d_entity_status_new(const CoreEntityStatus &c)
{
  auto res = D_ENTITY_STATUS(g_object_new(d_entity_status_get_type(), NULL));
  // placement new; the object should have the memory for the CoreEntityStatus,
  // but assignment is not possible with the include list new(&(res->s))
  // CoreEntityStatus(c);
  new (&(res->s)) CoreEntityStatus(c);
  return res;
}

static void d_entity_status_class_init(DEntityStatusClass *klass) {}
static void d_entity_status_init(DEntityStatus *self) {}

static GListModel *expand_entity(gpointer _item, gpointer user_data)
{
  auto item = D_ENTITY_STATUS(_item);
  if (item->s.children.size()) {
    auto lm = g_list_store_new(d_entity_status_get_type());
    for (auto const &c : item->s.children) {
      auto child = d_entity_status_new(*c);
      g_list_store_append(lm, child);
      g_object_unref(child);
    }
    return G_LIST_MODEL(lm);
  }
  return G_LIST_MODEL(NULL);
}

// prepare for an action name and callback table
typedef void (GtkDuecaView::*cbfun)(GSimpleAction *, GVariant *, gpointer);
struct action_and_callback
{
  const char *aname;
  cbfun fcn;
};

bool GtkDuecaView::complete()
{
  // load the button widgets
  load_dueca_buttons();

  // install actions for the menu
  static action_and_callback actions[] = {
    { "about", &GtkDuecaView::cbShowAbout },
    { "openadditional", &GtkDuecaView::cbExtraModDialog },
    { "quit", &GtkDuecaView::cbShowQuit }
  };

  for (const auto &a : actions) {

    auto action = g_simple_action_new(a.aname, NULL);
    auto cb = gtk_callback(a.fcn, this);
    g_signal_connect(action, "activate", G_CALLBACK(cb->callback()), cb);

    // add the action to the application?
    g_action_map_add_action(G_ACTION_MAP(GtkHandler::application()),
                            G_ACTION(action));
  }

  /* Switch activity on. */
  update_interface.setTrigger(waker);
  update_interface.switchOn(0);

  // read the gui definition
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
    // GtkDuecaView, no DUSIME, no information
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

    // set up entities store, has expand / tree list model
    entities_list = GTK_COLUMN_VIEW(window["entity_status"]);
    entities_store = g_list_store_new(d_entity_status_get_type());
    auto emodel = gtk_tree_list_model_new(G_LIST_MODEL(entities_store), FALSE,
                                          FALSE, expand_entity, NULL, NULL);
    auto eselection = gtk_single_selection_new(G_LIST_MODEL(emodel));
    gtk_column_view_set_model(entities_list, GTK_SELECTION_MODEL(eselection));

    // set up nodes store
    nodes_list = GTK_COLUMN_VIEW(window["nodes_list"]);
    nodes_store = g_list_store_new(d_node_status_get_type());
    auto nselection = gtk_single_selection_new(G_LIST_MODEL(nodes_store));
    gtk_column_view_set_model(nodes_list, GTK_SELECTION_MODEL(nselection));
    for (unsigned n = 0; n < unsigned(NodeManager::single()->getNoOfNodes());
         n++) {
      g_list_store_append(nodes_store, d_node_status_new(n));
    }

    // rapid access to commonly used widgets
    hw_off = window["hw_off"];
    hw_safe = window["hw_safe"];
    hw_on = window["hw_on"];
    emergency = window["emergency"];

    // switch on the stop button
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hw_off), TRUE);

    // connect callbacks
    GladeCallbackTable cb_links[] = {

      // hardware/dueca buttons
      { "hw_on", "clicked", gtk_callback(&GtkDuecaView::cbOn2) },
      { "hw_safe", "clicked", gtk_callback(&GtkDuecaView::cbSafe2) },
      { "hw_off", "clicked", gtk_callback(&GtkDuecaView::cbOff2) },

      // factories for column views, setup
      { "entity_module_fact", "setup",
        gtk_callback(&GtkDuecaView::setupExpander) },
      { "entity_mstatus_fact", "setup",
        gtk_callback(&GtkDuecaView::setupLabel) },
      { "entity_sstatus_fact", "setup",
        gtk_callback(&GtkDuecaView::setupLabel) },
      { "entity_node_fact", "setup", gtk_callback(&GtkDuecaView::setupLabel) },
      { "nodes_number_fact", "setup", gtk_callback(&GtkDuecaView::setupLabel) },
      { "nodes_state_fact", "setup", gtk_callback(&GtkDuecaView::setupLabel) },

      { "entity_module_fact", "bind",
        gtk_callback(&GtkDuecaView::bindModuleName) },
      { "entity_mstatus_fact", "bind",
        gtk_callback(&GtkDuecaView::bindModuleStatus) },
      { "entity_sstatus_fact", "bind",
        gtk_callback(&GtkDuecaView::bindSimulationStatus) },
      { "entity_node_fact", "bind",
        gtk_callback(&GtkDuecaView::bindModuleNode) },
      { "nodes_number_fact", "bind",
        gtk_callback(&GtkDuecaView::bindNodeNumber) },
      { "nodes_state_fact", "bind",
        gtk_callback(&GtkDuecaView::bindNodeState) },
      { "dueca_if", "close-request", gtk_callback(&GtkDuecaView::deleteView) },
      /* These are not menu items with activities
                                      { "quit", "activate",
                             gtk_callback(&GtkDuecaView::cbWantToQuit)
                               },  { "about", "activate",
                               gtk_callback(&GtkDuecaView::cbShowAbout)
                                 }, { "read_extra_mod", "activate",
                                        gtk_callback(&GtkDuecaView::cbExtraModDialog)
               },
                                    */
      { NULL, NULL, NULL, NULL }
    };
    window.connectCallbacks(reinterpret_cast<gpointer>(this), cb_links);

    // menu actions

    // add a gesture controller for the emergency widget
    auto emergency = window["emergency"];
    auto ctrl = gtk_gesture_click_new();
    auto cbgest = gtk_callback(&GtkDuecaView::cbEmerg2, this);
    g_signal_connect(ctrl, "released", cbgest->callback(), cbgest);
    gtk_widget_add_controller(emergency, GTK_EVENT_CONTROLLER(ctrl));

    // additional windows that might be opened
    string commonglade = DuecaPath::prepend("common_if-gtk4.ui");

    {
      GladeCallbackTable cb_links[] = { { "button_really_quit", "clicked",
                                          gtk_callback(
                                            &GtkDuecaView::cbQuit2) },
                                        { NULL, NULL, NULL, NULL } };

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
    if (!gw_common["dont_stop_warning"]) {
      /* DUECA UI.

         Could not create the stop warning dialog. Check DUECA
         installation and paths.
      */
      E_CNF(" failed to create stop warning dialog");
      return false;
    }
    if (!gw_common["dont_change_a_running"]) {
      /* DUECA UI.

         Could not create the "change" warning dialog. Check DUECA
         installation and paths.
      */
      E_CNF(" failed to create change warning dialog");
      return false;
    }
    if (!gw_common["select_md2"]) {
      /* DUECA UI.

         Could not create the "additional model" dialog. Check DUECA
         installation and paths.
      */
      E_CNF(" failed to create addition model dialog");
      return false;
    }
  }

  // default images on the buttons
  gtk_dueca_button_load_image(hw_off, 0);
  gtk_dueca_button_load_image(hw_safe, 0);
  gtk_dueca_button_load_image(hw_on, 0);
  gtk_dueca_emergency_load_image(emergency, 0);

  // set the application on the window
  gtk_window_set_application(GTK_WINDOW(window["dueca_if"]),
                             GtkHandler::application());

  return res;
}

bool GtkDuecaView::isPrepared() { return true; }

GtkDuecaView::~GtkDuecaView()
{
  //
}

bool GtkDuecaView::PositionAndSize(const vector<int> &p)
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
    { "ui-file",
      new VarProbe<GtkDuecaView, vstring>(REF_MEMBER(&GtkDuecaView::gladefile)),
      "Supply the filename for the glade interface definition. If not\n"
      "supplied, it uses a default window" },
    { "shutdown-script",
      new VarProbe<GtkDuecaView, vstring>(
        REF_MEMBER(&GtkDuecaView::shutdownscript)),
      "Script called to shut down the DUECA nodes" },
    { "position-size",
      new MemberCall<GtkDuecaView, vector<int>>(&GtkDuecaView::PositionAndSize),
      "Specify the position, and optionally also the size of the interface\n"
      "window." },
    { NULL, NULL, "Creates a main window for control of DUECA/DUSIME" }
  };
  return table;
}

GAction *GtkDuecaView::requestViewEntry(const char *name, const char *label,
                                        void *object)
{
  auto viewmenu = G_MENU(window.getObject("view_menu"));

  // code the action name with the pointer of the object/view to toggle
  auto actionname = boost::str(boost::format("app.toggle_view_%s") % name);

  // create and install the action, for a checkbox, action should have boolean
  // state and no parameter
  auto initoff = g_variant_new_boolean(FALSE);
  auto action =
    g_simple_action_new_stateful(actionname.substr(4).c_str(), NULL, initoff);

  // code the window in the action's data table, and connect to our
  // toggling callback
  g_object_set_data(G_OBJECT(action), "window", object);
  g_signal_connect(action, "activate", G_CALLBACK(hide_or_show_view), action);

  // add the action to the application?
  g_action_map_add_action(G_ACTION_MAP(GtkHandler::application()),
                          G_ACTION(action));

  // create a new item in the menu
  auto newitem = g_menu_item_new(label, actionname.c_str());
  g_menu_append_item(viewmenu, newitem);

  return G_ACTION(action);
}

bool GtkDuecaView::toggleView(GAction *action)
{
  auto oldstate = g_action_get_state(action);
  g_signal_emit_by_name(G_OBJECT(action), "activate", NULL);
  auto isopened = g_variant_get_boolean(oldstate);
  g_variant_unref(oldstate);
  return bool(!isopened);
}

void GtkDuecaView::cbQuit(GtkButton *button, gpointer gp)
{
  // quit button needs confirmation
  followup = Quit;
  waker.requestAlarm(SimTime::getTimeTick() + 5 * second);

  // put yellow on the quit button
  // gtk_widget_modify_style (window["button_quit"], rc_altstyle);
  // gtk_style_context_add_class(
  //  gtk_widget_get_style_context(window["button_quit"]), "quit-yellow");

  // enable confirm button + make it green
  gtk_widget_set_sensitive(window["button_confirm"], 1);
  // gtk_widget_modify_style (window["button_confirm"], rc_cfaltstyle);
  // gtk_style_context_add_class(
  //  gtk_widget_get_style_context(window["button_confirm"]), "confirm-green");
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
  // gtk_style_context_add_class(
  //  gtk_widget_get_style_context(window["button_stop"]), "button-amber");
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
  // gtk_style_context_add_class(
  //  gtk_widget_get_style_context(window["button_safe"]), "quit-yellow");
}

void GtkDuecaView::cbRun(GtkButton *button, gpointer gp)
{
  EntityManager::single()->controlEntities(2);
  followup = Run;
  waker.requestAlarm(SimTime::getTimeTick() + 2 * second);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(window["button_safe"]), FALSE);
  gtk_widget_set_sensitive(window["button_stop"], 0);
  // gtk_widget_modify_style (window["button_run"], rc_altstyle);
  // gtk_style_context_add_class(
  //  gtk_widget_get_style_context(window["button_run"]), rc_altstyle);
}

void GtkDuecaView::cbShutDown(GtkButton *button, gpointer gp)
{
  // quit button needs confirmation
  followup = Shutdown;
  waker.requestAlarm(SimTime::getTimeTick() + 5 * second);

  // put yellow on the shutdown button
  // gtk_widget_modify_style (window["button_shutdown"], rc_altstyle);
  // gtk_style_context_add_class(
  //  gtk_widget_get_style_context(window["button_shutdown"]), rc_altstyle);

  // enable confirm button, make it green
  gtk_widget_set_sensitive(window["button_confirm"], 1);
  // gtk_widget_modify_style (window["button_confirm"], rc_cfaltstyle);
  // gtk_style_context_add_class(
  //  gtk_widget_get_style_context(window["button_confirm"]), rc_cfaltstyle);
}

void GtkDuecaView::cbConfirm(GtkButton *button, gpointer gp)
{
  switch (followup) {
  case Quit:
    NodeManager::single()->breakUp();
    // gtk_widget_modify_style (window["button_quit"], rc_style);
    // clean_style(window["button_quit"]);
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
    // clean_style(window["button_shutdown"]);
    break;
  default:
    break;
  }
  followup = None;

  // gtk_widget_modify_style (window["button_confirm"], rc_style);
  // clean_style(window["button_confirm"]);
  gtk_widget_set_sensitive(window["button_confirm"], 0);
}

void GtkDuecaView::updateInterface(const TimeSpec &time)
{
  switch (followup) {
  case Quit:
  case Shutdown:
    gtk_widget_set_sensitive(window["button_confirm"], 0);
    followup = None;
    break;

  case Stop:
    if (EntityManager::single()->getConfirmedState() ==
        ModuleState::InitialPrep) {
      // clean_style(window["button_stop"]);
    }
    else {
      waker.requestAlarm(SimTime::getTimeTick() + second);
    }
    break;

  case Safe:
    if (EntityManager::single()->getConfirmedState() == ModuleState::Prepared &&
        NodeManager::single()->isDuecaSynced()) {
      // clean_style(window["button_safe"]);
      gtk_widget_set_sensitive(window["button_run"], 1);
    }
    else {
      waker.requestAlarm(SimTime::getTimeTick() + second);
    }
    break;

  case Run:
    if (EntityManager::single()->getConfirmedState() == ModuleState::On) {
      // clean_style(window["button_run"]);
    }
    else {
      waker.requestAlarm(SimTime::getTimeTick() + second);
    }
    break;

  default:
    break;
  }
}

void GtkDuecaView::cbShowAbout(GSimpleAction *action, GVariant *arg,
                               gpointer user_data)
{
  gtk_widget_set_visible(gw_common["about2"], TRUE);
}

void GtkDuecaView::cbCloseAbout(GSimpleAction *action, GVariant *arg,
                                gpointer user_data)
{
  gtk_widget_set_visible(gw_common["about2"], TRUE);
}

void GtkDuecaView::cbShowQuit(GSimpleAction *action, GVariant *arg,
                              gpointer user_data)
{
  if (EntityManager::single()->stopIsOK()) {
    gtk_widget_set_visible(gw_common["really_quit"], TRUE);
  }
  else {
    gtk_widget_set_visible(gw_common["dont_stop_warning"], TRUE);
  }
}

static void process_file_result(GObject *dialog, GAsyncResult *res,
                                gpointer view)
{
  auto file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(dialog), res, NULL);
  if (file) {
    Environment::getInstance()->readMod(g_file_peek_path(file));
    g_object_unref(file);
  }
}

void GtkDuecaView::cbExtraModDialog(GSimpleAction *action, GVariant *arg,
                                    gpointer user_data)
{
  if (DUECA_NS::EntityManager::single()->stopIsOK()) {
    gtk_file_dialog_open(GTK_FILE_DIALOG(window["additional_def"]),
                         GTK_WINDOW(window["dueca_if"]), NULL,
                         process_file_result, this);
  }
  else {
    gtk_widget_set_visible(gw_common["dont_change_a_running"], TRUE);
  }
}

void GtkDuecaView::cbWantToQuit(GtkWidget *button, gpointer gp)
{
  if (EntityManager::single()->stopIsOK()) {
    gtk_widget_set_visible(gw_common["really_quit"], TRUE);
  }
  else {
    gtk_widget_set_visible(gw_common["dont_stop_warning"], TRUE);
  }
}

gboolean GtkDuecaView::deleteView(GtkWindow *window, gpointer user_data)
{
  cbWantToQuit(NULL, NULL);
  return TRUE;
}

void GtkDuecaView::cbQuit2(GtkWidget *button, gpointer gp)
{
  NodeManager::single()->breakUp();
  gtk_widget_set_visible(gw_common["really_quit"], FALSE);
}

void GtkDuecaView::updateEntityButtons(const ModuleState &confirmed_state,
                                       const ModuleState &command_state,
                                       bool emergency_flag)
{
  if (!simple_io) {
    switch (confirmed_state.get()) {
    case ModuleState::InitialPrep:
      if (command_state == confirmed_state) {
        gtk_dueca_button_load_image(hw_off, 2);
      }
      gtk_widget_set_sensitive(hw_off, TRUE);
      gtk_widget_set_sensitive(hw_safe, TRUE);
      gtk_widget_set_sensitive(hw_on, FALSE);
      break;
    case ModuleState::Safe:
      gtk_dueca_button_load_image(hw_safe, 2);
      gtk_widget_set_sensitive(hw_off, TRUE);
      if (!emergency_flag) {
        gtk_widget_set_sensitive(hw_safe, TRUE);
      }
      gtk_widget_set_sensitive(hw_on, FALSE);
      break;
    case ModuleState::Prepared:
      gtk_dueca_button_load_image(hw_safe, 2);
      gtk_widget_set_sensitive(hw_off, TRUE);
      if (!emergency_flag) {
        gtk_widget_set_sensitive(hw_safe, TRUE);
        gtk_widget_set_sensitive(hw_on, TRUE);
      }
      break;
    case ModuleState::On:
      if (command_state == confirmed_state) {
        gtk_dueca_button_load_image(hw_on, 2);
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

void GtkDuecaView::refreshEntitiesView()
{
  if (!entities_store)
    return;

  // gtk_widget_queue_draw(GTK_WIDGET());
}

void *GtkDuecaView::insertEntityNode(const char *name, void *vparent,
                                     int dueca_node, StatusT1 *obj)
{
  static unsigned ident = 0;
  if (!entities_store)
    return NULL;

  if (vparent) {
    unsigned idx = 0;
    auto item =
      D_ENTITY_STATUS(g_list_model_get_item(G_LIST_MODEL(entities_store), idx));
    if (reinterpret_cast<void *>(item->s.ident) == vparent) {
      item->s.children.emplace_back(
        new CoreEntityStatus(name, obj, dueca_node, ++ident));
    }
    else if (!item->s.children.empty()) {
      for (auto &c : item->s.children) {
        if (reinterpret_cast<void *>(c->ident) == vparent) {
          c->children.emplace_back(
            new CoreEntityStatus(name, obj, dueca_node, ++ident));
        }
      }
    }
  }
  else {
    g_list_store_append(entities_store, d_entity_status_new(CoreEntityStatus(
                                          name, obj, dueca_node, ++ident)));
  }

  return reinterpret_cast<void *>(ident);
}

void GtkDuecaView::refreshNodesView()
{
  if (!nodes_store)
    return;
  // gtk_widget_queue_draw(GTK_WIDGET(nodes_list));
}

void GtkDuecaView::cbOn2(GtkWidget *widget, gpointer user_data)
{
  if (EntityManager::single()->controlEntities(2)) {
    // allowed to switch on, loading image, and reset the save image
    gtk_dueca_button_load_image(widget, 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hw_safe), FALSE);
    gtk_widget_set_sensitive(hw_off, FALSE);
    gtk_dueca_button_load_image(hw_safe, 0);
  }
  else {
    // transition not possible, reset to unpressed
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
  }
}

void GtkDuecaView::cbEmerg2(GtkGestureClick *click, gint n_press, gdouble x,
                            gdouble y, gpointer user_data)
{
  if (safe_armed) {
    // pressed for the second time. Exciting. Does the event indicate
    // that the lower portion was pressed?
    if (y > 42) {

      EntityManager::single()->emergency();
      NodeManager::single()->emergency();
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(emergency), TRUE);
      gtk_dueca_button_load_image(hw_on, 0);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hw_on), FALSE);
      gtk_dueca_button_load_image(hw_safe, 1);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hw_on), TRUE);

      // set all buttons to insensitive, everything should be off,
      gtk_widget_set_sensitive(hw_safe, FALSE);
      gtk_widget_set_sensitive(hw_on, FALSE);
      gtk_widget_set_sensitive(hw_off, FALSE);
    }
    else {
      safe_armed = false;
      gtk_dueca_button_load_image(emergency, 0);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(emergency), FALSE);
    }
  }
  else {
    safe_armed = true;
    // set the yes/return face
    gtk_dueca_button_load_image(emergency, 1);
    // but leave the toggle button out
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(emergency), FALSE);
  }
}

void GtkDuecaView::cbOff2(GtkWidget *widget, gpointer user_data)
{
  if (EntityManager::single()->controlEntities(0)) {
    gtk_dueca_button_load_image(widget, 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hw_safe), FALSE);
    gtk_widget_set_sensitive(hw_on, FALSE);
    gtk_dueca_button_load_image(hw_safe, 0);
  }
  else {
    // transition not possible, reset to unpressed
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
  }
}

void GtkDuecaView::cbSafe2(GtkWidget *widget, gpointer user_data)
{
  if (EntityManager::single()->controlEntities(1)) {
    gtk_dueca_button_load_image(widget, 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hw_on), FALSE);
    gtk_dueca_button_load_image(hw_on, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hw_off), FALSE);
    gtk_dueca_button_load_image(hw_off, 0);
  }
  else {
    // transition not possible, reset to unpressed
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
  }
}

void GtkDuecaView::setupExpander(GtkSignalListItemFactory *fact,
                                 GtkListItem *item, gpointer user_data)
{
  auto label = gtk_label_new("");
  auto expander = gtk_tree_expander_new();
  gtk_tree_expander_set_child(GTK_TREE_EXPANDER(expander), label);
  gtk_list_item_set_child(item, expander);
}

void GtkDuecaView::setupLabel(GtkSignalListItemFactory *fact, GtkListItem *item,
                              gpointer user_data)
{
  auto label = gtk_label_new("");
  gtk_list_item_set_child(item, label);
}

void GtkDuecaView::bindModuleName(GtkSignalListItemFactory *fact,
                                  GtkListItem *item, gpointer user_data)
{
  auto expander = gtk_list_item_get_child(item);
  auto row = gtk_list_item_get_item(item);
  auto obj =
    D_ENTITY_STATUS(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  auto label = gtk_tree_expander_get_child(GTK_TREE_EXPANDER(expander));
  if (obj->s.children.size()) {
    gtk_tree_expander_set_list_row(GTK_TREE_EXPANDER(expander),
                                   GTK_TREE_LIST_ROW(row));
  }
  else {
    gtk_tree_expander_set_list_row(GTK_TREE_EXPANDER(expander), NULL);
  }
  gtk_label_set_label(GTK_LABEL(label), obj->s.name.c_str());
}

void GtkDuecaView::bindModuleStatus(GtkSignalListItemFactory *fact,
                                    GtkListItem *item, gpointer user_data)
{
  auto label = gtk_list_item_get_child(item);
  auto row = gtk_list_item_get_item(item);
  auto obj =
    D_ENTITY_STATUS(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  gtk_label_set_label(GTK_LABEL(label),
                      obj->s.status->getModuleState().getString());
}

void GtkDuecaView::bindSimulationStatus(GtkSignalListItemFactory *fact,
                                        GtkListItem *item, gpointer user_data)
{
  auto label = gtk_list_item_get_child(item);
  auto row = gtk_list_item_get_item(item);
  auto obj =
    D_ENTITY_STATUS(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  gtk_label_set_label(GTK_LABEL(label),
                      obj->s.status->getSimulationState().getString());
}

void GtkDuecaView::bindModuleNode(GtkSignalListItemFactory *fact,
                                  GtkListItem *item, gpointer user_data)
{
  auto label = gtk_list_item_get_child(item);
  auto row = gtk_list_item_get_item(item);
  auto obj =
    D_ENTITY_STATUS(gtk_tree_list_row_get_item(GTK_TREE_LIST_ROW(row)));
  gtk_label_set_label(GTK_LABEL(label),
                      boost::str(boost::format("%d") % obj->s.nodeno).c_str());
}

void GtkDuecaView::bindNodeNumber(GtkSignalListItemFactory *fact,
                                  GtkListItem *item, gpointer user_data)
{
  auto label = gtk_list_item_get_child(item);
  auto obj = D_NODE_STATUS(gtk_list_item_get_item(item));
  gtk_label_set_label(GTK_LABEL(label),
                      boost::str(boost::format("%d") % obj->nodeno).c_str());
}

void GtkDuecaView::bindNodeState(GtkSignalListItemFactory *fact,
                                 GtkListItem *item, gpointer user_data)
{
  auto label = gtk_list_item_get_child(item);
  auto obj = D_NODE_STATUS(gtk_list_item_get_item(item));
  gtk_label_set_label(
    GTK_LABEL(label),
    getString(NodeManager::single()->getNodeState(obj->nodeno)));
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
