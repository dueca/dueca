/* ------------------------------------------------------------------   */
/*      item            : TimingViewGtk.cxx
        made by         : Rene' van Paassen
        date            : 020225
        category        : body file
        description     :
        changes         : 020225 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define TimingViewGtk_cxx

#include <dueca-conf.h>

// this module is conditional on having and using libglade

#include "TimingViewGtk.hxx"
#include "GtkDuecaView.hxx"
#include "DuecaPath.hxx"
#include <iomanip>
#include <time.h>
#if defined(HAVE_SSTREAM)
#include <sstream>
#elif defined(HAVE_STRSTREAM)
#include <strstream>
#else
#error "Need strstream or sstream"
#endif
#include <WrapSendEvent.hxx>

//#define I_SYS
#define W_CNF
#define E_CNF
#include "debug.h"

#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include "dueca.h"
#include <WrapSendEvent.hxx>

// only useful if interfaces present

DUECA_NS_START

/** Toolkit-dependent GUI information. */
class TimingViewGtk::GuiInfo
{
  friend class TimingViewGtk;

  /** File for the interface definition. */
  std::string                                 gladefile;

  /** Object that reads the glade file, and builds up the window. */
  GtkGladeWindow  gwindow;

  /** Table with timing results. */
  GtkWidget* table;

  /** List store for timing results table */
  GtkListStore* table_store;

  /** The item we add to the menu of DUECA's main window. */
  GtkWidget* menuitem;

  /** List store for sync status table */
  GtkListStore* synctable_store;

  /** Constructor. */
  GuiInfo(const vstring& gladefile) :
    gladefile(gladefile),
    table(NULL),
    menuitem(NULL)
  { }
};

TimingViewGtk::TimingViewGtk(Entity* e, const char* part,
                       const PrioritySpec& ps) :
  TimingView(e, part, ps),
  gui(*(new GuiInfo(DuecaPath::prepend("timingview.glade3"))))
{
  // check the presence of a DuecaView object, for getting initial
  // access to the interface
  if (GtkDuecaView::single() == NULL) {
    /* DUECA UI.

       To use timingview, DuecaView needs to be configured first. */
    W_CNF(getId() << " TimingView needs DuecaView!");
    can_start = false;
    return;
  }
}

// table with callback functions for glade xml window.
static GladeCallbackTable cb_links[] = {
  { "timingview_clear",     "clicked",
    gtk_callback(&TimingViewGtk::clearView)},
  { "update_sync",          "clicked",
    gtk_callback(&TimingViewGtk::requestSync), reinterpret_cast<gpointer>(0) },
  { "clear_sync",           "clicked",
    gtk_callback(&TimingViewGtk::requestSync), reinterpret_cast<gpointer>(1) },
  { "timingview_close",     "clicked",
    gtk_callback(&TimingViewGtk::activateMenuItem)},
  { "timingview_window", "delete_event",
    gtk_callback(&TimingViewGtk::deleteView) },
  { NULL, NULL, NULL }
};

bool TimingViewGtk::complete()
{
  gui.gwindow.readGladeFile(gui.gladefile.c_str(), "timingview_window",
                            reinterpret_cast<gpointer>(this), cb_links);
  GtkWidget* window = gui.gwindow["timingview_window"];
  gui.table = gui.gwindow["timingtable"];

  // the list store is defined in the xml window definition
  gui.synctable_store = GTK_LIST_STORE
    (gui.gwindow.getObject("synctable_store"));

  gui.table_store = GTK_LIST_STORE
    (gui.gwindow.getObject("table_store"));
  GtkTreeIter iter;

  /* contents:
     0 node
     1 difference to 0
     2 # early
     3 # late
     4 # double
     5 # nowait
     6 latest
     7 earliest
     8 stepsize
  */

  for (int ii = no_nodes; ii--; ) {
    gtk_list_store_append(gui.synctable_store, &iter);
    gtk_list_store_set(gui.synctable_store, &iter, 0, ii,
                       1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, -1);
  }

  // request the DuecaView object to make an entry for my window,
  // opening it on activation
  gui.menuitem = GTK_WIDGET
    (GtkDuecaView::single()->requestViewEntry
     ("Timing View", GTK_WIDGET(window)));

  return can_start;
}

TimingViewGtk::~TimingViewGtk()
{
  // remove the gtk windows?
}

void TimingViewGtk::appendReport(const std::string& maker_and_act,
                              const TimeTickType& tstart,
                              const TimingResults& data)
{
  // append log to the list
  GtkTreeIter iter;
  gtk_list_store_append(gui.table_store, &iter);
  gtk_list_store_set(gui.table_store, &iter,
                     0, maker_and_act.c_str(),
                     1, tstart,
                     2, data.min_start,
                     3, data.avg_start,
                     4, data.max_start,
                     5, data.min_complete,
                     6, data.avg_complete,
                     7, data.max_complete,
                     8, data.n_warning,
                     9, data.n_critical,
                     10, data.n_user, -1);
  num_rows++;

  // remember number of rows
  // if this is too much, delete the oldest data
  if (num_rows++ > 100) {
    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gui.table_store), &iter);
    gtk_list_store_remove(gui.table_store, &iter);
    num_rows--;
  }
}

void TimingViewGtk::clearView(GtkButton *button, gpointer user_data)
{
  GtkTreeIter iter;
  while (num_rows) {
    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gui.table_store), &iter);
    gtk_list_store_remove(gui.table_store, &iter);
    num_rows--;
  }
}

void TimingViewGtk::updateSync(int node, const SyncReport& report)
{
  GtkTreeIter iter;
  gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gui.synctable_store), &iter);
  for (int ii = node; ii--; ) {
    gtk_tree_model_iter_next(GTK_TREE_MODEL(gui.synctable_store), &iter);
  }
  gtk_list_store_set(gui.synctable_store, &iter,
                     1, report.difference,
                     2, report.no_early,
                     3, report.no_late,
                     4, report.no_double_waits,
                     5, report.no_cancelled_waits,
                     6, report.latest_wrt_ideal,
                     7, report.earliest_wrt_ideal,
                     8, report.average_step_size, -1);
}

void TimingViewGtk::activateMenuItem(GtkButton *button, gpointer user_data)
{
  g_signal_emit_by_name(G_OBJECT(gui.menuitem), "activate", NULL);
}

gboolean TimingViewGtk::deleteView(GtkWidget *window, GdkEvent *event,
                                  gpointer user_data)
{
  g_signal_emit_by_name(G_OBJECT(gui.menuitem), "activate", NULL);

  // with this, the click is handled.
  return TRUE;
}

void TimingViewGtk::requestSync(GtkButton *button, gpointer user_data)
{
  bool clearflag = user_data != NULL;
  /* DUECA UI.

     Information on the callback for requesting sync. */
  I_SYS(getId() << " request sync with " << clearflag);
  if (!request_report.isValid()) {
    /* DUECA UI.

       The channel for requesting timing information is not valid. */
    W_SYS(getId() << " channel for requests not valid");
    return;
  }
  wrapSendEvent(request_report, new SyncReportRequest(clearflag),
                SimTime::now());
}

DUECA_NS_END



