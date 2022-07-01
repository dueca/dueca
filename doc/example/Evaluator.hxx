/* ------------------------------------------------------------------   */
/*      item            : Evaluator.hxx
        made by         : Rene' van Paassen
        date            : 001214
        category        : header file
        description     :
        changes         : 001214 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Evaluator_hh
#define Evaluator_hh

#ifdef Evaluator_cc
#endif

#include <libguile.h>
#include <gtk/gtk.h>
#include <dusime.h>
#include "SpacePlaneY.hxx"
#include "DisplaySelect.hxx"
#include <Snapshot.hxx>
#include "Variance.hxx"
#include "Interpolator.hxx"

class DirectPacker;

class Evaluator: public SimulationModule
{
  StreamChannelReadToken<SpacePlaneY>            input;
  EventChannelReadToken<DisplaySelect>           display_select;
  GtkWidget                                      *window;
  GtkEntry                                       *h_altitude, *h_sinkrate,
                                                 *h_sigma_y_1, *h_sigma_z_1,
                                                 *h_sigma_y_2, *h_sigma_z_2,
                                                 *h_sigma_y_3, *h_sigma_z_3,
                                                 *h_delta_y, *h_delta_z;
  bool                                           is_reset;
  Variance                                       v_initialy, v_initialz,
                                                 v_flarey, v_flarez,
                                                 v_finaly, v_finalz;
  int                                            tunnel;
  static const unsigned int                      NTUNNELS;
  vector<Interpolator>                           path;
  vector<double>                                 x_flare, x_final;
  double                                         z_stop;
  float                                          altitude, sinkrate,
                                                 present_x,
                                                 delta_y, delta_z;
  Callback<Evaluator>                            cb, cb2;
  ActivityCallback                               keep_up;
  ActivityCallback                               feedback;
  bool                                           feedback_timing_set;

public:
  static const char* const                       classname;
  static const ParameterTable*                   getMyParameterTable();

public:
  Evaluator(Entity* e, const char* part, const PrioritySpec& ts);

  /// destructor
  ~Evaluator();

  /// further specification, update rate
  bool isPrepared();

  /// start responsiveness to input data
  void startModule(const TimeSpec &time);

  /// stop responsiveness to input data
  void stopModule(const TimeSpec &time);

  /// specify prediction time
  bool setFile(const vstring& f);

  /// specify rate at which display is updated
  bool setFeedbackRate(const TimeSpec& ts);

  bool setFlare(const double& f);
  bool setFinal(const double& f);

private:

  /// the method that implements the main calculation
  void doCalculation(const TimeSpec& ts);
  void feedBack(const TimeSpec& ts);

protected:

  /// restoring state from snapshot
  void loadSnapshot(const TimeSpec& t, const Snapshot& snap);
};


#endif
