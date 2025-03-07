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

#include "ActivityLister.hxx"
#include "gtk/gtk.h"
#define ActivityView_cc

#include <dueca-conf.h>
#include "ActivityView.hxx"
#include "PrioritySpec.hxx"
#include "NodeManager.hxx"
#include "Ticker.hxx"
#include "SimTime.hxx"
#include "GtkDuecaView.hxx"
#include "DuecaPath.hxx"
#include "ActivityLog.hxx"
#include <ActivityDescriptions.hxx>
#include <cmath>
#include <time.h>
#include "dueca.h"
#include <DataWriter.hxx>
#include <boost/format.hpp>

#define W_STS
#define E_STS
#include <debug.h>

/** Pixel distance between activity lines on the display. */
#define LINESPACE 8
/** If defined, also give an alphanumeric output on the log. */
#undef DO_PRINT

#define DEBPRINTLEVEL -2
#include <debprint.h>

DUECA_NS_START

struct ActivityViewGui
{
  // Gui of this module is conditional on having and using libglade

  /** File for the interface definition. */
  std::string gladefile;

  /** Window based on glade file. */
  GtkGladeWindow window;

  /** List model, numerical output. */
  GListStore *act_store;

  /** different canvas widgets. */
  GtkWidget **canvas;

  /** Menu item. */
  GAction *menuitem;

  /** Constructor. */
  ActivityViewGui(const vstring &gladefile) :
    gladefile(gladefile),
    canvas(NULL),
    menuitem(NULL)
  {}
};

// for the view listy
struct _DActivityInfo
{
  GObject parent;
  ActivityStrings act;
};

G_DECLARE_FINAL_TYPE(DActivityInfo, d_activity_info, D, ACTIVITY_INFO, GObject);
G_DEFINE_TYPE(DActivityInfo, d_activity_info, G_TYPE_OBJECT);

static void d_activity_info_class_init(DActivityInfoClass *klass)
{
  //
}

static void d_activity_info_init(DActivityInfo *self)
{
  //
}

static DActivityInfo *d_activity_info_new(const ActivityStrings &desc)
{
  auto res = D_ACTIVITY_INFO(g_object_new(d_activity_info_get_type(), NULL));
  res->act = desc;
  return res;
}

ActivityView::ActivityView(Entity *e, const char *part,
                           const PrioritySpec &ps) :
  ActivityViewBase(e, part, ps),
  gui(*new ActivityViewGui(DuecaPath::prepend("activity_view-gtk4.ui")))
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

// table with callback functions for glade xml window.
static GladeCallbackTable cb_links[] = {
  { "close", "clicked", gtk_callback(&ActivityView::cbClose) },
  { "update", "clicked", gtk_callback(&ActivityView::cbUpdate) },
  { "viewspan", "value-changed", gtk_callback(&ActivityView::cbViewSpan) },
  { "recordspan", "value-changed", gtk_callback(&ActivityView::cbRecordSpan) },
  { "viewscroll_adjustment", "value-changed",
    gtk_callback(&ActivityView::cbViewScroll) },
  { "activity_view", "close-request", gtk_callback(&ActivityView::deleteView) },
  { "fact_tick", "setup", gtk_callback(&ActivityView::cbSetupLabel) },
  { "fact_offset", "setup", gtk_callback(&ActivityView::cbSetupLabel) },
  { "fact_timestamp", "setup", gtk_callback(&ActivityView::cbSetupLabel) },
  { "fact_dt", "setup", gtk_callback(&ActivityView::cbSetupLabel) },
  { "fact_module", "setup", gtk_callback(&ActivityView::cbSetupLabel) },
  { "fact_name", "setup", gtk_callback(&ActivityView::cbSetupLabel) },
  { "fact_tick", "bind", gtk_callback(&ActivityView::cbBindTick) },
  { "fact_offset", "bind", gtk_callback(&ActivityView::cbBindOffset) },
  { "fact_timestamp", "bind", gtk_callback(&ActivityView::cbBindTimestamp) },
  { "fact_dt", "bind", gtk_callback(&ActivityView::cbBindDt) },
  { "fact_module", "bind", gtk_callback(&ActivityView::cbBindModule) },
  { "fact_name", "bind", gtk_callback(&ActivityView::cbBindName) },
  { NULL, NULL, NULL }
};

bool ActivityView::complete()
{
  // use the glade file to create the window
  bool res =
    gui.window.readGladeFile(gui.gladefile.c_str(), "activity_view",
                             reinterpret_cast<gpointer>(this), cb_links);

  if (!res) {
    /* DUECA UI.

       Cannot find the glade file defining the base GUI. Check DUECA
       installation and paths.
    */
    E_CNF(getId() << classname << " Opening "
                  << DuecaPath::prepend(gui.gladefile) << " failed");
    return false;
  }

  // combine the activitity list with a store/model
  auto aclist = GTK_COLUMN_VIEW(gui.window["activitylist_view"]);

  gui.act_store = g_list_store_new(d_activity_info_get_type());

  auto selection = gtk_single_selection_new(G_LIST_MODEL(gui.act_store));
  gtk_column_view_set_model(aclist, GTK_SELECTION_MODEL(selection));

  // add canvases for the different nodes
  gui.canvas = new GtkWidget *[DUECA_NS::NodeManager::single()->getNoOfNodes()];

  // GestureClick ?

  for (unsigned ii = 0;
       ii < unsigned(DUECA_NS ::NodeManager::single()->getNoOfNodes()); ii++) {

    // create the canvas
    GtkGesture *controller = gtk_gesture_click_new();
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(controller),
                                               GTK_PHASE_TARGET);
    gui.canvas[ii] = gtk_drawing_area_new();
    gtk_widget_add_controller(gui.canvas[ii], GTK_EVENT_CONTROLLER(controller));
    gtk_widget_set_size_request(gui.canvas[ii], 400, 28);
    g_object_ref(gui.canvas[ii]);

    g_object_set_data(G_OBJECT(gui.canvas[ii]), "node",
                      reinterpret_cast<gpointer>(ii));
    g_object_set_data(G_OBJECT(controller), "node",
                      reinterpret_cast<gpointer>(ii));

    // configure size and place it in the box
    gtk_box_append(GTK_BOX(gui.window["linebox"]), gui.canvas[ii]);

    // add a callback for expose and one for realize
    g_signal_connect(G_OBJECT(gui.canvas[ii]), "realize",
                     G_CALLBACK(+[](GtkWidget *w, gpointer av) {
                       reinterpret_cast<ActivityView *>(av)->cbConfigure(w,
                                                                         NULL);
                     }),
                     this);
    gtk_drawing_area_set_draw_func(
      GTK_DRAWING_AREA(gui.canvas[ii]),
      +[](GtkDrawingArea *w, cairo_t *cr, int width, int height, gpointer av) {
        reinterpret_cast<ActivityView *>(av)->cbDraw(w, cr, width, height);
      },
      this, NULL);
    g_signal_connect(
      G_OBJECT(controller), "pressed",
      G_CALLBACK(+[](GtkGestureClick *gesture, gint n_press, gdouble x,
                     gdouble y, gpointer av) {
        reinterpret_cast<ActivityView *>(av)->cbDrawAreaButtonPress(
          n_press, x, y,
          reinterpret_cast<unsigned long>(
            g_object_get_data(G_OBJECT(gesture), "node")));
      }),
      reinterpret_cast<gpointer>(this));
    g_signal_connect(
      G_OBJECT(controller), "released",
      G_CALLBACK(+[](GtkGestureClick *gesture, gint n_press, gdouble x,
                     gdouble y, gpointer av) {
        reinterpret_cast<ActivityView *>(av)->cbDrawAreaButtonRelease(
          n_press, x, y,
          reinterpret_cast<unsigned long>(
            g_object_get_data(G_OBJECT(gesture), "node")));
      }),
      reinterpret_cast<gpointer>(this));
    gtk_widget_set_visible(gui.canvas[ii], TRUE);
  }

  // for good measure
  gtk_widget_set_visible(gui.window["linebox"], TRUE);

  // request the DuecaView object to make an entry for my window,
  // opening it on activation
  gui.menuitem = GtkDuecaView::single()->requestViewEntry(
    "activity", "Activity View", GTK_WIDGET(gui.window["activity_view"]));

  return true;
}

ActivityView::~ActivityView()
{
  for (int ii = 0; ii < DUECA_NS ::NodeManager::single()->getNoOfNodes();
       ii++) {
    g_object_unref(gui.canvas[ii]);
  }
  delete[] gui.canvas;
  g_object_unref(gui.menuitem);

  delete &gui;
}

void ActivityView::cbUpdate(GtkButton *button, gpointer gp)
{
  DEB("cbUpdate callback");

  // only accept if there is no sweep pending
  if (!sweep_done) {
    DEB("ActivityView sweep pending, return");
    return;
  }

  DEB("ActivityView calling update");
  // reset the logs, mark the canvases as dirty, and request redraw
  for (int node = current_logs.size(); node--;) {
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
  gtk_entry_buffer_set_text(
    gtk_entry_get_buffer(GTK_ENTRY(gui.window["starttick"])), s_tick,
    strlen(s_tick));

  // remove the selection
  hl.end = 0;
}

void ActivityView::cbClose(GtkButton *button, gpointer gp)
{
  // do not do this directly, but go through the menu item. Also
  // updates the flag there + closes the window.
  // g_signal_emit_by_name(G_OBJECT(gui.menuitem), "activate", NULL);
  GtkDuecaView::toggleView(gui.menuitem);
}

gboolean ActivityView::deleteView(GtkWidget *window, gpointer user_data)
{
  // g_signal_emit_by_name(G_OBJECT(gui.menuitem), "activate", NULL);
  GtkDuecaView::toggleView(gui.menuitem);

  // with this, the click is handled.
  return TRUE;
}

void ActivityView::cbViewSpan(GtkWidget *spin, gpointer gp)
{
  // vspan = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
  vspan = gtk_range_get_value(GTK_RANGE(spin));
  GtkAdjustment *adj2 = gtk_scrollbar_get_adjustment(
    GTK_SCROLLBAR(gui.window.getObject("viewscroll")));
  gtk_adjustment_set_page_size(adj2, vspan);//->page_size = vspan;
  gtk_adjustment_set_upper(adj2, dspan);//->upper = dspan;

  // request update lines
  for (int node = current_logs.size(); node--;) {
    g_object_set_data(G_OBJECT(gui.canvas[node]), "dirty",
                      reinterpret_cast<gpointer>(1));
  }

  gtk_widget_queue_draw(gui.window["linebox"]);

  // remove the selection
  hl.end = 0;
}

void ActivityView::cbRecordSpan(GtkWidget *spin, gpointer gp)
{
  // dspan = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
  dspan = gtk_range_get_value(GTK_RANGE(spin));
  GtkAdjustment *adj2 = gtk_scrollbar_get_adjustment(
    GTK_SCROLLBAR(gui.window.getObject("viewscroll")));
  gtk_adjustment_set_upper(adj2, dspan);
}

int ActivityView::cbViewScroll(GtkAdjustment *scroll, gpointer gp)
{

  for (int node = current_logs.size(); node--;) {
    g_object_set_data(G_OBJECT(gui.canvas[node]), "dirty",
                      reinterpret_cast<gpointer>(1));
  }

  // remove the selection
  hl.end = 0;

  gtk_widget_queue_draw(gui.window["linebox"]);

  // handled the event, now let others ...
  return FALSE;
}

int ActivityView::cbDraw(GtkDrawingArea *w, cairo_t *cr, int width, int height)
{
  DEB("draw event area " << long(g_object_get_data(G_OBJECT(w), "node")));

  int node = long(g_object_get_data(G_OBJECT(w), "node"));
  // int dirty = long(g_object_get_data(G_OBJECT(w), "dirty"));
  // if (dirty) {

    // clear background
  cairo_set_source_rgb(cr, 0., 0., 0.);
  cairo_paint(cr);

    // if applicable, draw highlighted area selected for list
  if (hl.end && hl.node == node) {
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
    cairo_rectangle(
      cr, hl.start,
      (current_logs[node].getNumLevels() - hl.level - 1) * LINESPACE + 2,
      hl.end - hl.start, LINESPACE);
    cairo_fill(cr);
  }

    // what part to draw depends on the scroll bar and the selected
    // view span
  GtkAdjustment *va = gtk_scrollbar_get_adjustment(
    GTK_SCROLLBAR(gui.window.getObject("viewscroll")));
  float tick_start = (ticks_per_sec * gtk_adjustment_get_value(va));
  float tick_end = tick_start + vspan * ticks_per_sec;

  DEB(getId() << classname << " scroll value " << gtk_adjustment_get_value(va)
              << " tick start " << tick_start << " tick end " << tick_end);

  if (current_logs[node].areTheLogsComplete()) {
    ActivityLister l = current_logs[node].startInvestigation(0);

    int ycoord = LINESPACE;
    unsigned winwidth = width;
    for (int prio = l.getMaxPrio(); prio--;) {
      ActivityLine lin =
        l.getNextActivity(prio, tick_start, tick_end, winwidth);
      while (lin.type != ActivityLine::Blank) {
        switch (lin.type) {
        case ActivityLine::Blank:
          break;
        case ActivityLine::Run:
          cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
          cairo_set_line_width(cr, 3.0);
          cairo_move_to(cr, lin.x0, ycoord - 6);
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
          cairo_move_to(cr, lin.x0, ycoord - 6);
          cairo_line_to(cr, lin.x0, ycoord);
          if (lin.x0 != lin.x1)
            cairo_line_to(cr, lin.x1, ycoord);
          cairo_stroke(cr);
        }
        DEB("line for level " << prio << " coord " << lin.x0 << ' ' << ycoord
                              << ' ' << lin.x1);
        lin = l.getNextActivity(prio, tick_start, tick_end, winwidth);
      }
      ycoord += LINESPACE;
    }

    g_object_set_data(G_OBJECT(w), "dirty", reinterpret_cast<gpointer>(0));
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

gboolean ActivityView::cbConfigure(GtkWidget *w, gpointer user_data)
{
  int node = long(g_object_get_data(G_OBJECT(w), "node"));
  // int dirty = int(g_object_get_data(G_OBJECT(w), "dirty"));

  DEB(getId() << " " << classname << " cbConfigure " << node);

  // check whether widget is large enough to accommodate all lines
  auto winheight = gtk_widget_get_height(w);
  if (current_logs[node].areTheLogsComplete() &&
      winheight <
        current_logs[node].getNumLevels() * LINESPACE + LINESPACE / 2) {
    gtk_widget_set_size_request(GTK_WIDGET(gui.canvas[node]), 400,
                                current_logs[node].getNumLevels() * LINESPACE +
                                  LINESPACE / 2);
  }

  // set dirty, so a redraw will actually be done
  g_object_set_data(G_OBJECT(w), "dirty", reinterpret_cast<gpointer>(1));

  // configure drawing contests, only works first time
  return TRUE;
}

void ActivityView::cbDrawAreaButtonPress(gint n_press, gdouble x, gdouble y,
                                         unsigned area)
{
  DEB(getId() << classname << " button press, at " << x << ',' << y);

  // figure out which of the drawing areas
  hlnew.node = area;

  // check manager number from y coordinate
  hlnew.level = int(current_logs[hlnew.node].getNumLevels() -
                    (y - LINESPACE / 2) / LINESPACE);

  // remember where button was pressed,
  hlnew.start = int(x);
  hlnew.end = 1;

  DEB(getId() << classname << " new hl set up, node=" << hlnew.node
              << " level=" << hlnew.level << " start=" << hlnew.start);
}

void ActivityView::cbDrawAreaButtonRelease(gint n_press, gdouble x, gdouble y,
                                           unsigned area)
{
  DEB(getId() << classname << " button release, at " << x << ',' << y);

  // check that release corresponds to press
  int node = area;
  int level =
    int(current_logs[node].getNumLevels() - (y - LINESPACE / 2) / LINESPACE);

  if (!hlnew.end || node != hlnew.node || level != hlnew.level ||
      hlnew.start == int(x)) {
    DEB(getId() << classname << " different node/level; " << node << ' '
                << level);
    hlnew.end = 0;
  }

  // define highlight area, regardless of moving backwards with mouse
  if (hlnew.start < int(x)) {
    hlnew.end = int(x);
  }
  else {
    hlnew.end = hlnew.start;
    hlnew.start = int(x);
  }

  GtkWidget *w = gui.canvas[node];
  auto winwidth = gtk_widget_get_width(w);
  // figure out start and end times.
  GtkAdjustment *va = gtk_scrollbar_get_adjustment(
    GTK_SCROLLBAR(gui.window.getObject("viewscroll")));
  float tick_start =
    ticks_per_sec * (gtk_adjustment_get_value(va) +
                     float(hlnew.start) / float(winwidth) * vspan);
  float tick_end = ticks_per_sec * (gtk_adjustment_get_value(va) +
                                    float(hlnew.end) / float(winwidth) * vspan);

  DEB(getId() << classname << " looking from " << tick_start << " to "
              << tick_end);

  // fill list with highlighted actions
  g_list_store_remove_all(gui.act_store);

  try {
    ActivityLister l = current_logs[node].startInvestigation(0);
    bool more = true;
    while (more) {

      ActivityStrings desc =
        l.getNextActivityDesc(level, tick_start, tick_end, more);
      g_list_store_append(gui.act_store, d_activity_info_new(desc));
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
  hl = hlnew;
  hlnew.end = 0;
  g_object_set_data(G_OBJECT(w), "dirty", reinterpret_cast<gpointer>(1));
  gtk_widget_queue_draw(w);
}

template <class T> class VirtualJoinWithStealing : public VirtualJoin<T>
{
public:
  /** Constructor. Essential to initialize data pointer to NULL */
  VirtualJoinWithStealing(DataReaderBase &r) :
    VirtualJoin<T>(r)
  {}

  /** Release a previous access */
  inline const void release(ChannelReadToken &token)
  {
    if (VirtualJoin<T>::data_ptr) {
      DataReaderBaseAccess::releaseAccessKeepData(token,
                                                  VirtualJoin<T>::data_ptr);
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
  graphene_rect_t alc;
  if (!gtk_widget_compute_bounds(gui.canvas[node_id], gui.canvas[node_id],
                                 &alc)) {
    /* DUECA UI.

       Cannot, for some reason, get the size of the current activity drawing
       canvas.
    */
  }
  else if (graphene_rect_get_height(&alc) <
           current_logs[node_id].getNumLevels() * LINESPACE + LINESPACE / 2) {
    gtk_widget_set_size_request(
      gui.canvas[node_id], 400,
      current_logs[node_id].getNumLevels() * LINESPACE + LINESPACE / 2);
    gtk_widget_queue_resize(gui.canvas[node_id]);
    DEB("Drawing area resize");
  }
  else {
    DEB("Queueing a redraw");
    gtk_widget_queue_draw(gui.canvas[node_id]);
  }
}

void ActivityView::cbSetupLabel(GtkSignalListItemFactory *f, GtkListItem *item,
                                gpointer user_data)
{
  auto label = gtk_label_new("");
  gtk_list_item_set_child(item, label);
}

void ActivityView::cbBindTick(GtkSignalListItemFactory *f, GtkListItem *item,
                              gpointer user_data)
{
  auto label = gtk_list_item_get_child(item);
  auto act = D_ACTIVITY_INFO(gtk_list_item_get_item(item));
  gtk_label_set_text(GTK_LABEL(label), act->act.tick);
}

void ActivityView::cbBindOffset(GtkSignalListItemFactory *f, GtkListItem *item,
                                gpointer user_data)
{
  auto label = gtk_list_item_get_child(item);
  auto act = D_ACTIVITY_INFO(gtk_list_item_get_item(item));
  gtk_label_set_text(GTK_LABEL(label), act->act.offset);
}

void ActivityView::cbBindTimestamp(GtkSignalListItemFactory *f,
                                   GtkListItem *item, gpointer user_data)
{
  auto label = gtk_list_item_get_child(item);
  auto act = D_ACTIVITY_INFO(gtk_list_item_get_item(item));
  gtk_label_set_text(GTK_LABEL(label), act->act.ts);
}

void ActivityView::cbBindDt(GtkSignalListItemFactory *f, GtkListItem *item,
                            gpointer user_data)
{
  auto label = gtk_list_item_get_child(item);
  auto act = D_ACTIVITY_INFO(gtk_list_item_get_item(item));
  gtk_label_set_text(GTK_LABEL(label), act->act.dt);
}

void ActivityView::cbBindModule(GtkSignalListItemFactory *f, GtkListItem *item,
                                gpointer user_data)
{
  auto label = gtk_list_item_get_child(item);
  auto act = D_ACTIVITY_INFO(gtk_list_item_get_item(item));
  gtk_label_set_text(GTK_LABEL(label), act->act.module);
}

void ActivityView::cbBindName(GtkSignalListItemFactory *f, GtkListItem *item,
                              gpointer user_data)
{
  auto label = gtk_list_item_get_child(item);
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  auto act = D_ACTIVITY_INFO(gtk_list_item_get_item(item));
  gtk_label_set_text(GTK_LABEL(label), act->act.activity);
}

DUECA_NS_END
