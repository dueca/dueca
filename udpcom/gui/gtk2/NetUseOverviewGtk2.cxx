/* ------------------------------------------------------------------   */
/*      item            : NetUseOverviewGtk2.cxx
        made by         : Rene' van Paassen
        date            : 210420
        category        : body file
        description     :
        changes         : 210420 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define NetUseOverviewGtk2_cxx
#include "NetUseOverviewGtk2.hxx"
#include <dueca/debug.h>
#include <dueca/NodeManager.hxx>
#include <dueca/gui/gtk2/GtkDuecaView.hxx>
#include <boost/lexical_cast.hpp>
#include <dueca/DuecaPath.hxx>
#include <sstream>
#include <iomanip>

#define DEBPRINTLEVEL -1
#include <debprint.h>

DUECA_NS_START;

const char* const NetUseOverviewGtk2::classname = "net-view";

NetUseOverviewGtk2::NetUseOverviewGtk2(Entity* e, const char* part,
                                       const PrioritySpec& ps) :
  NetUseOverview(e, part, ps),
  gladefile(DuecaPath::prepend("net_use.glade")),
  window(),
  timingcanvas(NULL),
  tlabel(NULL),
  sutlabel(NULL),
  menuitem(NULL),
  loadcanvas(),
  timing_log(),
  capacity_log()
{
  //
}


NetUseOverviewGtk2::~NetUseOverviewGtk2()
{
  //
}

static int cbDraw(GtkWidget *w, GdkEventExpose *event, gpointer av)
{ return reinterpret_cast<NetUseOverviewGtk2*>(av)->cbDraw(w, event); }
static int cbConfigure(GtkWidget *w, GdkEventConfigure *event, gpointer av)
{ return reinterpret_cast<NetUseOverviewGtk2*>(av)->cbConfigure(w, event); }

bool NetUseOverviewGtk2::complete()
{
  bool res = NetUseOverview::complete();
  if (!res) return res;

  static GladeCallbackTable cb_table[] = {
    { "close", "clicked",
      gtk_callback(&_ThisModule_::cbClose) },
    { NULL, NULL, NULL }
  };

  res = res && window.readGladeFile
    (gladefile.c_str(), "net_use_view",
     reinterpret_cast<gpointer>(this), cb_table);

  if (!res) {
    /* DUECA UI.

       Cannot find the glade file defining the network use GUI. Check
       DUECA installation and paths.
    */
    E_CNF("Failed to open net use overview " << gladefile);
    return res;
  }

  // access the timing canvas, and the box with canvases
  timingcanvas = window["timing_view"];
  g_object_set_data(G_OBJECT(timingcanvas), "node",
                    reinterpret_cast<gpointer>(-1));
  g_signal_connect(G_OBJECT(timingcanvas), "configure_event",
                     G_CALLBACK(DUECA_NS::cbConfigure),
                     reinterpret_cast<gpointer>(this));
  g_signal_connect(G_OBJECT(timingcanvas), "expose-event",
                   G_CALLBACK(DUECA_NS::cbDraw),
                   reinterpret_cast<gpointer>(this));
  gtk_widget_show(timingcanvas);
  GtkWidget *canvasbox = window["graph_box"];

  // create new canvases for the net use
  for (int ii = 0; ii < NodeManager::single()->getNoOfNodes(); ii++) {
    DEB("Creating use canvas for node " << ii);
    std::stringstream tttext;
    tttext << "<big>Histogram of the packet size sent by node "
           << ii << "</big>" << std::endl
           << "<span foreground=\"green\">green</span>/"
           << "<span foreground=\"red\">red</span> "
           << "indicates the size of regular data" << std::endl
           << "gray bars show the size with bulk data included.";
    GtkWidget *canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(canvas, 220, 80);
    g_object_ref(canvas);
    g_object_set_data(G_OBJECT(canvas), "node",
                        reinterpret_cast<gpointer>(ii));
    gtk_box_pack_start(GTK_BOX(canvasbox), canvas,
                       TRUE, FALSE, 2);
    g_signal_connect(G_OBJECT(canvas), "configure-event",
                     G_CALLBACK(DUECA_NS::cbConfigure),
                     reinterpret_cast<gpointer>(this));
    g_signal_connect(G_OBJECT(canvas), "expose-event",
                     G_CALLBACK(DUECA_NS::cbDraw),
                     reinterpret_cast<gpointer>(this));
    gtk_widget_set_tooltip_markup(canvas, tttext.str().c_str());
    gtk_widget_show(canvas);
    loadcanvas.push_back(canvas);
    capacity_log.push_back(NetCapacityLog(ii));
  }

  menuitem = GTK_WIDGET
    (GtkDuecaView::single()->requestViewEntry
     ("Net Use View", GTK_WIDGET(window["net_use_view"])));
  tlabel = window["maxtime_label"];
  sutlabel = window["sutlabel"];

  gtk_widget_hide(window["net_use_view"]);
  
  return res;
}

void NetUseOverviewGtk2::updateTiming(const NetTimingLog& data)
{
  DEB("Update timing " << data);
  timing_log = data;
  gtk_widget_queue_draw(timingcanvas);
}

void NetUseOverviewGtk2::updateLoad(const NetCapacityLog& data)
{
  DEB("Update load " << data);
  capacity_log[data.node_id] = data;
  gtk_widget_queue_draw(loadcanvas[data.node_id]);
}

int NetUseOverviewGtk2::cbDraw(GtkWidget* w, GdkEventExpose *event)
{
  int node = long(g_object_get_data(G_OBJECT(w), "node"));

  cairo_t *cr;
  cr = gdk_cairo_create (gtk_widget_get_window (w));

  DEB("draw event area " << node);

  const int y0 = 70;
  const int dy = -60;

  if (node == -1) {
    // timing view

    // clear background
    GtkAllocation alc; gtk_widget_get_allocation(w, &alc);
    cairo_set_source_rgb(cr, 0., 0., 0.);
    cairo_paint(cr);

    // draw bar graph
    //unsigned winwidth = alc.width;
    cairo_set_line_width(cr, 4.0);
    cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
    for (unsigned ii = 0; ii < timing_log.times.size(); ii++) {
      if (ii == capacity_log[node].total.size() - 2) {
        cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
      }
      cairo_move_to(cr, 5+ii*10, y0);
      cairo_line_to(cr, 5+ii*10, y0+dy*timing_log.histTime(ii));
      cairo_stroke(cr);
    }

    // set max time on label
    gtk_label_set_markup
      (GTK_LABEL(tlabel),
       (boost::lexical_cast<std::string>(timing_log.t_max) +
        std::string(" <span>[&#956;s]</span>")).c_str());

    // message setup time
    {
      std::stringstream str;
      str << std::fixed << std::setprecision(1)
          << timing_log.net_permessage
          << " [&#956;s] + " << std::setprecision(2)
          << timing_log.net_perbyte << " [&#956;s/byte]";
      gtk_label_set_markup
        (GTK_LABEL(sutlabel), str.str().c_str());
    }
  }
  else {

    // one of the net use views

    // clear background
    GtkAllocation alc; gtk_widget_get_allocation(w, &alc);
    cairo_set_source_rgb(cr, 0., 0., 0.);
    cairo_paint(cr);

    // draw in gray, background of total net use
    cairo_set_line_width(cr, 4.0);
    cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
    for (unsigned ii = capacity_log[node].total.size(); ii--; ) {
      cairo_move_to(cr, 15+ii*20, y0);
      cairo_line_to(cr, 15+ii*20, y0+dy*capacity_log[node].histTotal(ii));
      cairo_stroke(cr);
    }

    // draw in green/red, regular net use
    cairo_set_source_rgb(cr, 0.0, 0.9, 0.0);
    for (unsigned ii = 0; ii < capacity_log[node].total.size(); ii++) {
      if (ii == capacity_log[node].total.size() - 1) {
        cairo_set_source_rgb(cr, 0.9, 0.0, 0.0);
      }
      cairo_move_to(cr, 5+ii*20, y0);
      cairo_line_to(cr, 5+ii*20, y0+dy*capacity_log[node].histRegular(ii));
      cairo_stroke(cr);
    }
  }
  cairo_destroy(cr);
  return FALSE;
}

int NetUseOverviewGtk2::cbConfigure(GtkWidget* w, GdkEventConfigure *ev)
{
  //int node = long(g_object_get_data(G_OBJECT(w), "node"));

  DEB(getId() << " " << classname << " cbConfigure " << node);
  GtkAllocation alc; gtk_widget_get_allocation(w, &alc);

  // check whether widget is large enough 
  //if (alc.height < 80 || alc.width < 220) {
    gtk_widget_set_size_request(w, 220, 80);
    //}

  // create a new pixmap that has to fill the widget
  //  pixmap = gdk_pixmap_new
  // (gtk_widget_get_window(w), alc.width, alc.height, -1);
  //g_object_set_data(G_OBJECT(w), "pixmap",
  //                    reinterpret_cast<gpointer>(pixmap));

  return TRUE;
}


void NetUseOverviewGtk2::cbClose(GtkButton* button, gpointer gp)
{
  // do not do this directly, but go through the menu item. Also
  // updates the flag there + closes the window.
  g_signal_emit_by_name(G_OBJECT(menuitem), "activate", NULL);
}

DUECA_NS_END;
