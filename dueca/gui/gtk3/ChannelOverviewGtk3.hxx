/* ------------------------------------------------------------------   */
/*      item            : ChannelOverviewGtk3.hxx
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

#ifndef ChannelOverviewGtk3_hxx
#define ChannelOverviewGtk3_hxx

// include the dusime header
#include <dueca.h>

// include headers for functions/classes you need in the module
#include <ChannelOverview.hxx>
#include "GtkGladeWindow.hxx"


DUECA_NS_START

/** A view on the DUECA channels

    The instructions to create an module of this class from the Scheme
    script are:

    \verbinclude channel-view.scm
 */
class ChannelOverviewGtk3:
  public ChannelOverview
{
  /** self-define the module type, to ease writing the parameter table */
  typedef ChannelOverviewGtk3 _ThisModule_;

private: // simulation data

  /** glade file */
  std::string           gladefile;

  /** glade file for monitor windows */
  std::string           monitor_gladefile;

  /** gtk window */
  GtkGladeWindow        window;

  /** Tree store for the object with data and widgets */
  GtkTreeStore          *store;

  /** Widget in the main menu */
  GtkWidget             *menuitem;

  /** Enumeration values for the store */
  enum StoreFields {
    S_channelnum,
    S_channelname,
    S_entrynum,
    S_entrylabel,
    S_writerid,
    S_writername,
    S_ES,
    S_dataclass,
    S_writecount,
    S_readerid,
    S_readername,
    S_readcount,
    S_creationid,
    S_ischanentry,
    S_iswriteentry,
    S_isreadentry,
    S_selection,
    S_sequential,
    S_viewopen,
    S_numcolumns
  };

  /** Name information window */
  struct InfoWindow {
    /** The window */
    GtkWidget           *infowindow;
    /** A label field */
    GtkWidget           *label;
    /** current text */
    std::string         text;
    /** Is visible */
    bool                visible;
    /** Create */
    void init();
    /** Update */
    void update(const gchararray& txt, gint x, gint y);
    /** Close again */
    void hide();
  };

  /** Pop-up overview window, for tooltip-like information on data etc.*/
  InfoWindow iwindow;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const           classname;

  /** Return the parameter table. */
  static const ParameterTable*       getMyParameterTable();

  /** Sorting helper */
  static gint sort_on_name(GtkTreeModel *model, GtkTreeIter *a,
			   GtkTreeIter *b, gpointer userdata);

  /** Sorting helper */
  static gint sort_on_number(GtkTreeModel *model, GtkTreeIter *a,
			     GtkTreeIter *b, gpointer userdata);
  
public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  ChannelOverviewGtk3(Entity* e, const char* part, const PrioritySpec& ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~ChannelOverviewGtk3();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec& ts);

  /** Request check on the timing. */
  bool checkTiming(const vector<int>& i);

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
  void cbClose(GtkButton* button, gpointer gp);
  /** refresh read/write count */
  void cbRefreshCounts(GtkButton* button, gpointer gp);
  /** window delete selected */
  gboolean cbDelete(GtkWidget *window, GdkEvent *event, gpointer user_data);
  /** hover in the tree area */
  gboolean cbHover(GtkWidget *window, GdkEventMotion *event,
                   gpointer user_data);
  /** leave the tree area */
  gboolean cbLeave(GtkWidget *window, GdkEvent *event, gpointer user_data);

public:
  /** Call from an opened monitor to close again */
  void closeMonitor(unsigned channelno, unsigned entryno);

  /** Toggle callback */
  void monitorToggle(GtkCellRendererToggle *cell, gchar *path_str);
};

DUECA_NS_END

#endif
