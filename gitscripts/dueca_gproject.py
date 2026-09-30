#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK
# -*-python-*-
"""item            : dueca-gproject
made by         : RvP
date            : 2021
category        : python program
description     : DUECA project management with git
language        : python
changes         :
copyright       : 2021 TUDelft-AE-C&S
copyright       : 2022 Rene van Paassen
license         : EUPL-1.2
"""


# uses GitPython

import sys
import os
import warnings
from datetime import date
import subprocess
import re
import argparse
from argparse import Namespace
from collections import ChainMap
import tempfile
import git
import argcomplete
from lxml import etree

try:
    # production mode
    from duecautils import verboseprint
    from duecautils.modules import Modules, projectSplit, checkGitUrl, RootMap, MainOrMaster
    from duecautils.machinemapping import NodeMachineMapping
    from duecautils.githandler import GitHandler
    from duecautils.verboseprint import dprint
    from duecautils.policy import Policies
    from duecautils.xmlutil import XML_interpret_bool, XML_tag, XML_comment

except (ImportError, ModuleNotFoundError):
    from duecautils.duecautils import verboseprint
    from duecautils.duecautils.modules import Modules, projectSplit, checkGitUrl, RootMap, MainOrMaster
    from duecautils.duecautils.machinemapping import NodeMachineMapping
    from duecautils.duecautils.githandler import GitHandler
    from duecautils.duecautils.verboseprint import dprint
    from duecautils.duecautils.policy import Policies
    from duecautils.duecautils.xmlutil import XML_interpret_bool, XML_tag, XML_comment

# suppress argparse warnings for now, remove this once Ubuntu 18.04 and 20.04 are no longer
# supported, and argparse calls can be fixed
warnings.filterwarnings("ignore", category=DeprecationWarning)

# """
# Git interaction with dueca-project

# When using 'git' as back-end for dueca-project, each project is housed
# in its own git repository. The dueca-project interface handles one
# remote repository for the current project, and can accept different
# repositories for borrowed modules.
# """

helptext = """
Project script for adapting a DUECA project.

This new project structure uses cmake for configuration and git for
version control. A project folder contains the following structure

Project                           - main project
Project/Project                   - project folder
Project/Project/comm-objects      - communication objects
Project/Project/AModule           - an "own" module in the project
Project/Project/.config           - configuration folder
Project/Project/run               - run folder
Project/OtherProject/BModule      - a borrowed module from another project
                                    (separate shallow git check-out)

The .config folder has the following configuration:
    machine                       - a file with the class name for this
                                    machine, e.g. "solo" or "ig"
    machinemapping.xml            - a file describing the mapping between
                                    nodes (computers) and machine
                                    class
    class/                        - a folder with all configured machine
                                    classes
    class/solo                    - folder for the solo class
    class/solo/modules.xml        - file with a list of modules for this class
    class/solo/config.cmake       - per-class cmake configuration additions

In our old Makefile-based structure, each computer/node had basically its
own machine class. The main makefile was adapted to reflect the dependency
on platform libraries, and the modules.<machine> file was adapted to
indicate which modules are needed for the node.

In the new structure, a "machine class" is introduced. All computers that
share a software set-up can share a machine class. An set of example
classes for e.g. the SIMONA Research Simulator:

    - ecs     - Experiment control station, logging, interfaces
    - efis    - Flight instruments code
    - ig      - Image generator, with scene graph 3D vis
    - control - Control loading, with EtherCAT IO, etc.
    - host    - Host computer, mainly calculation, Motion IO

When deploying a simulation on a specific hardware platform, you
indicate what machine class you need on a particular computer.

Note that there are three different places to insert your CMake
instructions; the main CMakeLists.txt file, which is used by all builds,
the CMakeLists.txt files in each module and in each comm-objects folder,
used only if that module or a DCO file is needed on a machine class,
and the .config/class/<machinclass>/config.cmake file, which is specific
to a class of machines. Hints for using the new structure:

    . Adjust main CMakeLists.txt only for choices common to *all* machine
      classes; usually this is only the script language choice.
    . Add platform-dependent IO in the .config/<machine class>/config.cmake
      file. Thus for machine-dependent IO libraries. Also the choice for
      a dueca gui (gtk2, gtk3, glut, none, etc.) is made here.
    . Use CMake to detect and add all libraries needed by your module code
      locally in the module's CMakeLists.txt file. This ensures that when
      a module is borrowed, compilation of that module automatically adds
      the right compile flags, and any library dependencies are added.

Environment variables:

    To provide some flexibility in the location and access of the git
    repositories, environment variables can be used to define custom
    prefixes for your repositories.

    DAPPS_GITROOT: will replace the prefix dgr:///, for example
       DAPPS_GITROOT=git@myserver:myuser/ , will convert dgr:///MyProject.git
       to point to git@myserver:myuser/MyProject.git

    DAPPS_GITROOT_myprefix: will replace prefix drgmyprefix:/// with its
       contents. for example DAPPS_GITROOT_base=git@myserver:mygroup/
       will convert dgrbase:///CommonProject.git to point to
       git@myserver:mygroup/CommonProject.git
"""

mainparser = argparse.ArgumentParser(description="control your dueca project")
mainparser.add_argument(
    "--verbose", action="store_true", help="Verbose run with information output"
)
subparsers = mainparser.add_subparsers(help="commands", title="commands")


def get_dueca_version():
    """Detect current dueca version

    Returns
    -------
    str
        DUECA version string
    """
    dc = subprocess.run(("dueca-config", "--version"), stdout=subprocess.PIPE)
    return dc.stdout.strip().decode("UTF-8")


_dueca_cnf_defaults = {
    "this-node-id": 0,
    "no-of-nodes": 1,
    "send-order": 0,
    "highest-manager": 4,
    "run-in-multiple-threads?": True,
    "rt-sync-mode": 2,
    "graphic-interface": "gtk3",
    "tick-base-increment": 100,
    "tick-compatible-increment": 100,
    "tick-time-step": 0.01,
    "communication-interval": 50,
    "if-address": "127.0.0.1",
    "mc-address": "224.0.0.1",
    "mc-port": 7500,
    "master-host": "correct this value",
    "packet-size": 4096,
    "bulk-max-size": 128 * 1024,
    "comm-prio-level": 3,
    "unpack-prio-level": 2,
    "bulk-unpack-prio-level": 2,
    "dueca-version": get_dueca_version(),
    "date": date.today().strftime("%d-%b-%Y"),
}


def _gui_choices():
    return ("none", "gtk4", "gtk3", "gtk2", "glut", "glut-gui")


def read_transform_and_write(f0: str, f1: str, subst: dict, insert = None):
    """Read a template file, and write to output file with substitutions

    Parameters
    ----------
    f0 : str
        Template file to read
    f1 : str
        Output file to write
    subst : dict
        Key/value strings, which will be used to replace @key@ text
    insert : dict | None, optional
        Key/line strings, which will be used to insert lines when key
        is found in a line of the template file

    Returns
    -------
    str
        Name of the resulting file
    """

    with open(f0, "r", encoding="UTF-8") as fr:
        if insert:
            lines = []
            for l in fr.readlines():
                lines.append(l)
                for k, txt in insert.items():
                    if k in l:
                        lines.append(txt)
                        lines.append("\n")
            fdata = "".join(lines)
        else:
            fdata = "".join(fr.readlines())

    for k, v in subst.items():
        if f"@{k}@" in fdata:
            fdata = str(v).join(fdata.split(f"@{k}@"))
    with open(f1, "w", encoding="UTF-8") as fw:
        fw.write(fdata)
    return f1


def create_and_copy(
    dirs: list,
    files: list,
    subst: dict,
    keepcurrent: bool = False,
    inform: bool = False,
    insert = None,
):
    """Create folders and copy files

    Parameters
    ----------
    dirs : list[str]
        List of folders to create
    files : list[str]
        List of files to create from templates
    subst : dict
        Substitution to modify files
    keepcurrent : bool, optional
        If true, does not overwrite, by default False
    inform : bool, optional
        Flag for feedback, by default False
    insert : dict | None, optional
        Additional lines to insert in files, by default None

    Returns
    -------
    list[str]
        List of created files

    Raises
    ------
    Exception
        If folder cannot be created
    ve
        If folder names are incorrect
    """
    for _d in dirs:
        try:
            d = _d.format(**subst)
            if not os.path.exists(d):
                dprint("creating dir", d)
                os.mkdir(d)
            else:
                if keepcurrent:
                    pass
                else:
                    raise FileExistsError(f"Failed to create directory {d}")
        except ValueError as ve:
            print(f"Problem formatting '{_d}'", file=sys.stderr)
            raise ve

    dc = subprocess.run(
        ("dueca-config", "--path-datafiles"), stdout=subprocess.PIPE, check=True
    )
    duecabase = (
        dc.stdout.strip().decode("UTF-8")
        + os.sep
        + "data"
        + os.sep
        + "default"
        + os.sep
    )

    fnew = []
    for f in files:
        f1 = f[1].format(**subst)
        if keepcurrent and os.path.isfile(f1):
            if inform:
                print(f"Keeping existing '{f1}'")
            continue
        if inform:
            print(f"Created '{f1}'")
        dprint("writing", f1)
        fnew.append(read_transform_and_write(duecabase + f[0], f1, subst, insert))
    return fnew


def get_dueca_prefix():
    """Obtain the DUECA install prefix

    Returns
    -------
    str
        Install prefix for DUECA
    """
    dc = subprocess.run(
        ("dueca-config", "--prefix"), stdout=subprocess.PIPE, check=True
    )
    return dc.stdout.strip().decode("UTF-8")


def git_lsremote(url):
    """List all git remote refs

    Parameters
    ----------
    url : str
        GIT URL

    Returns
    -------
    dict
        remote GIT refs
    """
    remote_refs = {}
    g = git.cmd.Git()
    for ref in g.ls_remote(url).split("\n"):
        hash_ref_list = ref.split("\t")
        remote_refs[hash_ref_list[1]] = hash_ref_list[0]
    return remote_refs


_mcdecode = re.compile(r'set\s*\(\s*GUI\_COMPONENT\s+"([a-zA-Z0-9-]+)"\s*\)')


def get_machineclass_gui(mclass):
    """Determine the gui library used in this project

    Parameters
    ----------
    mclass : str
        Machine class

    Returns
    -------
    str|None
        Name of the GUI library (typically 'gtk3', 'gtk4' or 'none')
    """
    with open(f".config/class/{mclass}/config.cmake", encoding="UTF-8") as f:
        for l in f:
            res = _mcdecode.match(l)
            if res:
                return res.group(1)
    return "none"


def git_remote_url(base: str, project: str):
    """Get remote GIT url

    Parameters
    ----------
    base : str
        Base path for repository group
    project : str
        Project name

    Returns
    -------
    str
        Complete git url
    """
    return base + f"/{project}.git"


def git_ensure_remote_clean(remote: str, project: str):
    """If using a remote git for new projects, ensure it has not been used

    Parameters
    ----------
    remote : str
        URL for remote git
    project : str
        project name

    Raises
    ------
    Exception
        Remote already has a dueca project
    Exception
        Remote name is not correct
    """
    # ensure the remote is clean
    with tempfile.TemporaryDirectory() as tmpdir:
        git.Repo.clone_from(remote, tmpdir, depth=1, shallow_submodules=True)
        files = os.listdir(tmpdir)
        if (
            "run" in files
            or ".config" in files
            or "CMakeLists.txt" in files
            or len(files) > 1
        ):
            raise FileExistsError(f"Remote copy at {remote} is not clean")

    # ensure the remote and project name match
    if (
        remote[-4:] != ".git"
        or (not project)
        or remote[-len(project) - 4 : -4] != project
    ):
        raise AttributeError(
            f"Last component of remote name '{remote}'"
            f" should end with .git and match project name '{project}'"
        )


def trim_lines(text: str):
    """Clean up text read from XML

    Parameters
    ----------
    text : str
        Input text

    Returns
    -------
    str
        Text with empty leading and trailing empty lines removed,
        and trailing whitespace stripped
    """
    lines = [l.replace("\t", "        ") for l in text.splitlines()]
    if lines and not lines[0].strip():
        del lines[0]
    if lines and not lines[-1].strip():
        del lines[-1]

    nspace = len(lines[0]) - len(lines[0].lstrip())
    return "\n".join(
        [
            (len(l) - len(l.lstrip()) >= nspace and l[nspace:].rstrip()) or l.strip()
            for l in lines
        ]
    )


def project_name_from_url(remote: str):
    """Decode project name from url

    Parameters
    ----------
    remote : str
        Project url

    Returns
    -------
    str
        Just the project name
    """
    return remote[:-4].split("/")[-1]


class NewProject:
    """Create a new project."""

    @classmethod
    def args(cls, xtrsub):
        """Add subcommand to a command line parser

        Parameters
        ----------
        xtrsub :
            Current list of subparsers
        """
        parser = xtrsub.add_parser("new", help="Create a new project")
        parser.add_argument(
            "--name", type=str, required=True, help="A name for the new project"
        )
        parser.add_argument(
            "--script",
            type=str,
            default="python",
            choices=("scheme", "python"),
            help="Specify scripting language, 'scheme' or 'python'",
        )
        parser.add_argument(
            "--gui",
            type=str,
            default="gtk3",
            choices=_gui_choices(),
            help="GUI system for the default solo class",
        )
        parser.add_argument(
            "--remote",
            type=str,
            help="URL of a remote repository, if not supplied, your new"
            " project will only be local.",
        )
        parser.set_defaults(handler=NewProject)

    dirs = (
        "{project}",
        "{project}/{project}",
        "{project}/{project}/build",
        "{project}/{project}/comm-objects",
        "{project}/{project}/.config",
        "{project}/{project}/.config/class",
        "{project}/{project}/.config/class/solo",
        "{project}/{project}/run",
        "{project}/{project}/run/run-data",
        "{project}/{project}/run/solo",
        "{project}/{project}/run/solo/solo",
    )

    files = (
        ("CMakeLists.txt.app", "{project}/{project}/CMakeLists.txt"),
        ("CMakeLists.txt.com", "{project}/{project}/comm-objects/CMakeLists.txt"),
        ("machine", "{project}/{project}/.config/machine"),
        ("machinemapping.xml", "{project}/{project}/.config/machinemapping.xml"),
        ("modules.xml", "{project}/{project}/.config/class/solo/modules.xml"),
        ("config.cmake", "{project}/{project}/.config/class/solo/config.cmake"),
        ("links.script", "{project}/{project}/run/solo/solo/links.script"),
        ("clean.script", "{project}/{project}/run/solo/solo/clean.script"),
        ("comm-objects.lst", "{project}/{project}/comm-objects/comm-objects.lst"),
        ("policylist.xml", "{project}/{project}/.config/policylist.xml"),
        ("build.gitignore", "{project}/{project}/build/.gitignore"),
        ("project.gitignore", "{project}/{project}/.gitignore"),
        ("run-data-README.md", "{project}/{project}/run/run-data/README.md"),
        ("project-README.md", "{project}/{project}/README.md"),
    )

    sfile = (
        ("dueca.cnf.in", "{project}/{project}/run/solo/solo/dueca.cnf"),
        ("dueca.mod.in", "{project}/{project}/run/solo/solo/dueca.mod"),
    )
    pfile = (
        ("dueca_cnf.py.in", "{project}/{project}/run/solo/solo/dueca_cnf.py"),
        ("dueca_mod.py.in", "{project}/{project}/run/solo/solo/dueca_mod.py"),
    )

    def __call__(self, ns):
        """Performs the actual work

        Parameters
        ----------
        ns : dict
            Parsed command line options

        Raises
        ------
        FileExistsError
            Files/folders already created.
        """
        # check that the local disk is free
        if os.path.exists(ns.name):
            raise FileExistsError(
                f"Folder {ns.name} already exists, cannot create project"
            )

        # check that the remote project is clean/has not code
        if ns.remote:
            RootMap().addProjectRemote(ns.remote)
            git_ensure_remote_clean(RootMap().urlToAbsolute(ns.remote), ns.name)
            remoteurl = RootMap().urlToRelative(ns.remote, ns.name)
        else:
            remoteurl = ""

        create_and_copy(
            NewProject.dirs,
            NewProject.files,
            {
                "project": ns.name,
                "url": remoteurl,
                "gui": ns.gui,
                "scriptlang": ns.script,
                "class": "solo",
            },
        )

        # initialize git repository
        repo = git.Repo.init("{project}/{project}".format(project=ns.name))
        repo.active_branch.rename("main")

        # add the default config files
        cnfdef = ChainMap(
            {"graphic-interface": ns.gui, "project": ns.name}, _dueca_cnf_defaults
        )
        if ns.script == "python":
            create_and_copy([], NewProject.pfile, cnfdef)
        else:
            create_and_copy([], NewProject.sfile, cnfdef)

        # add all files to git
        repo.index.add(repo.untracked_files)

        # commit the results
        repo.index.commit(f"Initial commit for files in project {ns.name}")

        # add the remote and push results
        if ns.remote:
            repo.create_remote("origin", RootMap().urlToAbsolute(ns.remote))
            repo.git.push("--set-upstream", "origin", "main")

        print(f"Created new DUECA project {ns.name}")


NewProject.args(subparsers)


class CloneProject:
    """Clone an existing project from a remote repo"""

    command = "clone"

    @classmethod
    def args(cls, xtrsub):
        """Add subcommand to a command line parser

        Parameters
        ----------
        xtrsub :
            Current list of subparsers
        """
        parser = xtrsub.add_parser(
            CloneProject.command, help="Clone/check out an existing project"
        )
        parser.add_argument(
            "--remote",
            type=str,
            required=True,
            help="The URL of the project repository",
        )
        parser.add_argument(
            "--node",
            type=str,
            default="solo",
            help="Node for which the project is cloned",
        )
        parser.add_argument(
            "--version",
            type=str,
            default="master",
            help="git version, branch, etc., default master",
        )
        parser.add_argument(
            "--no-refresh",
            action="store_true",
            default=False,
            help="Do not refresh or check out borrowed modules/dco",
        )
        parser.add_argument(
            "--full",
            action="store_true",
            help='Always do a full checkout, also when node is not "solo"',
        )
        parser.set_defaults(handler=CloneProject)

    def __call__(self, ns):

        _, name = projectSplit(ns.remote)

        # check that the local disk is free
        if os.path.exists(name):
            raise FileExistsError(
                f"Folder {name} already exists, cannot clone project there."
            )

        os.mkdir(name)

        repo = git.Repo.init(f"{name}/{name}")
        orig = repo.create_remote("origin", RootMap().urlToAbsolute(ns.remote))
        os.chdir(f"{name}/{name}")  # now in new project dir

        # force full checkout for solo/development
        if ns.node == "solo":
            ns.full = True

        # sparse checkout, only the essential as specified in modules.xml
        if not ns.full:
            repo.git.config("core.sparseCheckout", "true")

            # init the sparse checkout file with default files and folders
            with open(".git/info/sparse-checkout", "w", encoding='UTF-8') as ms:
                ms.write(
                    "run/*\n.config/*\ncomm-objects/*\n"
                    "CMakeLists.txt\nREADME.md\n.gitignore\nbuild/*\n"
                )

        # pull the existing code, and create master/selected branches
        try:
            orig.fetch()
        except git.GitCommandError as e:
            print(
                f"Cannot fetch from {RootMap().urlToAbsolute(ns.remote)}: {e}\n"
                "Clone failed, check url and access rights"
            )
            sys.exit(-1)
        dprint("check out on branch", ns.version)
        remotemain = MainOrMaster(orig)
        if ns.version == "master":
            ns.version = str(remotemain)
        if ns.version not in ("master", "main"):
            branch = repo.create_head(ns.version, orig.refs[ns.version])
            branch.set_tracking_branch(orig.refs[ns.version])

        # checkout the selected branch, and merge with current new branch
        repo.git.checkout(ns.version)
        repo.git.merge()

        # depending on the node selected, use the mapping of node->machine
        # to find the machine class
        here = os.getcwd()
        nmm = NodeMachineMapping(here)
        mclass = nmm.getClass(ns.node)

        # write the machine file with the machine class
        with open(".config/machine", "w", encoding="UTF-8") as f:
            f.write(mclass + "\n")

        # get the current modules list
        mod = Modules()
        machine = mod.mclass
        dprint("Module set for machine class", mod.mclass)

        if not ns.full:
            dprint("Sparse checkout, own modules", mod.getOwnModules())
            # add the module folders to the sparse checkout
            with open(".git/info/sparse-checkout", "a", encoding="UTF-8") as ms:
                for m in mod.getOwnModules():
                    dprint(f"adding module {m} to sparse")
                    ms.write(f"{m}/*\n")

            # update the fetch
            repo.git.pull()
            repo.git.checkout(ns.version)

        if not ns.no_refresh:
            mod.refreshBorrowed()

        os.chdir("../..")

        print(f"Cloned project to {here} for machine class {machine}")
        if ns.version in ("master", "main"):
            print(
                "\nYou checked out a master or main branch. When starting\n"
                "developing, use git to switch to a development branch"
            )


CloneProject.args(subparsers)


class OnExistingProject:
    """Base class for actions that are performed on an existing project"""

    def __init__(self, _command: str, *_args, **_kwargs):
        """Base class for actions

        Parameters
        ----------
        _command : str
            Current command

        Raises
        ------
        FileNotFoundError
            When the current path, or none of its ancestors, look like
            a main project folder
        """

        # to remember push and pops
        self.dirpath = []
        self.scriptlang = None

        # figure out the projectdir and project name
        curpath = os.getcwd().split(os.sep)
        self.inprojectdir = True
        while len(curpath):
            projectdir = "/".join(curpath)
            if (
                os.path.exists(f"{projectdir}/.config/machine")
                and os.path.exists(f"{projectdir}/CMakeLists.txt")
                and os.path.isdir(f"{projectdir}/run")
                and os.path.isdir(f"{projectdir}/.git")
            ):
                break
            del curpath[-1]
            self.inprojectdir = False

        if len(curpath) < 2 or curpath[-1] != curpath[-2]:
            print(
                f"Could not find project folder in {os.getcwd()} or parents\n"
                f"(checked for .config/machine, CMakeLists.txt, .git/ and run/ folders)",
                file=sys.stderr,
            )
            raise FileNotFoundError("Cannot find a project directory here")

        self.project = curpath[-1]
        self.projectdir = projectdir

    def push_dir(self, pdir=None):
        """Enter given folder

        Parameters
        ----------
        pdir : str, optional
            Change dir, by default None to enter project dir
        """
        self.dirpath.append(os.getcwd())
        os.chdir(pdir or self.projectdir)

    def pop_dir(self):
        """Return to old folder"""
        os.chdir(self.dirpath[-1])
        del self.dirpath[-1]

    def check_scriptlang(self):
        """Verify and return which script language is being used in the project

        Returns
        -------
        str
            Script language, currently 'scheme' or 'python'
        """

        if self.scriptlang is None:
            self.push_dir()
            try:
                cm = subprocess.run(
                    ["cmake", "--build", "build", "--", "scriptlang"],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    check=True,
                )
                self.scriptlang = cm.stdout.strip().decode("UTF-8").split()[0]
            except Exception as e:
                print(
                    f"Could not determine script language, {e}\n"
                    "failed command: cmake --build build -- scriptlang",
                    file=sys.stderr,
                )
            self.pop_dir()
        return self.scriptlang


# checked
class NewModule(OnExistingProject):
    """Create a new module in this project"""

    command = "new-module"

    def __init__(self, *args, **kwargs):
        super().__init__(NewModule.command, *args, **kwargs)

    @classmethod
    def args(cls, xtrsub):
        """Add subcommand to a command line parser

        Parameters
        ----------
        xtrsub :
            Current list of subparsers
        """
        parser = xtrsub.add_parser(
            NewModule.command, help="Add a new module to the project"
        )
        parser.add_argument(
            "--name", type=str, required=True, help="A name for the new module"
        )
        parser.add_argument(
            "--pseudo",
            action="store_true",
            help="Make this a pseudo module without any source code (data only)",
        )
        parser.add_argument(
            "--inactive",
            action="store_true",
            help="Create the module, but do not include (on this node's)"
            " compilation/simulation",
        )
        parser.set_defaults(handler=NewModule)

    dirs = ("{module}",)

    files = (
        ("CMakeLists.txt.mod", "{module}/CMakeLists.txt"),
        ("comm-objects.lst", "{module}/comm-objects.lst"),
    )
    filesalt = (("README-pseudomodule.md", "{module}/README.md"),)

    def __call__(self, ns):

        # check that the module name is available
        try:
            self.push_dir()
            m = Modules()
            if not m.isNewModule(self.project, ns.name):
                raise ValueError(
                    f"The module {self.project}/{ns.name} already exists, "
                    "use an editor to adjust .config/<<mclass>>/modules\n"
                    "and run 'dueca-gproject sync'"
                )

            # create the new files
            create_and_copy(
                NewModule.dirs,
                (not ns.pseudo and NewModule.files) or NewModule.filesalt,
                {"module": ns.name, "project": self.project},
            )

            # add to the module configuration
            g = GitHandler()
            m.addModule(
                self.project,
                ns.name,
                None,
                None,
                pseudo=ns.pseudo,
                inactive=ns.inactive,
            )

            # add the files to git
            g.addFolder(ns.name)
        finally:
            self.pop_dir()

        print(f"Created new DUECA module {ns.name} in project {self.project}")


NewModule.args(subparsers)


# checked
class BorrowModule(OnExistingProject):
    """Borrow a module from another project"""

    command = "borrow-module"

    def __init__(self, *args, **kwargs):
        super().__init__(BorrowModule.command, *args, **kwargs)

    @classmethod
    def args(cls, xtrsub):
        """Add subcommand to a command line parser

        Parameters
        ----------
        xtrsub :
            Current list of subparsers
        """
        parser = xtrsub.add_parser(
            BorrowModule.command, help="Borrow a module from another project."
        )
        parser.add_argument(
            "--name", type=str, required=True, help="The name of the module"
        )
        parser.add_argument(
            "--remote",
            type=str,
            required=True,
            help="Remote URL for the project from which the module is borrowed",
        )
        parser.add_argument(
            "--pseudo",
            action="store_true",
            help="This is a pseudo (no code, data only) module",
        )
        parser.add_argument(
            "--version",
            type=str,
            help="Version, branch, commit revision to borrow. If empty,"
            "the main branch is used.",
        )
        parser.set_defaults(handler=BorrowModule)

    def __call__(self, ns):

        try:
            self.push_dir()

            m = Modules()
            project = project_name_from_url(ns.remote)

            if not m.isNewModule(project, ns.name):
                raise FileExistsError(
                    f"Module {project}/{ns.name} already borrowed,"
                    " try a 'dueca-gproject refresh'"
                )

            m.addModule(project, ns.name, ns.version, ns.remote, ns.pseudo)
            m.refreshBorrowed()

        finally:
            self.pop_dir()

        print(f"Borrowing DUECA module {ns.name} from project {project}")


BorrowModule.args(subparsers)


class BorrowProject(OnExistingProject):
    """Specify a project location for borrowing DCO files"""

    command = "borrow-project"

    def __init__(self, *args, **kwargs):
        super().__init__(BorrowProject.command, *args, **kwargs)

    @classmethod
    def args(cls, xtrsub):
        """Add subcommand to a command line parser

        Parameters
        ----------
        xtrsub :
            Current list of subparsers
        """
        parser = xtrsub.add_parser(
            BorrowProject.command, help="Add a reference to another project."
        )
        parser.add_argument(
            "--remote", type=str, required=True, help="Remote URL for the project"
        )
        parser.add_argument(
            "--version",
            type=str,
            help="Version, branch, commit revision to borrow. If empty,"
            "the main branch is used.",
        )
        parser.set_defaults(handler=BorrowProject)

    def __call__(self, ns):

        try:
            self.push_dir()

            m = Modules()
            project = project_name_from_url(ns.remote)

            if not m.isNewProject(project):
                raise FileExistsError(
                    f"Project {project} already borrowed,"
                    " try a 'dueca-gproject refresh'"
                )

            m.addModule(project, None, ns.version, ns.remote)
            m.refreshBorrowed()

        finally:
            self.pop_dir()

        print(f"Borrowing DUECA project {project}")


BorrowProject.args(subparsers)


class CopyModule(OnExistingProject):
    """Copy a module from another project"""

    command = "copy-module"

    def __init__(self, *args, **kwargs):
        super().__init__(CopyModule.command, *args, **kwargs)

    @classmethod
    def args(cls, xtrsub):
        """Add subcommand to a command line parser

        Parameters
        ----------
        xtrsub :
            Current list of subparsers
        """
        parser = xtrsub.add_parser(
            CopyModule.command, help="Copy a module from another project."
        )
        parser.add_argument(
            "--name", type=str, required=True, help="The name of the copied module."
        )
        parser.add_argument(
            "--remote",
            type=str,
            required=True,
            help="Remote URL from where the module is copied.",
        )
        parser.add_argument(
            "--version",
            type=str,
            default="main",
            help="Version, branch, export revision to copy. If empty,"
            "the main branch is used.",
        )
        parser.add_argument(
            "--newname",
            type=str,
            default="",
            help="New name of the copied module, if specified",
        )
        parser.set_defaults(handler=CopyModule)

    def __call__(self, ns):

        try:
            self.push_dir()

            m = Modules()
            project = project_name_from_url(ns.remote)

            newname = ns.newname or ns.name
            if not m.isNewModule(project, newname):
                raise FileExistsError(
                    "Cannot copy, there already is a module of this name"
                )

            g = GitHandler(self.project)
            g.copyModule(
                project,
                ns.name,
                newname,
                ns.version,
                RootMap().urlToAbsolute(ns.remote),
            )
            m.addModule(self.project, newname, ns.version, g.getUrl())

        finally:
            self.pop_dir()

        print(f"Copied DUECA module {ns.name} from project {project}" f" as {newname}")


CopyModule.args(subparsers)


class Refresh(OnExistingProject):
    """Refresh/git pull all borrowed projects and modules"""

    command = "refresh"

    def __init__(self, *args, **kwargs):
        super().__init__(Refresh.command, *args, **kwargs)

    @classmethod
    def args(cls, xtrsub):
        """Add subcommand to a command line parser

        Parameters
        ----------
        xtrsub :
            Current list of subparsers
        """
        parser = xtrsub.add_parser(
            Refresh.command, help="Refresh borrowed modules and comm-objects."
        )
        parser.add_argument(
            "--force",
            action="store_true",
            default=False,
            help="Force refresh, even if no changes detected",
        )
        parser.add_argument(
            "--auto-borrow-for-dco",
            action="store_true",
            default=False,
            help="Try to automatically borrow projects based on DCO entries\n"
            "Careful. This requires that the donating url matches the project url",
        )
        parser.add_argument(
            "--auto-find-url",
            action="store_true",
            default=False,
            help="Verify the presence of a URL before using it, and if\n"
            "needed, search/adapt the url by checking defined roots",
        )
        parser.add_argument(
            "--machineclass",
            type=str,
            default="",
            nargs="?",
            help="Switch to a different machine class before the refresh",
        )
        parser.set_defaults(handler=Refresh)

    def __call__(self, ns):

        try:
            self.push_dir()
            if ns.machineclass:
                mclasses = os.listdir(".config/class")
                if ns.machineclass not in mclasses:
                    raise ValueError(f"Machine class {ns.machineclass} does not exist")

                with open(f"{self.projectdir}/.config/machine", "w", encoding='UTF-8') as m:
                    m.write(str(ns.machineclass) + "\n")

            else:
                with open(f"{self.projectdir}/.config/machine", "r", encoding='UTF-8') as m:
                    ns.machineclass = m.read().strip()

            m = Modules()
            m.refreshBorrowed(
                auto_dco=ns.auto_borrow_for_dco, auto_url=ns.auto_find_url
            )

        finally:
            self.pop_dir()

        print("Refreshed code for borrowed modules, machine", ns.machineclass)


Refresh.args(subparsers)


class NewPlatform(OnExistingProject):
    """Create files and folders for a new platform"""

    command = "new-platform"

    startfile = (("RunProject", "{projectdir}/run/{platform}/{project}"),)

    def __init__(self, *args, **kwargs):
        super().__init__(NewPlatform.command, *args, **kwargs)

    @classmethod
    def args(cls, xtrsub):
        """Add subcommand to a command line parser

        Parameters
        ----------
        xtrsub :
            Current list of subparsers
        """
        parser = xtrsub.add_parser(
            NewPlatform.command, help="Create a new platform for deployment"
        )
        parser.add_argument(
            "--name",
            required=True,
            type=str,
            help="Name for the new deployment platform",
        )
        parser.add_argument(
            "--masternode", type=str, default="", help="Name for the timing master node"
        )
        parser.add_argument(
            "--zeronode", type=str, default="", help="Name of the no 0 node"
        )
        parser.add_argument(
            "--othernodes",
            type=str,
            nargs="+",
            default=[],
            help="Names of other nodes in the platform",
        )
        parser.set_defaults(handler=NewPlatform)

    def __call__(self, ns):

        try:
            self.push_dir()

            if os.path.exists(f"{self.projectdir}/run/{ns.name}"):
                raise FileExistsError(f"Platform {ns.name} already exists")
            os.mkdir(f"{self.projectdir}/run/{ns.name}")
            g = GitHandler(self.project)
            g.addFolder(f"{self.projectdir}/run/{ns.name}")

            if (ns.masternode and ns.zeronode) or len(ns.othernodes):
                tofill = {
                    "project": self.project,
                    "projectdir": self.projectdir,
                    "platform": ns.name,
                    "othernodes": " ".join(ns.othernodes),
                    "zeronode": ns.zeronode,
                    "masternode": ns.masternode,
                    "xnodes": "",
                    "lnodes": "|".join(ns.othernodes + [ns.zeronode, ns.masternode]),
                }
                create_and_copy([], NewPlatform.startfile, tofill)
                g.addFiles((f"{self.projectdir}/run/{ns.name}/{self.project}",))

        finally:
            self.pop_dir()

        print(f"Added platform {ns.name}")


NewPlatform.args(subparsers)


class NewNode(OnExistingProject):
    """Create a new node."""

    command = "new-node"

    def __init__(self, *args, **kwargs):
        super().__init__(NewNode.command, *args, **kwargs)

    dirs = ("{projectdir}/run/{platform}/{node}",)

    files = (
        ("links.script", "{projectdir}/run/{platform}/{node}/links.script"),
        ("clean.script", "{projectdir}/run/{platform}/{node}/clean.script"),
    )

    sfile = (
        ("dueca.cnf.in", "{projectdir}/run/{platform}/{node}/dueca.cnf"),
        ("dueca.mod.in", "{projectdir}/run/{platform}/{node}/dueca.mod"),
    )
    pfile = (
        ("dueca_cnf.py.in", "{projectdir}/run/{platform}/{node}/dueca_cnf.py"),
        ("dueca_mod.py.in", "{projectdir}/run/{platform}/{node}/dueca_mod.py"),
    )

    @classmethod
    def args(cls, xtrsub):
        """Add subcommand to a command line parser

        Parameters
        ----------
        xtrsub :
            Current list of subparsers
        """
        parser = xtrsub.add_parser(
            NewNode.command, help="Create a new node for deployment"
        )
        parser.add_argument(
            "--name", required=True, type=str, help="Name for the new node/computer"
        )
        parser.add_argument(
            "--platform",
            required=True,
            type=str,
            help="Platform where the node should be created",
        )
        parser.add_argument(
            "--num-nodes",
            required=True,
            type=int,
            help="Number of nodes participating on the platform",
        )
        parser.add_argument(
            "--node-number",
            required=True,
            type=int,
            help="Unique number for this node (0 <= number < num-nodes)",
        )
        parser.add_argument(
            "--if-address",
            default="0.0.0.0",
            type=str,
            help="Address of the own interface",
        )
        parser.add_argument(
            "--highest-priority",
            default=4,
            type=int,
            help="Priority of the highest priority manager",
        )
        parser.add_argument(
            "--cmaster", type=str, help="IP address or hostname of communication master"
        )
        parser.add_argument(
            "--gui",
            type=str,
            default="gtk3",
            choices=_gui_choices(),
            help="GUI system for the node",
        )
        parser.add_argument(
            "--machine-class",
            type=str,
            default="solo",
            help="Machine class mapping for this node",
        )
        parser.add_argument(
            "--script",
            type=str,
            default=None,
            choices=("scheme", "python", None),
            help="Script language for configuration (only needed if\n"
            "script cannot be automatically detected)",
        )
        parser.set_defaults(handler=NewNode)

    def __call__(self, ns, scriptlets=None):

        try:
            self.push_dir()

            if os.path.exists(f"{self.projectdir}/run/{ns.platform}/{ns.name}"):
                raise FileExistsError(f"Node {ns.name} already exists in {ns.platform}")
            if ns.node_number < 0 or ns.node_number >= ns.num_nodes:
                raise ValueError("Node number must be smaller than number of nodes")

            scriptlang = self.check_scriptlang()

            if ns.script and ns.script != scriptlang:
                print(
                    "Warning, you seem to have selected a script language"
                    " that does not match the one in the code",
                    file=sys.stderr,
                )
                scriptlang = ns.script
            elif scriptlang:
                pass
            elif not scriptlang and ns.script:
                scriptlang = ns.script
            else:
                raise ValueError(
                    "Cannot determine script language, please run cmake"
                    " configuration or specify the script language"
                )

            tofill = ChainMap(
                {
                    "projectdir": self.projectdir,
                    "platform": ns.platform,
                    "node": ns.name,
                    "no-of-nodes": ns.num_nodes,
                    "this-node-id": ns.node_number,
                    "send-order": (ns.cmaster and 1) or 0,
                    "highest-manager": ns.highest_priority,
                    "graphic-interface": ns.gui,
                    "if-address": ns.if_address,
                    "master-host": ns.cmaster or ns.if_address,
                },
                _dueca_cnf_defaults,
            )

            create_and_copy(NewNode.dirs, NewNode.files, tofill)
            nfiles = (ns.node_number and 1) or 2
            if scriptlang == "python":
                create_and_copy([], NewNode.pfile[:nfiles], tofill, insert=scriptlets)
            else:
                create_and_copy([], NewNode.sfile[:nfiles], tofill)

            # add the mapping between node and machine class
            nm = NodeMachineMapping(self.projectdir)
            nm.newMapping(ns.name, ns.machine_class, True)

            g = GitHandler()
            g.addFolder(f"{self.projectdir}/run/{ns.platform}/{ns.name}")

        finally:
            self.pop_dir()

        print(
            f"Added node {ns.name} in {ns.platform}, script {scriptlang}\n"
            f"graphics {ns.gui}, no {ns.node_number}/{ns.num_nodes}"
            f" connecting {ns.cmaster}"
        )


NewNode.args(subparsers)


class NewMachineClass(OnExistingProject):
    """Create files and folders to define a new machine class"""

    command = "new-machine-class"

    def __init__(self, *args, **kwargs):
        super().__init__(NewMachineClass.command, *args, **kwargs)

    dirs = ("{projectdir}/.config/class/{mclass}",)
    files = (
        ("config.cmake", "{projectdir}/.config/class/{mclass}/config.cmake"),
        ("modules.xml", "{projectdir}/.config/class/{mclass}/modules.xml"),
    )

    @classmethod
    def args(cls, xtrsub):
        """Add subcommand to a command line parser

        Parameters
        ----------
        xtrsub :
            Current list of subparsers
        """
        parser = xtrsub.add_parser(
            NewMachineClass.command, help="Create a new machine class"
        )
        parser.add_argument(
            "--name", required=True, type=str, help="Name for the new class"
        )
        parser.add_argument(
            "--gui",
            type=str,
            default="none",
            choices=_gui_choices(),
            help="GUI system to include in the class",
        )
        parser.add_argument(
            "--switch",
            action="store_true",
            default=False,
            help="Switch over to the new class",
        )
        parser.set_defaults(handler=NewMachineClass)

    def __call__(self, ns):

        try:
            self.push_dir()

            if os.path.exists(f"{self.projectdir}/.config/class/{ns.name}"):
                raise FileExistsError(f"Machine class {ns.name} already present")

            g = GitHandler()
            tofill = {
                "projectdir": self.projectdir,
                "mclass": ns.name,
                "project": self.project,
                "url": g.getUrl(),
                "gui": (ns.gui != "none" and ns.gui) or "",
            }

            create_and_copy(NewMachineClass.dirs, NewMachineClass.files, tofill)
            g.addFolder(f"{self.projectdir}/.config/class/{ns.name}")

            if ns.switch:
                with open(f"{self.projectdir}/.config/machine", "w", encoding='UTF-8') as m:
                    m.write(ns.name + "\n")

            # when created from a config, more information is available
            try:
                if ns.modules:
                    mods = Modules(self.projectdir, ns.name)

                    for url, m, v, pseudo, inactive in ns.modules:
                        project = project_name_from_url(url)
                        mods.addModule(project, m, v, url, pseudo, inactive)

            except AttributeError:
                pass
            try:
                if ns.config:
                    with open(
                        f"{self.projectdir}/.config/class/{ns.name}/" "config.cmake",
                        "a", encoding='UTF-8'
                    ) as f:
                        f.write(ns.config)
            except AttributeError:
                pass

        finally:
            self.pop_dir()

        print(f"Added new machine class {ns.name}")


NewMachineClass.args(subparsers)


class PreparePlatform(OnExistingProject):
    """Prepare a platform based on an xml definition"""

    command = "prepare-platform"

    def __init__(self, *args, **kwargs):
        super().__init__(PreparePlatform.command, *args, **kwargs)

    @classmethod
    def args(cls, xtrsub):
        """Add subcommand to a command line parser

        Parameters
        ----------
        xtrsub :
            Current list of subparsers
        """
        parser = xtrsub.add_parser(
            PreparePlatform.command,
            help="Prepare a platform deployment according to template",
        )
        parser.add_argument("--name", type=str, help="Name for the new platform")
        parser.add_argument(
            "--template",
            type=str,
            help="Platform name, or file with the platform template",
        )
        parser.add_argument(
            "--nodes",
            type=str,
            nargs="+",
            help="Selection of nodes, if not all nodes used",
        )
        parser.set_defaults(handler=PreparePlatform)

    def __call__(self, ns):
        """Use the parameters passed in the ns object to create a
        platform configuration.

        Parameters
        ----------
        ns : argparse struct
            Entered or default parameters

        Raises
        ------
        e
            _description_
        """
        if ns.template:
            _tmpl = ns.template
        else:
            _tmpl = f"platform-{ns.name}.xml"

        # find the file in one of the dirs
        prefix = get_dueca_prefix()

        template = ""
        for d in ("", f"{os.environ.get('HOME')}/.config/dueca/",
                  "/etc/dueca/", f"{prefix}/share/dueca/data/default/"):
            if os.path.exists(f"{d}{_tmpl}"):
                template = f"{d}{_tmpl}"
                break

        if not template:
            print(f"Could not find template {_tmpl} in .config/dueca, /etc/dueca or {prefix}/share/dueca/data/default", file=sys.stderr)
            return

        nmc = NewMachineClass()
        npc = NewPlatform()
        nnc = NewNode()

        with open(template, "r", encoding='utf-8') as f:
            tree = etree.XML(f.read())
            if not XML_tag(tree, 'configuration'):
                print(f"File {template}, expected main tag <configuration>", file=sys.stderr)
                return

            # find and add all machine classes
            for elt in tree:
                if XML_comment(elt):
                    pass

                elif XML_tag(elt, "machineclasses"):

                    for mclass in elt:

                        if XML_comment(mclass):
                            continue

                        elif not XML_tag(mclass, "machineclass"):
                            print(f"Unexpected xml tag <{mclass.tag}> under <machineclasses>", file=sys.stderr)
                            continue

                        # print(mclass)
                        mname = mclass.get("name")
                        gui = mclass.get("gui", "none")
                        config = ""
                        modules = []
                        for c in mclass:
                            if XML_comment(c):
                                continue
                            elif XML_tag(c, "config"):
                                config = trim_lines(c.text)
                                continue
                            elif not XML_tag(c, "modules"):
                                print(f"Unexpected xml tag <{c.tag}> under <machineclass>", file=sys.stderr)
                                continue

                            for p in c:
                                if XML_comment(p):
                                    continue
                                # these should now be project, just like in modules.xml
                                elif not XML_tag(p, "project"):
                                    print(f"Unexpected xml tag <{p.tag}> under <modules>", file=sys.stderr)
                                    continue

                                # default action, this is a project
                                url, version = None, None

                                # contains url, possibly version, and multiple module
                                for uvm in p:

                                    if XML_comment(uvm):
                                        pass
                                    elif XML_tag(uvm, "url"):
                                        url = uvm.text
                                    elif XML_tag(uvm, "version"):
                                        version = uvm.text
                                    elif XML_tag(uvm, "module"):
                                        # process the new module
                                        modname = uvm.text
                                        pseudo = XML_interpret_bool(uvm.get("pseudo", False))
                                        inactive = XML_interpret_bool(uvm.get("inactive", False))
                                        if url:
                                            modules.append((url, modname, version, pseudo, inactive))
                                        else:
                                            print(f"Need to define url before module {modname}", file=sys.stderr)
                                    else:
                                        print(f"Unexpected xml tag <{uvm.tag}> under <project>", file=sys.stderr)
                                        continue

                        # add the machine class if applicable
                        try:
                            n = Namespace(
                                name=mname,
                                gui=gui,
                                switch=False,
                                config=config,
                                modules=modules,
                            )
                            # print("nmc with", n)
                            nmc(n)
                        except Exception as e:
                            print(e, file=sys.stderr)

                elif XML_tag(elt, "platform"):
                    pname = ns.name or elt.get("name")
                    pcomm_master = elt.get("comm-master")

                    # get list of nodes
                    nodes = []
                    scriptlets = {}
                    for e in elt:
                        if XML_tag(e, "scriptlet"):
                            place = e.get("place")
                            scriptlets[place] = trim_lines(e.text)

                        elif XML_tag(e, "node"):
                            nodes.append(
                                Namespace(
                                    highest_priority=e.get("highest-prio", 4),
                                    name=e.get("name"),
                                    script=self.check_scriptlang(),
                                    machine_class=e.get("machineclass"),
                                    node_number=e.get("node-number", None),
                                    if_address=e.get("if-address", "0.0.0.0"),
                                    ismaster=e.get("comm-master", False),
                                )
                            )

                    # assert node numbers
                    nums = set(range(len(nodes)))

                    # convert and check any manually specified numbers
                    for n in nodes:
                        try:
                            nno = int(n.node_number)
                            nums.remove(nno)
                            n.node_number = nno
                        except KeyError as e:
                            if nno >= len(nodes):
                                print(
                                    "Node number too high" f" {nno} >= {len(nodes)}",
                                    file=sys.stderr,
                                )
                            else:
                                print(
                                    f"Number {nno} not available,"
                                    " specified multiple times?",
                                    file=sys.stderr,
                                )
                            raise e
                        except Exception as e:
                            pass

                    #  assign remaining numbers from the set
                    for n in nodes:
                        if n.node_number is None:
                            n.node_number = nums.pop()

                        # complete the data
                        n.cmaster = (not n.ismaster and pcomm_master) or None
                        n.gui = get_machineclass_gui(n.machine_class)
                        n.platform = pname
                        n.num_nodes = len(nodes)

                    # make the platform
                    npc(
                        Namespace(
                            name=pname,
                            masternode=pcomm_master,
                            zeronode=[n.name for n in nodes if n.node_number == 0][0],
                            othernodes=[
                                n.name
                                for n in nodes
                                if n.node_number != 0 and not n.ismaster
                            ],
                            lnodes=[n.name for n in nodes],
                        )
                    )
                    ns.name = None

                    for n in nodes:
                        # create the node
                        nnc(n, scriptlets)
                else:
                    print(f"Unexpected xml tag <{elt.tag}> under <configuration>", file=sys.stderr)
                    return

        print("Created platform, machine classes and nodes, based on" f" {template}")


PreparePlatform.args(subparsers)


class RunPolicies(OnExistingProject):
    """Apply available policies to the project"""

    command = "policies"

    def __init__(self, *args, **kwargs):
        super().__init__(RunPolicies.command, *args, **kwargs)

    @classmethod
    def args(cls, xtrsub):
        """Add subcommand to a command line parser

        Parameters
        ----------
        xtrsub :
            Current list of subparsers
        """
        parser = xtrsub.add_parser(
            RunPolicies.command, help="Check and optionally implement coding policies"
        )
        parser.add_argument(
            "--policiesurl", type=str, nargs="+", help="Location of applicable policies"
        )
        parser.add_argument(
            "--explain",
            action="store_true",
            default=False,
            help="Explain condition testing",
        )
        parser.add_argument(
            "--apply", type=str, nargs="+", help="Labels for all the policies to apply"
        )
        parser.add_argument(
            "--apply-all",
            action="store_true",
            default=False,
            help="Automatically apply all found policies",
        )
        parser.add_argument(
            "--skip", type=str, nargs="+", help="Skip the listed policies"
        )
        parser.add_argument(
            "--include-default",
            action="store_true",
            default=False,
            help="Also test default policy locations when given a url",
        )
        parser.add_argument(
            "--force",
            action="store_true",
            default=False,
            help="Force application, even is the policy is considered "
            "to have already been applied",
        )
        parser.add_argument(
            "--dry-run",
            type=str, nargs="+", help="Labels for all the policies to try"
        )
        parser.set_defaults(handler=RunPolicies)

    def __call__(self, ns):

        self.push_dir()
        # dprint(f"considering folder {self.projectdir}")
        policies = Policies(
            self.projectdir, ns.include_default, ns.policiesurl, explain=ns.explain
        )

        # dprint(ns, ns.apply_all)
        if ns.apply:
            report = policies.apply(policylist=ns.apply, force=ns.force)
            # print(report)
            if report:
                print("Applied given policies:\n", "\n".join(report))
            else:
                print("The given policy cannot be applied")
        elif ns.dry_run:
            report = policies.apply(policylist=ns.dry_run, force=ns.force, dryrun=True)
            # print(report)
            if report:
                print("Dryrun given policies:\n", "\n".join(report))
            else:
                print("The given policy cannot be applied")
        elif ns.apply_all:
            report = policies.apply(policylist=None)
            if report:
                print("Applied the following policies:\n", "\n".join(report))
            else:
                print("There are no policies that can be applied")
        elif ns.skip:
            report = policies.skip(policylist=ns.skip)
            if report:
                print("Ignoring given policies:\n", "\n".join(report))
            else:
                print("Not applicable, cannot ignore given policies")
        else:
            report = policies.inventory()
            if report:
                print("Applicable policies:\n ", "\n  ".join(report))
            else:
                print("No applicable policies.")

        self.pop_dir()


RunPolicies.args(subparsers)


class SearchProject:
    """Search for a project in configured git locations"""

    command = "search"

    @classmethod
    def args(cls, xtrsub):
        """Add subcommand to a command line parser

        Parameters
        ----------
        xtrsub :
            Current list of subparsers
        """
        parser = xtrsub.add_parser(
            SearchProject.command,
            help="Search for a project in currently configured git roots",
        )
        parser.add_argument(
            "--name", type=str, help="Name of the project to search for"
        )
        parser.set_defaults(handler=SearchProject)

    def __call__(self, ns):
        tryurl = f"dgr:///{ns.name}.git"

        def noprint(*args, **kwargs):
            pass

        newurl, result = checkGitUrl(url=tryurl, print=noprint)
        if result:
            print(f"Found {ns.name} at the following URL")
            print(f"  {newurl}")
        else:
            print(f"Could not find {ns.name} at any of the following URL's")
            for u in RootMap().values():
                print(f"  {u}{ns.name}.git")


SearchProject.args(subparsers)


class BuildProject(OnExistingProject):
    """Configure and/or build the current project"""

    command = "build"

    vsdirs = (".vscode",)
    vsfiles = (
        ("project.vscode.launch.json", ".vscode/launch.json"),
        ("project.vscode.tasks.json", ".vscode/tasks.json"),
        ("project.vscode.settings.json", ".vscode/settings.json"),
        ("project.clang-format", ".clang-format"),
    )

    def __init__(self, *args, **kwargs):
        super().__init__(BuildProject.command, *args, **kwargs)

    @classmethod
    def args(cls, xtrsub):
        """Add subcommand to a command line parser

        Parameters
        ----------
        xtrsub :
            Current list of subparsers
        """
        parser = xtrsub.add_parser(
            BuildProject.command,
            help="Configures a project (if not configured yet) and builds" " the code",
        )
        parser.add_argument(
            "--clean",
            dest="clean",
            action="store_true",
            default=False,
            help="Clean all code from the build folder, don't configure",
        )
        parser.add_argument(
            "--rebuild",
            action="store_true",
            default=False,
            help="Clean, then reconfigure and rebuild",
        )
        parser.add_argument(
            "-D",
            "--option",
            type=str,
            nargs="*",
            default=[],
            help="Provide additional options for the configure stage",
        )
        parser.add_argument(
            "--debug",
            dest="debug",
            action="store_true",
            default=False,
            help="Configure with debug mode",
        )
        parser.add_argument(
            "--vscode",
            action="store_true",
            default=False,
            help="Prepare or augment a vscode folder with build and debug instructions",
        )
        parser.add_argument(
            "--verbose",
            dest="buildverbose",
            action="store_true",
            default=False,
            help="Do a verbose build",
        )
        parser.set_defaults(handler=BuildProject)

    def __call__(self, ns):

        self.push_dir(f"{self.projectdir}/build")
        dprint(f"Build, arguments {ns}")
        if ns.clean or ns.rebuild:
            try:
                files = [str(f) for f in os.listdir(".") if f != ".gitignore"]
                # dprint([ 'rm', '-rf'] + files)
                cm = subprocess.run(
                    ["rm", "-rf"] + files, stdout=subprocess.PIPE, check=True
                )
                for line in cm.stdout:
                    print(line.decode())
                dprint(f"Clean result {cm}")
                if os.path.islink("../compile_commands.json"):
                    os.remove("../compile_commands.json")
            except Exception as e:
                print(f"Could not clean out the build folder, {e}", file=sys.stderr)

        if not (ns.vscode or ns.clean):
            try:
                if not os.path.isfile("./Makefile"):
                    options = [(o[0] == "-" and o) or f"-D{o}" for o in ns.option]
                    if ns.debug:
                        options.append("-DCMAKE_BUILD_TYPE=Debug")
                    options.append("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
                    print(
                        "Configuring the build dir with options\n  ", " ".join(options)
                    )
                    cm = subprocess.run(["cmake", ".."] + options, check=True)
                    dprint(f"CMake result {cm}")

                    # symlink the compile_commands.json file if present
                    if os.path.isfile("compile_commands.json") and not os.path.exists(
                        "../compile_commands.json"
                    ):
                        self.push_dir(f"{self.projectdir}")
                        os.symlink(
                            "build/compile_commands.json", "compile_commands.json"
                        )
                        self.pop_dir()

                print("Running the build")
                import multiprocessing

                command = ["make", f"-j{multiprocessing.cpu_count()}"]
                if ns.buildverbose:
                    command.append("VERBOSE=1")
                cm = subprocess.run(command, check=True)
                dprint(f"Build result {cm}")
            except Exception as e:
                print(f"Failed to run configure or build, {e}", file=sys.stderr)
        self.pop_dir()

        if ns.vscode:
            self.push_dir(self.projectdir)
            create_and_copy(BuildProject.vsdirs, BuildProject.vsfiles, {}, True, True)
            self.pop_dir()


BuildProject.args(subparsers)

# parse arguments
# testargs = [
#    'policies',
#    '--policiesurl=file:///home/repa/dueca/test/gitscript/example-policies.xml']

if __name__ == "__main__":
    argcomplete.autocomplete(mainparser)

    pres = mainparser.parse_args(sys.argv[1:])

    if pres.verbose:
        verboseprint._verbose_print = True

    # if successful, a handler has been provided
    try:
        hclass = pres.handler
    except AttributeError:
        mainparser.print_usage()
        sys.exit(-1)

    # run the handler
    handler = hclass()
    handler(pres)
