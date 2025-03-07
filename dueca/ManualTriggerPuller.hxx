/* ------------------------------------------------------------------   */
/*      item            : ManualTriggerPuller.hxx
        made by         : Rene van Paassen
        date            : 241209
        category        : header file
        description     :
        changes         : 241209 first version
        language        : C++
        copyright       : (c) 2024 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#pragma once

#include "Trigger.hxx"

DUECA_NS_START;
/** Manual/Custom triggering.

    A ManualTriggerPuller can be used to provide custom triggering from your
    module's code. Note that these are seldom needed, see the explanation
    given with the TimeWarp class for an example.
 */
class ManualTriggerPuller : public TriggerPuller
{

public:
  /** Constructor */
  ManualTriggerPuller(const std::string &name = std::string());

  /** Destructor */
  ~ManualTriggerPuller();

  /** Provide triggering activation for a specific time

      @param ts   Time span for which triggering should take place.
  */
  using TriggerPuller::pull;
};

DUECA_NS_END;