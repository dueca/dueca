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
#include "GtkGladeWindow.hxx"
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
  gboolean deleteView(GtkWidget *window, GdkEvent *event, gpointer user_data);

  /** Collect new activity log data. */
  void cbUpdate(GtkButton* button, gpointer gp);

  /** Update view span. */
  void cbViewSpan(GtkWidget* spin, gpointer gp);

  /** Update record span. */
  void cbRecordSpan(GtkWidget* spin, gpointer gp);

  /** Update view window after scrolling. */
  int cbViewScroll(GtkWidget *w, GdkEventButton *e, gpointer gp);

  /** Redraw stuff. */
  int cbDraw(GtkWidget *w, cairo_t *cr, gpointer user_data);

  /** realize widget stuff. */
  int cbConfigure(GtkWidget *w, GdkEventConfigure *event);

  /** React to a button press on a drawing area, by starting a
      highlight. */
  int cbDrawAreaButtonPress(GtkWidget *w, GdkEventButton *ev);

  /** React to a button release on a drawing area, by completing a
      highlight. */
  int cbDrawAreaButtonRelease(GtkWidget *w, GdkEventButton *ev);
};

DUECA_NS_END
#endif
