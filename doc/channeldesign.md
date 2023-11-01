# Unified Channel

## Interaction stories

### Creation of a write entry

- constructor ChannelWriteToken
  * ChannelManager::single()->findOrCreateChannel
  * addWriteToken

- addWriteToken
  - creates a new `UChannelEntry`
  - on the entry, creates a new writer handle
  - the writer handle is added to a `writing_clients` list
  - a `UChannelCommRequest::NewEntryReq` is queued in `config_requests`

- UnifiedChannel::service()
  - processes or sends the `config_requests` to the ChannelMaster

- ChannelMaster::process()
  - checks the new request for consistency 
  - issues an id for the new entry
  - returns result in a `UChannelCommRequest::NewEntryConf` on the
    `config_changes` queue

- UnifiedChannel::updateConfiguration()
  - if needed, stretch the entries array
  - when remote, create the `UChannelEntry`
  - when local, find the created `UChannelEntry` from the `writing_clients` list
  - extend `latest_config_change` list with a `ReadConfigurationChange::NewEntry`
  - set the entry to be valid
  - find in the `entrymap`, or extend the `entrymap`, a UCDataclassLink objects for the dataclass name and all parent class.
  - For each entry in the entrymap, run over the read clients, check for a match between the client and the entry, and run:
    - Any callback functions installed for validity
    - A trigger link request for clients that want triggering
  - Set depth/span minima on this entry
  - process any cached data for the channel entry, i.e. data that may have come in from the sending end before the entry was valid. 
  - run watcher callbacks to inform about the new entry.

- UnifiedChannel::refreshClientHandle()
  - this is called whenever a read token tries to access data or metainfo
  - checks whether needed (new config) then calls `refreshClientHandleInner`

- UnifiedChannel::refreshClientHandleInner() (each read handle)
  - check if this entry needs to be read by the token (data compatible, etc.)
  - for the `latest_config_change` call `linkReadClientToEntry`
  - when any changes, further update the config_version

- UnifiedChannel::linkReadClientToEntry()
  - create an `UCEntryClientLink`
  - when sequential reading for this entry, call `latchSequentialRead`
  - when not sequential, update span and depth
  - add read entry to the UChannelEntry, `reportClient`
  - the entry can now be read with the token

### Creation of a read entry

- constructor ChannelReadToken
- 