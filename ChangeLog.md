# ChangeLog

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)

## [4.0.0] - 2023-06-15

- add a fix_optional object, to accomodate optional / nil values from
  msgpack unpacks
- re-write of code generator to:
  * use jinja2 templates
  * change the packing, so that members are always packed in the order
    in which they appear in the object
  * use template magic to pack/unpack different types, no more need for
    IterableType/FixIterableType distinctions
- test for "incomplete + nil" msgpack unpack
- elaborated templating with dco_traits
- improved/updated printing of dco objects

## [3.2.12] - 2023-06-12

- Add a fixvector_withdefault variant with default value option
- Test hdf5 logging for various datatypes
- Handle msgpack nil value for list or for resizing arrays
- More flexibly handle msgpack reading:
  * accept int, float and double for c++ float and double 
  * accept nil to clear variable size arrays, lists and maps
  * accept nil to fixvector_withdefault to set default value

## [3.2.11] - 2023-05-23

- Fix in the appdevelopmentg.md documentation
- Fix the hdf5 logging of std::map members as vararray of 
  hdf5 composite objects
- Add tests hdf5 logging
- Speed up doc targets, avoid rebuild

## [3.2.10] - 2023-05-12

- Ignore messagepack data for DCO members that are not present
- Use rsvg-convert instead of inkscape for svg -> png
- Improve the line counting for the message list documentation
- Fixed a bug in websocket server; connected endpoints from
  an info endpoint would also be listed as direct read endpoints

## [3.2.9] - 2023-04-19

- Enable use of abbreviated url when creating projects
- Fixes in various python code, to correctly read files on ubuntu 18.04
- Add a note on the ulimit problems in ubuntu 22.04
- Remove the FORCE_PYTHON_MALLOC from all but ubuntu 20.04 builds
- Test allowed memory limit before attempting an mlockall

## [3.2.8] - 2023-03-25

- Add a script for scheme to python configuration conversion
- Some minor fixes in dueca-gproject
- Improved comments in the config.cmake templates

## [3.2.7] - 2023-PI

- Add check to codegen, to ensure the right objects are generated
- Expand the information on websockets
- For completeness, add Base64File object to Snapshot
- Fixes to msgpack generation
- Check out README files when borrowing modules from other projects
- Pull directly from github release when generating files for obs build
- Add scripts to facilitate creation of runtest scenarios

## [3.2.6] - 2023-02-07

- Fix script language detection in dueca-gproject
- Correct cmake test for websocket headers
- Finally fixed running the runtests under Wayland
- Use unified channel tokens for internal DUECA communication
- Add a new-dco script, to get nicer .dco files
- Documentation and style tweaks

## [3.2.5] - 2023-01-09

- Some documentation updates (git setup, simplesimulation, GtkGladeWindow,
  Condition)
- dueca-gproject improvements (handling sparse setup, url translation)
- Use std::shared_ptr instead of boost::shared_ptr
- Accomodate Python 3.11 where available
- Improved detection PYSITEDIR in CMake conf
- Check script language in dueca-gproject

## [3.2.4] - 2022-11-17

- Remove gtk2-related classes from the documentation; having the same
  classes in two different versions confused doxygen
- Add an option to the code generator for struct packing alignment
- Various documentation fixes
- Correct a segmentation fault in WebSocketsServer, when closing a
  connection that used the default to get entry0

## [3.2.3] - 2022-11-03

- Clearer message on problems with dueca-gproject refresh
- Enhancement to DCO-generated enums, to allow inspection of enum member
  names
- Extended the GtkGladeWindow for gtk3 to automatically connect DCO
  members based on a link between member name and widget ID, and to
  automatically load GtkComboBox options from enum members.

## [3.2.2] - 2022-10-24

- Add depenency on toml11 for initial state saving
- Fix snapshot unpacking code
- Add an option to the websocket server for immediate start
- Make the EasyId helper accessible to client code
- Complete AsyncQueueMT with an emplace_back method
- Fixes to cvs to git conversion
- Various minor doc and error message improvements
- Fixes to HDF5 logging, notably with enum handling

## [3.2.1] - 2022-07-24

- Adjust build config files to use dueca as package name (formerly dueca2)
- Add code of conduct as per github template
- Add an AssociateObject class, for passing NamedObject / Id capabilities
  to helper classes
- Expose EasyId.hxx to user code
- Correct an issue where a deleted entry is not correctly cleared and
  re-used in a dueca process with multiple nodes and thus packer
  clients on the channel
- Replace damaged png files
- Correct required version for using Python pre-init
- Correct the spec files to build on Fedora 36
- Take a longer timeout on the testrunner, tests failed on slightly slower
  machines
- Small correction to the cvs/make build system when using alternate
  DUECA versions (TU Delft specific)
- Some fixes in documentation

## [3.2.0] - 2022-06-12

- Fixes for DCO code generation with messagepack option
- First version to go open source; license file addition, header edits
- Change to use of AsyncQueueMT for code with previously AsyncList, after
  finding race condition errors. AsyncQueueMT is transparently substituted,
  added compatibility routines and cleaned the code.
- Addition of ddff data format logging (3 variants).
- Addition of pyddff module.
- Addition of generic routine for Snapshot/Initial condition collection,
  storage and retrieval.
- Addition of generic routine and interface for replay storage and replay.

## [3.1.8] - 2022-04-20

- fix dueca-gproject handling of branches
- Information on tuning pulsaudio/sound on linux workstations
- Fix erroneous origin information in channel overview
- update quaternion code
- fix net use display of timing info

## [3.1.7] - 2022-02-05

- Due to memory problems with the Python scripting on Ubuntu 20.04 (valgrind
  warnings at all boost::python::exec calls), using PYMEM\_ALLOCATOR\_MALLOC
  on debian builds; seems to solve the issues.
- In attempt to find the cause for above problem, changed the order of
  declaring Python classes and functions, so parent classes are always
  created first. Old method simply used a try/catch approach.

## [3.1.6] - 2022-02-02

- Add simulation for udpcom errors with double & triple coalescing
- Fixes and enhancments udpcom, can select lowdelay and SO_PRIORITY for
  udp socket
- Improvements to msgpack include files and code generation
- gproject CMake build system fixes, notably dependency calculation for use of
  other modules
- Fix detection of script language for dueca-gproject new-module
- OSX fixes
- Auto-repair option to find project location from `DAPPS_GPROJECT_...`
  defines
- fix for test/runtest, properly set LD_LIBRARY_PATH
- work on udpcom logic
- cleanup intrusive pointer use in various classes
- add a simulation of net packet coalescing effect to udp comm logic
- fix the getNumVisibleSet / haveVisibleSets counting for entries w/o data
- add emplace to AsyncList
- add iterator constructors to dueca vectors
- add a test/runtest, runs example dueca project under Xvfb
- corrections on net-view windows (for viewing network use)
- add net-view by default on >1 nodes and new net comm
- pass network address to peer node, for automatic config of own address
- fix dueca-gproject to handle older versions of the git module
- add multiple `DAPPS_GPROJECT_...` defines, for multiple shorthand url's
- fix for the code generator with enum code

## [3.1.4] - 2021-09-27

- re-work packing and unpacking of DCO objects to use visitor pattern,
  include in the code generator. Functor still to do.
- Code generation for Enum-only dco object, both enum/enum class. Added
  msgpack and hdf options for enum objects.
- Minor tweaks to documentation.
- Fix build on openSUSE Leap 15.3
- Improvements to code generator, to correctly consider whitespace when
  needed

## [3.1.3] - 2021-09-09

- Add packing and unpacking of DCO objects to-from MessagePACK
- Extend the code generator with the capacity to generate a packable/
  unpackable enum, both C-style and c++-11 style enum class, and the
  capability to specify enum values

## [3.1.2] - 2021-08-19

- Completion of the code to develop with git/cmake
- Updates to dueca-cvs-to-git, works pretty well now
- Implementation of a policy facility; policies defined in xml files can
  be automatically applied to project code; can add/remove/swap dco files,
  add/remove/swap borrowed or used modules, search for text patterns and
  replace/insert/delete text.

## [3.1.1] - 2021-06-08

- Fixes for Fedora 34
- Added dueca-holdpackages script, to block unwanted update on ubuntu
- Improved ssl detection/use for web / websocket server
- Fixes for inter communication, handle buffer exhausted
- Various documentation fixes

## [3.1.0] - 2021-04-20

- Change the way the static version is built; with a library suffix. The
  versioned build and .pc files are adapted accordingly.
- Improved the detection & trimming of the filename path, for properly
  generating error messages.
- Detect the need for and link -latomic, primarly for arm 32bit platform
- Edits to make dueca compile on OSX
- Use more modern, and autogenerated, PrimaryControls for the doc
- Fix bugs in producing data for the net use display, in udpcom
- Change AmorphStore (un)packing of std::strings to allow for null
  characters (triggered from a silly bug in udpcom)
- correct code generation for hdf5 with externally defined enums
- use a random group id in udpcom, to make it more robust to stray
  multicast messages
- check for correct configuration of number of nodes when udpcom connects

## [3.0.3] - 2021-04-20 (not published!)

- Add a net use display with timing & load
- Fix a bug in packing/channel entry combination, when a channel is
  rendered invalid

## [3.0.2] - 2021-04-19

- initial changes for arm (32 bit)
- bugfix for netcommunicator, set-up
- updates for the default profile environment, dueca.cnf [OS]
- updates dueca_cnf.py [RvP]

## [3.0.1] - 2021-03-25

- Fixed install of duecautils python module
- Documentation updates xml and json conversion
- Corrections to the cmake/git build system
- Added XMLtoDCO and DCOtoXML conversion utilities
- Added a "smartstring" class, packable, and able to convert DCO objects
  to/from XML or JSON
- fixed the count in ChannelReadToken::getNumVisibleSets

## [3.0.0] - 2021-03-03

- Major change is the use of git for project version control and cmake for
  project build
- Fix in TimeWarp, which prevented its use
- Merged CMAKE development branch. New script dueca-gproject
- Conversion script added, dueca-cvs-to-git
- Updates and fixes to the cmake-based build system
- New python support library duecautils

## [2.7.13] - 2021-02-24

- fix channelview failing in some cases with int too large for 4-byte
- extend HDF5Replayer with a means to control/reload the file
- fix in HDF5Logger so that new filenames are generated

## [2.7.12] - 2021-02-23

- tweaks to build settings for leap 15.3
- small improvement to hdf5 logger errors
- add a test to ensure locale with decimal dot is used

## [2.7.11] - 2021-02-04

- correct pkgconfig install for debian packages
- add libssl-dev to the websock build requirements for debian
- add a page on "other software to install"
- fix fltk compilation
- make udp net communication insensitive to startup order

## [2.7.10] - 2021-01-19

- Extend json conversion to accomodate both strict and extended JSON
  added option to websock server to use either
- Correct websockserver to send latest data; discarding older stuck data,
  preventing buffer build-up

## [2.7.9] - 2021-01-18

- bugfixes on dueca-copy-project
- add names to all script init functions, to better produce debugging info
- fix for Fedora 33 builds
- modify websocket server to include an option to aggressively replace
  the provider of preset entries
- some minor updates to the print messages for uninitialized tokens

## [2.7.8] - 2020-12-11

- Adding sorttable.js, for correct view of log messages in documentation
- Various fixes to pkgconfig files, improves linking with python-based projects
- Update to the activityview gtk3 glade file
- Various small improvements to log messages
- Add the new udp communication to the default dueca.cnf files
- Better logging on script reading/configuring phases

## [2.7.7] - 2020-09-21

- Fixed library line calculation for the dueca**pc files, to correctly
  determine needed python libraries
- Added a documentation page with a list of all logpoints in dueca

## [2.7.6] - 2020-09-02

- Added relevant comments to all logging/messaging statements
- Created a python script to assemble the logging statement comments into
  an excel file
- Install the new excel file, can be used to obtain further information
  on error messages
- Several minor fixes
  * add simple-web-server as a build dependency
  * correct the code generation for enumerated array size
  * create a group rtdueca on install, if not yet existing
  * fixes to dueca-codegen
  * improvement to compile and link flag reporting in dueca-config

## [2.7.5] - 2020-07-10

- Split off work on web interface for plotting to dplotter
- Modifications to the websocket server
  * add a simple web server
  * new shorter URL's for the endpoints (breaks compatibility, adds consistency
  * documentation changes

## [2.7.4] - 2020-07-03

### Changed

- Various documentation improvement
- Further update to recovery modes NetCommunicator
- Fix regression in the response to criticalError and criticalErrorNodewide

## [2.7.3] - 2020-06-10

### Changed

- Moved from SVN repository to gitlab.tudelft.nl, no history transfer
- Improved messaging on connection failure net communication (inter and udpcom)
- Development builds now with ninja
- Globally cleaned up whitespace and tabs
- Fixes to OSX build, switched to using Python 3.8
- new ChangeLog.md file
- fix in net peer, peer count mechanism
