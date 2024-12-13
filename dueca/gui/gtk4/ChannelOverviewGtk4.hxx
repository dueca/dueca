/* ------------------------------------------------------------------   */
/*      item            : ChannelOverviewGtk4.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Mon May  7 15:45:06 2018
        category        : header file
        description     :
        changes         : Mon May  7 15:45:06 2018 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ChannelOverviewGtk4_hxx
#define ChannelOverviewGtk4_hxx

// include the dusime header
#include <dueca.h>

// include headers for functions/classes you need in the module
#include <ChannelOverview.hxx>
#include "GtkGladeWindow.hxx"
#include "gtk/gtk.h"

DUECA_NS_START

// GObject derived struct to pass data between interface and application
struct _DChannelInfo;
struct ChannelOverviewGtk4Private;

/** A view on the DUECA channels

    The instructions to create an module of this class from the Scheme
    script are:

    \verbinclude channel-view.scm
 */
class ChannelOverviewGtk4 : public ChannelOverview
{
  /** self-define the module type, to ease writing the parameter table */
  typedef ChannelOverviewGtk4 _ThisModule_;

private: // simulation data
  //ChannelOverviewGtk4Private *self;

  /** glade file */
  std::string gladefile;

  /** glade file for monitor windows */
  std::string monitor_gladefile;

  /** gtk window */
  GtkGladeWindow window;

  /** Widget for this module/window in the main menu */
  GAction *menuaction;

  /** Window widget */
  GtkWidget *channel_window;

  /** Tree widget */
  GtkColumnView *channel_tree;

  /** Store with the channel data */
  GListStore *store;
  
public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char *const classname;

  /** Return the parameter table. */
  static const ParameterTable *getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  ChannelOverviewGtk4(Entity *e, const char *part, const PrioritySpec &ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~ChannelOverviewGtk4();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec &ts);

  /** Request check on the timing. */
  bool checkTiming(const vector<int> &i);

protected:
  /** update view */
  void reflectChanges(unsigned channelid);
  /** update view */
  void reflectChanges(unsigned channelid, unsigned entryid);
  /** update view */
  void reflectChanges(unsigned channelid, unsigned entryid, unsigned readerid);
  /** update counts */
  void reflectCounts();
  /** redraw view */
  void showChanges();

private:
  /** close callback */
  void cbClose(GtkButton *button, gpointer gp);
  /** refresh read/write count */
  void cbRefreshCounts(GtkButton *button, gpointer gp);
  /** window delete selected */
  gboolean cbHide(GtkWidget *window, gpointer user_data);

  /** setup for a label */
  void cbSetupLabel(GtkSignalListItemFactory *fact, GtkListItem *item,
                    gpointer user_data);

  /** expander with label */
  void cbSetupExpander(GtkSignalListItemFactory *fact, GtkListItem *item,
                       gpointer user_data);

  /** image */
  void cbSetupImage(GtkSignalListItemFactory *fact, GtkListItem *item,
                    gpointer user_data);

  /** checkbox */
  void cbSetupCheckbox(GtkSignalListItemFactory *fact, GtkListItem *item,
                       gpointer user_data);

  /** bind to a column */
  void cbBindChannelNum(GtkSignalListItemFactory *fact, GtkListItem *item,
                        gpointer user_data);

  /** bind to a column */
  void cbBindChannelName(GtkSignalListItemFactory *fact, GtkListItem *item,
                         gpointer user_data);

  /** bind to a column */
  void cbBindEntryNum(GtkSignalListItemFactory *fact, GtkListItem *item,
                      gpointer user_data);

  /** bind to a column */
  void cbBindWriterId(GtkSignalListItemFactory *fact, GtkListItem *item,
                      gpointer user_data);

  /** bind to a column */
  void cbBindES(GtkSignalListItemFactory *fact, GtkListItem *item,
                gpointer user_data);

  /** bind to a column */
  void cbBindNWrites(GtkSignalListItemFactory *fact, GtkListItem *item,
                     gpointer user_data);

  /** bind to a column */
  void cbBindReaderId(GtkSignalListItemFactory *fact, GtkListItem *item,
                      gpointer user_data);

  /** bind to a column */
  void cbBindNReads(GtkSignalListItemFactory *fact, GtkListItem *item,
                    gpointer user_data);

  /** bind to a column */
  void cbBindSel(GtkSignalListItemFactory *fact, GtkListItem *item,
                 gpointer user_data);

  /** bind to a column */
  void cbBindSeq(GtkSignalListItemFactory *fact, GtkListItem *item,
                 gpointer user_data);

  /** bind to a column */
  void cbBindView(GtkSignalListItemFactory *fact, GtkListItem *item,
                  gpointer user_data);

public:
  /** Call from an opened monitor to close again */
  void closeMonitor(unsigned channelno, unsigned entryno);

  /** Toggle callback, to open/close monitor */
  void monitorToggle(GtkToggleButton *btn, _DChannelInfo *path_str);
};

DUECA_NS_END

#endif
