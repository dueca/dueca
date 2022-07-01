/* ------------------------------------------------------------------   */
/*      item            : ReplicatorConfigExtra.cxx
        made by         : Rene' van Paassen
        date            : 170205
        category        : Additional code for ReplicatorConfig
        description     :
        changes         : 170205 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// compatibility with build generation
#define __CUSTOM_COMPATLEVEL_110

#define __CUSTOM_AMORPHRESTORE_CONSTRUCTOR
ReplicatorConfig::ReplicatorConfig(dueca::AmorphReStore& s) :
  mtype(Undefined),
  slave_id(0),
  channel_id(0),
  entry_id(0),
  tmp_entry_id(0),
  name(),
  time_aspect(dueca::Channel::AnyTimeAspect),
  arity(dueca::Channel::ZeroOrMoreEntries),
  packmode(dueca::Channel::OnlyFullPacking),
  tclass(dueca::Channel::Regular),
  dataclass(),
  data_magic()
{
  unPackData(s);
  DOBS("amorph constructor ReplicatorConfig");
}

#define __CUSTOM_FUNCTION_UNPACKDATA
void ReplicatorConfig::unPackData(::dueca::AmorphReStore& s)
{
  ::unPackData(s, this->mtype);
  ::unPackData(s, this->slave_id);

  DOBS("unPackData ReplicatorConfig");
  switch(mtype) {
  case AddChannel:
    s.unPackData(name);
    s.unPackData(channel_id);
    break;
  case AddEntry: {
    s.unPackData(channel_id);
    s.unPackData(entry_id);
    s.unPackData(tmp_entry_id);
    s.unPackData(name);
    time_aspect = dueca::Channel::EntryTimeAspect(uint8_t(s));
    arity = dueca::Channel::EntryArity(uint8_t(s));
    packmode = dueca::Channel::PackingMode(uint8_t(s));
    tclass = dueca::Channel::TransportClass(uint8_t(s));
    ::dueca::unpackiterable(s, this->dataclass,
                            dueca::pack_traits<classlist_t >());
    ::dueca::unpackiterable(s, this->data_magic,
                            dueca::pack_traits<magiclist_t >());
  }
    break;
  case RemoveEntry:
    s.unPackData(channel_id);
    s.unPackData(entry_id);
    break;
  case RemoveChannel:
    s.unPackData(channel_id);
  case DeleteSlave:
  case InitialConfComplete:
  case Undefined:
  case HookUp:
  case ConfigureSlave:
    break;
  }
}

#define __CUSTOM_FUNCTION_PACKDATADIFF
void ReplicatorConfig::unPackDataDiff(dueca::AmorphReStore& s)
{
  // not a real unpackdatadiff
  this->unPackData(s);
}

#define __CUSTOM_FUNCTION_UNPACKDATADIFF
void ReplicatorConfig::packDataDiff(::dueca::AmorphStore& s, const ReplicatorConfig& ref) const
{
  // not a real packdiff
  this->packData(s);
}

#define __CUSTOM_FUNCTION_PACKDATA
void ReplicatorConfig::packData(::dueca::AmorphStore& s) const
{
  DOBS("packData ReplicatorConfig");
  ::packData(s, this->mtype);
  ::packData(s, this->slave_id);

  DOBS("unPackData ReplicatorConfig");
  switch(mtype) {
  case AddChannel:
    s.packData(name);
    s.packData(channel_id);
    break;
  case AddEntry: {
    s.packData(channel_id);
    s.packData(entry_id);
    s.packData(tmp_entry_id);
    s.packData(name);
    s.packData(uint8_t(time_aspect));
    s.packData(uint8_t(arity));
    s.packData(uint8_t(packmode));
    s.packData(uint8_t(tclass));
    ::dueca::packiterable(s, this->dataclass,
                          dueca::pack_traits<classlist_t >());
    ::dueca::packiterable(s, this->data_magic,
                          dueca::pack_traits<magiclist_t >());
  }
    break;
  case RemoveEntry:
    s.packData(channel_id);
    s.packData(entry_id);
    break;
  case RemoveChannel:
    s.packData(channel_id);
  case DeleteSlave:
  case InitialConfComplete:
  case Undefined:
  case HookUp:
  case ConfigureSlave:
    break;
  }
}
