/* ------------------------------------------------------------------   */
/*      item            : ChannelManager.hh
        made by         : Rene' van Paassen
        date            : 990617
        category        : header file
        description     :
        changes         : 990617 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ChannelManager_hh
#define ChannelManager_hh

#include "ChannelCountRequest.hxx"
#include "ChannelCountResult.hxx"
#include "ChannelMonitorRequest.hxx"
#include "ChannelMonitorResult.hxx"
#include "ChannelChangeNotification.hxx"
#include "ChannelEndUpdate.hxx"
#include "ChannelReadToken.hxx"
#include "ChannelWriteToken.hxx"
#include "ChannelIdList.hxx"
#include "ChannelOrganiser.hxx"
//#include "registry.hxx"
#include "vectorMT.hxx"
#include "Callback.hxx"
#include "Activity.hxx"
#include "StateGuard.hxx"
#include "NamedObject.hxx"
#include "ScriptCreatable.hxx"
#include <map>
#include <fstream>

#include <dueca_ns.h>
DUECA_NS_START

struct ChannelDistribution;
struct ParameterTable;
class UnifiedChannel;

/** This defines a singleton object, that manages all administrative
    data of the channels. The channelmanager in node 0, in addition,
    keeps the complete registry of channels. */
class ChannelManager: public ScriptCreatable,
                      public NamedObject
{
  /** Lock/guard */
  mutable StateGuard     guard;

  /** Pointer to the singleton. */
  static ChannelManager* singleton;

  /** Remember the number of this node. */
  LocationId location;

  /// Flag to indicate initialising, stand-alone phase.
  bool stand_alone;

  /** This is the next local channel id to be given out. Local channel
      ends can be made without reference to the central ChannelManager
      registry. */
  ObjectId local_channel_id_count;

  /// This is the next channel id to be given out
  ObjectId channel_id_count;

  /// temporary storage space for channels that await the global-id
  map<NameSet,UnifiedChannel*> channel_waitroom;

  /** Define a collection of information for the locally present
      channel ends */
  typedef vectorMT<ChannelIdList> channel_registry_type;

  /// A local registry containing id's of all channels known to me
  channel_registry_type channel_registry;

  /** Defines a collection of channel organisation information. */
  typedef vectorMT<ChannelOrganiser> channel_organisation_type;

  /** Channel organisation information; only used on node 0. */
  channel_organisation_type channel_organisation;

  /** An access token to request a change to a channel configuration.
      e.g. for new channels, additional users (read/write) of the
      channel. */
  ChannelWriteToken *channel_requests_w;

  /** An access token to read the updates given out by the central
      registry. */
  ChannelReadToken *channel_updates_r;

  /** An access token to read the change request. This is only used if
      this is the node 0 ChannelManager, the one with the registry. */
  ChannelReadToken *channel_requests_r;

  /** An access token to write the updates. */
  ChannelWriteToken *channel_updates_w;

  /** respond to channel count request */
  ChannelReadToken *r_countreq;

  /** send channel count results */
  ChannelWriteToken *w_countres;

  /** respond to channel monitor request */
  ChannelReadToken *r_monitorreq;

  /** send channel monitor results */
  ChannelWriteToken *w_monitorres;

  /// First callback object
  Callback<ChannelManager> cb1;

  /// Second callback object
  Callback<ChannelManager> cb2;

  /// Second callback object
  Callback<ChannelManager> cb3;

  /// Second callback object
  Callback<ChannelManager> cb4;

  /** An activity that handles an incoming update. */
  ActivityCallback react_to_update;

  /** An activity that handles incoming change requests. Only used in
      node 0 ChanneManager. */
  ActivityCallback *react_to_change_request;

  /** Handle count requests */
  ActivityCallback react_to_countrequest;

  /** Handle monitor requests */
  ActivityCallback react_to_monitorrequest;

  /** react to updates channel valid */
  Callback<ChannelManager> updates_valid;

  /** react to requests channel valid */
  Callback<ChannelManager> requests_valid;

  /** A stream, for debugging purposes, on which the list of channels
      is written. */
  ofstream channel_dump;

  /** A flag to indicate that the next channel that is made is a local
      channel. */
  bool make_one_local;

  UnifiedChannel* service_channel;

private:

  /** The next channel made after this call is local. Note that making
      a channel locally is useless, with three exceptions. This is not
      for the general public! Do not use! */
  void nextIsLocalChannel();

  /** Connect a locally constructed channel end to the communications
      net. */
  void connectLocalChannel(int node);

public:
  /// Constructor.
  ChannelManager();

  /** Call to continue with completion of the object, called by the
      Scheme routine */
  bool complete();

  /** Type name information */
  const char* getTypeName();

  /** Obtain a pointer to the parameter table. */
  static const ParameterTable* getParameterTable();

  /// Destructor.
  ~ChannelManager();

  /** Call to complete the ChannelManager. Called by Environment. */
  void completeCreation();

public:
  // Makes this class callable from scheme.
  SCM_FEATURES_DEF ;

  /** Print to stream. */
  //std::ostream& print(std::ostream& o) const;

  /// Return a pointer to the singleton.
  static ChannelManager* const single();

private:
  /// Read configuration requests and process these.
  void handleChannelConfigurationRequest(const TimeSpec &t);

  /// Read update notifications for the channel registry and process these.
  void handleChannelRegistryUpdate(const TimeSpec &t);

  /** Count requests */
  void handleCountRequests(const TimeSpec &t);

  /** Monitor requests */
  void handleMonitorRequests(const TimeSpec &t);

public:
  /// respond to an id request from a new channel end.
  void requestId(UnifiedChannel *chn, const NameSet& name_set);

  /** Check in a writing end for a channel, done when channel
      transport not configured yet. */
  void reportWritingEnd(const GlobalId& end_id,
                        const NameSet &name_set,
                        Channel::TransportClass tclass);

  /// Find a channel, based on its name
  UnifiedChannel *findChannel(const NameSet &name_set) const throw();

  /** Find a channel, or create a local end if it not existed */
  UnifiedChannel *findOrCreateChannel(const NameSet & name_set);

  /// Get a channel reference based on id
  UnifiedChannel *getChannel(const GlobalId &id) const;

  /// Get a channel reference based on id
  UnifiedChannel *getChannel(const ObjectId id) const;

  /** Obtain the name of a channel, for channels defined locally. */
  const NameSet& getNameSet(const ObjectId id) const;

  /** Obtain the name of a channel, channnels defined globally, works
      only in node 0. */
  const NameSet& getGlobalNameSet(const ObjectId id) const;

  /** Forgot this. */
  void invalidateId(UnifiedChannel *chn, GlobalId& id);

  /** Return the object type, member of dueca. */
  ObjectType getObjectType() const {return O_Dueca;}

  /** Determine whether a certain channel, given by an id, has a local
      end here. */
  bool channelHasLocalEnd(ObjectId channel_id) const;

private:
  /** Callback, when the updates information channel becomes valid */
  void updatesChannelValid(const TimeSpec& ts);

  /** Callback, when the requests channel becomes valid */
  void requestsChannelValid(const TimeSpec& ts);

private:
  /** copy constructor deleted */
  ChannelManager(const ChannelManager& o);

  /** assignment deleted */
  ChannelManager& operator = (const ChannelManager& o);

};


DUECA_NS_END

PRINT_NS_START
//inline ostream& operator << (ostream& o, const DUECA_NS::ChannelManager& m)
//{ return m.print(o); }
PRINT_NS_END

#endif
