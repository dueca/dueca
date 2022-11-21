/* ------------------------------------------------------------------   */
/*      item            : GenericToken.cxx
        made by         : Rene' van Paassen
        date            : 980219
        category        : body file
        description     : The accessToken objects are used in conjunction
                          with the channel system. Channels need to know
                          who the objects are that access their data, so
                          a count can be made.
        changes         : 980211 Modified to have templated tokens,
                          that include the reference to the channel
                          end. Easier for the app developer: token =
                          lock+key
                          980512 Concentrating the creation of
                          ChannelEnds in the access tokens, adding
                          write access tokens. Realizes a distributed
                          factory!
                          010322 Added service routines to help the
                          derived/templated access tokens, helps cut
                          down the include tree
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include <boost/lexical_cast.hpp>
#include <Callback.hxx>
#include "GenericToken.hxx"
#include "UnifiedChannel.hxx"
#include "Activity.hxx"
#include "Exception.hxx"
#include "ChannelEndUpdate.hxx"
#include "ChannelManager.hxx"
#include <DataClassRegistry.hxx>
#include <DataSetConverter.hxx>
#include <SimTime.hxx>
#define W_CHN
#include <debug.h>
#include <dueca-conf.h>
#include <boost/lexical_cast.hpp>
#define DEBPRINTLEVEL -1
#include <debprint.h>


DUECA_NS_START

GenericToken::GenericToken(const GlobalId &holder, const NameSet& name,
                           const std::string& dataclassname) :
  TriggerPuller(std::string("Token(") + name.name + std::string("@") +
                boost::lexical_cast<std::string>(holder) +
                std::string(")")),
  holder(holder),
  local_name(name),
  dataclassname(dataclassname),
  converter(DataClassRegistry::single().getConverter(dataclassname)),
  magic_number(converter->getMagic())
{
  DEB("GenericToken created");
}

GenericToken::~GenericToken()
{
  // have to check this
}

void GenericToken::checkChannelEndPresent() const
{
  if (!channel) {
    throw(MissingChannelEnd(holder, holder));
  }
}

const GlobalId& GenericToken::getChannelId() const
{
  checkChannelEndPresent();
  return channel->getId();
}

boost::weak_ptr<DCOMetaFunctor>
GenericToken::getMetaFunctorBase(const std::string& fname) const
{
  return DataClassRegistry::single().getMetaFunctor(dataclassname, fname);
}


DUECA_NS_END
