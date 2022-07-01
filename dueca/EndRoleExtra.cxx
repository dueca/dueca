/* ------------------------------------------------------------------   */
/*      item            : EndRoleExtra.cxx
        made by         : Rene' van Paassen
        date            : 1301002
        category        : additional body code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110

EndRole operator | (const EndRole& e1, const EndRole& e2)
{
  if (e2.t == EndRole::None || e1.t == e2.t) return EndRole(e1);
  if (e1.t == EndRole::None) return EndRole(e2);
  return EndRole(EndRole::SendingReceiving);
}

EndRole operator & (const EndRole& e1, const EndRole& e2)
{
  if (e2.t == EndRole::SendingReceiving || e1.t == e2.t)
    return EndRole(e1);
  if (e1.t == EndRole::SendingReceiving) return EndRole(e2);
  return EndRole(EndRole::None);
}


const char* const EndRole::getString() const
{
  return DUECA_NS::getString(this->t);
}
