# Application Development with CMake, git and dueca-gproject {#appdevelg}

This section describes the structure of "new" DUECA projects, i.e.,
those that are using git for version control and CMake for the build
system, and how to use CMake, git and the support script
dueca-gproject.

## Structure of a DUECA project

The DUECA middleware provides support for development of real-time
simulation or data acquisition projects on desktop computers, and
deployment of these projects on simulation or data acquisition
facilities that typically use networked computers with IO capabilities
and hardware to drive these facilities and collect and process
real-time data. The transition to and from desktop development and
deployment on different facilities should be as smooth as possible,
enabling rapid prototyping and quick turn-around times between
development, deployment and test. To enable this, DUECA uses version
control for both the simulation code, and for the configurations
needed for each deployment platform, be it development desktop,
simulator or otherwise. To be able to read the following description,
please consider the following definitions:

### Definitions

<table>
<tr><th> Concept </th><th>   Description </th></tr>

<tr><td> platform </td> <td> A DUECA configuration. This includes the
                             software selection (which modules to
                             run), the configuration files for a
                             single computer, or set of computers,
                             belonging to a certain simulation
                             set-up. A default platform is created for
                             development desktops, it is traditionally
                             called "solo".  </td></tr>

<tr><td> node </td><td> A DUECA process within a specific
                        platform. Normally a single computer will run
                        such a process, but in some cases more than
                        one DUECA process can be run on a
                        computer. The development platform "solo"
                        typically has a single node called
                        "solo". </td></tr>

<tr><td> machine class </td><td> The software and hardware
	                             configuration for a specific node
	                             type. Each node needs certain
	                             software and/or hardware to do its
	                             function. For example the SIMONA
	                             Research Simulator has three
	                             computers for the image
	                             generation. These run DUECA nodes
	                             traditionally called srsig1, srsig2,
	                             and srsig3, i.e., named after the
	                             computers they run on.<br> These
	                             nodes all use the same hardware
	                             (graphics cards) and software, such
	                             as scene graph libraries, so they are
	                             defined by machine class
	                             srs-ig. </td></tr>

<tr><td> project </td><td> The set of software running a simulation,
                           with configuration files for the platforms,
                           the nodes, and definitions of the machine
                           classes is called a DUECA project. Project
                           software, configuration data and
                           description of the platforms, nodes, etc.,
                           is stored in a single (git)
                           repository. </td></tr>

<tr><td> DCO object </td><td> DCO or DUECA Communication Objects are
                              the objects (essentially c++ structs),
                              that can be communicated or sent around
                              in a DUECA project. DCO objects are
                              described in files with a .dco suffix,
                              and a code generator can convert these
                              into c++ code. </td></tr>

<tr><td> module </td><td> A module is the smallest software unit in a
                          DUECA project.  Each module will be given an
                          object ID within a running DUECA project,
                          and modules are named. A module can
                          communicate with other modules in DUECA
                          through "channels" with DCO
                          objects. Regarding project structure, each
                          module is typically given its own folder in
                          the project. All module files and code
                          should be placed in that folder. </td></tr>

<tr><td> pseudo-module </td><td> A module folder that only contains
                                 data files, no code. </td></tr>
</table>

A number of different types of configuration files is used. These are the
following:

<table> <tr><th> File </th><th> Description </th></tr>

<tr><td> comm-objects.lst</td><td>Simple text file that lists which
                                  DCO objects are used by a
                                  module. Each module has one
                                  comm-objects.lst file. </td></tr>

<tr><td> modules.xml </td><td> A file defining which modules are used
                               by a specific machine class; there is
                               one modules.xml file per machine
                               class. It essentially describes all
                               DUECA-based software needed for that
                               type of machine. </td></tr>

<tr><td> machinemapping.xml </td><td> A file defining which machine
                                     class to use for a specific
                                     node. </td></tr>

<tr><td> config.cmake </td><td>A file defining additional build
                                 instructions (libraries to search and
                                 link, include paths, etc.) for a
                                 certain machine class. </td></tr>

<tr><td> project CMakeLists.txt</td><td>A CMake file defining build
                                instructions common to a whole
                                project. Typically this is limited to
                                the selection of which script language
                                will be used, and of the most basic
                                DUECA components common to all nodes
                                in a project. </td></tr>

<tr><td> module CMakeLists.txt</td><td>Per-module CMake file defining
                                build instructions specific to the
                                module, typically these are used to
                                indicate when a module needs a
                                particular library. </td></tr>

<tr><td> dueca.cnf or dueca_cnf.py</td><td>A per-node configuration
                                file, describing DUECA start
                                conditions, typically how to
                                communicate to other DUECA nodes,
                                selection of real-time conditions, and
                                an index indicating which node this is
                                in a platform. </td></tr>

<tr><td> comm-objects </td><td>A folder with all the DCO objects
                                belonging to a project. Technically
                                this is treated like any other module
                                folder, with a slightly different
                                CMakeLists.txt file that runs the code
                                generation, from the .dco files to c++
                                code. </td></tr>

<tr><td> dueca.mod or dueca_mod.py</td><td>A per-platform
                                configuration file, present in the
                                folder of the node with node id "0",
                                that describes the software to be run
                                and its configuration on the
                                platform. It creates (from the script
                                language; scheme or Python), the
                                different module objects in the
                                simulation and gives these their
                                configuration parameters. </td></tr>

<tr><td> .config </td><td>A folder (hidden, since it starts with a
                                dot) with the machine class
                                definitions and machine configuration
                                </td></tr>

<tr><td> machine </td><td>A small text file in the .config folder that
                                indicates to which machine class the
                                current checked-out project code
                                corresponds. This enables the
                                selection of machine-class specific
                                build instructions and DUECA
                                modules. The contents of this file are
								created when you clone the project for a
								specific node; based on machinemapping.xml, the
								appropriate class is chosen. This file is
								not checked in to the repository.</td></tr>

</table>

### Folder structure

DUECA promotes rapid prototyping and re-use of software by allowing
projects to "borrow" modules from other projects. The CMake structure
and the dueca-gproject script support the easy integration of complete
modules. To make this possible, and avoid conflicts with code in the
"own" project, a specific folder structure is used for checked-out
projects. An example, give that our own project is given the bland
name "MyProject", and another project with the name "OtherProject",
would give the following hypothetical structure:

<table>

<tr><td> MyProject </td><td> encapsulating folder </td></tr>

<tr><td> MyProject/MyProject </td><td> "Project" folder </td></tr>

<tr><td> MyProject/MyProject/.config </td><td> Configuration files
                                     folder </td></tr>

<tr><td> MyProject/MyProject/.config/class </td><td> Main folder for
                                     all machine classes </td></tr>

<tr><td> MyProject/MyProject/.config/class/solo </td><td>
                                     Configuration for the default
                                     "solo" machine class, for the
                                     default "solo" node on the
                                     default "solo" platform that is
                                     typically used for development
                                     (config.cmake and modules.xml)
                                     </td></tr>

<tr><td> MyProject/MyProject/.config/class/srs-ecs </td><td> (as an
                                     example). Configuration for ecs
                                     (Experiment Control Station)
                                     class on the Simona Research
                                     Simulator </td></tr>

<tr><td> MyProject/MyProject/run </td><td> Folder for all platform
                                     configurations </td></tr>

<tr><td> Myproject/MyProject/run/run-data </td><td> Folder with common
                                     data (e.g., experiment start
                                     files) for all possible platforms
                                     in this project. </td></tr>

<tr><td> MyProject/MyProject/run/solo </td><td> solo platform
                                     folder. </td></tr>

<tr><td> MyProject/MyProject/run/solo/solo </td><td> Node "solo" on
                                     the (development) platform
                                     "solo". Contains configuration
                                     files for running. </td></tr>

<tr><td> MyProject/MyProject/run/SRS </td><td> example, SRS platform
                                     </td></tr>

<tr><td> MyProject/MyProject/run/SRS/srsecs </td><td> Folder for the
                                     run configuration of srsecs node
                                     in the SRS platform </td></tr>

<tr><td> MyProject/MyProject/build </td><td> Folder for the built
                                     executable, and intermediate
                                     build products </td></tr>

<tr><td> MyProject/MyProject/comm-objects </td><td> Communication
                                     objects (.dco files) defined in
                                     MyProject </td></tr>

<tr><td> MyProject/MyProject/a-module </td><td> Module folder for a
                                     module in MyProject </td></tr>

<tr><td> MyProject/OtherProject </td><td> Folder for code borrowed
                                     from another project </td></tr>

<tr><td> MyProject/OtherProject/comm-objects </td><td> Communication
                                     objects (.dco files) defined in
                                     OtherProject </td></tr>

<tr><td> MyProject/OtherProject/some-module </td><td> Module folder
                                     for a module "borrowed" from
                                     OtherProject </td></tr>

</table>

When developing (you used `dueca-gproject` to clone the code for
machine class "solo"), the full MyProject code will be checked out
from the repository. For borrowed projects, the modules.xml file for
the machine class "solo" is inspected, and only those modules that are
actually borrowed and needed are checked out from these other
repositories. Then all the comm-objects.lst files are parsed, and the
comm-objects folders are checked out from those projects that supply
DCO files. TLDR; `dueca-gproject` collects all files you need to build
and run from the repository.

Note that when you are deploying your code on another computer for
another platform (not solo), such as one of the computers running your
simulator, a "sparse" checkout is done, and only those modules that
are actually needed (also from the "own" project), are checked out.

## Interaction with git version control {#appdevelg_gitinter}

A few commands in the `dueca-gproject` script use git version control
through the script itself, notably `new`, `clone` and `refresh`. For
all other interaction with the version control system, the
developer/user should directly use git commands.

If you work locally on your project, often from your development
desktop, you work with a local git repository, and it will be located
in the hidden `.git` folder of your project. To synchronize code and
keep back ups, dueca-gproject also uses an upstream remote repository.

You might also want to split work over different projects. As an
example, for a simulation experiment you are going to connect your
program to an eye tracker. Since the eye-tracker connection might be
useful for multiple projects, it is best to develop its modules, and
possibly some test facilities, in a separate project, and when the eye
tracking works, borrow its modules into your experiment project.

You can use your account on a gitlab or github server to locate the
remote repositories, for example, you created these empty repositories:

    git@gitlab.somewhere.org:i-am-a-gitlab-user/MyNewEyeTracker.git
	git@gitlab.somewhere.org:i-am-a-gitlab-user/MyExperiment.git

These repositories' url's therefore all start with the same path,
differing only by repository/project name. By defining this in an
"environment variable" in your shell, you do not need to specify the
url's for these commonly used projects. In your `.profile` or
`.bash_profile` start script, specify the following environment
variable:

    export DAPPS_GITROOT=git@gitlab.somewhere.org:i-am-a-gitlab-user/

Now you can use the shorthands `dgr:///MyNewEyeTracker.git` and
`MyExperiment.git`, and these will also be stored in the project configuration. If you later move repositories, or someone else clones your efforts to their own repository, the translation will also work for them.

If you borrow from a project somewhere else, you can always specify the
full URL. The common steps outlined below will present you with a mix
of `git` and `dueca-gproject` commands to maintain your project.

## Shortened URL's for multiple base groups {#appdevelg_shorturl}

Over the (extended) lifetime of a DUECA project, it might happen that
the project code lives in different `git` repositories. As a simple
example, someone attempting to fix or extend the WorldView project
located at `git@github.com:dueca/WorldView.git` might need to clone
this project in github to his/her personal repository, then work on
that, and use a pull request to request a merge of the work with the
WorldView project. For its housekeeping, the `dueca-gproject` stores
the location of the repository URL's, and these would need to be
modified whenever a repository moves.

To fix that, there are a number of shortcut URL's that are
automatically or custom defined. Any shortcut `origin:///` will
automatically be set to the location of the 'origin' repository of the
checked out project itself as returned from:

	git remote -v

A number of other shortcut URL's can be defined through SHELL
environment variables. These can then be used to represent the
locations of git repositories from where you might pull borrowed
modules. These shortcut URL's all start with the string `dgr`.

The `DAPPS_GITROOT` environment variable maps to the shortcut URL
`dgr:///`, and will indicate which URL prefix is the default for
borrowing from projects where only the project name is given.

To organise a set of repositories with projects, multiple environment
variables can be defined. By defining `DAPPS_GITROOT_<prefix>`,
project URL's starting with `dgr<prefix>:///` will have that tag
replaced by the contents of the environment variables. At Control and
Simulation we commonly define:

<table>
<tr><th>variable</th><th>value</th><th>replaces prefix</th><th>Description</th></tr>

<tr><td>`DAPPS_GITROOT_pub`</td>
<td>`git@github.tudelft.nl:dueca/`</td>
<td>`dgrpub:///`</td>
<td>Open-sourced DUECA modules on GitHub</td></td>

<tr><td>`DAPPS_GITROOT_base`</td>
<td>`git@gitlab.tudelft.nl:ae-cs-dueca-base/`</td>
<td>`dgrbase:///`</td>
<td>Common modules for re-use</td></td>

<tr><td>`DAPPS_GITROOT_active`</td>
<td>`git@gitlab.tudelft.nl:ae-cs-dueca-active/`</td>
<td>`dgractive:///`</td>
<td>Widely used projects that are held active on the facilities, for demo's
etc.</td></tr>

<tr><td>`DAPPS_GITROOT_archive`</td>
<td>`git@gitlab.tudelft.nl:ae-cs-dueca-archive/`</td>
<td>`dgrarchive:///`</td>
<td>Completed student projects.</td></tr>

<tr><td>`DAPPS_GITROOT_yard`</td>
<td>`git@gitlab.tudelft.nl:ae-cs-dueca-yard/`</td>
<td>`dgryard:///`</td>
<td>Older projects, converted from our old CVS repository, that
    mostly need clean-up.</td></tr>

<tr><td>`DAPPS_GITROOT_students`</td>
<td>`git@gitlab.tudelft.nl:ae-cs-dueca-students/`</td>
<td>`dgrstudents:///`</td>
<td>Currently active student projects.</td></tr>
</table>

When specifying a repository to the `dueca-gproject` script, the
shortened URL may then be used. The repository will be stored in the
`modules.xml` files with the shortened URL and converted when passed
to `git`. It will therefore be necessary to have the proper
environment variables defined when you work with the project again
later.

See also the description on [repository organisation](@ref reposetup).

## Common steps

### Making a new project

To create a new project, you first need an empty, bare git repository
somewhere. Initialize a git repository somewhere (local or remote)
with exactly the name that you want to give to your project. Then,
with the url of the fresh repository, create the initial template of
the project. You have to be able to write to this repository. This
will leave a local clone for the "solo" machine class on your disk, so
do this at the proper location:

    dueca-gproject new --name MyNewProject --script python|scheme \
                       --gui gtk3 --remote <empty-git-url>

If you do not provide the remote url, you will be left with only a local
project, git wizards know what to do with this.

The script language is either python or scheme, where the more modern
python option is default

The graphical user interface will be applied as option to the "solo"
machine class. Unless you need compatibility with the aging "gtk2",
use the current default "gtk3".

### Continuing development of an existing project

Normally, obtaining a local copy from a git-based repository is
performed by a `git clone` command. However, to get the proper file
structure, and pull the modules that are borrowed from other projects,
it is better to do this through the dueca-gproject command.

    dueca-gproject clone --remote <git url> \
                         [--node solo] [--version some-version]

The script uses the mapping between nodes and machine class to check
out the software corresponding to the right machine class. The "solo"
node, machine class and platform are the default. By specifying a git
branch, a git hash or a git version tag `--version`, you get the
software corresponding to that branch, version tag or hash.

A second command updates all the modules and software borrowed from
other projects:

    dueca-gproject refresh [--force] [--auto-borrow-for-dco]

The `--force` option might be obvious, it forces a refresh. The
`--auto-borrow-for-dco` is related to the way `dueca-gproject` finds
the `comm-objects` folders belonging to other projects. Any project
that you borrow a DCO file from must be listed in the `modules.xml`
file for the machine class, since the git url will be found there. If
the git url cannot be found, the `--auto-borrow-for-dco` will assume
that the project you are trying to borrow from is one of the base
projects under `DAPPS_GITROOT`, and silently add the project to your
`modules.xml` file.

Note that these commands are the only ones that use git behind the
scenes. For all other interaction you need to directly work with git.
That means that if you already have a checked-out clone of your
project on your computer, a good way to continue working is:

    # first get all updates that were made to this project elsewhere
	git pull
	# and ensure any changes to borrowed modules and dco's are in
	dueca-gproject refresh

See also the next section.

### Using git; daily work; pull, commit, push

After the initial clone or the creation of a new project, you would
want to keep track of all the work you do on your project with
git. Working with git can be quite complex, and a quick search on the
internet will give you many tutorials and solutions. However if you
follow a number of simple practices you will seldom run into trouble,
and it will be easy:

- You will want to keep you local work in sync with the upstream
  repository. Since at one point you might be working intermittently
  on your development desktop or laptop, and on one or more of the
  computers for the simulator that you are using for deployment, it is
  good practice to synchronize your work with the upstream
  repository. Before starting work on a new computer, do a pull:

      git pull

- Whenever you add a new file to your project, tell git that you want
  to keep this file in the repository, by issuing an add command:

      git add <new-file>

- When you have finished a job on your project, you for example added
  the code for a module, worked on that, and added and created the DCO
  file that you are going to use, create a commit. A commit tells git
  that you have reached a state of your project that you think should
  work, and that you want to remember. Add a meaningful commit
  message.

      git commit the..files..I..worked..on

  or

      git commit -a

  You can either specify which files you want in the commit, or with
  the `-a` option specify all tracked files that have been
  changed. You can use the `-m` option to add a commit message, as an
  example:

	  git commit -a -m "Added an option to configure wind conditions"

- Likewise, after you are done with working on your project for the
  day, or for now (e.g., because you are running to the lab to test
  your project there), ensure that the remote repository has your
  latest changes. Commit, if you have not done that yet, and do a
  push:

      git push

  When you do walk over to the lab, start with a pull on all machines
  where you will be editing and testing.

### Using git; branching, squashing and merging

Like most version control systems (and maybe even better), git can
track multiple versions of you software. Most commonly, developers use
"branches" to organise their work into specific topics. If you don't
tell git otherwise, it will use the default branch "master". Using
master can be a bit problematic, because many remote repositories (on
gitlab.tudelft.nl for example), will limit the ways in which you can
interact with master. It is way better to use a self-defined branch.

If, as a student, you are given an existing project to work on, the
project will still keep its name, and you will be making a "clone" on
the repository server (e.g., gitlab.tudelft.nl) to work with. Once you
have that clone, use that url to clone the project to your desktop or
laptop, and create a meaningful branch, e.g., `jdoe-new-eid-display`,
assuming jdoe is your TU Delft netid. When you check-out the project
on the simulator, also specify that branch using the `--version` flag.

Initially you will probably work off that new branch, but when you get
more proficient, you might be tempted to try something experimental,
and you can always create new branches from whichever version/branch
you currently have.

As you continue your work, you will find that it is best to subdivide
the work into multiple steps. You might first create your new
display. After initially creating that, and testing that it works, do
a commit, with a descriptive message, so for example:

    dueca-gproject new-module eid-approach-display

    # ... steps in which you add files, and edit these

    # tell git that we have new files, e.g.:
    git add EidApproachDisplay.hxx EidApproachDisplay.cxx

    # make a commit
    git commit -a -m "created new EidApproachDisplay"

Typically, when testing, you might find that there are small details
to be fixed. It is not uncommon to have a whole series of smaller
edits and commits, until you are satisfied with the result. To remove
these from the history, and keep a cleaner overview for later, git
lets you "squash" these commits together.

When you have finished your project, and are running the experiment,
it is good practice to record which exact version of the software was
used to run the experiment. Add a git tag to do that:

    git tag -a jdoe-eval-experiment \
            -m "Version used for the first evaluation experiment"
    # ensure that the tag is also on the remote repository
    git push origin jdoe-eval-experiment

In this example the commit message was added on the command line with
the '-m' flag. You can of course also add the message in the editor.

Summarizing:

- Don't work on "master", work on a branch.

- Use your netid as a prefix to the branch names, to prevent possible
  conflicts with other people's branch names.

- Squash these "o, and I forgot this" commits into the commit describing
  the main work.

- By all means, mark the specific version you use for an experiment with
  a descriptive tag.

### Borrowing a module

In many cases, your project can use existing modules. Particularly for
using lab hardware (steering wheels, sticks, control columns, motion
systems) and common things like outside visuals, 3D sound, the modules
have already been written and can be re-used. You can add such a
module to your project by (for example with FlexiStick):

    dueca-gproject borrow-module --name flexi-stick \
                   --remote dgrbase:///FlexiStick.git

In this example, a somewhat particular url is used for the location of
the FlexiStick project from which we borrow. Since FlexiStick is one
of the standard DUECA projects used in our group, the special URL
`dgrbase:///` can be used. When interacting with git, this will be
replaced by the value of the `DAPPS_GITROOT_base` environment variable.

Of course, if you borrow from a project not in that location, specify
the full git url.

### Creating a module

Creating a new module with `dueca-gproject` creates the folder for the
new module and default `comm-objects.lst` and `CMakeLists.txt`
files. The command is simply:

    dueca-gproject new-module --name my-new-module

Optional arguments are `--inactive`, which will indicate that the
module is not to be compiled for the machine class you are developing
for, and `--pseudo`, which indicates that you are creating a pseudo
module, simply space for data files, and no code.

## Manual corrections

At some point it may be quicker to manually edit the configuration
files; here is some explanation on how and when to do this.

### modules.xml file

The `modules.xml` files are located under `.config/class/&lt;machine
class&gt;/`. They list, per machine class, what modules are included
for the machine class, and which modules are borrowed. In addition,
the project url for the own project and each borrowed project is
recorded there.

If your project moves to another remote url, this project url is to be
adjusted.

The borrowed modules, or the projects from which DCO objects are
borrowed, must be listed here with their url's. These must be one of
the many formats of git url that are possible, or use a shorthand
prefix, e.g., the `dgr:///` prefix, or a `dgrbase:///` prefix to
indicate a url based on the `DAPPS_GITROOT` or `DAPPS_GITROOT_base`
environment variable.

Here is a small example from a `modules.xml` file.

    <project>
      <url>dgr:///DrivingSimulator.git</url>
      <module>ECI</module>
      <module>CarDynamics</module>
      <module>Dashboard</module>
      <module>IDSS</module>
      <module>SensorSuite</module>
    </project>

For each project, either the own project or a project from which
modules are borrowed, has a project entry here, with the project url
and a list of borrowed modules. When deploying a project on a
different machine class, it might be quicker to open the `modules.xml`
file for that class, and copy over the relevant modules from the
`modules.xml` file for the solo machine class. After this, run a
`dueca-gproject` refresh to actually get the needed modules and dco
files.

### DCO objects and comm-objects.lst

Each module folder also has a `comm-objects.lst` file. That file is
converted into a `comm-objects.h` file that is included in the module
code, and used for determining which dco files have to be converted
with the code generator. Simply edit these files, adding the needed
dco files; lines starting with a "#" or empty lines will be ignored.

### A module's CMakeLists.txt

The `CMakeLists.txt` files in the module folders are the proper place
to add dependencies on external libraries. See also the page on
[using cmake](@ref cmake).

## Run configuration and deployment

### Preparing a platform

A platform consists of all configuration needed to run your DUECA
project on a specific installation, such as a simulator. When running
on, e.g., the SIMONA Research Simulator, each computer involved in the
simulation runs the code for a specific DUECA node, and the code for
your project needs to be cloned to and compiled on all these
computers, with the configuration appropriate to the machine
class. You can manually prepare a platform, or use a template to start
creating the platform, if the specific combination of computers you
want to run is already defined in the template.

For manual creation of a platform, you need to:

- Create the platform
- Create the machine classes for all nodes
- Create nodes on the platform with the appropriate machine class

For SIMONA, an example would be:

    dueca-gproject new-platform --name SRS

    # machine classes
    dueca-gproject new-machine-class --name srs-ecs --gui gtk3
    dueca-gproject new-machine-class --name srs-io --gui none
    dueca-gproject new-machine-class --name srs-host --gui none
    dueca-gproject new-machine-class --name srs-efis --gui none
    dueca-gproject new-machine-class --name srs-sound --gui none
    dueca-gproject new-machine-class --name srs-ig --gui none

    # nodes
    dueca-gproject new-node --name srsecs --machine-class srs-ecs \
                   --platform SRS --num-nodes 9 --node-number 0 \
                   --if-address 192.168.2.11 --cmaster srsctrlecat \
                   --gui gtk3
    dueca-gproject new-node --name srsctrlecat --machine-class srs-io \
                   --platform SRS --num-nodes 9 --node-number 1 \
                   --if-address 192.168.2.21 \
                   --gui none
    dueca-gproject new-node --name srsefis1 --machine-class srs-efis \
                   --platform SRS --num-nodes 9 --node-number 2 \
                   --if-address 192.168.2.9 --cmaster srsctrlecat \
                   --gui none
    dueca-gproject new-node --name srsefis2 --machine-class srs-efis \
                   --platform SRS --num-nodes 9 --node-number 3 \
                   --if-address 192.168.2.23 --cmaster srsctrlecat \
                   --gui none
    dueca-gproject new-node --name srssound --machine-class srs-sound \
                   --platform SRS --num-nodes 9 --node-number 4 \
                   --if-address 192.168.2.23 --cmaster srsctrlecat \
                   --gui none
    dueca-gproject new-node --name srshost --machine-class srs-host \
                   --platform SRS --num-nodes 9 --node-number 5 \
                   --if-address 192.168.2.4 --cmaster srsctrlecat \
                   --gui none
    dueca-gproject new-node --name srsig1 --machine-class srs-ig \
                   --platform SRS --num-nodes 9 --node-number 6 \
                   --if-address 192.168.2.6 --cmaster srsctrlecat \
                   --gui none
    dueca-gproject new-node --name srsig2 --machine-class srs-ig \
                   --platform SRS --num-nodes 9 --node-number 7 \
                   --if-address 192.168.2.7 --cmaster srsctrlecat \
                   --gui none
    dueca-gproject new-node --name srsig3 --machine-class srs-ig \
                   --platform SRS --num-nodes 9 --node-number 8 \
                   --if-address 192.168.2.8 --cmaster srsctrlecat \
                   --gui none

You see that this takes quite some steps. Now you still have to edit
the `modules.xml` files for each of the new machine classes to include
the proper modules for this machine class.

There is a shortcut if you have a configuration file for the platform:

    dueca-gproject prepare-platform --name SRS \
                   --template /usr/share/dueca/data/default/platform-srs.xml

This populates the proper machine classes, and creates the platform
and nodes. Optionally, you can uses a `--nodes` argument selecting
only a part of the nodes.

### Preparing a node's run configuration

When a DUECA executable is started, it is started from the node's run
folder. This folder should have the files, or links to the files, that
are needed by the dueca process for this node.

Each node has a run configuration under `run/&lt;platform&gt;/`, named
after the node. Most nodes have three files in there:

- `links.script`, a script that should be "source-d" to run (type
  `. links.script` in the shell). Adapt this script to create links or
  otherwise populate the nodes run folder, typically these are links
  to files in `/run/run-data` or to files in pseudo-modules.

- `clean.script`. This script should undo the effects of
  `links.script`, by cleaning out all links and generated files.

- `dueca.cnf` (scheme version) or `dueca_cnf.py` (python
  version). This is the configuration file that your DUECA process
  needs to collaborate with the other DUECA processes on the
  simulator. The file as created by the `new-node` command is usually
  correct. The only thing that sometimes is adjusted is the number of
  threads that should be run by DUECA.

Node number 0 is the node with the experiment control interface. This
node also has the `dueca.mod` or `dueca_mod.py` file that defines the
complete interface. In general, you can create this file using the
`dueca.mod` or `dueca_mod.py` from the development stage in the `solo`
node as a template, and modify it by assigning all modules to their
proper node.

### Deployment on a platform

For deployment on a platform, the platform, machine classes and nodes
can be created from your development desktop or laptop, as shown
above. After that, walk over to (or remote login to) the computers of
the simulator, and on each computer clone the project with
`dueca-gproject`. In general, the projects are located under the
`dapps` or `gdapps` folder of the username under which you log in to the
simulator computers. This uses the mapping between node and machine class 
to select the proper machine class, for example:

    dueca-gproject clone --remote <project url> --node srsecs

There are a number of common pitfalls when deploying a project on the
computers/nodes of a platform:

- Each `modules.xml` file for a specific machine class should contain
  the modules that you borrow, and entries for the projects from which
  you borrow comm-objects folders. You might have to go in and copy
  these from the solo machine `modules.xml` file.

- When you borrow dco files that use other, nested dco files, you need
  to also borrow these nested dco files, and specify these in the
  `comm-objects.lst` files. Code generation and compilation works
  correctly also if you list these dco files in only one of the
  `comm-objects.lst` files. If you distribute the modules over
  different nodes, you may find that one of the `comm-objects.lst`
  files was not yet correct; this generally shows up as a failure to
  find the (generated) headers of the included dco objects.

- When the machine class and node are not in the `machinemapping.xml`
  file, the clone/checkout will default to solo.

- the machine-class specific `config.cmake` file contains code to
  include/link the used widget library (typically gtk2 or gtk3), and
  to link platform-specific (typically IO) libraries. These typically
  need to be adjusted.

### Correcting or expanding a platform

You might want to add a node to a platform later. You can simply add
the node using the `new-node` command. When you have done that, verify
that the node numbers do not conflict, and that the total number of
nodes is correct in all `dueca_cnf.py`/`dueca.cnf` files.
