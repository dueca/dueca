/* ------------------------------------------------------------------   */
/*      item            : ReplayMasterGtk4.hxx
        made by         : Rene van Paassen
        date            : 220418
        category        : header file
        description     :
        changes         : 220418 first version
        language        : C++
        copyright       : (c) 22 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ReplayMasterGtk4_hxx
#define ReplayMasterGtk4_hxx

// enabling/disabling selection of replays depending on selected inco
// https://stackoverflow.com/questions/60065110/how-to-make-an-item-of-a-gtkcombobox-disabled-unclickable
// https://stackoverflow.com/questions/45900849/how-to-populate-a-gtkmenu-with-some-gtkmenuitem-and-set-its-parent-in-glade

// include the dusime header
#include <dueca/dueca_ns.h>
#include <dusime/ReplayMaster.hxx>
#include <dusime/SnapshotInventory.hxx>
#include <dueca/gui/gtk4/GtkGladeWindow.hxx>
#include <gtk/gtk.h>

DUECA_NS_START;

/** Interface for replay and snapshot information

 */
class ReplayMasterGtk4 : public Module
{
  /** self-define the module type, to ease writing the parameter table */
  typedef ReplayMasterGtk4 _ThisModule_;

private:
  /** Inventory with available inco's */
  SnapshotInventory::pointer inco_inventory;

  /** And the local replaymaster with information on the recordings */
  ReplayMaster::pointer replays;

  /** glade file */
  std::string gladefile;

  /** gtk window */
  GtkGladeWindow window;

  /** Tree store for the object with information on replay stretches
      and inco and widgets for control. */
  GListStore *replay_store;

  /** Widget in the main DUECA menu */
  GAction *menuaction;

  /** File for existing replay actions */
  std::string reference_file;

  /** File suffix for recording actions */
  std::string store_file;

  /** remember whether files have been initialized */
  bool files_initialized;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char *const classname;

  /** Return the parameter table. */
  static const ParameterTable *getParameterTable();

public:
  /** Constructor */
  ReplayMasterGtk4(Entity *e, const char *part, const PrioritySpec &ps);

  /** Continued construction. */
  bool complete();

  /** Destructor */
  ~ReplayMasterGtk4();

  /** Window position */
  bool setPositionAndSize(const std::vector<int> &p);

  /** Start signal */
  void startModule(const TimeSpec &time);

  /** Start signal */
  void stopModule(const TimeSpec &time);

  /** Indicate ready for run */
  bool isPrepared();

private:
  /** close callback */
  void cbClose(GtkButton *button, gpointer gp);

  /** Send an initial configuration, if enabled */
  void cbSendInitial(GtkButton *button, gpointer gp);

  /** Prepare a replay stretch */
  void cbSendReplay(GtkButton *button, gpointer gp);

  /** Select the action for after a replay */
  void cbSelectTodoAfter(GObject *widget, GParamSpec *pspec, gpointer gp);

  /** Closing via the window manager */
  gboolean cbDelete(GtkWidget *window, gpointer user_data);

  /** Change the selection of initial conditions */
  void cbSelectReplay(GtkSelectionModel *sel, guint selected, guint nsel,
                      gpointer gp);

  /** Prepare the recording */
  void cbRecordPrepare(GtkButton *button, gpointer gp);

  /** Update recording name */
  void cbRecordName(GtkWidget *text, gpointer gp);

  /** set up label field */
  void cbSetupLabel(GtkSignalListItemFactory *fact, GtkListItem *object,
                    gpointer user_data);

  /** bind replay table field */
  void cbBindReplayName(GtkSignalListItemFactory *fact, GtkListItem *object,
                        gpointer user_data);

  /** bind replay table field */
  void cbBindReplayDate(GtkSignalListItemFactory *fact, GtkListItem *object,
                        gpointer user_data);

  /** bind replay table field */
  void cbBindReplayDuration(GtkSignalListItemFactory *fact, GtkListItem *object,
                            gpointer user_data);

  /** bind replay table field */
  void cbBindReplayInitial(GtkSignalListItemFactory *fact, GtkListItem *object,
                           gpointer user_data);
};

DUECA_NS_END;

#endif
