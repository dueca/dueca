/* ------------------------------------------------------------------   */
/*      item            : TimingViewGtk.hxx
        made by         : Rene van Paassen
        date            : 020225
        category        : header file
        description     :
        changes         : 020225 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef TimingViewGtk_hxx
#define TimingViewGtk_hxx

#ifdef TimingViewGtk_cxx
#endif

#include <TimingView.hxx>
#include <gtk/gtk.h>

struct ParameterTable;

DUECA_NS_START

/**  Implements the Gtk3 interface for TimingView
 */
class TimingViewGtk : public TimingView
{
  /// class that hides gui implementation
  class GuiInfo;

  /// object with the gui implementation
  GuiInfo &gui;

public:
  /** Constructor, follows standard module construction form. */
  TimingViewGtk(Entity *e, const char *part, const PrioritySpec &ps);

  /** Destructor. */
  ~TimingViewGtk();

  /** Completion action. */
  bool complete();

  /** Clears all stuff in the view. */
  void clearView(GtkButton *button, gpointer user_data);

  /** Close the view, indirectly, by activating the menu item. */
  void activateMenuItem(GtkButton *button, gpointer user_data);

  /** Request and update for the syncing information. */
  void requestSync(GtkButton *button, gpointer user_data);

  /** callback, close the view on deletion by window manager. */
  gboolean deleteView(GtkWidget *window, gpointer user_data);

  /** Write a sync report to the interface */
  void updateSync(int node, const SyncReport &report);

  /** Write a timing report on the interface. */
  void appendReport(const std::string &maker_and_act,
                    const TimeTickType &tstart, const TimingResults &data);

  /** set up a label item widget */
  void cbSetupLabel(GtkSignalListItemFactory *fact, GtkListItem *object,
                    gpointer user_data);

  /** bind timing */
  void cbBindTimingNode(GtkSignalListItemFactory *fact, GtkListItem *object,
                        gpointer user_data);

  /** bind timing */
  void cbBindTimingDiff(GtkSignalListItemFactory *fact, GtkListItem *object,
                        gpointer user_data);

  /** bind timing */
  void cbBindTimingNEarly(GtkSignalListItemFactory *fact, GtkListItem *object,
                          gpointer user_data);

  /** bind timing */
  void cbBindTimingNLate(GtkSignalListItemFactory *fact, GtkListItem *object,
                         gpointer user_data);

  /** bind timing */
  void cbBindTimingNDouble(GtkSignalListItemFactory *fact, GtkListItem *object,
                           gpointer user_data);

  /** bind timing */
  void cbBindTimingNNoWait(GtkSignalListItemFactory *fact, GtkListItem *object,
                           gpointer user_data);

  /** bind timing */
  void cbBindTimingLatest(GtkSignalListItemFactory *fact, GtkListItem *object,
                          gpointer user_data);

  /** bind timing */
  void cbBindTimingEarliest(GtkSignalListItemFactory *fact, GtkListItem *object,
                            gpointer user_data);

  /** bind timing */
  void cbBindTimingStepsz(GtkSignalListItemFactory *fact, GtkListItem *object,
                          gpointer user_data);

  /** bind summary */
  void cbBindSummaryActivity(GtkSignalListItemFactory *fact,
                             GtkListItem *object, gpointer user_data);

  /** bind summary */
  void cbBindSummaryLogtime(GtkSignalListItemFactory *fact, GtkListItem *object,
                            gpointer user_data);

  /** bind summary */
  void cbBindSummaryMinStart(GtkSignalListItemFactory *fact,
                             GtkListItem *object, gpointer user_data);

  /** bind summary */
  void cbBindSummaryAvgStart(GtkSignalListItemFactory *fact,
                             GtkListItem *object, gpointer user_data);

  /** bind summary */
  void cbBindSummaryMaxStart(GtkSignalListItemFactory *fact,
                             GtkListItem *object, gpointer user_data);

  /** bind summary */
  void cbBindSummaryMinComplete(GtkSignalListItemFactory *fact,
                                GtkListItem *object, gpointer user_data);

  /** bind summary */
  void cbBindSummaryAvgComplete(GtkSignalListItemFactory *fact,
                                GtkListItem *object, gpointer user_data);

  /** bind summary */
  void cbBindSummaryMaxComplete(GtkSignalListItemFactory *fact,
                                GtkListItem *object, gpointer user_data);

  /** bind summary */
  void cbBindSummaryNWarn(GtkSignalListItemFactory *fact, GtkListItem *object,
                          gpointer user_data);

  /** bind summary */
  void cbBindSummaryNCrit(GtkSignalListItemFactory *fact, GtkListItem *object,
                          gpointer user_data);

  /** bind summary */
  void cbBindSummaryNUser(GtkSignalListItemFactory *fact, GtkListItem *object,
                          gpointer user_data);
};

DUECA_NS_END
#endif
