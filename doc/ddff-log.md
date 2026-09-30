# Logging with DDFF {#ddff}

## Introduction

DDFF or Delft Data File Format, is a datafile layout introduced for DUECA/DUSIME
record and replay facilities, and it is also available and useful for generic
logging. The low-level file format is itself based on the
[msgpack](https://msgpack.org/) binary packing and unpacking format.
To facilitate recording and replay of (generally the
input of) a simulation, the DDFF loggers provide real-time logging and reading
of data. This logging backbone can be used in a number of different ways:

- Easy logging of data that is already sent over channels. The "ddff-logger"
  module can be configured to read one or more data channels, or also
  autodetect entries appearing in data channels, and write these to a ddff file.

- Custom logging or re-reading of data. Although initially not recommended,
  it is also possible to access the ddff logging components yourself. Since
  in most cases the ddff-logger can provide all logging needs, manual
  creation of logs and sending data is not needed. If you still want to use
  this option, take a look at the test code in the `test/ddff` folder.

- Record and replay facilities for simulations. If you want to replay
  simulation runs, the coolest way to go about this is by recording all
  variable (user) input, and replaying that to drive the exact same
  simulation again. Such deterministic simulations are the holy grail
  of simulation professionalism. If you can manage an exact replay from a
  replay of only the input, you can show that:
  - You eliminated all race conditions and other sources of non-determinism.
  - You can capture and restore a complete representation of your simulation's
    state.

DUECA also provides facilities for logging in the HDF5 format. Unfortunately,
logging real-time data in that format can require quite some processing power,
and if you try to log from somewhere between 10 and 20 channels, the hdf5
logger may clog up your simulation. If that happens (or anyway), you can
easily switch over to DDFF, since the modules have compatible arguments
for their configuration. A Python-based program can be used to convert your
DDFF file into a HDF5 format that is very close to the type of file DUECA
would directly produce, so this step will have minimal impact on your data
processing. The ddff reading is also directly usable from your own Python
scripts.

## Streams and segments

In its most basic form, a DDFF file provides several 'streams' of msgpack
encoded data in a single file. However, normally DUECA logs to files with
an inventory and segments.

The inventory is a description of the contents of the different streams.
It is written as a series of msgpack objects, into stream #0. This data
contains:

- A key, i.e., a name for the data; in the example below "/data/vehicle"
  becomes such a key.
- An integer id, which denotes the stream number in the file
- A free-text label. In the logging performed by DUECA, the label is a
  JSON-encoded description of the DCO objects.

The DCO object data is envoded in msgpack arrays. An overall array of three
elements codes time tick, time span, and the DCO object contents in an array.

On top of that, the file can contain multiple segments. The segment information
links a name (something like "recording1") with a stretch of data in the file.
To this end, the file block offsets, starting points in these blocks, and the
end of the recording is stored in the segment information in stream #1. By
decoding the segment information in stream #1, the start and end points in
the file for each recording segment can be found. If not controlled through
the log configuration (see below), a datafile will simply contain a single
segment.

## Logging

To add the logging module, add ddff to the DUECA components in your main
CMakeLists.txt, for example:

~~~~{.cmake}
set(DUECA_COMPONENTS ${SCRIPTLANG} extra dusime udp ddff)
~~~~

The logging module can then be added to the `dueca_mod.py` configuration file,
Logging works by subscribing to the specified channels. DCO objects in those
channels need to be generated with the

~~~~{.scheme}
  (Option msgpack)
~~~~

option. Preferably give the logger its own priority. Timing does not need
to be fast, a few invocations per second is enough, the logger will simply
collect the data from the specified channel, and code these in the DDFF
logging format. Each channel or each entry will be written into a logging
stream,

There are two common ways of specifying log entries.
- The first is targeted to a single entry, either the first one in a
  channel, or an entry specified by a specific label. Here you also
  specify a data type, this will be how the data is read from the channel
  and logged.
- The second is by means of specifiying a channel to watch. When entries
  appear in that channel, they will be logged to a logging stream. These
  will simply be logged with the data type appearing in the channel.

The following snippet shows these both:

~~~~{.py}
    mymods.append(
        dueca.Module("ddff-logger", "", log_priority).param(
            ("set-timing", log_timing),
            ("log-entry",  # Vehicle output
              (f"VehicleCabPosition://{entity}",
                "VehicleCabPosition",
                "/data/vehicle" ) ),
            ("watch-channel",
              ("BaseObjectMotion://world",
               "/data/traffic" ) ),
            ("filename-template",
             "mydatalog-%Y%m%d_%H%M%S.ddff" )

          ) )
~~~~

Timing for the logged data can be influenced by a number of keywords

- `immediate-start` will immediately start the logging, and not stop until
  the DUECA process closes off. Do this in cases you want to start logging
  early.

- By default, the logging is done in `Advance` or `Replay` mode only. To
  specify channels that also log in `HoldCurrent` mode, set the `log-always`
  flag to `True`. Logging entries defined after that will be logged while the
  logger is running. This can be especially useful to log the configuration
  events you use in your simulation.

- To reduce the data rate, use the `reduction` variable with a time
  specification. Logging defined after this setting, is logged according to that
  time specification. Note that this only makes sense for stream channels.

The logger can also be controlled and monitored through DUECA channels. You
can specify a configuration channel with `DUECALogConfig` events. These can
be used to define sections in your file, or specify that you want to log to
a new file. Logging status can be reported to a channel with `DUECALogStatus`
events, giving information on file size and logging process.

~~~~{.py}
            ...
            ("config-channel",
              f"DUECALogConfig://{entity}"),
            ("status-channel",
              f"DUECALogStatus://{entity}"),
            ...
~~~~

## Reading the data

The pyddff Python module is installed with a DUECA installation, and can
also be separately downloaded and installed through pypi.

~~~~{.bash}
> pip install --user pyddff
~~~~

This also install the program `ddff-convert`. See `ddff-convert help` to
get instructions. When working with python and the `pyddff` module directly,
you can efficiently convert the data to a convenient format.

~~~~{.bash}
from pyddff import DDFFSegments

# Open a file
df = DDFFSegments("mylogfile.ddff")

# see what streams are defined
print(df.keys())

# see what segments are defined
print(df.tags())

# for stream data, it is useful to read streams into dictionaries of
# numpy arrays. For "simple" data (floats, doubles, ints, strings, or
# fixed-length arrays of these), this results in 1 or 2-dimensional
# numpy arrays.
# without arguments get_data will get all data, with a period or index,
# it will collect only that segment.
# this returns time `tick`, `span` as numpy arrays, and `d` a dictionary
# of numpy arrays with data.
t, span, d = df['/data/traffic'].get_data()

# for events, it makes more sense to iterate over these, you will get
# the event data in a dictionary, one by one
for t, event in df['/data/config'].items():
    print(t, event)
~~~~

## Record and replay

When using record and replay, DDFF files (for each entity involved in record
and replay, one file for each node where record and replay is used), are
used to store data for a simulation where recording is used. As an example,
the control loading simulation may record all user input. For a replay,
the desired data segment is loaded, and in the `Replay` mode, that data
is read and produces an exact copy of the output during the recording.

For control loading devices, it would of course be cool it the device moved
as it did during the recording. For some other devices (buttons or the like),
this may not be possible. To accomodate this, there two ways of recording
and replaying are implemented:

- Literal copy of the data sent over a channel, and replaying will re-send
  that same data (the simplest).
- Custom replay, where any data codable in a DCO object can be recorded, and
  at replay, this data is read back by the program and used to drive the
  simulation, feedback, or output.

This uses the [DataRecorder](@ref dueca::DataRecorder) class.

~~~~{.hxx}
  /// Recorder for record and replay
  DataRecorder myrecorder;
~~~~

For the constructor, a datarecorder does not need arguments. However in the
`isPrepared` function, you need to link it to the channel you use, or
give it a custom configuration:

~~~~{.cxx}
  // in isPrepared, after the token you are using is ok:
  if (res) {
    // you can call this any number of times
    myrecorder.complete(getEntity(), my_token);

    // check the recorder, if happy, it keeps res true:
    CHECK_RECORDER(myrecorder);
  }
~~~~

Alternatively, if you want to custom create the recorder:

~~~~{.cxx}
    myrecorder.complete(getEntity(), "unique key", "MyDCOClass");
~~~~

You should now (however you completed your recorder), (try to) record whenever
your module is in "Advance" mode. This may be custom data, or simply the
data you sent over the channel(s), if that is enough for you:

~~~~{.cxx}
  DataWriter<MyData> dw(my_token, ts);

  // write the data
  ...

  // only when in advance, use the written data for the recording
  if (getCurrentState() == SimulationState::Advance) {
     my_recorder.record(ts, dw.data());
  }
~~~~

There is a slight modification if you want to record event data. With that
type of data, in each simulation update step there may be zero, one or multiple
events written (and thus also recorded). Simply record as above, but indicate
that it is for events, and mark each completed simulation period:

~~~~{.cxx}
  while (writing_my_event) {
    DataWriter<MyObject> dw(w_mytoken, ts);

    // write the data to dw.data(), etc. ...
    if (getCurrentState() == SimulationState::Advance) {
      my_recorder.record(ts.getValidityStart(), dw.data());
    }
  }

  // make sure that the recording is marked as complete for this
  // update
  if (getCurrentState() == SimulationState::Advance) {
    my_recorder.markRecord(ts);
  }
~~~~

It is also possible to record any other DCO class data, and you do not
need to do this with a DataWriter / write token.

For playback, only in `Replay` mode, either directly read from the recorder,
and use that data, or use the recorder to directly replay on the channel:

~~~~{.cxx}
  // example of read and use
  case Replay: {
    MyDCOClass obj;
    DataTimeSpec ts2;
    my_recorder.replay(ts, obj, ts2);

    ... do stuff with the data
  }
~~~~

The `.replay` call returns true if there was data to replay for the period,
and for event data you can test this in a while loop to get all recorded
events.

If you don't need to see or check the data during replay, you can directly
use the `channelReplay` method with the token:

~~~~{.cxx}
  if (getCurrentState() == SimulationState::Replay) {
    my_recorder.channelReplay(ts, w_mytoken);
  }
~~~~

When replaying a piece of recorded data, time specifications likely differ
between the recorded and replayed stretch. All timing is automatically
translated by the recorder.

A standard interface and record and replay controller are provided by DUSIME
to help you manage and select recorded data.

## Record and Replay configuration

To configure record and replay, you need to add a number of elements to
the `dueca_mod.py` file. For each entity that you want to include in
the record and replay, you need to add a replay controller, and it also
makes sense to add a separate controller/handler for the initial model
state (snapshots).

~~~~{.py}
# suppose you named your entity
entity = 'PHLAB'

# the initial state inventory is part of the DUECA modules
DUECA_mods.append(
    dueca.Module("initials-inventory", entity, admin_priority).param(
                 # reference_file=f"initials-{entity}.toml",
                 store_file=f"initials-{entity}-new.toml"))
DUECA_mods.append(
    dueca.Module("replay-master", entity, admin_priority).param(
                 # reference_files=f"recordings-{entity}.ddff",
                 store_files=f"recordings-{entity}-new.ddff"))
~~~~

This adds new windows to the view menu of the DUECA interface. If you
have a reference file, the initial model states and recordings from
the reference file will become available for use right away, and
together with any new states or recordings you capture, these will
be added to the store files.

After a run, a new initials file will be available on node 0 of
your DUECA process. Files for the recordings are stored locally, so
if you have multiple DUECA nodes, and record data on these, multiple
recording files will be generated, each with its part of the data
needed for replay.

In addition to the replay master, each DUECA node where recording or
replay takes place, needs a `ReplayFiler` object, one per entity serviced.
Simply create that in the dueca_mod.py file, and assign it to a variable.

~~~~{.py}
filer = dueca.ReplayFiler(entity)
~~~~

The filer will connect to its corresponding master, and ensure that
any recorders you use can store their data and retrieve replay data.

## Relation to msgpack

The DDFF file format uses msgpack to convert the DUECA data to file.
When logging a DCO object, msgpack codes this as an array with
each element of the array corresponding to a data member in the DCO
object. The msgpack format can also be used in general inter-process
communication, the websocket server can use this format, and in the
past it was also used to communicate with the PupilLabs Core eye
tracker. In those cases it is better to code the DCO objects as object
structs (i.e., coding variable name + value), rather than as arrays.
This way of coding is actually the default. Let's show this with a
simple example, first a DCO object:

~~~~{.scheme}
(Type int)
(Object MyObject
  (Option msgpack)
  (int a (Default 1))
  (int b (Default 2))
)
~~~~

If you want to pack the data, you could use a DUECA messagebuffer:
~~~~{.cxx}
dueca::MessageBuffer buf(200);
MyObject o1; // will have default data
msgpack::packer<dueca::MessageBuffer> pk(buf); pk.pack(o1);
~~~~

In this case, the object will be packed as an object. The msgpack format
is binary, so a simple dump of the datafile cannot be inspected. You can
use the `pyddff` python module to read this data. If you could convert to
the human-readable JSON format, you would get something like this:

~~~~{.json}
{ 'a': 1, 'b': 2 }
~~~~

If you want to communicate with external software using msgpack, for example
for the Pupil Labs Core eye tracker, data in this format can be used. The
DCO objects with a `msgpack` option (with sometimes an additional trick),
can be used to pack and unpack these messages.

To make the logging a bit more compact, the DCO objects data is packed in
an array, rather than a dictionary/struct, for the ddff format. If you
would translate this to the equivalent JSON, you get something like:

~~~~{.json}
[ 1, 2 ]
~~~~

A C++ templating trick is used to choose between the two possible packing
formats. To use this in your own code, do something like:

~~~~{.cxx}
dueca::MessageBuffer buf(200);
MyObject o1; // will have default data
msgpack::packer<dueca::MessageBuffer> pk(buf); pk.pack(mark_for_dco_msgpack(o1));
~~~~

Now the DCO object will be packed as an array. If the DCO object happens to contain
other DCO objects as nested data members (either directly, or in lists, arrays, maps
or the like), these will also be packed as arrays.
