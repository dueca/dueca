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
class TimingViewGtk: public TimingView
{
  /// class that hides gui implementation
  class GuiInfo;

  /// object with the gui implementation
  GuiInfo &gui;

public:
  /** Constructor, follows standard module construction form. */
  TimingViewGtk(Entity* e, const char* part, const PrioritySpec& ps);

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
  gboolean deleteView(GtkWidget *window, GdkEvent *event, gpointer user_data);

  /** Write a sync report to the interface */
  void updateSync(int node, const SyncReport& report);

  /** Write a timing report on the interface. */
  void appendReport(const std::string& maker_and_act,
                    const TimeTickType& tstart,
                    const TimingResults& data);
};

DUECA_NS_END
#endif
