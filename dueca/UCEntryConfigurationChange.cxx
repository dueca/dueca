/* ------------------------------------------------------------------   */
/*      item            : UCEntryConfigurationChange.cxx
        made by         : Rene' van Paassen
        date            : 231103
        category        : body file
        description     :
        changes         : 231103 first version
        language        : C++
        copyright       : (c) 23 TUDelft-AE-C&S
*/

#include "UCEntryConfigurationChange.hxx"

DUECA_NS_START;

EntryConfigurationChange::EntryConfigurationChange() :
  changetype(Sentinel),
  tocheck(0U),
  entry(nullptr),
  next(nullptr)
{ }

void EntryConfigurationChange::setData(ConfigEvent ct, unsigned nreaders, 
                                      UChannelEntryPtr entry)
{
  changetype = ct;
  tocheck = nreaders;
  this->entry = entry;
}

EntryConfigurationChangePtr EntryConfigurationChange::markHandled()
{
  tocheck--;
  return next;
}

void EntryConfigurationChange::insert(EntryConfigurationChangePtr toinsert)
{
  toinsert->next = next;
  next = toinsert;
}

DUECA_NS_END;
