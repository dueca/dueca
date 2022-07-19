#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Feb 22 15:39:49 2021

@author: repa
"""

import git
import os
from lxml import etree
from .xmlutil import XML_comment, XML_tag, XML_interpret_bool, XML_TagUnknown
import re
import sys
from .verboseprint import dprint
from .commobjects import CommObjectsList
import tempfile

# regex for decoding project URL
_decodeprj = re.compile(r"^(.+)/(.+)\.git$")
def projectSplit(url: str):
    """
    Split the project URL into a repository/project part

    Parameters
    ----------
    url : str
        Project URL.

    Raises
    ------
    e
        Exception given when URL does not follow the required pattern.

    Returns
    -------
    tuple of str
        Repository URL and project name.

    """
    try:
        match = _decodeprj.fullmatch(url)
        # dprint("splitting url", url, " into ", match.group(1), '/', match.group(2))
        return match.group(1), match.group(2)
    except Exception as e:
        print(f"Cannot split project url from {url}")
        raise e

# singleton for the mapping of shortened URL's
class RootMap(dict):

    def __new__(cls):
        """
        Create new map linking abbreviation URL prefixes to full URL

        Given environment keys like 'DAPPS_GITROOT_example', a mapping is
        realised from abbreviated url drgexample:/// to the contents of the
        environment variable.

        Parameters
        ----------
        cls : TYPE
            Class variable.

        Returns
        -------
        Singleton instance of RootMap.

        """
        if not hasattr(cls, 'instance'):
            cls.instance = super(RootMap, cls).__new__(cls)

            for k, v in os.environ.items():
                if (k.startswith('DAPPS_GITROOT_') and \
                    len(k) > len('DAPPS_GITROOT_')) or \
                    k == 'DAPPS_GITROOT':
                    if not v.endswith('/'):
                        print(f"{k} should end with a '/'", file=sys.stderr)
                        v = v + '/'
                    key = (k == 'DAPPS_GITROOT' and 'dgr') or \
                        'dgr'+k[len('DAPPS_GITROOT_'):]
                    cls.instance[key] = v
                    dprint(f"adding shortcut {key} for {v}")

        return cls.instance

    def addProjectRemote(self, url: str):
        """
        Set the specific url remote:/// to the actual remote upstream

        Parameters
        ----------
        url : str
            Remote url.

        Returns
        -------
        None.

        """
        urlbase = '/'.join(url.split('/')[:-1]) + '/'
        dprint(f"adding shortcut remote for {urlbase}")
        if 'remote' in self and self['remote'] != urlbase:
            print("Overwriting remote origin with", urlbase)
        self['remote'] = urlbase

    def urlToAbsolute(self, url):
        """
        Convert a shorthand url to longhand form.

        Parameters
        ----------
        url : str
            Shorthand url, starting with 'dgr:///' or 'dgr.*:///'.

        Returns
        -------
        str
            Complete url.

        """
        for u, root in self.items():
            if url.startswith(f'{u}:///'):
                dprint(f"Translating {url} to {root}{url[len(u)+4:]}")
                return root + url[len(u)+4:]
        return url

    def urlToRelative(self, url):
        """
        Convert a URL using gitroot to relative.

        Parameters
        ----------
        url : str
            Shorthand url, starting with 'dgr:///' or 'dgr.*:///' or
            'remote:///' .

        Returns
        -------
        str
            Complete url.

        """
        for u, root in self.items():
            if url.startswith(root) and \
                (u != 'origin' or
                 url[len(root):-4] == ProjectRepo().project):
                dprint(f"Translating {url} to {u}:///{url[len(root):]}")
                return f'{u}:///' + url[len(root):]
        return url



# singleton for the project folder's git repository
class ProjectRepo(git.Repo):

    def __new__(cls, initdir=None):
        if not hasattr(cls, 'instance'):
            if initdir is None:
                raise ValueError("Problem with ProjectRepo init")
            pname, pdir, dum = findProjectDir(initdir)
            cls.instance = super(ProjectRepo, cls).__new__(cls)
            cls.instance.projectdir = pdir
            cls.instance.project = pname
            cls.instance.repo = git.Repo.init(pdir)
            RootMap().addProjectRemote(cls.instance.repo.remotes.origin.url)

        return cls.instance




def checkGitUrl(repo: git.Repo = None, url: str='', print=print):
    """
    Determine existence of a url, if this url is not found, try to
    find it in one of the pre-configured roots.

    Parameters
    ----------
    url : str
        Git URL (absolute, not a shorthand).
    clean : bool
        Modified if url is changed.

    Returns
    -------
    updated url, flag indicating change.
    """
    # temporary repo for commands
    if repo is None:
        tmpdir = tempfile.TemporaryDirectory()
        repo = git.Repo.init(tmpdir)

    # first check whether the unchanged url is valid
    try:
        repo.git.ls_remote(url)
        dprint(f"Verified url {url} as valid")
        return url, False
    except git.GitCommandError as e:
        dprint(f"Cannot find remote url {url}, git error {e}")
        pass

    # now try all roots
    urbase, project = projectSplit(url)
    for u, root in RootMap().items():
        try:
            repo.git.ls_remote(f'{root}{project}.git')
            print(f'Automatic re-map of url {url} to '
                  f'{u}:///{project}.git (at {root})')
            return f'{root}{project}.git', True
        except git.GitCommandError as e:
            dprint(f"No luck finding '{root}/{project}.git, git error {e}'")
    print(f"Cannot find url {url}, incomplete refresh, check modules.xml")
    return '', False

def findProjectDir(initdir):

    if initdir is not None:
        curpath = os.path.abspath(initdir).split(os.sep)
    else:
        curpath = os.getcwd().split(os.sep)


    inprojectdir = True
    while len(curpath):
        projectdir = '/'.join(curpath)
        if os.path.exists(f'{projectdir}/.config/machine') and \
            os.path.exists(f'{projectdir}/CMakeLists.txt') and \
            os.path.isdir(f'{projectdir}/run'):
                break
        del curpath[-1]
        inprojectdir = False

    if len(curpath) < 2 or curpath[-1] != curpath[-2]:
        print(f"Could not find project folder from {initdir}")
        raise Exception("Run this from within a project")

    return curpath[-1], projectdir, inprojectdir

class Module:

    def __init__(self, name=None, xmlnode=None, xmlroot=None,
                 pseudo=False, inactive=False):

        if xmlnode is not None:
            # read from the given xml node
            self.xmlnode = xmlnode
        elif xmlroot is not None and name is not None:
            # new module create/define xml node
            self.xmlnode = etree.SubElement(xmlroot, 'module')
            self.xmlnode.text = name
            if pseudo:
                self.xmlnode.set("pseudo", "true")
            if inactive:
                self.xmlnode.set("inactive", "true")
        else:
            raise ValueError(
                "Create Module representation from xml, or name and root")

    def delete(self):
        if self.xmlnode is not None:
            del self.xmlnode
            self.xmlnode = None

    def __str__(self):
        return self.xmlnode.text.strip()

    def needbuild(self):
        return not XML_interpret_bool(self.xmlnode.get("pseudo", False)) \
            and not XML_interpret_bool(self.xmlnode.get("inactive", False))


class Project:
    def __init__(self, url=None, version=None,
                 xmlnode=None, xmlroot=None):

        self.repo_cycle = 0
        self.xmlnode = xmlnode

        if xmlnode is not None:
            pass

        elif xmlroot is not None and url is not None:

            # newly constructed
            self.url = RootMap().urlToAbsolute(url)
            self.name = projectSplit(self.url)[1]
            self.version = version
            self.modules = []

        else:
            raise ValueError("Incorrect set of arguments for project")

        self._sync(xmlroot)
        # print(self.name, self.modules)

    def _sync(self, xmlroot=None):

        if self.xmlnode is None:

            # create a new node and fill it
            self.xmlnode = etree.SubElement(xmlroot, 'project')
            u = etree.SubElement(self.xmlnode, 'url')
            u.text = RootMap().urlToRelative(self.url)
            if self.version:
                v = etree.SubElement(self.xmlnode, 'version')
                v.text = self.version
            for m in self.modules:
                n = etree.SubElement(self.xmlnode, 'module')
                n.text = m
            self.clean = True
            return

        try:
            # recover from the node
            self.version = None
            self.url = None
            self.modules = []
            self.name = None
            for elt in self.xmlnode:

                if XML_comment(elt):
                    continue

                if XML_tag(elt, 'url'):

                    self.url = RootMap().urlToAbsolute(elt.text.strip())

                elif XML_tag(elt, 'version'):
                    self.version = elt.text.strip()

                elif XML_tag(elt, 'module'):
                    self.modules.append(Module(xmlnode=elt))

                else:
                    raise XML_TagUnknown(elt)

            if self.url is None:
                raise ValueError("Need <url> tag in <project>")
            else:
                self.name = projectSplit(self.url)[1]

        except Exception as e:
            print("Failure in decoding a <project> block")
            raise e

    def deleteModule(self, module):
        try:
            idx = map(str, self.modules).index(module)
            del self.modules[idx]
            dprint(f"Deleted module {module} from project {self.name}")
        except ValueError:
            print(f"Delete: Cannot find {module} in {self.name}")

    def createModule(self, module: str, url: str, pseudo, inactive):

        if url and self.url != url:
            raise Exception(
                f'URL conflict trying to extend modules from {self.name}'
                f' old url: {self.url} new:{url}')
        m = Module(name=module, xmlroot=self.xmlnode,
                   pseudo=pseudo, inactive=inactive)
        self.modules.append(m)
        dprint(f"Added module {module} to project {self.name}")

    def hasModule(self, module: str):
        return module in map(str, self.modules)


class Modules:

    def __init__(self, calldir='.', mclass=None):

        # get project folder
        self.clean = None
        self.repo_cycle = 1
        self.repo = ProjectRepo(calldir).repo
        self.ownproject = ProjectRepo(calldir).project
        self.projectdir = ProjectRepo(calldir).projectdir
        # print("Modules for", self.ownproject, "in", self.projectdir)

        # find the machine class
        if mclass is None:
            with open(f'{self.projectdir}/.config/machine', 'r') as f:
                self.mclass = f.readline().strip()
        else:
            self.mclass = mclass

        # file name defining the modules list
        self.fname = f'{self.projectdir}/.config/class/{self.mclass}/modules.xml'

        # for clean is None, this reads the modules list
        self._sync()

    def _addToSparse(self, prj: Project, lines: list) -> None:
        """
        Add file names or patterns to a sparse-checkout file for a project
        that is serving as a donor. If the folder is not present,
        it is created and the initial clone/pull is done

        Parameters
        ----------
        prj : Project
            Donating project.
        lines : list
            Lines to be added to the file.

        Returns
        -------
        None

        """

        # when the folder is not present, create it, and clone the upstream
        if not os.path.isdir(f'../{prj.name}'):

            # folder not present? make and init
            dprint("adding project", prj.name)

            # create and initialize folder / git
            os.mkdir(f'../{prj.name}')
            rrepo = git.Repo.init(f'../{prj.name}')
            if self.auto_url:
                prj.url, changes = checkGitUrl(rrepo, prj.url)
                if changes:
                    for e in prj.xmlnode:
                        if XML_tag(e, 'url'):
                            e.text = RootMap().urlToRelative(prj.url)
                self.clean = False

            rrepo.create_remote('origin', prj.url)
            rrepo.git.config('core.sparseCheckout', 'true')
        else:

            # folder already there, get the repo object
            rrepo = git.Repo(f'../{prj.name}')

        # early exit for own project, unless it is checked out sparse
        cread = rrepo.config_reader()
        if prj.name == self.ownproject and  \
            not cread.get_value('core', 'sparseCheckout', False):
            return

        # copy the lines to a set, so that any double addition can be
        # avoided
        to_add = set(lines)

        # open the sparse_checkout file, check what is already there
        try:
            with open(f'../{prj.name}/.git/info/sparse-checkout', 'r') as ms:
                for l in ms:
                    if l.strip() in to_add:
                        to_add.remove(l.strip())

            # add any remaining lines
            if len(to_add):
                with open(f'../{prj.name}/.git/info/sparse-checkout',
                          'a') as ms:
                    for l in to_add:
                        ms.write(l + '\n')

        # simply create when it was not yet there
        except FileNotFoundError:
            with open(f'../{prj.name}/.git/info/sparse-checkout', 'w') as ms:
                for l in to_add:
                    ms.write(l + '\n')

        # what version/branch are we currently on
        branch = str(rrepo.active_branch)

        # version, branch or tag
        if prj.name == self.ownproject:
            # for the "own" project, select the git-selected version
            version = branch
        else:
            # for all others, listen to the version in the modules.xml file
            version = (prj.version != 'HEAD' and prj.version) or 'master'
            if version != branch:
                print(f"Borrowed code from {prj.name} was on {branch}"
                      f" changing to {version} based on modules.xml")

        # get tags and the items in sparse checkout
        rrepo.remote().fetch()

        # create a branch if needed
        if version not in rrepo.heads:
            dprint(f"In {prj.name}, creating branch {version}, adding to {rrepo.heads}")
            branch = rrepo.create_head(version, rrepo.remote().refs[version])
            branch.set_tracking_branch(rrepo.remote().refs[version])


        # and check it out
        rrepo.git.checkout(version)

        if self.repo_cycle == prj.repo_cycle:

            # already fetched from the repo, just re-checkout, to get
            # the sparsity adjustments
            # rrepo.git.checkout(prj.version)
            return

        # now get the stuff
        try:

            # since we don't expect edits in borrowed, ff merge
            rrepo.git.merge()
            # rrepo.remote().pull(version)  # had this one commented, using the
            # rrepo.remote().fetch()      # fetch, but why?

            # figure out all local branches
            #for br in rrepo.heads:
            #    if br.name == version:
            #        # make sure we have this one
            #        br.checkout()
            #        # since we don't expect edits in borrowed, ff merge
            #        rrepo.git.merge()
            #        return

            # there was no such branch locally, create with track
            #rrepo.git.checkout(f'origin/{version}', track=True)
            #prj.repo_cycle = self.repo_cycle

        except git.exc.GitCommandError as e:
            print(f"Cannot pull/checkout comm-objects in {prj.name}, {e}")

    def _analyseCommObjectFile(self, p, m, call_for_new_project=None):
        if os.path.isfile(f'{self.projectdir}/../{p}/{m}/CMakeLists.txt'):
            dprint(f"Refresh dco, analysing {p}/{m}/comm-objects.lst")
            colist = CommObjectsList(f'{self.projectdir}/../{p}/{m}')
            for idco in colist:
                prj = idco.base_project
                dco = idco.dco
                if prj not in self.comm_borrows and \
                    call_for_new_project is not None:
                    dprint(f"Refresh dco, chain to {prj} for {dco}")
                    self.comm_borrows[prj] = set((dco,))
                    call_for_new_project(prj)
                else:
                    self.comm_borrows[prj].add(dco)
        else:
            dprint(f"Refresh dco, no {p}/{m}/CMakeLists.txt, assume pseudo")

    def _resetCommBorrows(self, recurse=True, auto_dco=False):

        if auto_dco:
            if recurse:
                fcn = self._chainCommObjectDepsOrAddProject
            else:
                fcn = self._noChainCommObjectDepsOrAddProject
        else:
            if recurse:
                fcn = self._chainCommObjectDeps
            else:
                fcn = self._noChainCommObjectDeps

        self.comm_borrows = dict()
        # copy into a list, bc the number of projects may changes
        for pname, p in list(self.projects.items()):
            for m in p.modules:
                self._analyseCommObjectFile(pname, str(m), fcn)

    def _chainCommObjectDeps(self, p: str):

        # get a ref to the project
        prj = self.projects.get(p, None)
        if prj is None:
            raise Exception(
                f"No URL known for DCO objects borrowed from {p},"
                f" add an entry to {self.projectdir}/"
                f".config/class/{self.mclass}/modules.xml")

        # is it there already
        self._addToSparse(prj, ['comm-objects/*'])

        # now recursively run the comm-objects.lst in comm-objects
        self._analyseCommObjectFile(p, 'comm-objects',
                                    self._chainCommObjectDeps)

    def _noChainCommObjectDeps(self, p:str):

        prj = self.projects.get(p, None)
        if prj is None:
            print(f"Not recursing, but likely need DCO from {p}"
                  f" adding an entry to {self.projectdir}/"
                  f".config/class/{self.mclass}/modules.xml")

    def _chainCommObjectDepsOrAddProject(self, p: str):

        # get a ref to the project
        prj = self.projects.get(p, None)
        if prj is None:
            try:
                self.addModule(p, None, url=f'dgr:///{p}.git')
                prj = self.projects.get(p, None)
            except Exception as e:
                raise Exception(
                    f"Cannot automatically add DCO object project borrow {p}"
                    f" to modules.xml, error {e}")

        # is it there already
        self._addToSparse(prj, ['comm-objects/*'])

        # now recursively run the comm-objects.lst in comm-objects
        self._analyseCommObjectFile(p, 'comm-objects',
                                    self._chainCommObjectDeps)

    def _noChainCommObjectDepsOrAddProject(self, p:str):

        prj = self.projects.get(p, None)
        if prj is None:
            try:
                self.addModule(prj, None, url=f'dgr:///{prj}.git')
            except Exception as e:
                raise Exception(
                    f"Cannot automatically add DCO object project borrow {prj}"
                    f" to modules.xml, error {e}")

    def _sync(self):

        if self.clean:
            return

        if self.clean is None:

            dprint(f"Reading {self.fname}")
            # empty slate
            self.projects = dict()

            # read from file
            try:
                parser = etree.XMLParser(remove_blank_text=True)
                with open(self.fname, 'rb') as mds:

                    self.xmltree = etree.XML(mds.read(), parser=parser)
                    for node in self.xmltree:
                        if XML_comment(node):
                            pass
                        elif XML_tag(node, 'project'):
                            #dprint(f"project {project}")
                            prj = Project(xmlnode=node)
                            #dprint(f"project {prj.name}, modules {prj.modules}")
                            if prj.name in self.projects:
                                raise ValueError(
                                    f"Multiple entries for project {prj.name}")
                            self.projects[prj.name] = prj
                        else:
                            raise XML_TagUnknown(node)
            except Exception as e:
                print("Failed to parse module information"
                      f" from {self.fname}: {e}")
                raise e

            self.clean = True
            return

        # try a rename of the current file
        try:
            os.rename(self.fname, self.fname + '~')
        except FileNotFoundError:
            pass

        # write back to file
        dprint(f"Re-writing {self.fname}")
        etree.ElementTree(self.xmltree).write(
                self.fname, pretty_print=True, encoding='utf-8',
                xml_declaration=True)
        self.clean = True

    def isNewModule(self, project, module):
        return project not in self.projects or \
            module not in self.projects[project].modules

    def isNewProject(self, project):
        return project not in self.projects

    # not good enough yet
    def addModule(self, project, module, version=None, url=None,
                  pseudo=False, inactive=False):
        if project in self.projects and \
            (module is None or self.projects[project].hasModule(module)):
            raise Exception(
                f"Module {project}/{module} already in"
                f" .config/class/{self.mclass}/modules.xml")

        if project not in self.projects:
            if not url:
                raise Exception(f"Supply project URL for new module {module}")
            dprint(f"Adding a project {project} at {url}")
            self.projects[project] = Project(url=url, version=version,
                                             xmlroot=self.xmltree)
            if module is not None:
                self.projects[project].createModule(module, url,
                                                    pseudo, inactive)
        else:
            self.projects[project].createModule(module, url,
                                                pseudo, inactive)

        self.clean = False
        self._sync()

    def deleteModule(self, project, module):
        if project in self.projects and \
            self.projects[project].hasModule(module):
            self.projects[project].deleteModule(module)
        self.clean = False
        self._sync()

    def getOwnModules(self, project=None, onlycode=False):
        #dprint(f"getOwnModules from {self.projects}")
        if project is None:
            project = self.ownproject

        if project in self.projects.keys():
            #dprint(f"getOwnModules for {project}")
            return self.projects[project].modules
        #dprint(f"getOwnModules project {project} not found")
        return []

    def hasModule(self, project, module):
        return project in self.projects and \
            self.projects[project].hasModule(module)

    def refreshBorrowed(self, auto_dco=False, auto_url=False):

        #print(self.projects)
        self.repo_cycle = self.repo_cycle + 1
        self.auto_url = auto_url

        for (name, p) in self.projects.items():

            if name == self.ownproject:
                dprint(f"Refresh own modules {self.ownproject}, not borrowing")
                rrepo = ProjectRepo().repo
                # version = (p.version != 'HEAD' and p.version) or 'master'
                rrepo.remote('origin').pull()
                #rrepo.git.checkout(version)
                continue

            if not p.modules:
                dprint(f"Refresh, no modules in {name}, skipping for borrow")
                continue

            # addToSparse creates the borrowed repo if needed, does a
            # fetch for the project's claimed version, and a checkout
            self._addToSparse(p, [f'{m}/*' for m in p.modules])

            dprint(f"Pulled {len(p.modules)} module from {name}")

        # next assemble all borrowed comm-objects
        # this will call addToSparse to add the comm-objects folder(s)
        self._resetCommBorrows(auto_dco=auto_dco)

        # re-write modules file if changes applied
        if not self.clean:
            self._sync()
        self.auto_url = False

    def compactPrint(self, only_active=False):
        lst = [ f'{prj.name}/{m}' for
               prj in self.projects.values()
                   for m in prj.modules
                   if ((not only_active) or m.needbuild()) ]
        return ';'.join(lst)

    def debugPrint(self, only_active=False):
        lst = [ f'{prj.name}/{m}' for
               prj in self.projects.values()
                   for m in prj.modules
                   if ((not only_active) or m.needbuild()) ]
        return '\n'.join(lst)

    def relName(self, fname: str):
        """
        Return a relative name for a file

        Parameters
        ----------
        fname : str
            File name.

        Returns
        -------
        str.

        """
        if fname[0] != '/':
            return fname
        nelts = len(self.projectdir.split(os.sep)) - 1
        return os.sep.join(fname.split(os.sep)[nelts:])
