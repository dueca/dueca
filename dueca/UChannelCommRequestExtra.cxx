/* ------------------------------------------------------------------   */
/*      item            : UChannelCommRequestExtra.cxx
        made by         : Rene' van Paassen
        date            : 140101
        category        : body file
        description     :
        changes         : 140101 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110

#define __CUSTOM_AMORPHRESTORE_CONSTRUCTOR
UChannelCommRequest::UChannelCommRequest(AmorphReStore& s):
        type(UChannelMessageType(uint8_t(s))),
        extra(0U),
        data0(s),
        data1(0)
{
  DOBS("custom amorph constructor UChannelCommRequest");
  if (type <= UChannelCommRequest::RemoveSaveupCmd) return;

  ::unPackData(s, this->data1);
  if (type <= UChannelCommRequest::TimeJump) return;

  ::unPackData(s, this->extra);
  ::unPackData(s, this->origin);
  ::unPackData(s, this->dataclassname);
  ::unPackData(s, this->entrylabel);
}

#define __CUSTOM_FUNCTION_UNPACKDATA
void UChannelCommRequest::unPackData(AmorphReStore& s)
{
  DOBS("custom unPackData UChannelCommRequest");
  ::unPackData(s, this->type);
  ::unPackData(s, this->data0);
  if (type <= UChannelCommRequest::RemoveSaveupCmd) return;

  ::unPackData(s, this->data1);
  if (type <= UChannelCommRequest::TimeJump) return;

  ::unPackData(s, this->extra);
  ::unPackData(s, this->origin);
  ::unPackData(s, this->dataclassname);
  ::unPackData(s, this->entrylabel);
}

#define __CUSTOM_FUNCTION_PACKDATA
void UChannelCommRequest::packData(AmorphStore& s) const
{
  DOBS("packData UChannelCommRequest");
  ::packData(s, this->type);
  ::packData(s, this->data0);
  if (type <= UChannelCommRequest::RemoveSaveupCmd) return;

  ::packData(s, this->data1);
  if (type <= UChannelCommRequest::TimeJump) return;

  ::packData(s, this->extra);
  ::packData(s, this->origin);
  ::packData(s, this->dataclassname);
  ::packData(s, this->entrylabel);
}

#define __CUSTOM_FUNCTION_PRINT
std::ostream & UChannelCommRequest::print(std::ostream& s) const
{
  s << "UChannelCommRequest("
    << "type=" << this->type
    << ",extra=" << std::hex << int(this->extra)
    << ",data0=" << this->data0
    << ",data1=" << this->data1 << std::dec
    << ",origin=" << this->origin
    << ",dataclassname=" << this->dataclassname
    << ",entrylabel=" << this->entrylabel
    << ')';
  return s;
}
