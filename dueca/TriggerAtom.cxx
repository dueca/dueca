/* ------------------------------------------------------------------   */
/*      item            : TriggerAtom.cxx
        made by         : Rene' van Paassen
        date            : 141111
        category        : body file
        description     :
        changes         : 141111 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define TriggerAtom_cxx
#include "TriggerAtom.hxx"
#include "Trigger.hxx"

DUECA_NS_START;

TriggerAtom::TriggerAtom() :
  target(NULL),
  ts(0),
  id(0)
{

}


TriggerAtom::~TriggerAtom()
{

}

void TriggerAtom::propagate() const
{
  target->trigger(ts, id);
}

DUECA_NS_END;
