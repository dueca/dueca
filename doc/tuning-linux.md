# Tuning Linux Workstations {#tunelinux}

Making the most out of your hardware

## Introduction {#tunelinux_intro}

DUECA is a middleware environment for real-time distributed
calculation. However, that does not mean that DUECA can run always
real-time, for running real-time, DUECA relies on the services of the
underlying operating system. This page describes some tricks for
tuning your Linux system so that it can run real-time tasks.

## Priorities and memory lock {#tunelinux_priorities}

One of the first things to do is to ensure that the dueca processes do
not run in normal scheduling mode, but in a realtime priority
mode. There are two things that make this possible:

- If the process runs with superuser (root) permission, then a
  real-time priority can be selected at any time.

- The selection of real-time priority can be handed out in a
  fine-grained manner by the pam modules. To adjust this priority, edit
  the file `/etc/security/limits.conf`, or add a file to
  `/etc/security/limits.d`

It may be clear that the second option is to be preferred; in
addition, by not making your process run as root, the log files that
will be written are accessible from your normal user id.

Another thing that must be handled is the memory lock. In Linux and
most unix systems, a process can be (partially or fully) swapped to a
disk partition (the swap partition) if not enough memory is
available. This must be prevented at all times for real-time
processes, so the process must be locked in memory. I suggest the
following adjustments:

- Define a group of users that may run real-time tasks,
  e.g., `rtdueca`, and add the users that need real-time priorities to this
  group

- Allow real-time priority and memlock via pam, add a file
  `dueca.conf` in the directory `/etc/security/limits.d`:

      @rtdueca        -        rtprio                100
      @rtdueca        -        nice                  -20
      @rtdueca        -        memlock                unlimited

The option '-' enforces both soft and hard limits together; the
default limit is changed to the new value, which is probably the best
option for a "production" machine in a simulation set-up. If you want
to change the limits on a developer workstation, you might consider
using the option 'hard' for only setting the hard limits. The
developer can then use the "ulimit" shell command to change the soft
limits and allow real-time running, or not use the command, and run
dueca non-realtime, e.g. when developing.

Modern Linux kernels and systems use aggressive energy saving
strategies. This commonly destroys the real-time performance; as an
example on modern DELL workstations with Ubuntu 18.04, wake-up
latencies as long as 360 microseconds will be produced. This can be
corrected by switching off sleep modes in the kernel. DUECA will, if
memory lock has been succesful, try to also do this by seting the
kernel to low-latency behaviour through the `/dev/cpu_dma_latency`
file. A standard DUECA rpm or deb installation will include a udev
rules file that makes this device file writeable for the rtdueca
group. If you want this for another user, adapt the
`90-rtdueca-cpulatency.rules` file.

##  Ubuntu 22 problems {#tunelinux_devel_22}

The default setting for the memory lock limit on Ubuntu 22.04 is
almost 500 kB. (to be checked with `ulimit -a`).

For development, people normally do not adjust the limits. In common
uses, this will allow your DUECA process to lock the memory at
start-up, but after this, it is common that the DUECA process grows to
beyond 500 kB. When that happens, memory allocation fails, and the
process crashes.

So for development on Ubuntu 22 machines, set the memory lock limit to
zero. This prevents locked memory alltogether; in the terminal where
you are running DUECA, or in your `.bashrc` file, set:

    ulimit -l 0

## Start-up scripts {#tunelinux_scripts}

To facilitate starting a DUECA process on multiple computers, start-up
scripts have been included in the dueca distribution. These scripts
facilitate starting DUECA remotely. Currently, the following are
available:

    /usr/bin/rrundueca
    /usr/bin/rrunduecaX
    /usr/bin/rrunduecaXfvwm
    /usr/bin/rrunduecaXmwm
    /usr/bin/rrunduecaXxf

These start, respectively (1) a bare (no X) dueca, (2) dueca after
starting a bare X server, and the remaining three start the X server,
a window manager (fvwm, Motif Window Manager and xfce respectively)
and dueca. The latter three are commonly used in testing, when you need
several windows with, e.g., plots for debugging.

These scripts are typically run over ssh. You need to ensure
unhindered (no password) logins from the machine where you do the
remote logins. There is a script (actually, a piece of script) called
`GenericStart` that can do most of the magic for you. This will be
explained at the hand of an example:

     #!/bin/bash

     # In this lab, we use ssh to remote login on Linux
     RSHL=ssh

     # the main directory with the configuration files for this simulation
     MAINDIR=/home/fltsim/dapps/ESVS2/ESVS2/run/HMILab

     # A list of all nodes, except node no 0 and timing masternode
     NODES="dutmms3 dutmms2 dutmms6"

     # Node 0, the node with the dueca interface
     ZERONODE="dutmms1"

     # timing master node, will initiate the communication
     MASTERNODE="dutmms4"

     # Here we define what type of machine each node is. These are extended
     # regular expressions

     # nodes with Linux, and start up X
     XNODES="dutmms[326]"

     # nodes with Linux, but don't start X
     LNODES="dutmms[14]"

     # for debugging, you can build one or more of the nodes with debug symbols
     # and manually start them with the debugger. List these nodes in MNODES
     # MNODES="dutmms1"

     # beyond this point, you should not have to modify anything
     source `dueca-config --path-datafiles`/data/GenericStart

A full description of all possible options is in the `GenericStart`
file. The principle is simple; in your script, you define which nodes
need to be started, and how; X, or X with a window manager, or no
X.

In the above examples, there are three "normal" nodes, the timing
master node and node 0.

The normal nodes are all started with X (XNODES), and thus with the
script rrunduecaX. As an example, the DUECA process on `dutmms3` will be
started from:

    /home/fltsim/dapps/ESVS2/ESVS2/run/HMILab/dutmms3

so it will look for its configuration files there. Other nodes will be
started from their respective directories.

Note that there are many more options for the `GenericStart` script;
check with

    bash /usr/share/dueca/data/GenericStart

## Real-time kernels {#tunelinux_kernel}

I have experimented with several options for real-time kernels, and
have decided that most of the functionality needed for simulation,
with a minimum of hassle, can be found in Linux kernels with the
`PREEMPT_RT` patch set applied. In such a set-up, use nanosleep for
timing (see configuring `dueca.cnf`). Timing is generally accurate to
within 20 microseconds, and the clock on these system is based on
one-shot timers, so there are no granularity problems (i.e. the clock
can wait until exactly the time you want it to, it does not under- or
overshoot the target time by maximally one clock period, which is
typically 10 or 1 ms).  I maintain a set of these kernels on the
opensuse build service, project `home:repabuild:preempt` .

On Ubuntu, you can install a real-time kernel with:

    apt install linux-image-preempt linux-headers-preempt linux-modules-preempt

## Installing nvidia drivers {#tunelinux_nvidia}

To find out which NVIDIA driver is suitable for your set-up (if you
have an NVIDIA card, that is), run:


    user@linux:~$ ubuntu-drivers devices

    == /sys/devices/pci0000:64/0000:64:00.0/0000:65:00.0 ==
    modalias : pci:v000010DEd00001CB2sv00001028sd000011BDbc03sc00i00
    vendor   : NVIDIA Corporation
    model    : GP107GL [Quadro P600]
    driver   : nvidia-driver-470 - distro non-free recommended
    driver   : nvidia-driver-390 - distro non-free
    driver   : nvidia-driver-470-server - distro non-free
    driver   : nvidia-driver-418-server - distro non-free
    driver   : nvidia-driver-460-server - distro non-free
    driver   : nvidia-driver-450-server - distro non-free
    driver   : nvidia-driver-460 - distro non-free
    driver   : xserver-xorg-video-nouveau - distro free builtin


You see that this gives you information on the installed graphics
card, and what drivers are compatible with that card. Choose a version
and install:

    apt install nvidia-driver-470

(of course, Supply the right version for the nvidia module here)

Compilation of the kernel module will now fail, because NVIDIA by
default does not accept to build for the PREEMPT kernel series. With a
little script you can patch the NVIDIA sources, and then let dpkg
correct the configuration of the installed nvidia-dkms package, so
it builds and installs the kernel module:


    # run a script that modifies the nvidia package code to be compatible
    # with a PREEMPT built kernel
    dkms-allow-preempt

    # re-run the failed package install configuration, which should now
    # correctly build and install the nvidia modules for the PREEMPT kernel
    dpkg --configure -a


## Xorg settings for display and outside visual {#tunelinux_display}

Nothing is more annoying than being halfway an experiment, and seeing
the display go blank. To prevent that, adjust the Serverflags section
in your xorg.conf file, or in a file in `/etc/X11/xorg.conf.d`, I am suggesting
`/etc/X11/xorg.conf.d/10-dontblank.conf`:


    Section "ServerFlags"
      Option "BlankTime" "600"
      Option "StandbyTime" "610"
      Option "SuspendTime" "620"
      Option "OffTime" "630"
    EndSection


This will give you 10 hours (600 minutes) of running until the screen
blanks.

## Ethernet transmit settings {#tunelinux_eth}

## With ethtool {#tunelinux_eth_ethtool}

For a set of DUECA nodes that communicates over Ethernet, it might be
important to tweak the ethernet card settings. Using ethtool, one can
for most cards modify the timing settings; this is particularly
important for Gigabit ethernet cards, that are in many cases tuned to
maximize throughput, at the expense of some latency. The technique
used is coalescing, gathering several packets before generating an
interrupt.

In real-time communication, latency is more important; the following
command turns the coalescing of, by setting the delay for interrupt
generation after packet reception to 0 and the number of frames to
collect before generating an interrupt to 1.


    ethtool -C eth0 adaptive-rx off adaptive-tx off \
            rx-usecs 0 rx-usecs-irq 0 rx-frames 1 rx-frames-irq 1 \
            tx-usecs 0 tx-usecs-irq 0 tx-frames 1 tx-frames-irq 1


If you want this change to persist, add it to a start-up script that
is executed after the network cards are configured.

## Module settings {#tunelinux_eth_module}

Some ethernet cards cannot be controlled with ethtool, but the
settings of the kernel module that controls the card can be supplied
when the module is loaded. The `e1000e` driver used for many Intel cards
can be configured in this way. There are several steps in this process:

- Find out which cards are controlled by the e1000e driver, in which order,
  The following script by Jonathon Reinhard might help:
  <a HREF="https://gist.github.com/JonathonReinhart/573694d541dc2108f7629aaa615cef3b/raw/2e078f301b5833d86d4b6992ad642a70425870e4/what_eth_drivers.sh">
  what_eth_drivers.sh</a>

- Create a file `e1000e.conf` in `/etc/modprobe.d`, with the contents

      options e1000e InterruptThrottleRate=3,0


  Note that the `InterruptThrottleRate` takes an array (comma-separated), as
  argument, each argument corresponds to a card controlled by the driver.
  "3" is the default value, indicating automated coalescing of interrupts,
  "0" turns off the coalescing. Use that option for the cards involved in real-
  time communication, thus adapt the array of arguments as needed.

- Activate the driver with the new options through:

      modprobe -r e1000e && modprobe e1000e


- To make the change implemented on start-up of the kernel, reconfigure the
  initial ram disk with:

      update-initramfs -uk all


For other drivers, use the modinfo command to determine which load
options are possible.

## Starting up graphical displays {#tunelinux_X}

In some cases, it is handy to drive several displays off a single
computer. It is possible to give each video card in that computer its
own X server. These must then be specified in the xorg configuration,
and selected when starting the X server. There are several concepts to
consider:

- An X server can be specified by defining a ServerLayout. The server
  layout combins input devices (mice, keyboards), with a screen. As an
  example, the sides screen of the projection in the HMI laboratory:

      Section "ServerLayout"
        Identifier        "sides"
        Screen            0   "ScreenSides"
        InputDevice     "Mouse1" "CorePointer"
        InputDevice     "Keyboard1" "CoreKeyboard"
      EndSection


- The next step is to consistently attach the keyboard and mouse input
  devices to physical hardware. In this case, we don't connect the
  mouse to anything, but you might want to use udev to get a
  consistent name for e.g. a usb mouse or touchscreen, and then link
  that input:

  Here a mouse attached to nothing (dev/null):

      Section "InputDevice"
        Identifier  "Mouse1"
        Driver      "mouse"
        # Option    "Protocol" "auto"
        Option            "Device" "/dev/null"
        Option            "ZAxisMapping" "4 5 6 7"
      EndSection


  And a standard system keyboard:

      Section "InputDevice"
        Identifier  "Keyboard1"
        Driver      "kbd"
      EndSection

  These are both used in the above ServerLayout, binding them to a server.

- The screen needs to further define the monitor and the
  associated/connected graphics card:

      Section "Screen"
        Identifier "ScreenSides"
        Device     "Card1"
        Monitor    "Monitor2"
        SubSection "Display"
                Viewport   0 0
                Depth     24
        EndSubSection
      EndSection


  The identifier links back to the server layout.

- Then the card, since multiple cards are used in the system, needs to
  be identified. Run the lspci command to find your graphics cards
  (and some trial to see which is which), and specify a card for the X
  configuration. In this case, two projectors are attached to the
  card, and we use the nvidia "TwinView" option to treat these two as
  a single large display. Later, in generating the image, you can use
  two viewports or two windows to draw the graphics.

      Section "Device"
        Identifier  "Card1"
        Driver      "nvidia"
        VendorName  "nVidia Corporation"
        BoardName   "Unknown Board"
        BusID       "PCI:23:0:0"
        Option      "TwinView" "true"
        Option      "TwinViewOrientation" "RightOf"
        Option      "UseEdidFreqs" "true"
        Option      "ProbeAllGpus" "false"
        Option      "NoLogo" "true"
      EndSection

  The BusID and ProbeAllGpus options are used to isolate the single
  graphics card.

- The monitor definition for the screen is default, since the card can
  already determine the monitor resolution and update rate:

      Section "Monitor"
        Identifier   "Monitor2"
        VendorName   "Monitor Vendor"
        ModelName    "Monitor Model"
      EndSection

## Identifying and labelling hardware {#tunelinux_hardware}

Many lower-cost IO devices use USB interface to communicate with the
computer. In case you want to read these from your simulation program,
you need to figure out which of the many event, mouse or joystick
device files you have in your `/dev` folder is the one that you need to
attach to, and it can be particularly annoying that usb devices end up
at a different device file when other devices are plugged in or
not. In this case udev scripts can provide persistence.

Supposing you figured out that `/dev/input/event3` is attached to your
touchscreen. A handy little program for checking this is `evtest`. After
a next reboot, this device may have moved to `/dev/input/event5` . To
get a persistent name, you may add a symbolic link pointing to the
device when it becomes available.

At any moment, a device is uniquely identified by its device path (but
note that that may change after a reboot). You can get udevadm to get
information on the device in the following manner

    udevadm info -a -p $(udevadm info -q path -n /dev/input/event3)


You will get information on the device itself and on the parents of
the device. You can select the device for udev rules by specifying
attributes of the device itself and of one of the parent devices (with
`==` between the attribute and the value!). Sometimes you can select on,
e.g., device name and possibly serial number. If you have only one of
these devices, the selection is simple, just select on the name of the
thing, like:

    SUBSYSTEM=="input", ATTRS{name}=="Wacom BambooPT 2FG Small Pen"


If you have multiple devices of the same type, and they are not
distinguished by serial or other tags, you can try to match on the
physical bus location of the USB connection. As long as you don't
re-plug the devices, this will stay constant, here is an example match
on the plug/bus:

    SUBSYSTEM=="input", ATTRS{phys}=="usb-0000:00:1d.0-1.6.4/input0"


Using the match, create a rules file in the `/etc/udev/rules.d` folder,
e.g., `90-touch1.rules`:

    SUBSYSTEM=="input", ATTRS{phys}=="usb-0000:00:1d.0-1.6.4/input0", GROUP="users", MODE="0660", SYMLINK+="touchinput1"


This modifies the file mode, group name and adds a symbolic link in
`/dev`. Note that here you use a single equal sign (`=`), or an addition
(`+=`) between actions and arguments.

## Handy stuff for touchscreens {#tunelinux_touch}

To find out, after all the connecting, what devices are available, you
can use the `xrandr` and `xinput` programs.

- Step 1, find out which output devices are attached to my graphics card:

      xrandr

This produces devices like `DP-0`, etc.

- Step 2, find the x input devices:

      xinput list


Your touchscreen should have a number here, we assume 14 for now.

- Now map the touchscreen to the required device/display:

      xinput --map-to-output 14 DP-0


- If you want to make the touchscreen scaling permanent, check the
  matrix with:

      xinput list-props 14


You can set the coordinate transformation matrix in the
xorg.conf, or (if you have only one graphics output/x window) add it
to the udev rules magic; using the same example:

    SUBSYSTEM=="input", ATTRS{phys}=="usb-0000:00:1d.0-1.6.4/input0", ENV{WL_OUTPUT}="DP-0",ENV{LIBINPUT_CALIBRATION_MATRIX}="0.545455 0.000000 0.000000 0.000000 0.900000 0.000000 0.000000 0.000000 1.000000"


And in xorg, in the InputDevice, add:

    Option "TransformationMatrix" "0.545455 0.000000 0.000000 0.000000 0.900000 0.000000 0.000000 0.000000 1.000000"


## Sound tuning {#tunelinux_sound}

This description on sound tuning is not yet complete. Here, we assume an
Ubuntu 20.04 or 22.04 workstation. Sound is controlled over pulseaudio.

To detect which hardware devices are seen by the operating system, check
first with the ALSA facility aplay:


    aplay -l


To verify if another program is accessing your sound devices directly:


    sudo fuser -v /dev/snd/*


To check that pulseaudio has control your sound devices, run pactl.


    pactl list sinks

    pactl list cards


Volume control gui with pavucontrol.

If there is a complaint about bluetooth and pulseaudio in the
journalctl log, and there is no bluetooth device to configure, run:


    sudo apt-get remove --auto-remove pulseaudio-module-bluetooth


Since the dueca process is run over an ssh login -- unless you are
running node 0 on the desktop you are starting from -- it is better to
run the pulseaudio as a system service, use
[this configuration]
(https://github.com/shivasiddharth/PulseAudio-System-Wide)

Edit the `/etc/pulse/client.conf` file, set


    autospawn = no
    default-server = /var/run/pulse/native


Reboot if needed, verify that there is only one pulseaudio daemon running;


    srs@srsefis2:~ ps aux | grep pulseaudio
    pulse       1003  0.0  0.1 313524 13120 ?        S<sl Jan26   0:00 /usr/bin/pulseaudio --daemonize=no --system --realtime --log-target=journal


Further instructions (setting group membership!) [here](https://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/User/SystemWide/)

Add the simulation user to the groups `audio` and `pulse-access`.

Selecting specific cards, if needed, at
https://unix.stackexchange.com/questions/473846/how-does-pulseaudio-determine-which-alsa-devices-to-make-available-or-not

This requires a reboot before it works.

## Bluetooth devices

### Exploration and checking

For exploration and checking of the connection with a bluetooth
device, the `bluetoothctl` and `hcitool` command line tools and small
python scripts are useful. To get these to work properly when not
running them as root, set up the required permissions:

	sudo setcap 'cap_net_raw,cap_net_admin+eip' $(readlink -f $(which python3))
	sudo setcap 'cap_net_raw+ep' $(readlink -f $(which hcitool))

In many cases, you will need the mac address of a sensor for
connection. To scan for sensors in range:

	bluetoothctl scan on

	...

	[NEW] Device 20:1B:88:03:85:AC 20-1B-88-03-85-AC
	[CHG] Device A4:C1:38:E8:8D:79 RSSI: -52
	[CHG] Device 20:1B:88:03:85:AC RSSI: -83
	[CHG] Device 20:1B:88:03:85:AC Name: Mi True Wireless EBs Basic 2
	[CHG] Device 20:1B:88:03:85:AC Alias: Mi True Wireless EBs Basic 2

The example here is for a Mi wireless humidity and temperature sensor. 

When using python scripts to communicate with bluetooth low energy
devices, the `bleak` module is quite succesful.

`hcitool` can be used to modify latency of bluetooth connections, see this 
(discussion on an archlinus forum)[https://bbs.archlinux.org/viewtopic.php?id=248133]

### Control from DUECA

For IO with bluetooth devices from a DUECA module, the `libblepp`
library can be used. The latest versions have a configurable receive
buffer, which is needed for some devices that send large (usually
fragmented in packages) messages. The python3 scripts seem to have no
problem there, so if you tested a communication with python, and it
does not seem to work with libblepp, check with Wireshark what the
message size used by the device is.

