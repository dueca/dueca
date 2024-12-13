/* ------------------------------------------------------------------   */
/*      item            : NetUseOverviewGtk4.hxx
        made by         : Rene van Paassen
        date            : 210420
        category        : header file
        description     :
        changes         : 210420 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef NetUseOverviewGtk4_hxx
#define NetUseOverviewGtk4_hxx

#include <udpcom/NetUseOverview.hxx>
#include <vector>
#include <gtk/gtk.h>
#include <GtkGladeWindow.hxx>

DUECA_NS_START;

class NetUseOverviewGtk4: public NetUseOverview
{
  /** self-define the module type, to ease writing the parameter table */
  typedef NetUseOverviewGtk4 _ThisModule_;

private: // simulation data
  /** glade file */
  std::string                        gladefile;

  /** gtk window */
  GtkGladeWindow                     window;

  /** Canvas for timing results */
  GtkWidget                         *timingcanvas;

  /** Label for largest time */
  GtkWidget                         *tlabel;

  /** Label for message set-up and per-byte speed */
  GtkWidget                         *sutlabel;

  /** Menu item */
  GAction                           *menuitem;

  /** Canvases for load results */
  std::vector<GtkWidget*>            loadcanvas;

  /** Latest timing log data */
  NetTimingLog                       timing_log;

  /** Latest use log data */
  std::vector<NetCapacityLog>        capacity_log;
  
public:
  /** Name of the module. */
  static const char* const           classname;

  /** Continued construction */
  bool complete() override;

public:
  /** Constructor */
  NetUseOverviewGtk4(Entity* e, const char* part, const
                     PrioritySpec& ps);

  /** Destructor */
  ~NetUseOverviewGtk4();

  /** update timing */
  void updateTiming(const NetTimingLog& data) override;

  /** update load */
  void updateLoad(const NetCapacityLog& data) override;

  /** Redraw stuff. */
  int cbDraw(GtkDrawingArea *w, cairo_t *cr, int width, int height);

  /** realize widget stuff. */
  int cbConfigure(GtkWidget *w, gpointer user_data);

  /** Close the window. */
  void cbClose(GtkButton* button, gpointer gp);

  /** Close the window. */
  gboolean deleteView(GtkWindow* win, gpointer gp);
};

DUECA_NS_END;

#endif
