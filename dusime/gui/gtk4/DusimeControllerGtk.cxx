/* ------------------------------------------------------------------   */
/*      item            : DusimeControllerGtk.cxx
        made by         : Rene' van Paassen
        date            : 001010
        category        : body file
        description     :
        changes         : 001010 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define DusimeControllerGtk_cc
#include "GtkDuecaButtons.hxx"
#include "DusimeControllerGtk.hxx"
#include "ParameterTable.hxx"
#define DO_INSTANTIATE
#include "Callback.hxx"
#include "EventAccessToken.hxx"
#include "Event.hxx"
#include <Ticker.hxx>
#include <cmath>
#include <EventReader.hxx>
#include <EventWriter.hxx>
#include <EntityManager.hxx>
#include <GtkDuecaView.hxx>
#include <MemberCall.hxx>
#include <VarProbe.hxx>

#include <debug.h>

DUECA_NS_START

const char* const DusimeControllerGtk::classname = "dusime";

DusimeControllerGtk::DusimeControllerGtk(Entity* e, const char* part,
                                   const PrioritySpec& ps) :
  DusimeController(e, part, ps),
  inactive(NULL),
  holdcurrent(NULL),
  calibrate(NULL),
  advance(NULL),
  replay(NULL),
  snap(NULL)
{
  //
}


DusimeControllerGtk::~DusimeControllerGtk()
{
  //
}

bool DusimeControllerGtk::isPrepared()
{
  if (inactive == NULL && use_gui) {

    // access the window's widgets
    GtkGladeWindow &gwin = GtkDuecaView::single()->accessMainView();

    // get quick widget refs
    inactive = gwin["inactive"];
    holdcurrent = gwin["holdcurrent"];
    calibrate = gwin["hw_calibrate"];
    advance = gwin["advance"];
    replay = gwin["replay"];
    snap = gwin["snap"];

    // check that our buttons are there, might be different interface
    if (!inactive || !holdcurrent || !calibrate ||
        !advance || !replay) {
      /* DUSIME UI.

         Cannot find gui buttons in GTK window code, check that you
         have a proper glade / GtkBuilder file specified for the main
         DUECA window */
      E_CNF(getId() << '/' << classname << " Cannot get gui buttons");
      return false;
    }

    // connect callbacks
    GladeCallbackTable cb_links[] = {
      { "inactive",     "button_release_event",
        gtk_callback(&DusimeControllerGtk::cbInactive) },
      { "holdcurrent",     "button_release_event",
        gtk_callback(&DusimeControllerGtk::cbHoldCurrent) },
      { "hw_calibrate",     "button_release_event",
        gtk_callback(&DusimeControllerGtk::cbCalibrate) },
      { "replay",     "button_release_event",
        gtk_callback(&DusimeControllerGtk::cbReplay) },
      { "advance",     "button_release_event",
        gtk_callback(&DusimeControllerGtk::cbAdvance) },
      { "snap",        "clicked",
        gtk_callback(&DusimeControllerGtk::cbSnapShot) },
      { NULL, NULL, NULL, NULL } };
    gwin.connectCallbacks(reinterpret_cast<gpointer>(this), cb_links);

    // make them all inactive
    refreshButtonState(SimulationState::Neutral);
  }

  // now call parent's method
  return DusimeController::isPrepared();
}

void DusimeControllerGtk::refreshEntitiesView()
{
  if (use_gui)
    GtkDuecaView::single()->refreshEntitiesView();
}

void DusimeControllerGtk::refreshButtonState(const SimulationState& btn_state)
{
  // button list: inactive, holdcurrent, calibrate, advance, replay, snap
  static const gboolean button_sens[][6] = {
    // for HoldCurrent state
    { TRUE, TRUE, TRUE, TRUE, FALSE /* TRUE */, TRUE},
    // for Advance state
    { FALSE, TRUE, FALSE, TRUE, FALSE, TRUE},
    // for Replay state
    { FALSE, TRUE, FALSE, TRUE, TRUE, TRUE},
    // for Inactive state
    { TRUE, TRUE, FALSE, FALSE, FALSE, FALSE},
    // for Ina->Hold
    { FALSE, TRUE, FALSE, FALSE, FALSE, FALSE},
    // for Cal->Hold
    { FALSE, FALSE, TRUE, FALSE, FALSE, FALSE},
    // for Adv->Hold
    { FALSE, TRUE, FALSE, FALSE, FALSE, FALSE},
    // for Rep->Hold
    { FALSE, TRUE, FALSE, FALSE, FALSE, FALSE},
    // for Hold->Ina
    { TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},
    // for Neutral
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},
    // for Undefined
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},
    // for HoldCurrent without calibration
    { TRUE, TRUE, TRUE, FALSE, FALSE, FALSE}
    };


  static const int button_image[][5] = {
    { 0, 2, 0, 0, 0},     // for HoldCurrent state
    { 0, 0, 0, 2, 0},     // for Advance state
    { 0, 0, 0, 0, 2},     // Replay state
    { 2, 0, 0, 0, 0},     // Inactive
    { 0, 1, 0, 0, 0},     // Ina->Hold
    { 0, 0, 1, 0, 0},     // Cal->Hold
    { 0, 1, 0, 0, 0},     // Adv->Hold
    { 0, 1, 0, 0, 0},     // Rep->Hold
    { 1, 0, 0, 0, 0},     // Hold->Ina
    { 0, 0, 0, 0, 0},     // Neutral
    { 1, 1, 1, 1, 1}      // Undefined
  };

  static const int button_active[][5] = {
    { FALSE, TRUE, FALSE, FALSE, FALSE},
    { FALSE, FALSE, FALSE, TRUE, FALSE},
    { FALSE, FALSE, FALSE, FALSE, TRUE},
    { TRUE, FALSE, FALSE, FALSE, FALSE},
    { FALSE, TRUE, FALSE, FALSE, FALSE},
    { FALSE, TRUE, FALSE, FALSE, FALSE},
    { FALSE, TRUE, FALSE, FALSE, FALSE},
    { FALSE, TRUE, FALSE, FALSE, FALSE},
    { FALSE, FALSE, FALSE, FALSE, FALSE},
    { FALSE, FALSE, FALSE, FALSE, FALSE}
  };

  // don't act if not attached to gui
  if (not use_gui) return;

  // given the current confirmed state, set the button sensitivity
  int idx_sens =
    (not calibrated and btn_state == SimulationState::HoldCurrent) ?
    int(SimulationState::Undefined) + 1 : int(btn_state.get());

  gtk_widget_set_sensitive(inactive, button_sens[idx_sens][0]);
  gtk_widget_set_sensitive(holdcurrent, button_sens[idx_sens][1]);
  gtk_widget_set_sensitive(calibrate, button_sens[idx_sens][2]);
  gtk_widget_set_sensitive(advance, button_sens[idx_sens][3]);
  gtk_widget_set_sensitive(replay, button_sens[idx_sens][4]);
  gtk_widget_set_sensitive(snap, button_sens[idx_sens][5]);

  // given the current confirmed state, set the button image
  gtk_dueca_button_set_image
    (inactive, button_image[int(btn_state.get())][0]);
  gtk_dueca_button_set_image
    (holdcurrent, button_image[int(btn_state.get())][1]);
  gtk_dueca_button_set_image
    (calibrate, button_image[int(btn_state.get())][2]);
  gtk_dueca_button_set_image
    (advance, button_image[int(btn_state.get())][3]);
  gtk_dueca_button_set_image
    (replay, button_image[int(btn_state.get())][4]);

  // There is a single button active/pressed, others are not
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON(inactive), button_active[int(btn_state.get())][0]);
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON(holdcurrent), button_active[int(btn_state.get())][1]);
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON(calibrate), button_active[int(btn_state.get())][2]);
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON(advance), button_active[int(btn_state.get())][3]);
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON(replay), button_active[int(btn_state.get())][4]);

  // control the DuecaView buttons too
  DuecaView::single()->requestToKeepRunning
    (btn_state.get() != SimulationState::Inactive &&
     btn_state.get() != SimulationState::Neutral);
}

void DusimeControllerGtk::setReplayPrepared(bool replay_prepared)
{
  gtk_widget_set_sensitive(replay, replay_prepared ? TRUE : FALSE);
}

void DusimeControllerGtk::cbSnapShot(GtkButton* button, gpointer gp)
{
  this->DusimeController::takeSnapshot();
}

gboolean DusimeControllerGtk::cbReplay(GtkWidget *widget,
                                       GdkEventButton *event,
                                       gpointer user_data)
{
  if (event->type != GDK_BUTTON_RELEASE || event->button != 1)
    return TRUE;

  this->controlModel(SimulationState::Replay);
  return TRUE;
}


gboolean DusimeControllerGtk::cbCalibrate(GtkWidget *widget,
                                          GdkEventButton *event,
                                          gpointer user_data)
{
  if (event->type != GDK_BUTTON_RELEASE || event->button != 1)
    return TRUE;

  // try the state change. If good, set the button to transitioning
  this->controlModel(SimulationState::Calibrate_HoldCurrent);
  return TRUE;
}


gboolean DusimeControllerGtk::cbInactive(GtkWidget *widget,
                                         GdkEventButton *event,
                                         gpointer user_data)
{
  if (event->type != GDK_BUTTON_RELEASE || event->button != 1)
    return TRUE;

  // try the state change. If good, set the button to transitioning
  this->controlModel(SimulationState::Inactive);
  return TRUE;
}


gboolean DusimeControllerGtk::cbHoldCurrent(GtkWidget *widget,
                                            GdkEventButton *event,
                                            gpointer user_data)
{
  if (event->type != GDK_BUTTON_RELEASE || event->button != 1)
    return TRUE;

  this->controlModel(SimulationState::HoldCurrent);
  return TRUE;
}


gboolean DusimeControllerGtk::cbAdvance(GtkWidget *widget,
                                        GdkEventButton *event,
                                        gpointer user_data)
{
  if (event->type != GDK_BUTTON_RELEASE || event->button != 1)
    return TRUE;

  // try the state change. If good, set the button to transitioning
  this->controlModel(SimulationState::Advance);
  return TRUE;
}


DUECA_NS_END;
