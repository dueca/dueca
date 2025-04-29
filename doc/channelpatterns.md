# Some Recipes for using DUECA Channels {#channelpatterns}

## Introduction

DUECA uses "channels" as a means to communicate between different parts of a simulation. Since a re-write (and the step in 2014 from the DUECA 0.x series to DUECA 2.x and above), these are implemented by the "UnifiedChannel" class. Its name suggests that these channels offer many possibilities, and that is true. While another [page](@ref dueca2) discusses the difference between the old DUECA channels and the new unified channel type, this page looks at the properties of the DUECA (unified) channels, and typical ways in which you may use these channels. 

## Events versus Stream

Channels are meant to transport simple C++ structs (the DCO objects, see the information on the [code generator](@ref codegenerator) ). A typical use of DCO objects in the context of flight simulation would be a DCO object (or multiple objects) transmitting the information from the pilot's input devices, such as sticks, throttle levers, etc. While the simulation is running, such devices always provide an input; there is position and force on the controls, etc. In DUECA, this is called the "time aspect", in this case the data is *Continuous*. When this data is written, the writing module provides a time span for the data; indicating from what time to what time this value is valid. Since having no value for a certain time can cause a problem elsewhere in the simulation, a stream channel needs to continuously be fed with data, and the validity intervals for that data need to match up while the simulation is running. 

For other parts of the simulation, it is not very convenient to transmit the data as continuous. For example when describing the key inputs to a flight management system, it is not necessary to continuously transmit "there was no key pressed" at times no key was pressed. And in some cases a pilot might hit two keys in the period for a single simulation model update. The time aspect for this data is defined as *Events* in DUECA. Event data is stamped with a single time point (not a time span!), and Event channel will happily accept multiple events for a single time, or none for that time. A channel with event data does not need to be filled continuously. 


## Endpoints and Entries, writing, reading

### Single writer, multiple readers, stream

 In the simple cases, a channel is written to by one module, there is only one DCO type written, and it can be read by multiple other modules. In that case the channel has one *entry*. 

![Channel with a single entry, one writing module, stream data, multiple readers](images/channel-single-stream.svg)

This illustration shows a channel with only one writing token (right hand side), and therefore only one entry, entry #0. There are two (but there could equally be 0 or more) reading tokens. 

An entry has an entry number, optionally it has a label (string), describing the entry, and it only accommodates one type of data, i.e., one DCO type. A channel with a single entry only has entry number 0. In cases where you build a simulation for a single aircraft, or a controller or data recording application for a single device, such channels are exactly what you need. 

Given a write token `w_token`, of type ChannelWriteToken, as a member variable of your DUECA class, the writing end of this channel can be created as:

    w_token(getId(), NameSet("MyData://myentity"), 
        "MyData", "entry label"),

You must have created and listed (in your `comm-objects.lst` file), the `MyData` DCO class. The arguments to this call are explained as follows:

- GlobalId `owner`, the ID of the owner, normally the id of your module instance, which can be obtained with `getId()`. 
- NameSet `channelname`, the name of the channel. This is used for the publish-subscribe mechanism. In the above example, the channel's name is created from a single string, often the name is composed of the "entity", "dataclass" and "part" components, so the above nameset can als be created like: `NameSet("myentity", "MyData", "")`.
-  std::string `entrylabel`, the entry label.
-  Channel::EntryTimeAspect `time_aspect`. The time aspect determines the temporal nature of the data. In this case we have continuous values (stream data), which is the default for this parameter. The alternative here is `Channel::Events`, for event-like data
-  Channel::EntryArity `arity`. In a writing token, the arity determines how many entries may be created. The default is `Channel::OnlyOneEntry`. This will prevent creation of multiple entries in this channel. 
-  Channel::PackingMode `packmode`. The packing mode determines how the data is coded for transmission between DUECA nodes (computers). The default is `Channel::OnlyFullPacking`, thus each time data is written in the channel and when this data needs to be transported, the full data is coded and sent. The alternative is `Channel::MixedPacking`. In that case, whenever the first message is sent, the full data is packed, thereafter, only the differences between the latest data and new data are coded. If you have largely static information in the channel, mixed packing may be more efficient.
- Channel::TransportClass `tclass`. The transport class determines the priviledge of transmission. `Channel::Regular` will ensure packing as a regular message, the alternative is `Channel::Bulk`, which is appropriate for large objects. These are packed after the regular data is packed, and the data may span several communication messages, so also large objects can be transmitted. 
- UCallbackOrActivity `when_valid`. This is a callback function that can be used to detect when a token becomes valid. In most cases, it is sufficient to check the validity of your token in the `isPrepared` method of your class, with the `CHECK_TOKEN` macro:

    CHECK_TOKEN(w_token);

- unsigned `reservations`. This is needed in special cases where no data from the channel may be lost in communication between two or more modules.

The read tokens for such a channel are created as follows:

    r_token(getId(), NameSet("MyData://myentity"), "MyData"),

These first three arguments are the same as for the write token. The further arguments for the first variation of the read token constructor (which in this case can all use the default value), are:;

- entryid_type `entryhandle`. This is the number of the accessed entry, in this case `0`.
- EntryTimeAspect `time_aspect`. The default is `Channel::AnyTimeAspect`, which means that the reading token will simply follow the writing token's suggestion on the temporal aspect of the data. In this case continuous data. 
- Channel::EntryArity `arity`. This has a slightly different meaning for the reading token. 
  - The default, `Channel::OnlyOneEntry`, will mean that the read token attaches itself to only one entry, in this case entry 0, and will become valid if that entry is compatible with the temporal aspect (which, with the given AnyTimeAspect, it will be), and the `data_class` is compatible, i.e., identical, or a parent class of the written dataclass.
  - `Channel::ZeroOrOneEntry` is similar, however the token is valid immediately.
  - `Channel::ZeroOrMoreEntries` should be combined with an `entry_any` value for the `entryhandle`. The token is immediately valid, and can be used to read any matching entry.
  - `Channel::OneOrMoreEntries` is similar, but the token is valid only when there is an entry in the channel.

- Channel::ReadingMode `rmode` has a default of `Channel::AdaptEventStream`. In this case, with continuous or stream data, it means that the timespec supplied when reading the channel is used to select data from the right time. If the channel contains event data, this reading mode is interpreted as `Channel::ReadAllData`. These reading modes are:
  - `Channel::ReadAllData`. Data in the channel is returned sequentially, oldest data first. It is not possible to read the same data twice (by the same read token). If a time specification is supplied when reading, the data time is compared to that specification, and only "older" data "<=" is returned. Note that you actually *must* read (or discard), the data, because the channel will try to reserve all that data for you.
  - `Channel::JumpToMatchTime`. This is the normal reading mode for stream data, the time specification supplied when reading is used to select the appropriate data. 
  - 