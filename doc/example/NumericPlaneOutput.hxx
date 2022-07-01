/* ------------------------------------------------------------------   */
/*      item            : NumericPlaneOutput.hxx
        made by         : Rene' van Paassen
        date            : 001004
        category        : header file
        description     :
        changes         : 001004 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef NumericPlaneOutput_hh
#define NumericPlaneOutput_hh

#ifdef NumericPlaneOutput_cc
#endif

#include <dusime.h>
#include <stringoptions.h>
#include "SpacePlaneY.hxx"
#include "PrimaryControls.hxx"
#include "DisplaySelect.hxx"
#include <fstream>
#include <iostream>

/** provides numeric output of a (space) plane output vector */

class NumericPlaneOutput: public SimulationModule
{
  ofstream                                       out0, out1;
  vstring                                        basename;
  int                                            number;
  bool                                           header_written;
  int                                            display_plus_tunnel;

public:
  StreamChannelReadToken<PrimaryControls>        t_controls;
  StreamChannelReadToken<SpacePlaneY>            t_output;
  EventChannelReadToken<DisplaySelect>           t_display_type;

  Callback<NumericPlaneOutput>                   cb;
  ActivityCallback                               do_output;

public:
  static const char* const                       classname;
  static const ParameterTable*                   getMyParameterTable();

public:

  /// constructor, should be called from scheme, via the ModuleCreator
  NumericPlaneOutput(Entity* e, const char* part, const PrioritySpec& ts);

  /// destructor
  ~NumericPlaneOutput();

  /// further specification, output file
  bool setOutputFile(const vstring& f);

public:
  /// start responsiveness to input data
  void startModule(const TimeSpec &time);

  /// stop responsiveness to input data
  void stopModule(const TimeSpec &time);

  bool isPrepared();

  void loadSnapshot(const TimeSpec& ts, const Snapshot& snap);

private:
  /// check whether header has been written, and if needed, write one
  void checkHeader();

  /// actions to take when using a new output file
  void switchFiles();

  /// the method that implements the main calculation
  void doStep(const TimeSpec& ts);
};
#endif
