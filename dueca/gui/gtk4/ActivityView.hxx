/* ------------------------------------------------------------------   */
/*      item            : ActivityView.hh
        made by         : Rene' van Paassen
        date            : 000830
        category        : header file
        description     :
        changes         : 000830 first version
                          170904 base on common ActivityViewBase
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        api             : DUECA_API
*/

#ifndef ActivityView_hh
#define ActivityView_hh

#ifdef ActivityView_cc
#endif

#include <gtk/gtk.h>
#include <ActivityViewBase.hxx>

DUECA_NS_START

struct ParameterTable;
struct ActivityViewGui;

/** This is a module that can generate an overview of the activity
    (timelines) in a set of connected DUECA nodes. This module can be
    created in the model script. See the description of TimingView for
    more information about how to set up the "dueca" part of the model
    script for your application. */
class ActivityView: public ActivityViewBase
{
  /** All GUI-related objects. */
  ActivityViewGui&                            gui;

public:

  /** Constructor. */
  ActivityView(Entity* e, const char* part, const PrioritySpec& ps);

  /** Destructor. */
  ~ActivityView();

  /** Completion, creates the window. */
  bool complete() final;

  /** Re-draw the display of activities. */
  void updateLines(unsigned idlog) final;

  /** Gtk window callbacks. */
  /** Close the window. */
  void cbClose(GtkButton* button, gpointer gp);

  /** callback, close the view on deletion by window manager. */
  gboolean deleteView(GtkWidget *window, gpointer user_data);

  /** Collect new activity log data. */
  void cbUpdate(GtkButton* button, gpointer gp);

  /** Update view span. */
  void cbViewSpan(GtkWidget* spin, gpointer gp);

  /** Update record span. */
  void cbRecordSpan(GtkWidget* spin, gpointer gp);

  /** Update view window after scrolling. */
  int cbViewScroll(GtkAdjustment *w, gpointer gp);

  /** Redraw stuff. */
  int cbDraw(GtkDrawingArea *w, cairo_t *cr, int width, int height);

  /** realize widget stuff. */
  int cbConfigure(GtkWidget *w, gpointer user_data);

  /** React to a button press on a drawing area, by starting a
      highlight. */
  void cbDrawAreaButtonPress(gint n_press, gdouble x, gdouble y, unsigned area);

  /** React to a button release on a drawing area, by completing a
      highlight. */
  void cbDrawAreaButtonRelease(gint n_press, gdouble x, gdouble y, unsigned area);

  /** Set-up a label text field for detail */
  void cbSetupLabel(GtkSignalListItemFactory* f, GtkListItem* obj, gpointer user_data);

  /** bind the tick data to the column view */
  void cbBindTick(GtkSignalListItemFactory* f, GtkListItem* obj, gpointer user_data);

  /** bind the offset data to the column view */
  void cbBindOffset(GtkSignalListItemFactory* f, GtkListItem* obj, gpointer user_data);

  /** bind the time stamp data to the column view */
  void cbBindTimestamp(GtkSignalListItemFactory* f, GtkListItem* obj, gpointer user_data);

  /** bind the deltat data to the column view */
  void cbBindDt(GtkSignalListItemFactory* f, GtkListItem* obj, gpointer user_data);

  /** bind the module name data to the column view */
  void cbBindModule(GtkSignalListItemFactory* f, GtkListItem* obj, gpointer user_data);

  /** bind the activity name data to the column view */
  void cbBindName(GtkSignalListItemFactory* f, GtkListItem* obj, gpointer user_data);
};

DUECA_NS_END
#endif
