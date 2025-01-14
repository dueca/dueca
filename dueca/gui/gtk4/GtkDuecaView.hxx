/* ------------------------------------------------------------------   */
/*      item            : GtkDuecaView.hxx
        made by         : Rene' van Paassen
        date            : 000721
        category        : header file
        description     :
        changes         : 000721 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef GtkDuecaView_hh
#define GtkDuecaView_hh

#include <memory>
#ifdef GtkDuecaView_cc
#endif

#include "DuecaView.hxx"
#include "Module.hxx"
#include "Activity.hxx"
#include "Callback.hxx"
#include "GtkGladeWindow.hxx"
#include "AperiodicAlarm.hxx"
#include <string>
#include <vector>
#include <dueca_ns.h>
#include <gtk/gtk.h>

DUECA_NS_START
struct ParameterTable;

// entity status, DUECA side
struct CoreEntityStatus
{
  // name of the entity/module
  std::string name;

  // status summary
  dueca::StatusT1 *status;

  // node number
  unsigned nodeno;

  // unique identification counter
  unsigned ident;

  // optionally, children
  std::list<std::shared_ptr<CoreEntityStatus>> children;

  // link to the status
  GObject *gstatus;

  // complete constructor
  CoreEntityStatus(const char *name, dueca::StatusT1 *status, unsigned nodeno,
                   unsigned ident);
};

/** Handles -- at least part of -- the communication with the
    experiment leader via the user interface. */
class GtkDuecaView : public Module, public DuecaView
{
  /** Singleton pointer. */
  static GtkDuecaView *singleton;

  /** File with the present interface. */
  std::string gladefile;

  /** Script to run if total shutdown requested. */
  std::string shutdownscript;

  /** Actions that need to be remembered for interface followup. */
  enum FollowUp {
    None,    /**< No action for confirmation currently. */
    Quit,    /**< Quit DUECA. */
    Shutdown,/**< Quit DUECA and shutdown systems. */
    Stop,    /**< Switch to idle mode. */
    Safe,    /**< Switch to safe mode. */
    Run      /**< Switch to run mode. */
  };

  /** Flag to remember confirmations. */
  FollowUp followup;

  /** Ticks in a second. */
  int second;

  /** Variant of the interface. */
  bool simple_io;

  /** map to quickly find status objects */
  std::map<unsigned,std::shared_ptr<CoreEntityStatus>> status_map;

  /** Widgets for entity level control. */
  GtkWidget *hw_off, *hw_safe, *hw_on, *emergency;

  /** List view of entities */
  GtkColumnView *entities_list;

  /** Model for the entities list widget. */
  GListStore *entities_store;

  /** List view of nodes */
  GtkColumnView *nodes_list;

  /** Model for the nodes list */
  GListStore *nodes_store;

  /** remember arming. */
  bool safe_armed;

  /** Window based on glade file. */
  GtkGladeWindow window;

  /** About information. */
  GtkGladeWindow gw_common;

  /** Callback object, for interface feedback. */
  Callback<GtkDuecaView> cb;

  /** Activity, update interface. */
  ActivityCallback update_interface;

  /** Aperiodic triggering. */
  AperiodicAlarm waker;

  /** Not a root class. */
  bool isRootClass();

public:
  /// Name of the module class.
  static const char *const classname;

  /** Table with parameters. */
  static const ParameterTable *getParameterTable();

public:
  /** Constructor. Follows normal module construction conventions. */
  GtkDuecaView(Entity *e, const char *part, const PrioritySpec &ps);

  /** Access the singleton pointer. */
  inline static GtkDuecaView *single() { return singleton; }

  /** Is called after the constructor and after insertion of parameter
      values; completes construction. */
  bool complete() final;

  /** Destructor. */
  ~GtkDuecaView();

  /** Specification of window size. */
  bool PositionAndSize(const vector<int> &p);

  /** Start the GtkDuecaView module. Is not really used, GtkDuecaView is
      intrinsically started. */
  void startModule(const TimeSpec &time) final;

  /** Stop the GtkDuecaView module. As for startModule, not really used. */
  void stopModule(const TimeSpec &time) final;

  /** Will always be prepared. */
  bool isPrepared() final;

  /** Update interface. */
  void updateInterface(const TimeSpec &time);

public:
  /** This call allows accessories in DUECA to get entries in the view
      menu. */
  GAction *requestViewEntry(const char *name, const char* label, void *object);

  /** Helper to change a view programatically */
  static bool toggleView(GAction *action);

  /** Access the main window. */
  inline GtkGladeWindow &accessMainView() { return window; }

public:
  /** Callback functions for interface actions. */

  /** Quit DUECA, this will need a confirmation! */
  void cbQuit(GtkButton *button, gpointer gp);

  /** Stop running. */
  void cbStop(GtkButton *button, gpointer gp);

  /** Go to safe mode. */
  void cbSafe(GtkButton *button, gpointer gp);

  /** Go to run mode. */
  void cbRun(GtkButton *button, gpointer gp);

  /** Stop Dueca and shutdown systems, this will need confirmation! */
  void cbShutDown(GtkButton *button, gpointer gp);

  /** Confirm drastic actions such as stop or quit. */
  void cbConfirm(GtkButton *button, gpointer gp);

  /** Callback functions for interface, alternative, more complex
      interface. */

  /** Bring up "about" dialog" */
  void cbShowAbout(GSimpleAction *menuaction, GVariant *parameter,
                   gpointer user_data);

  /** close the about dialog */
  void cbCloseAbout(GSimpleAction *menuaction, GVariant *parameter,
                    gpointer user_data);

  /** Bring up quit dialog. */
  void cbShowQuit(GSimpleAction *menuitem, GVariant *parameter,
                  gpointer user_data);

  /** File chooser for extra files. */
  void cbExtraModDialog(GSimpleAction *menuitem, GVariant *parameter,
                        gpointer user_data);

  /** callback, close the view on deletion by window manager. */
  gboolean deleteView(GtkWindow *window, gpointer user_data);

  /** Wanting to quit DUECA. */
  void cbWantToQuit(GtkWidget *widget, gpointer user_data);

  /** Quit dueca. */
  void cbQuit2(GtkWidget *widget, gpointer user_data);

  /** Switch entities off. */
  void cbOff2(GtkWidget *widget, gpointer user_data);

  /** Switch entities to safe running. */
  void cbSafe2(GtkWidget *widget, gpointer user_data);

  /** Switch entities on. */
  void cbOn2(GtkWidget *widget, gpointer user_data);

  /** Emergency stop. */
  void cbEmerg2(GtkGestureClick *click, gint n_press, gdouble x, gdouble y,
                gpointer user_data);

  /** column view setup call */
  void setupExpander(GtkSignalListItemFactory *fact, GtkListItem *item,
                     gpointer user_data);

  /** column view setup call */
  void setupLabel(GtkSignalListItemFactory *fact, GtkListItem *item,
                  gpointer user_data);

  /** column view bind call */
  void bindModuleName(GtkSignalListItemFactory *fact, GtkListItem *item,
                      gpointer user_data);

  /** column view bind call */
  void bindModuleStatus(GtkSignalListItemFactory *fact, GtkListItem *item,
                        gpointer user_data);

  /** column view bind call */
  void bindSimulationStatus(GtkSignalListItemFactory *fact, GtkListItem *item,
                            gpointer user_data);

  /** column view bind call */
  void bindModuleNode(GtkSignalListItemFactory *fact, GtkListItem *item,
                      gpointer user_data);

  /** column view bind call */
  void bindNodeNumber(GtkSignalListItemFactory *fact, GtkListItem *item,
                      gpointer user_data);

  /** column view bind call */
  void bindNodeState(GtkSignalListItemFactory *fact, GtkListItem *item,
                     gpointer user_data);

  /** Callback when nodes list visible */
  void cbNodesListVisible(GtkWidget *cv, gpointer user_data);

  /** Auxiliary, clean styles from buttons */
  void clean_style(GtkWidget *w);

  /** update buttons entity control */
  void updateEntityButtons(const ModuleState &confirmed_state,
                           const ModuleState &command_state,
                           bool emergency_flag) final;

  /** Insert a new entity node.
      \param  name      name for the node
      \param  parent    pointer to the node's parent, to be re-cast to
                        the toolkit's objects that represent a parent
      \param  obj       pointer to the object on the Dueca side.
      \returns          A pointer to the node on the toolkit side. */
  virtual void *insertEntityNode(const char *name, void *parent, int dueca_node,
                                 StatusT1 *obj) override;



  /** Refresh the entity list view. */
  void refreshEntitiesView() override;

  /** Refresh the nodes list view. */
  void refreshNodesView() override;

  /** Control the switch-off buttons */
  void requestToKeepRunning(bool keep_running) override;

  /** Reflect the change in a single node */
  void syncNode(void *nid) override;
};

DUECA_NS_END
#endif
