# Some Recipes for using DUECA Channels {#channelpatterns}

## Introduction

DUECA uses "channels" as a means to communicate between different parts of a simulation. Since a re-write (and the step in 2014 from the DUECA 0.x series to DUECA 2.x and above), these are implemented by the "UnifiedChannel" class. Its name suggests that these channels offer many possibilities, and that is true. While another [page](@ref dueca2) discusses the difference between the old DUECA channels and the new unified channel type, this page looks at the properties of the DUECA (unified) channels, and typical ways in which you may use these channels. 

## Events versus Stream

Channels are meant to transport simple C++ structs (the DCO objects, see the information on the [code generator](@ref codegenerator) ). A typical use of DCO objects in the context of flight simulation would be a DCO object (or multiple objects) transmitting the information from the pilot's input devices, such as sticks, throttle levers, etc. While the simulation is running, such devices always provide an input; there is position and force on the controls, etc. In DUECA, this is called the "time aspect", in this case the data is *Continuous*. When this data is written, the writing module provides a time span for the data; indicating from what time to what time this value is valid. Since having no value for a certain time can cause a problem elsewhere in the simulation, a stream channel needs to continuously be fed with data, and the validity intervals for that data need to match up while the simulation is running. 

For other parts of the simulation, it is not very convenient to transmit the data as continuous. For example when describing the key inputs to a flight management system, it is not necessary to continuously transmit "there was no key pressed" at times no key was pressed. And in some cases a pilot might hit two keys in the period for a single simulation model update. The time aspect for this data is defined as *Events* in DUECA. Event data is stamped with a single time point (not a time span!), and Event channel will happily accept multiple events for a single time, or none for that time. A channel with event data does not need to be filled continuously. 


## Endpoints and Entries, writing, reading

 In the simple cases, a channel is written to by one module, there is only one DCO type written, and it can be read by multiple other modules. In that case the channel has one *entry*. 

![Channel with a single entry, one writing module, multiple readers](images/channel_singleentry.svg)

An entry has an entry number, optionally it has a label (string), describing the entry, and it only accommodates one type of data, i.e., one DCO type. A channel with a single entry only has entry number 0. In cases where you build a simulation for a single aircraft, or a controller or data recording application for a single device, such channels are exactly what you need. 

