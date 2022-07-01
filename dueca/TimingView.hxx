/* ------------------------------------------------------------------   */
/*      item            : TimingView.hxx
        made by         : Rene van Paassen
        date            : 020225
        category        : header file
        description     :
        changes         : 020225 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        api             : DUECA_API
*/

#ifndef TimingView_hxx
#define TimingView_hxx

#ifdef TimingView_cxx
#endif

#include "dueca.h"
#include <TimingResults.hxx>
#include <SyncReportRequest.hxx>
#include <SyncReport.hxx>
#include <fstream>

DUECA_NS_START

/** This is a module that generates a summary of the timing results of
    any instrumented activities. This module may be created from the
    DUECA model script in dueca.mod, so normally you would put this in
    your dueca.mod script, like:

    \code
    (define dueca-internal
     (make-entity "dueca"
               (if (eq? this-node-id 0)
                (list
                (make-module 'dusime "" admin-priority)
                (make-module 'dueca-view "" admin-priority)
                (make-module 'activity-view "" admin-priority)
                (make-module 'timing-view "" admin-priority)
                ) () )
               ) )
    \endcode

    Note the order in which the dueca/dusime objects are
    defined. Currently, "dusime" needs to be present before dueca-view
    is made. ActivityView and TimingView use DuecaView to put
    themselves in the "View" menu. TimingView in addition requires
    ActivityView to get information about the names of different
    activities.

    Use the name "dueca" for your dueca entity, or the TimingView will
    not be able to correctly connect its channel.
*/
class TimingView: public Module
{
public:

  /** Name of the module class */
  static const char* const       classname;

  /** Table with adjustable parameters. */
  static const ParameterTable*   getParameterTable();

private:
  /** singleton pointer. */
  static TimingView*             singleton;

  /** Callback on token completion */
  Callback<TimingView>                        token_valid;

  /** Function on token completion. */
  void tokenValid(const TimeSpec& ts);

protected:
  /** Flag to remember token completion. */
  bool token_action;

  /// Access token for a channel with the incoming reports
  ChannelReadToken                      result_channel;

  /** Access token for a channel with which to send requests for sync
      data. */
  ChannelWriteToken                     request_report;

  /** Channel which gets back the reports. */
  ChannelReadToken                      result_report;

  /** Callback objects. \{ */
  Callback<TimingView>           cb1, cb2;  /// \}

  /** Activity, basically getting timing reports over the channel. */
  ActivityCallback               read_report;

  /** Second activity, for getting and processing sync reports. */
  ActivityCallback               read_sync;

  /** Flag to indicate all conditions OK. */
  bool                           can_start;

  /** Counter to remember the number of rows in the view. */
  int                            num_rows;

  /** Remember the number of nodes here. */
  int                            no_nodes;

  /** File for dumping timing logs. */
  std::ofstream                       timing_log;

public:
  /** Constructor, follows standard module construction form. */
  TimingView(Entity* e, const char* part, const PrioritySpec& ps);

  /** Destructor. */
  ~TimingView();

  /** Start the TimingView module. */
  void startModule(const TimeSpec& time);

  /** Stop the module. */
  void stopModule(const TimeSpec& time);

  /** Will be prepared when the channel is ready. */
  bool isPrepared();

  /** Return the singlton. */
  inline static TimingView* single() { return singleton; }

  /** Write a sync report to the interface */
  virtual void updateSync(int node, const SyncReport& report) = 0;

  /** Write a timing report on the interface. */
  virtual void appendReport(const std::string& maker_and_act,
                            const TimeTickType& tstart,
                            const TimingResults& data) = 0;

private:
  /** Read and process a report from the channel. */
  void readReport(const TimeSpec& ts);

  /** Read and process sync reports. */
  void readSync(const TimeSpec& ts);
};

DUECA_NS_END
#endif
