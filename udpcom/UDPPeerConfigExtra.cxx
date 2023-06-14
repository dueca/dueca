/* ------------------------------------------------------------------   */
/*      item            : UDPPeerConfigExtra.cxx
        made by         : Rene' van Paassen
        date            : 170205
        category        : Additional code for UDPPeerConfig
        description     :
        changes         : 170205 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// compatibility with build generation
#define __CUSTOM_COMPATLEVEL_110
#define __CUSTOM_COMPATLEVEL_111

#define __CUSTOM_AMORPHRESTORE_CONSTRUCTOR
UDPPeerConfig::UDPPeerConfig(dueca::AmorphReStore& s) :
  mtype(MessageType(uint8_t(s))),
  peer_id(peer_id_t(0)),
  target_cycle(0U)
{
  if (mtype <= ConfigurePeer) {
    ::unPackData(s, this->peer_id);
  }
  if (mtype <= DeletePeer) {
    ::unPackData(s, this->target_cycle);
  }
  DOBS("amorph constructor UDPPeerConfig");
}

#define __CUSTOM_FUNCTION_UNPACKDATA
void UDPPeerConfig::unPackData(::dueca::AmorphReStore& s)
{
  ::unPackData(s, this->mtype);
  if (mtype <= ConfigurePeer) {
    ::unPackData(s, this->peer_id);
  }
  if (mtype <= DeletePeer) {
    ::unPackData(s, this->target_cycle);
  }
  DOBS("unPackData UDPPeerConfig");
}

#define __CUSTOM_FUNCTION_PACKDATADIFF
void UDPPeerConfig::unPackDataDiff(dueca::AmorphReStore& s)
{
  // not a real unpackdatadiff
  this->unPackData(s);
}

#define __CUSTOM_FUNCTION_UNPACKDATADIFF
void UDPPeerConfig::packDataDiff(::dueca::AmorphStore& s, const UDPPeerConfig& ref) const
{
  // not a real packdiff
  this->packData(s);
}

#define __CUSTOM_FUNCTION_PACKDATA
void UDPPeerConfig::packData(::dueca::AmorphStore& s) const
{
  DOBS("packData UDPPeerConfig");
  ::packData(s, this->mtype);
   if (mtype <= ConfigurePeer) {
     ::packData(s, this->peer_id);
   }
   if (mtype <= DeletePeer) {
     ::packData(s, this->target_cycle);
  }
}
