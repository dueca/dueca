/* ------------------------------------------------------------------   */
/*      item            : ActivityView.cxx
        made by         : Rene' van Paassen
        date            : 000830
        category        : body file
        description     :
        changes         : 000830 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define ActivityView_cc

#include <dueca-conf.h>
#include "ActivityView.hxx"
#include "PrioritySpec.hxx"
#include "NodeManager.hxx"
#include "NameSet.hxx"
#include "Ticker.hxx"
#include "ChannelDistribution.hxx"
#include "SimTime.hxx"
#include "ParameterTable.hxx"
#include "GtkDuecaView.hxx"
#include "DuecaPath.hxx"
#include "ActivityLog.hxx"
#include <ActivityDescriptions.hxx>
#include <cmath>
#include <time.h>
#include "dueca.h"
#include <DataWriter.hxx>

#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif
#define W_STS
#define E_STS
#include <debug.h>

/** Pixel distance between activity lines on the display. */
#define LINESPACE 8
/** If defined, also give an alphanumeric output on the log. */
#undef DO_PRINT

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START

struct ActivityViewGui
{
  // Gui of this module is conditional on having and using libglade

  /** File for the interface definition. */
  std::string                                 gladefile;

  /** Window based on glade file. */
  GtkGladeWindow                              window;

  /** List model, numerical output. */
  GtkListStore*                               act_store;

  /** different canvas widgets. */
  GtkWidget**                                 canvas;

  /** Menu item. */
  GtkWidget*                                  menuitem;

  /** Constructor. */
  ActivityViewGui(const vstring& gladefile) :
    gladefile(gladefile),
    canvas(NULL),
    menuitem(NULL)
  { }
};

ActivityView::ActivityView(Entity* e, const char* part,
                     const PrioritySpec& ps) :
  ActivityViewBase(e, part, ps),
  gui(*new ActivityViewGui(DuecaPath::prepend("activity_view.glade3")))
{
  // check the presence of a DuecaView object, for getting initial
  // access to the interface
  if (DuecaView::single() == NULL) {
    /* DUECA UI.

       To use activityview, DuecaView needs to be configured first. */
    W_CNF(getId() << " ActivityView needs DuecaView!");
    can_start = false;
  }
}

// c-linkage callback functions
static int cbDraw(GtkWidget *w, cairo_t *cr, gpointer av)
{ return reinterpret_cast<ActivityView*>(av)->cbDraw(w, cr, NULL); }
static int cbConfigure(GtkWidget *w, GdkEventConfigure *event, gpointer av)
{ return reinterpret_cast<ActivityView*>(av)->cbConfigure(w, event); }
static gboolean cbDrawAreaButtonPress(GtkWidget *w,
                                      GdkEventButton *ev, gpointer av)
{ return reinterpret_cast<ActivityView*>(av)->cbDrawAreaButtonPress(w, ev); }
static gboolean cbDrawAreaButtonRelease(GtkWidget *w,
                                        GdkEventButton *ev, gpointer av)
{ return reinterpret_cast<ActivityView*>(av)->cbDrawAreaButtonRelease(w, ev); }

// table with callback functions for glade xml window.
static GladeCallbackTable cb_links[] = {
  { "close", "clicked", gtk_callback(&ActivityView::cbClose) },
  { "update", "clicked", gtk_callback(&ActivityView::cbUpdate) },
  { "viewspan", "value-changed", gtk_callback(&ActivityView::cbViewSpan) },
  { "recordspan", "value-changed", gtk_callback(&ActivityView::cbRecordSpan) },
  { "viewscroll", "button_release_event",
    gtk_callback(&ActivityView::cbViewScroll) },
  { "activity_view", "delete_event", gtk_callback(&ActivityView::deleteView) },
  { NULL, NULL, NULL }
};

bool ActivityView::complete()
{
  // use the glade file to create the window
  bool res = gui.window.readGladeFile
    (gui.gladefile.c_str(), "activity_view",
     reinterpret_cast<gpointer>(this), cb_links);

  if (!res) {
    /* DUECA UI.

       Cannot find the glade file defining the base GUI. Check DUECA
       installation and paths.
    */
    E_CNF(getId() << classname << " Opening " <<
          DuecaPath::prepend(gui.gladefile) << " failed");
    return false;
  }

  // combine the activitity list with a store
  GtkTreeView* aclist = GTK_TREE_VIEW(gui.window["activitylist_view"]);

  gui.act_store = GTK_LIST_STORE(gtk_tree_view_get_model(aclist));

  // add canvases for the different nodes
  gui.canvas = new GtkWidget*
    [DUECA_NS::NodeManager::single()->getNoOfNodes()];

  for (int ii = 0; ii <
         DUECA_NS ::NodeManager::single()->getNoOfNodes(); ii++) {

    // create the canvas
    gui.canvas[ii] = gtk_drawing_area_new();
    gtk_widget_add_events(gui.canvas[ii],
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    gtk_widget_set_size_request(gui.canvas[ii], 400, 28);
    g_object_ref(gui.canvas[ii]);

    g_object_set_data(G_OBJECT(gui.canvas[ii]), "node",
                        reinterpret_cast<gpointer>(ii));

    // configure size and place it in the box
    gtk_box_pack_start(GTK_BOX(gui.window["linebox"]), gui.canvas[ii],
                       FALSE, TRUE, 2);

    // add a callback for expose and one for realize
    g_signal_connect(G_OBJECT(gui.canvas[ii]), "configure_event",
                     G_CALLBACK(DUECA_NS::cbConfigure),
                     reinterpret_cast<gpointer>(this));
    g_signal_connect(G_OBJECT(gui.canvas[ii]), "draw",
                     G_CALLBACK(DUECA_NS::cbDraw),
                     reinterpret_cast<gpointer>(this));
    g_signal_connect(G_OBJECT(gui.canvas[ii]), "button-press-event",
                     G_CALLBACK(DUECA_NS::cbDrawAreaButtonPress),
                     reinterpret_cast<gpointer>(this));
    g_signal_connect(G_OBJECT(gui.canvas[ii]), "button-release-event",
                     G_CALLBACK(DUECA_NS::cbDrawAreaButtonRelease),
                     reinterpret_cast<gpointer>(this));
    gtk_widget_show(gui.canvas[ii]);
  }

  // for good measure
  gtk_widget_show(gui.window["linebox"]);

  // request the DuecaView object to make an entry for my window,
  // opening it on activation
  gui.menuitem = GTK_WIDGET
    (GtkDuecaView::single()->requestViewEntry
     ("Activity View", GTK_WIDGET(gui.window["activity_view"])));

  return true;
}


ActivityView::~ActivityView()
{
  for (int ii = 0; ii <
         DUECA_NS ::NodeManager::single()->getNoOfNodes(); ii++) {
    g_object_unref(gui.canvas[ii]);
  }
  delete [] gui.canvas;
  g_object_unref(gui.menuitem);

  delete &gui;
}

void ActivityView::cbUpdate(GtkButton* button, gpointer gp)
{
  DEB("cbUpdate callback");

  // only accept if there is no sweep pending
  if (!sweep_done) {
    DEB("ActivityView sweep pending, return");
    return;
  }

  DEB("ActivityView calling update");
  // reset the logs, mark the canvases as dirty, and request redraw
  for (int node = current_logs.size(); node--; ) {
    current_logs[node].resetLogs();
    g_object_set_data(G_OBJECT(gui.canvas[node]), "dirty",
                        reinterpret_cast<gpointer>(1));
    gtk_widget_queue_draw(gui.canvas[node]);
  }

  TimeTickType request_start =
    (max(prev_request_end, SimTime::now() + lookahead) /
     Ticker::single()->getCompatibleIncrement()) *
    Ticker::single()->getCompatibleIncrement();
  TimeTickType ispan = max(2, int(dspan * ticks_per_sec + 0.5));
  {
    DataWriter<ActivityLogRequest> r(send_request, SimTime::getTimeTick());
    r.data().span = ispan;
    r.data().start = request_start;
  }
  prev_request_end = request_start + ispan;

  // start up the sweeper
  sweep_alarm.requestAlarm(request_start + ispan +
                           Ticker::single()->getCompatibleIncrement());
  sweep_done = false;

  // add start tick to window
  char s_tick[12];
  snprintf(s_tick, 12, "%11d", request_start);
  gtk_entry_set_text(GTK_ENTRY(gui.window["starttick"]), s_tick);

  // remove the selection
  hl.end = 0;
}

void ActivityView::cbClose(GtkButton* button, gpointer gp)
{
  // do not do this directly, but go through the menu item. Also
  // updates the flag there + closes the window.
  g_signal_emit_by_name(G_OBJECT(gui.menuitem), "activate", NULL);
}

gboolean ActivityView::deleteView(GtkWidget *window, GdkEvent *event,
                                  gpointer user_data)
{
  g_signal_emit_by_name(G_OBJECT(gui.menuitem), "activate", NULL);

  // with this, the click is handled.
  return TRUE;
}

void ActivityView::cbViewSpan(GtkWidget* spin, gpointer gp)
{
  //vspan = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
  vspan = gtk_range_get_value(GTK_RANGE(spin));
  GtkAdjustment *adj2 =
    gtk_range_get_adjustment(GTK_RANGE(gui.window.getObject("viewscroll")));
  gtk_adjustment_set_page_size(adj2, vspan);//->page_size = vspan;
  gtk_adjustment_set_upper(adj2, dspan);//->upper = dspan;
#if GTK_CHECK_VERSION(3,18,0)
  // deprecated here
#else
  gtk_adjustment_changed(adj2);
#endif

  // request update lines
  for (int node = current_logs.size(); node--; ) {
    g_object_set_data(G_OBJECT(gui.canvas[node]), "dirty",
                        reinterpret_cast<gpointer>(1));
  }

  gtk_widget_queue_draw(gui.window["linebox"]);

  // remove the selection
  hl.end = 0;
}

bool ActivityView::setPositionAndSize(const std::vector<int>& ps)
{
  if (ps.size() == 2 || ps.size() == 4) {
    gui.window.setWindow(ps);
  }
  else {
    /* DUECA UI.

       ActivityView window setting needs 2 (for size) or 4 (also location)
       arguments. */
    E_CNF("ActivityView position size setting needs 2 or 4 arguments");
    return false;
  }
  return true;
}

void ActivityView::cbRecordSpan(GtkWidget* spin, gpointer gp)
{
  //dspan = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
  dspan = gtk_range_get_value(GTK_RANGE(spin));
  GtkAdjustment *adj2 =
    gtk_range_get_adjustment(GTK_RANGE(gui.window.getObject("viewscroll")));
  gtk_adjustment_set_upper(adj2, dspan);
#if GTK_CHECK_VERSION(3,18,0)
  // deprecated here
#else
  gtk_adjustment_changed(adj2);
#endif
}

int ActivityView::cbViewScroll(GtkWidget* scroll, GdkEventButton *e,
                               gpointer gp)
{
  for (int node = current_logs.size(); node--; ) {
    g_object_set_data(G_OBJECT(gui.canvas[node]), "dirty",
                        reinterpret_cast<gpointer>(1));
  }

  // remove the selection
  hl.end = 0;

  gtk_widget_queue_draw(gui.window["linebox"]);

  // handled the event, now let others ...
  return FALSE;
}

int ActivityView::cbDraw(GtkWidget* w, cairo_t *cr, gpointer data)
{
  DEB("draw event area "
      << long(g_object_get_data(G_OBJECT(w), "node")));

  int node = long(g_object_get_data(G_OBJECT(w), "node"));
  // int dirty = long(g_object_get_data(G_OBJECT(w), "dirty"));
  //if (dirty) {

    // clear background
    GtkAllocation alc; gtk_widget_get_allocation(w, &alc);
    cairo_set_source_rgb(cr, 0., 0., 0.);
    cairo_paint(cr);

    // if applicable, draw highlighted area selected for list
    if (hl.end && hl.node == node) {
      cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
      cairo_rectangle
        (cr, hl.start,
         (current_logs[node].getNumLevels() - hl.level - 1) * LINESPACE + 2,
         hl.end - hl.start, LINESPACE);
      cairo_fill(cr);
    }

    // what part to draw depends on the scroll bar and the selected
    // view span
    GtkAdjustment *va = gtk_range_get_adjustment
      (GTK_RANGE(gui.window.getObject("viewscroll")));
    float tick_start = (ticks_per_sec * gtk_adjustment_get_value(va));
    float tick_end = tick_start + vspan*ticks_per_sec;

    DEB(getId() << classname << " scroll value " <<
        gtk_adjustment_get_value(va) <<
        " tick start " << tick_start << " tick end " << tick_end);

    if (current_logs[node].areTheLogsComplete()) {
      ActivityLister l = current_logs[node].startInvestigation(0);

      int ycoord = LINESPACE;
      unsigned winwidth = alc.width;
      for (int prio = l.getMaxPrio(); prio--; ) {
        ActivityLine lin = l.getNextActivity
          (prio, tick_start, tick_end, winwidth);
        while (lin.type != ActivityLine::Blank) {
          switch(lin.type) {
          case ActivityLine::Blank:
            break;
          case ActivityLine::Run:
            cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
            cairo_set_line_width(cr, 3.0);
            cairo_move_to(cr, lin.x0, ycoord-6);
            cairo_line_to(cr, lin.x0, ycoord);
            if (lin.x0 != lin.x1)
              cairo_line_to(cr, lin.x1, ycoord);
            cairo_stroke(cr);
            break;
          case ActivityLine::Block:
            if (lin.x0 != lin.x1) {
              cairo_set_source_rgb(cr, 0.0, 0.6, 0.0);
              cairo_set_line_width(cr, 1.0);
              cairo_move_to(cr, lin.x0, ycoord);
              cairo_line_to(cr, lin.x1, ycoord);
              cairo_stroke(cr);
            }
            break;
          case ActivityLine::Graphics:
            cairo_set_source_rgb(cr, 0.0, 0.8, 1.0); // SkyBlue
            cairo_set_line_width(cr, 3.0);
            cairo_move_to(cr, lin.x0, ycoord-6);
            cairo_line_to(cr, lin.x0, ycoord);
            if (lin.x0 != lin.x1)
              cairo_line_to(cr, lin.x1, ycoord);
            cairo_stroke(cr);
          }
          DEB("line for level " << prio << " coord "
              << lin.x0 << ' ' << ycoord << ' ' << lin.x1);
          lin = l.getNextActivity(prio, tick_start, tick_end, winwidth);
        }
        ycoord += LINESPACE;
      }

      g_object_set_data(G_OBJECT(w), "dirty",
                        reinterpret_cast<gpointer>(0));
    }
    else {
      /* DUECA UI.

         Not all activity logs arrived.
      */
      I_STS(getId() << classname << " logs incomplete");
    }
    //}

  return FALSE;
}

int ActivityView::cbConfigure(GtkWidget* w, GdkEventConfigure *ev)
{
  int node = long(g_object_get_data(G_OBJECT(w), "node"));
  //int dirty = int(g_object_get_data(G_OBJECT(w), "dirty"));

  DEB(getId() << " " << classname << " cbConfigure " << node);
  GtkAllocation alc; gtk_widget_get_allocation(w, &alc);

  // check whether widget is large enough to accommodate all lines
  if (current_logs[node].areTheLogsComplete() &&
      alc.height < current_logs[node].getNumLevels() * LINESPACE +
      LINESPACE/2) {
    gtk_widget_set_size_request
      (GTK_WIDGET(gui.canvas[node]), 400,
                          current_logs[node].getNumLevels() * LINESPACE +
                          LINESPACE/2 );
  }

  // set dirty, so a redraw will actually be done
  g_object_set_data(G_OBJECT(w), "dirty", reinterpret_cast<gpointer>(1));

  // configure drawing contests, only works first time
  return TRUE;
}

int ActivityView::cbDrawAreaButtonPress(GtkWidget *w, GdkEventButton *ev)
{
  DEB(getId() << classname << " button " << ev->button <<
      " press, at " << ev->x << ',' << ev->y);

  // only react to 1st button
  if (ev->button != 1) return FALSE;

  // figure out which of the drawing areas
  hlnew.node = long(g_object_get_data(G_OBJECT(w), "node"));

  // check manager number from y coordinate
  hlnew.level = int(current_logs[hlnew.node].getNumLevels() -
                    (ev->y - LINESPACE/2) / LINESPACE);

  // remember where button was pressed,
  hlnew.start = int(ev->x);
  hlnew.end = 1;

  DEB(getId() << classname << " new hl set up, node=" << hlnew.node <<
      " level=" << hlnew.level << " start=" << hlnew.start);

  return TRUE;
}

int ActivityView::cbDrawAreaButtonRelease(GtkWidget *w, GdkEventButton *ev)
{
  DEB(getId() << classname << " button " << ev->button <<
      " release, at " << ev->x << ',' << ev->y);

  // only react to 1st button
  if (ev->button != 1) return FALSE;

  // check that release corresponds to press
  int node = long(g_object_get_data(G_OBJECT(w), "node"));
  int level = int(current_logs[node].getNumLevels() -
                  (ev->y - LINESPACE/2) / LINESPACE);

  if (!hlnew.end || node != hlnew.node || level != hlnew.level ||
      hlnew.start == int(ev->x)) {
    DEB(getId() << classname << " different node/level; " << node <<
        ' ' << level);
    hlnew.end = 0;
    return TRUE;
  }

  // define highlight area, regardless of moving backwards with mouse
  if (hlnew.start < int(ev->x)) { hlnew.end = int(ev->x); }
  else { hlnew.end = hlnew.start; hlnew.start = int(ev->x); }

  GtkAllocation alc; gtk_widget_get_allocation(w, &alc);

  // figure out start and end times.
  GtkAdjustment *va = gtk_range_get_adjustment
    (GTK_RANGE(gui.window.getObject("viewscroll")));
  float tick_start = ticks_per_sec *
    (gtk_adjustment_get_value(va) + float(hlnew.start) / float(alc.width) * vspan);
  float tick_end = ticks_per_sec *
    (gtk_adjustment_get_value(va) + float(hlnew.end) / float(alc.width) * vspan);

  DEB(getId() << classname << " looking from " << tick_start <<
      " to " << tick_end);

  // fill list with highlighted actions
  gtk_list_store_clear(gui.act_store);
  try {
    ActivityLister l = current_logs[node].startInvestigation(0);
    bool more = true;
    while(more) {

      GtkTreeIter iter;
      gtk_list_store_append(gui.act_store, &iter);

      ActivityStrings desc = l.getNextActivityDesc
        (level, tick_start, tick_end, more);
      gtk_list_store_set(gui.act_store, &iter,
                         0, desc.tick, 1, desc.offset, 2, desc.ts,
                         3, desc.dt, 4, desc.module, 5, desc.activity, -1);
      DEB(getId() << classname << " append description");
    }
  }
  catch (const WeaverKeyInvalid &e) {
    /* DUECA UI.

       Failed to create a list of selected activities.
    */
    W_STS(getId() << classname << " cannot create description list");
  }

  // queue redraw, if another node had the highlight area
  if (hl.end && hl.node != hlnew.node) {
    g_object_set_data(G_OBJECT(gui.canvas[hl.node]), "dirty",
                        reinterpret_cast<gpointer>(1));
    gtk_widget_queue_draw(gui.canvas[hl.node]);
  }

  // copy new list, prepare for re-start, and ask for redraw
  hl = hlnew; hlnew.end = 0;
  g_object_set_data(G_OBJECT(w), "dirty", reinterpret_cast<gpointer>(1));
  gtk_widget_queue_draw(w);

  return TRUE;
}

template<class T>
class VirtualJoinWithStealing: public VirtualJoin<T>
{
public:
  /** Constructor. Essential to initialize data pointer to NULL */
  VirtualJoinWithStealing(DataReaderBase& r): VirtualJoin<T>(r) {}

  /** Release a previous access */
  inline const void release(ChannelReadToken& token)
  {
    if (VirtualJoin<T>::data_ptr) {
      DataReaderBaseAccess::releaseAccessKeepData
        (token, VirtualJoin<T>::data_ptr);
    }
  }
};



void ActivityView::updateLines(unsigned node_id)
{
  DEB("Updatelines called on " << node_id);

  // indicate that the graphs can be re-drawn
  g_object_set_data(G_OBJECT(gui.canvas[node_id]), "dirty",
                    reinterpret_cast<gpointer>(1));

  // either configure or redraw
  GtkAllocation alc;
  gtk_widget_get_allocation(gui.canvas[node_id], &alc);
  if (alc.height <
      current_logs[node_id].getNumLevels()*LINESPACE + LINESPACE/2) {
    gtk_widget_set_size_request
      (gui.canvas[node_id], 400,
       current_logs[node_id].getNumLevels()*LINESPACE + LINESPACE/2);
    gtk_widget_queue_resize(gui.canvas[node_id]);
  }
  else {
    gtk_widget_queue_draw(gui.canvas[node_id]);
  }
}

DUECA_NS_END
