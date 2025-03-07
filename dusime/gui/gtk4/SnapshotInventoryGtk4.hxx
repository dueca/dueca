/* ------------------------------------------------------------------   */
/*      item            : SnapshotInventoryGtk4.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Mon May  2 17:16:41 2022
        category        : header file
        description     :
        changes         : Mon May  2 17:16:41 2022 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef SnapshotInventoryGtk4_hxx
#define SnapshotInventoryGtk4_hxx

// include the dusime header
#include <dueca.h>
#include <dusime/SnapshotInventory.hxx>
#include <dueca/gui/gtk4/GtkGladeWindow.hxx>

DUECA_NS_START;

/** Gui for handling the snapshots of a single entity. This uses the
    helper object SnapshotInventory to read and manage a collection of
    snapshots.

    The instructions to create an module of this class from the Scheme
    script are:

    \verbinclude initials-inventory.scm
 */
class SnapshotInventoryGtk4 : public Module
{
  /** self-define the module type, to ease writing the parameter table */
  typedef SnapshotInventoryGtk4 _ThisModule_;

private:
  /** Underlying inventory object */
  SnapshotInventory::pointer inventory;

  /** glade file */
  std::string gladefile;

  /** gtk window */
  GtkGladeWindow window;

  /** Gtk collection of the snapshot information. */
  GListStore *snaps_store;

  /** Widget in the main DUECA menu */
  GAction *menuaction;

  /** File with existing initials */
  std::string reference_file;

  /** File for any new additions/extensions */
  std::string store_file;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char *const classname;

  /** Return the parameter table. */
  static const ParameterTable *getParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script.

      The owning entity of an inventory will be "dueca", the part name
      indicates the managed entity of the inventory.
  */
  SnapshotInventoryGtk4(Entity *e, const char *part, const PrioritySpec &ts);

  /** Continued construction. */
  bool complete();

  /** Destructor. */
  ~SnapshotInventoryGtk4();

public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared();

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time);

public: // the member functions that are called for activities
  /** the method that implements the main calculation. */
  void doCalculation(const TimeSpec &ts);

private:
  /** Close the associated window */
  void cbClose(GtkWidget *btn, gpointer gp);

  /** Set a name for an upcoming snapshot */
  void cbSetName(GtkWidget *text, gpointer gp);

  /** Send a selected initial state */
  void cbSendInitial(GtkWidget *btn, gpointer gp);

  /** Select a different initial state */
  void cbSelection(GtkSelectionModel *sel, guint position, guint n_items, gpointer gp);

  /** Closing via the window manager */
  gboolean cbDelete(GtkWidget *window, gpointer user_data);

  /** Window position */
  bool setPositionAndSize(const std::vector<int> &p);

  /** set up label field */
  void cbSetupLabel(GtkSignalListItemFactory *fact, GtkListItem *object,
                    gpointer user_data);

  /** set up expander field */
  void cbSetupExpander(GtkSignalListItemFactory *fact, GtkListItem *object,
                       gpointer user_data);

  /** bind replay table field */
  void cbBindName(GtkSignalListItemFactory *fact, GtkListItem *object,
                  gpointer user_data);

  /** bind replay table field */
  void cbBindDateTime(GtkSignalListItemFactory *fact, GtkListItem *object,
                      gpointer user_data);

  /** bind replay table field */
  void cbBindOrigin(GtkSignalListItemFactory *fact, GtkListItem *object,
                    gpointer user_data);

  /** bind replay table field */
  void cbBindCoding(GtkSignalListItemFactory *fact, GtkListItem *object,
                    gpointer user_data);

  /** bind replay table field */
  void cbBindSample(GtkSignalListItemFactory *fact, GtkListItem *object,
                    gpointer user_data);
};

DUECA_NS_END;

#endif
