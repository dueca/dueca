/* ------------------------------------------------------------------   */
/*      item            : AccessToken.hh
        made by         : Rene' van Paassen
        date            : 980205
        category        : header file
        description     : The accessToken objects are used in conjunction
                          with the channel system. Channels need to know
                          who the objects are that access their data, so
                          a count can be made.
        changes         : 980211 Modified to have templated tokens,
                          that include the reference to the channel
                          end. Easier for the app developer: token =
                          lock+key
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef GenericToken_hh
#define GenericToken_hh

#include "GlobalId.hxx"
#include "ChannelDistribution.hxx"
#include <EndRole.hxx>
#include "Trigger.hxx"
#include "TransportClass.hxx"
#include "NameSet.hxx"
#include "ChannelDef.hxx"
#include <DCOMetaFunctor.hxx>
#include <boost/weak_ptr.hpp>
#include <DataClassRegistryPredef.hxx>

#include <dueca_ns.h>
DUECA_NS_START
class UnifiedChannel;
class ScheduleCondition;
class DataSetConverter;
class EventConverter;
struct NameSet;
class GenericCallback;

/** The accessToken objects are used in conjunction with the channel
    system. When a component requests access to a channel, an
    accessToken is created and returned to the component.

    For the component, the accessToken acts as a key. Using the
    accessToken on the channel, access to the data on the channel can
    be got.

    For the channel, the accessToken acts as an identifier. It
    contains the sequence number of the component, as used in the
    channel object, and a GlobalId of the component. These are used
    for accounting purposes */
class GenericToken: public TriggerPuller
{
private:
  /** ID pointing to the creator of the token. */
  GlobalId holder;

  /** Local name copy */
  NameSet local_name;

protected:
  /** name of the data type */
  std::string dataclassname;

  /** Converter for the data */
  const DataSetConverter* converter;

  /** magic number for the data class name claimed here */
  uint32_t magic_number;

  /** pointer to the channel */
  UnifiedChannel* channel;

private:
  /** Copy constructor. Private and not implemented/not used. */
  GenericToken(GenericToken& tk);

  /** Assignment. Likewise, not implemented. */
  GenericToken& operator = (const GenericToken& );

protected:
  /** Check preconditions. */
  void checkChannelEndPresent() const;

public:

  /** Return the ID of the owner. */
  inline const GlobalId& getTokenHolder() const { return holder; };

  /** Return the ID of the channel. */
  const GlobalId& getChannelId() const;

  /** Return the local name specified for the token */
  inline const NameSet& getName() const { return local_name; }

  /** Return the data class name */
  inline const std::string& getDataClassName() const { return dataclassname; }

  /** Access the data class magic */
  inline uint32_t getDataClassMagic() const { return magic_number; }

public:
  /** Destructor. */
  virtual ~GenericToken();

  /** Constructor. Creates an access point to a channel.
      @param holder    Identification of the creating client
      @param name      Name of the channel to connect to
      @param dataclassname Type of data to be written/read.
  */
  GenericToken(const GlobalId &holder, const NameSet& name,
               const std::string& dataclassname);

public:
  /** Returns true if the token is valid (and can thus be used).
      Note that tokens have to be checked before use.
      \returns  true if the token is ready for use, false otherwise. */
  virtual bool isValid() = 0;

private:
  /** Obtain the base DCOMetaFunctor
      @param fname Functor type name
      @throws      UndefinedFunctor  If there is no metafunctor of the
                   specified type */
  boost::weak_ptr<DCOMetaFunctor>
  getMetaFunctorBase(const std::string& fname) const;

public:

  /** Obtain a specific metafunctor for interaction with channel data.

      Interaction with channel data -- e.g., reading -- can be done in
      several ways. The "normal" mode is having access to the data
      class and using a DataReader. If you don't have, or don't want
      to, program code that uses a specific data class, typically for
      generic modules, you can use functors. The functionality for the
      functor must be coded for the DCO object, examples are stuff
      like "convert this DCO object into JSON". Look up the
      description in @ref dueca::DCOMetaFunctor for more details. This call
      returns a weak reference to a meta-functor that can be used to
      create functors. A created functor can then be used wih the
      applyFunctor call. The "right" functor, for the specific
      data type accessed by this read token, is automatically
      selected.

      @param fname Functor type name
      @throws      UndefinedFunctor If there is no metafunctor of the
                   specified type
      @throws      FunctorTypeMismatch If the metafunctor cannot be cast
                   to the requested type
      @returns     Reference to a metafunctor */
  template <class MFT>
  boost::weak_ptr<MFT> getMetaFunctor(const std::string& fname) const
  {
    boost::weak_ptr<DCOMetaFunctor> dcofnc = getMetaFunctorBase(fname);
    boost::weak_ptr<MFT> res = boost::dynamic_pointer_cast<MFT>(dcofnc.lock());
    if (res.expired()) {
      throw FunctorTypeMismatch();
    }
    return res;
  }
};

DUECA_NS_END
#endif
