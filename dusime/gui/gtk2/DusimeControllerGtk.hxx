/* ------------------------------------------------------------------   */
/*      item            : DusimeController.hxx
        made by         : Rene' van Paassen
        date            : 001010
        category        : header file
        description     :
        changes         : 001010 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DusimeControllerGtk_hh
#define DusimeControllerGtk_hh

#include "DusimeController.hxx"
#include "GtkGladeWindow.hxx"
#include <dueca_ns.h>

DUECA_NS_START

struct ParameterTable;

/** This is a definition of a singleton object that controls the
    Dusime modules (HardwareModule, SimulationModule) in a
    dueca/dusime system. This controller accepts signals from the
    interface (button presses etc.) and communicates with the
    different modules. */
class DusimeControllerGtk : public DusimeController
{
  /** Button to select inactive state. */
  GtkWidget* inactive;
  /** Button to select holdcurrent state. */
  GtkWidget* holdcurrent;
  /** Button to select calibrate state. */
  GtkWidget* calibrate;
  /** Button to select advance state. */
  GtkWidget* advance;
  /** Button to select replay state. \todo This button and the
      associated state are not yet implemented. */
  GtkWidget* replay;
  /** Snapshot button. */
  GtkWidget* snap;

public:
  /** Constructor, called from scheme as a standard module. */
  DusimeControllerGtk(Entity* e, const char* part,
                      const PrioritySpec& ps);

  /** Destructor. */
  ~DusimeControllerGtk();

  /** name of the class, as known to scheme. */
  static const char* const       classname;

public:
  /** Tell that I am prepared to run. */
  bool isPrepared();

  /** A method used by the DuecaView module, to inform me of the
      widget under my control. */
  void yourWidgets(GtkGladeWindow &gwin);

private:
  /** Pressed on replay button. */
  gboolean cbReplay(GtkWidget *widget, GdkEventButton *event,
                    gpointer user_data);

  /** Pressed on  button. */
  gboolean cbCalibrate(GtkWidget *widget, GdkEventButton *event,
                    gpointer user_data);

  /** Pressed on  button. */
  gboolean cbInactive(GtkWidget *widget, GdkEventButton *event,
                    gpointer user_data);

  /** Pressed on  button. */
  gboolean cbHoldCurrent(GtkWidget *widget, GdkEventButton *event,
                    gpointer user_data);

  /** Pressed on  button. */
  gboolean cbAdvance(GtkWidget *widget, GdkEventButton *event,
                    gpointer user_data);

  /** Snapshot send. */
  void cbSnapShot(GtkButton* button, gpointer gp);

  /** Have any GUIs refresh their view of the entities in DUECA */
  void refreshEntitiesView();

  /** update the view of all buttons */
  void refreshButtonState(const SimulationState& tstate);
};
DUECA_NS_END
#endif
