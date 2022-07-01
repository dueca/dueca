/* ------------------------------------------------------------------   */
/*      item            : RateReduction.hxx
        made by         : Rene' van Paassen
        date            : 001027
        category        : header file
        description     :
        changes         : 001027 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2

        WARNING         : This example has not been updated to current
                                 DUECA interfacing specifications
*/

#ifndef RateReduction_hh
#define RateReduction_hh

#ifdef RateReduction_cc
#endif

#include <dueca.h>
#include <PrimaryControls.hxx>
class PassiveManipulator;
class ParameterTable;

class RateReduction: public Module
{
  // output channels for the controls, and inco output channel
  StreamChannelWriteToken<PrimaryControls>         controls_out;
  StreamChannelReadToken<PrimaryControls>          controls_in;

  // activity
  Callback<RateReduction>                          cb1;
  ActivityCallback                                 reduce_rate;

public:
  const static ParameterTable*       getMyParameterTable();
  static const char* const           classname;

  /// constructor, should be called from scheme, via the ModuleCreator
  RateReduction(Entity* e, const char* part, const PrioritySpec& ts);

  /// destructor
  ~RateReduction();

  /// further specification, update rate
  bool setTiming(const TimeSpec& t);

public:
  /// start responsiveness to input data
  void startModule(const TimeSpec &time);

  /// stop responsiveness to input data
  void stopModule(const TimeSpec &time);

  /// check sanity of all data and report ok or not
  bool isPrepared();

private:

  /// the method that implements the main calculation
  void doStep(const TimeSpec& ts);
};




#endif
