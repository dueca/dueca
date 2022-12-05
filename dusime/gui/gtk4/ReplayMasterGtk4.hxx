/* ------------------------------------------------------------------   */
/*      item            : ReplayMasterGtk3.hxx
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

#ifndef ReplayMasterGtk3_hxx
#define ReplayMasterGtk3_hxx

// enabling/disabling selection of replays depending on selected inco
// https://stackoverflow.com/questions/60065110/how-to-make-an-item-of-a-gtkcombobox-disabled-unclickable
// https://stackoverflow.com/questions/45900849/how-to-populate-a-gtkmenu-with-some-gtkmenuitem-and-set-its-parent-in-glade

// include the dusime header
#include <dueca/dueca_ns.h>
#include <dusime/ReplayMaster.hxx>
#include <dusime/SnapshotInventory.hxx>
#include <dueca/gui/gtk3/GtkGladeWindow.hxx>

DUECA_NS_START;

/** Interface for replay and snapshot information

 */
class ReplayMasterGtk3: public Module
{
  /** self-define the module type, to ease writing the parameter table */
  typedef ReplayMasterGtk3 _ThisModule_;

private:

  /** Inventory with available inco's */
  SnapshotInventory::pointer         inco_inventory;

  /** And the local replaymaster with information on the recordings */
  ReplayMaster::pointer              replays;

  /** glade file */
  std::string                        gladefile;

  /** gtk window */
  GtkGladeWindow                     window;

  /** Tree store for the object with information on replay stretches
      and inco and widgets for control. */
  GtkListStore                      *replay_store;

  /** iterator for the initial store */
  GtkTreeIter                        replay_set_iter;

  /** Widget in the main DUECA menu */
  GtkWidget                         *menuitem;

  /** File for existing replay actions */
  std::string                        reference_file;

  /** File suffix for recording actions */
  std::string                        store_file;

  /** remember whether files have been initialized */
  bool                               files_initialized;
  
  /** Enumeration values for the store, these are  */
  enum StoreFieldsReplay {
    S_rec_id,            /**< Giving each a number */
    S_rec_name,          /**< Recording name */
    S_rec_date,          /**< Recording date */
    S_rec_span,          /**< Available span */
    S_rec_inco_name,     /**< To show, initial cond name */
    S_rec_numcol         /**< Number of columns */
  };

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const           classname;

  /** Return the parameter table. */
  static const ParameterTable*       getParameterTable();

public:
  /** Constructor */
  ReplayMasterGtk3(Entity* e, const char* part,
                   const PrioritySpec& ps);

  /** Continued construction. */
  bool complete();

  /** Destructor */
  ~ReplayMasterGtk3();

  /** Start signal */
  void startModule(const TimeSpec &time);

  /** Start signal */
  void stopModule(const TimeSpec &time);

  /** Indicate ready for run */
  bool isPrepared();

private:
  /** close callback */
  void cbClose(GtkButton* button, gpointer gp);

  /** Send an initial configuration, if enabled */
  void cbSendInitial(GtkButton* button, gpointer gp);

  /** Prepare a replay stretch */
  void cbSendReplay(GtkButton* button, gpointer gp);

  /** Change the selection of current recording */
  void cbChangeRecordingSelected(GtkComboBox* box, gpointer gp);

  /** Select the action for after a replay */
  void cbSelectHoldAfter(GtkWidget* widgets, gpointer gp);

  /** Select the action for after a replay */
  void cbSelectAdvanceAfter(GtkWidget* widgets, gpointer gp);

  /** Closing via the window manager */
  gboolean cbDelete(GtkWidget *window, GdkEvent *event, gpointer user_data);

  /** Change the selection of initial conditions */
  void cbSelectReplay(GtkTreeSelection *sel, gpointer gp);

  /** Prepare the recording */
  void cbRecordPrepare(GtkButton* button, gpointer gp);

  /** Update recording name */
  void cbRecordName(GtkWidget* text, gpointer gp);
};

DUECA_NS_END;

#endif
