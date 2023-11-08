# More Simple Simulation (#example2b)

## Introduction

The [A Simple Simulation](#example2) chapter explained how to create a
DUECA simulation with one "player", re-use existing modules, create new
modules, and configure a simulation for running on a multi-computer set-up.

Here we look at how to connect multiple simulations together. In this way,
a multiplayer simulation can be made. Each player has his/her own DUECA
controls, and can start and run independently from the other players.
However, data between players can be exchanged, so it is possible to
"see" the other players, by, e.g., showing their avatars / graphical models
in the world visualisation.

We will start by adding a simple module to our simulation to track whoever
is currently "in the game". Then we discuss ways of configuring the multi-
player simulation.

## Overview module

To track what is happening, we will create an overview module. Use the
`dueca-gproject` command to get a new module called
`monitor-teams`. Now enter the `monitor-teams` folder and create code
for the modules. Teams can join at any time, and start moving (with
the DUECA Advance mode) at any time, and each team will have their own
DUECA process to control. So it makes no sense to make the monitoring
module dependend on DUSIME state, we simply always monitor. The
monitoring module can therefore be a pure DUECA module:

~~~~{.bash}
[enter]$ cd monitor-teams
[enter]$ new-module dueca
Give a (full) name for the module: MonitorTeams
A description for the activity:  : show team status
~~~~

We can use the `BaseObjectMotion` DCO datatype from the `WorldView`
project to send information about moving objects around. To be able to
read that, this must be added to the `comm-objects.lst` file, add the
following line:

    WorldView/comm-objects/BaseObjectMotion.dco

We are already using the `WorldView` for the visualisation, so it is
given in the `modules.xml` file, and `dueca-gproject` will know where
to get the code from.

The DUECA interconnection facilities can also send information on
joining and leaving peers. That is given in a `ReplicatorInfo` DCO,
which is installed with the DUECA headers and libraries. Define two
read tokens in the `MonitorTeams.hxx` header file, called `r_announce`
and `r_world`, and uncomment the line that defines a clock
(`myclock`).

Our "overview" module will be very simple, we just print information
on joining and leaving peers, and information on the entries we find
in the channel with `BaseObjectMotion` objects. To simplify our
design, we add a second callback and a second activity to the module,
just for printing the notifications. This needs the following in the
header file:

* A callback object:
  ~~~~{.c++}
    /** Callback object for simulation calculation. */
    Callback<MonitorTeams>  cb2;
  ~~~~
  Add this below the first callback object.

* A second activity:
  ~~~~{.c++}
    /** Activity for simulation calculation. */
    ActivityCallback      do_notify
  ~~~~
  Add it below the first activity object.

- and a function that is to be called:
  ~~~~{.c++}
    /** print a notification about a leaving or joining peer */
    void doNotify(const TimeSpec& ts);
  ~~~~
  Add it below the `doCalculation` function declaration.

Now take a look at the `MonitorTeams.cxx` file. You may notice it is a
bit smaller than the `UFODynamics` files, it misses parts for the
DUSIME communication; different modes to run in, and sending and
receiving snapshots.

The announcement channel should have only one entry in it. It contains
`ReplicatorInfo` objects, and they are written as events. The channel
name will be configurable from the script, but we will use a simple
convention; `ReplicatorInfo://<entity name>`, where in this example
the entity name is will `central`. To have access to the definition,
we will include the proper header:

~~~~{.c++}
#include <dueca/inter/ReplicatorInfo.hxx>
~~~~

Now add the tokens to the constructor implementation:

~~~~{.c++}
  r_announce(getId(), NameSet(getEntity(), ReplicatorInfo::classname, part),
             ReplicatorInfo::classname, 0, Channel::Events),
  r_world(getId(), NameSet("world", BaseObjectMotion::classname, part),
          BaseObjectMotion::classname, entry_any, Channel::AnyTimeAspect,
          Channel::ZeroOrMoreEntries),

  // clock
  myclock(),

  // a callback object, pointing to the main calculation function
  cb1(this, &_ThisModule_::doCalculation),
  cb2(this, &_ThisModule_::doNotify),
  // the module's main activity
  do_calc(getId(), "check up", &cb1, ps),
  do_notify(getId(), "print notification", &cb2, ps)
~~~~

I also added the uncommented clock in the code snippet, the two
callback objects and the two activities.


