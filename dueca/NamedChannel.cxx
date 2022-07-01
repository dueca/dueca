/* ------------------------------------------------------------------   */
/*      item            : NamedChannel.cxx
        made by         : Rene' van Paassen
        date            : 990824
        category        : body file
        description     :
        changes         : 990824 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define NamedChannel_cc
#include "NamedChannel.hxx"
#include "NameSet.hxx"
#include "ChannelManager.hxx"
DUECA_NS_START

NamedChannel::NamedChannel(const NameSet& nameset) :
  my_id(),
  local(nameset)
{
  //
}

NamedChannel::~NamedChannel()
{
  // nothing in particular
}

const GlobalId& NamedChannel::getId() const
{
  return my_id;
}

const std::string NamedChannel::getEntity() const
{
  return getNameSet().getEntity();
}


const std::string NamedChannel::getClass() const
{
  return getNameSet().getClass();
}


const std::string NamedChannel::getPart() const
{
  return getNameSet().getPart();
}

const NameSet& NamedChannel::getNameSet() const
{
  return local;
}

void NamedChannel::setId(const GlobalId& gid)
{
  my_id = gid;
}

DUECA_NS_END
