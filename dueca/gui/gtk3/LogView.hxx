/* ------------------------------------------------------------------   */
/*      item            : LogView.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Fri Dec 15 12:45:47 2006
        category        : header file
        description     :
        changes         : Fri Dec 15 12:45:47 2006 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef LogView_hxx
#define LogView_hxx

// include the dusime header
#include <dueca.h>

// This includes headers for the objects that are sent over the channels
#include <LogMessage.hxx>
#include <LogPoint.hxx>
#include <LogLevelCommand.hxx>

// include headers for functions/classes you need in the module
#include "LogViewGui.hxx"
#include <fstream>

DUECA_NS_START

/** This module assembles error/warning etc. log messages from all
    over the dueca process, and shows these in a window.

    \verbinclude log-view.scm
 */
class LogView: public Module
{
private: // simulation data
  /** Gui for viewing log. */
  LogViewGui                           gui;

  /** To remember whether this was opened. */
  bool                                 opened;

  /** Flag to pause writing lines. */
  bool                                 is_paused;

  /** Number of lines that is being remembered when pausing. */
  unsigned                             max_stacked;

  /** Number of lines shown in the gui. */
  int                                  n_lines;

  /** File for dumping message logs. */
  ofstream                             message_log;

private: // channel access
  /** Callback on token completion */
  Callback<LogView>                        token_valid;

  /** Function on token completion. */
  void tokenValid(const TimeSpec& ts);

  /** Flag to remember token completion. */
  bool token_action;

  /** Read log messages. */
  ChannelReadToken                     r_message;
  //EventChannelReadToken<LogMessage>    r_message;

  /** Read log messages, again. for the file. */
  ChannelReadToken                     r_message2;
  //  EventChannelReadToken<LogMessage>    r_message2;

  /** Write log levels. */
  ChannelWriteToken                    w_level;
  //EventChannelWriteToken<LogLevelCommand>  w_level;

private: // activity allocation
  /** Callback object for log writing. */
  Callback<LogView>                    cb1, cb2;

  /** Activity for simulation calculation. */
  ActivityCallback                     do_calc;

  /** Activity for simulation calculation. */
  ActivityCallback                     do_file;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const           classname;

  /** Return the parameter table. */
  static const ParameterTable*       getParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  LogView(Entity* e, const char* part, const PrioritySpec& ts);

  /** Continued construction, opens the gui. */
  bool complete();

  /** Destructor. */
  ~LogView();

public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared();

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time);

public: // the member functions that are called for activities
  /** the method that implements the main calculation. */
  void doCalculation(const TimeSpec& ts);

  /** the method that implements the main calculation. */
  void doPrint(const TimeSpec& ts);

  /** Pause logging, if possible. */
  void pause(bool do_pause);

  /** Set the logging level for a certain node and message category. */
  void setLevel(const LogCategory* cat, int node, const char* level_as_text);
};

DUECA_NS_END
#endif
