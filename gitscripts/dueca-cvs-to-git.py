#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Feb 26 19:19:28 2021

@author: repa
"""

import tempfile
import subprocess
import os
import git
import re
import sys
import shutil
import argparse

try:
    from duecautils.modules import Modules
    from duecautils.machinemapping import NodeMachineMapping
    from duecautils import verboseprint
except ModuleNotFoundError:
    sys.path.append('/tmp/lib/python3.7/site-packages')
    from duecautils.modules import Modules
    from duecautils.machinemapping import NodeMachineMapping
    from duecautils import verboseprint

def transferFile(index, p0, p1, f):
    mftxt = '''# -*-make-*-
# This Makefile was used in the previous build structure of this DUECA
# project. It is kept here for a short while in the new CMake-based build
# structure as reference, to look up build options and used libraries, etc.
#
# When conversion is complete and tested, this file may be removed from
# the repository

'''
    if f.startswith('Makefile'):
        with open(f'{p0}/{f}', 'rb') as f0:
            with open(f'{p1}/{f}._ref', 'wb') as f1:
                f1.write(mftxt.encode('ascii'))
                for l in f0:
                    f1.write(l)
        index.add(f'{p1}/{f}._ref')
    else:
        shutil.copy(f'{p0}/{f}', f'{p1}/{f}')
        index.add(f'{p1}/{f}')

def copyModuleFiles(index, p0, p1):
    for f in os.listdir(p0):
        if os.path.isdir(f'{p0}/{f}'):
            os.mkdir(f'{p1}/{f}')
            copyModuleFiles(index, f'{p0}/{f}', f'{p1}/{f}')
        else:
            transferFile(index, p0, p1, f)

def gitAdd(index, path):
    if os.path.isdir(path):
        for f in os.listdir(path):
            gitAdd(index, f'{path}/{f}')
    else:
        index.add(f'{path}')

def findPatchFiles(project):
    global patchdir
    if os.path.isfile(f'{patchdir}/{project}.total.patch'):
        return [ f'{project}.total.patch' ]
    checkprj = re.compile(f'^{project}\\.([0-9]+)\\.patch')
    patches = [ p for p in os.listdir(patchdir)
        if checkprj.fullmatch(p) ]
    patches.sort()
    return patches

def findFreePatchFile(project):
    patches = findPatchFiles(project)
    checkprj = re.compile(f'^{project}\\.([0-9]+)\\.patch')
    num = 0
    if patches:
        match = checkprj.fullmatch(patches[-1])
        num = int(match.group(1))+1
    return f'{project}.{num:03d}.patch'

def findTotalPatchFile(project):
    return f'{project}.total.patch'

def parseGui(cnffile):
    gui = 'gtk3'

    if cnffile.endswith('.py'):
        gui_check = re.compile(
            '''graphic_interface[\t ]*=[\t ]*["'](.+)["'']''')
    else:
        gui_check = re.compile(
            '\\(define[ \t]+graphic-interface[ \t]+"(.+)"[ \t]*\)')

    with open(cnffile, 'r') as f:
        for l in f:
            res = gui_check.match(l)
            if res:
                gui = res.group(1)
                break
    if gui not in ('none', 'gtk3', 'gtk2', 'glut', 'glut-gui'):
        print(f"Warning: found old gui '{gui}', in {cnffile} defaulting to gtk2")
        gui = 'gtk2'
    return gui


def readModules(project, machineclass):

    res = []

    # decode the old modules list and add to the modules file
    with open(f'{project}/modules.{machineclass}', 'r') as f:
        for l in f:
            if not l.strip() or l.strip()[0] == '#':
                pass
            else:
                prj, mod = l.strip().split()[0].split('/')
                if (prj, mod) in res:
                    print(f"Duplicate module listing {prj}/{mod} in file"
                          f" '{project}/modules.{machineclass}'")
                else:
                    res.append((prj, mod))
    return res


cvsroot = os.environ.get('DAPPS_CVSROOT', None)
patchdir = os.environ.get('DUECA_CVSTOGITPATCHES',
                          '/home/repa/TUDelft/servers/cvstogit')
startdir = os.getcwd()

# already git-converted projects
gitgroups = [ ('ae-cs-dueca-base', 'git@gitlab.tudelft.nl'),
              ('ae-cs-dueca-active', 'git@gitlab.tudelft.nl'), 
              ('ae-cs-dueca-archive', 'git@gitlab.tudelft.nl'),
              ('ae-cs-dueca-ftis', 'git@gitlab.tudelft.nl'),
              ('ae-cs-dueca-yard', 'git@gitlab.tudelft.nl'),
              ('dueca', 'git@github.com') ]

def constructUrl(prj):
    global rundir, gitgroups
    for gg, grepo in gitgroups:
        if os.path.isdir(f'{rundir}/{gg}/{prj}'):
            print(f"Borrow from already converted {gg}/{prj}")
            return f'{grepo}:{gg}/{prj}.git'
        
    # assuming we are borrowing from a recent convert
    print(f"Borrow from now-converted project {prj}")
    return f'file://{rundir}/repo/{prj}.git'
            

parser = argparse.ArgumentParser(
        description=
            """
Convert dueca projects from CVS to git

Reads out a cvs-based project, creates a temporary git repository for it,
and transfers the project to a git+cmake structure.

Given a working folder, e.g., /tmp/convert, the cvs checkout will be in
/tmp/convert/old, the converted project will be in /tmp/convert/new, and
the git repository in /tmp/convert/repo

Useful environment variables:

    DAPPS_CVSROOT :         Location of the CVS projects
    DUECA_CVSTOGITPATCHES : Place to keep patch results while working on
                            conversion projects; patches describe cvs to
                            git conversion steps
    DAPPS_GITROOT :         Base folder/path for git repositories; any
                            repository matching this base will be converted
                            to a relative url.
    DAPPS_CONVERTBASE :     If supplied, folder for conversion work.

Typical working mode:

    - choose a folder for conversion, e.g., /tmp/convert
    - set DAPPS_GITROOT to file:///tmp/convert/repo/
    - set DUECA_CVSTOGITPATCHES to the place where you keep the patches
    - set DAPPS_CVSROOT to the CVS repository location
    - if continuing with an existing project, source the setenv file to
      set these parameters.
    - start converting "base" projects, that do not depend on other
      projects' dco files.
    - try compiling, editing, adjusting the projects, using dueca-gproject
    - when happy with the state of a project, from within the project folders
      run dueca-cvs-to-git --savediff, to save a diff step to the patches
      folders, or dueca-cvs-to-git --save-gitdiff, to save the total step
      from cvs conversion to final edited version.
    - when happy with the conversion of all projects, copy the git versions
      to permanent repositories and never look back.

The patch folder can keep temporary results; when converting a project from
cvs, the patches there are used to update the converted project. The "total"
patch is preferred, otherwise the partial patches will be used.
            """)
parser.add_argument(
    '--verbose', action='store_true',
    help="Verbose run with information output")
parser.add_argument(
    '--project', type=str,
    help="Name of the project to convert")
parser.add_argument(
    '--clean', action='store_true',
    help="Clean the cvs export, git repo and checked-out copy")
parser.add_argument(
    '--savediff', action='store_true',
    help='From an initial project folder, store the diff after edits/tweaks')
parser.add_argument(
    '--save-gitdiff', action='store_true',
    help='Save the total diff from cvs checkout to here')
parser.add_argument(
    '--base', type=str,
    help='base work area, if not supplied, DAPPS_CONVERTBASE is used, '
    'otherwise a random file will be chosen')
parser.add_argument(
    '--gitroot', type=str, default=os.environ.get('DAPPS_GITROOT', ''))
parser.add_argument(
    '--diff-location', type=str,
    default=patchdir,
    help='Folder with temporary project patches. These are replayed after\n'
    'a conversion, and new patches are placed there.')


# get the arguments
runargs = parser.parse_args(sys.argv[1:])

patchdir = runargs.diff_location
verbose = runargs.verbose
if not runargs.base and os.environ.get('DAPPS_CONVERTBASE', False):
    runargs.base = os.environ.get('DAPPS_CONVERTBASE')

if runargs.base and runargs.base[-1] == os.sep:
    runargs.base = runargs.base[:-1]
if verbose:
    verboseprint._verbose_print = True

if runargs.clean:

    if runargs.project:
        subprocess.run(('rm', '-rf', f'{runargs.base}/old/{runargs.project}'))
        subprocess.run(('rm', '-rf', f'{runargs.base}/repo/{runargs.project}.git'))
        subprocess.run(('rm', '-rf', f'{runargs.base}/new/{runargs.project}'))
        sys.exit(0)
    else:
        sys.exit(1)


if runargs.savediff:

    # should be from within a project
    mods = Modules(None)
    project = mods.ownproject
    tosave = findFreePatchFile(project)
    with open(f'{patchdir}/{tosave}', 'w') as pf:
        subprocess.run(('git', 'diff'), stdout=pf)
    print(f"Saved the edits to {tosave}")
    sys.exit(0)

if runargs.save_gitdiff:
    mods = Modules(None)
    project = mods.ownproject
    tosave = findTotalPatchFile(project)
    with open(f'{patchdir}/{tosave}', 'w') as pf:
        subprocess.run(('git', 'diff', 'from_cvs'), stdout=pf)
    print(f"Saved the edits to {tosave}")
    sys.exit(0)

if runargs.project:
    projects = [ runargs.project ]
else:
    projects = []

# create playground
if runargs.base:
    rundir = runargs.base
else:
    rundir = tempfile.mkdtemp()

for suffix in ('old', 'new', 'repo'):
    if not os.path.isdir(f'{rundir}/{suffix}'):
        os.mkdir(f'{rundir}/{suffix}')

# set an environment recreating file
if not os.path.isfile(f'{rundir}/setenv'):
    if not runargs.gitroot:
        runargs.gitroot = f'file://{rundir}/repo/'
    with open(f'{rundir}/setenv', 'w') as rf:
        rf.write(f"""# Set environment variables for this root
# source this file to continue development here

export DAPPS_CONVERTBASE={rundir}
export DAPPS_GITROOT={runargs.gitroot}
export DUECA_CVSTOGITPATCHES={patchdir}

# the default url's for DUECA projects
export DAPPS_GITROOT_base=git@gitlab.tudelft.nl:ae-cs-dueca-base/
export DAPPS_GITROOT_active=git@gitlab.tudelft.nl:ae-cs-dueca-active/
export DAPPS_GITROOT_archive=git@gitlab.tudelft.nl:ae-cs-dueca-archive/
export DAPPS_GITROOT_ftis=git@gitlab.tudelft.nl:ae-cs-dueca-ftis/
export DAPPS_GITROOT_yard=git@gitlab.tudelft.nl:ae-cs-dueca-yard/
""")
        if cvsroot is not None:
            rf.write(f"export DAPPS_CVSROOT={cvsroot}")

# machine class mapping
mc_mapping = dict(solo='solo',
                  dutmms3='hmi-ig',
                  dutmms3_0='hmi-ig',
                  dutmms3_1='hmi-ig',
                  dutmms9='hmi-ig',
                  dutmms14='hmi-ig',
                  dutmms14b='hmi-ig',
                  dutmms1='hmi-ecs',
                  dutmms4='hmi-io',
                  dutmms2='hmi-efis',
                  dutmms5='hmi-dash',
                  dutmms6='hmi-host',
                  srsig1='srs-ig',
                  srsig2='srs-ig',
                  srsig3='srs-ig',
                  srsctrlecat='srs-io',
                  srsctrl1='srs-io-old',
                  srsctrl2='srs-io-old',
                  srshost='srs-host',
                  srsecs='srs-ecs',
                  srssound='srs-sound',
                  srsefis1='srs-efis',
                  srsefis2='srs-efis',
                  srsefis3='srs-efis',
                  srsefis4='srs-efis',)


for project in projects:

    p_old = f'{rundir}/old/{project}'
    p_new = f'{rundir}/new/{project}/{project}'

    # an assumed master projects location
    #urlbase = 'file://{rundir}/repo/{prj}.git'


    # check out the CVS-based code
    os.chdir(f'{rundir}/old')
    subprocess.run(('cvs', '-d', cvsroot, 'export', '-r', 'HEAD', project))

    # initialize the git repository
    rrepo = git.Repo.init(f'{rundir}/repo/{project}.git', bare=True)

    # is there a dueca_cnf.py file somewhere?
    scriptlang = (os.path.isfile(
        f'{rundir}/old/{project}/run/solo/solo/dueca_cnf.py') and
        'python') or 'scheme'
    if scriptlang == 'scheme':
        gui_check = re.compile(
            '\\(define[ \t]+graphic-interface[ \t]+"(.+)"[ \t]*\)')
        cnffile = 'dueca.cnf'
    else:
        gui_check = re.compile(
            '''graphic_interface[\t ]*=[\t ]*["'](.+)["'']''')

        cnffile = 'dueca_cnf.py'

    # gui for the solo class
    gui = parseGui(f'{p_old}/run/solo/solo/{cnffile}')


    #%% create a fresh project with the same name
    os.chdir(f'{rundir}/new')
    subprocess.run(('dueca-gproject', 'new', '--name', project,
                    '--script', scriptlang, '--gui', gui, '--remote',
                    f'file://{rundir}/repo/{project}.git'))


    #%% find machine classes and platforms
    os.chdir(f'{p_new}')
    mclasses = [
        mc.split('.')[-1] for mc in os.listdir(f'{p_old}')
            if os.path.isfile(f'{p_old}/{mc}') and mc.startswith('modules.')]
    platforms = [
        d for d in os.listdir(f'{p_old}/run')
           if os.path.isdir(f'{p_old}/run/{d}') and
           d not in ('run-data', 'solo') ]
    all_modules = sorted([
        m for m in os.listdir(f'{p_old}')
            if os.path.isdir(f'{p_old}/{m}') and
            m not in ('run', 'comm-objects')])
    if os.listdir(runargs.diff_location):
        all_patches = [
            ]

    mclass_created = set()

    #%% create the machine classes in the new project
    for mc in mclasses:
        gui = 'none'

        # see if I can find the gui selected from config files
        for p in platforms:
            try:
                gui = parseGui(f'{p_old}/run/{p}/{mc}/{cnffile}')
            except FileNotFoundError:
                pass
            if gui != 'none':
                break

        print('machine', mc, 'gui', gui)

        # find the mapping
        if mc in mc_mapping:
            mclass = mc_mapping[mc]
        else:
            print(f'Cannot find a machine class mapping for {mc}, defaulting')
            mclass = mc

        # create the machineclass
        if mc != 'solo':
            if mclass not in mclass_created:
                subprocess.run(('dueca-gproject', 'new-machine-class',
                                '--name', mclass, '--gui', gui))
                mclass_created.add(mclass)
            else:
                print(f"Re-using machine class {mclass} for {mc}")

        # directly get at the modules file
        modules = Modules(p_new, mclass)

        # read list
        modulelist = readModules(p_old, mc)

        if mc != 'solo':
            for prj, mod in modulelist:
                print('prj', prj, 'mod', mod)
                if modules.hasModule(prj, mod):
                    pass
                else:
                    modules.addModule(prj, mod, None, constructUrl(prj))


    #%% directly move & overwrite the platform stuff
    repo = git.Repo(p_new)
    nmm = NodeMachineMapping(f'{p_new}')
    for p in platforms:
        subprocess.run(('dueca-gproject', 'new-platform', '--name', p))
        os.rename(f'{p_old}/run/{p}', f'{p_new}/run/{p}')
        gitAdd(repo.index, f'{p_new}/run/{p}')

    #%% overwrite the default solo
    copyModuleFiles(repo.index, f'{p_old}/run/solo/solo',
                    f'{p_new}/run/solo/solo')
    copyModuleFiles(repo.index, f'{p_old}/run/run-data',
                    f'{p_new}/run/run-data')


    #%% map nodes to a machine
    for p in platforms:
        nodes = [ n for n in os.listdir(f'{p_new}/run/{p}')
                  if os.path.isdir(f'{p_new}/run/{p}/{n}') ]
        for n in nodes:
            if n in mclasses:
                nmap = mc_mapping.get(n, n)
                try:
                    nmm.newMapping(n, nmap, True)
                    print(f"mapping machine {n} to class {nmap}")
                except Exception:
                    print(f"Incompatible mapping at {p}/{n}")
            else:
                nmm.newMapping(n, 'solo')

    #%% copy/keep files in the main project folder, skipping all modules.*
    for f in os.listdir(p_old):
        if os.path.isfile(f'{p_old}/{f}') and (not f.startswith('modules.')):
            print(f"transfer {p_old}/{f}")
            transferFile(repo.index, p_old, p_new, f)

    #%% copy all modules over on the solo checkout
    modulelist = readModules(p_old, 'solo')
    for m in all_modules:
        print('module', m)
        opts = ['dueca-gproject', 'new-module', '--name', m]
        if not os.path.isfile(f'{p_old}/{m}/Makefile'):
            opts.append('--pseudo')
        if (project, m) not in modulelist:
            opts.append('--inactive')

        subprocess.run(opts)
        copyModuleFiles(repo.index, f'{p_old}/{m}', f'{p_new}/{m}')

    # add the borrowed modules
    modules = Modules(p_new, 'solo')
    for (prj, mod) in modulelist:
        if prj != project:
            modules.addModule(prj, mod, None, constructUrl(prj))

    #%% copy comm-objects files
    copyModuleFiles(repo.index, f'{p_old}/comm-objects',
                    f'{p_new}/comm-objects')

    #%% check if we need to borrow projects for the dco
    modules = Modules(p_new, 'solo')
    modules._resetCommBorrows(recurse=False)
    for prj in modules.comm_borrows.keys():
        if prj != project and prj not in modules.projects:
            modules.addModule(prj, None, None, constructUrl(prj))

    changed_files = [ item.a_path for item in repo.index.diff(None) ]
    repo.index.add(changed_files)
    repo.index.commit('copied from CVS version')

    # now add a tag
    repo.create_tag('from_cvs', message='As converted from ')
    repo.remote().push('--tags')

    #%% patch file available?
    allok = True
    for patchfile in findPatchFiles(project):
        print(f'running patch file {patchdir}/{patchfile}')
        runres = subprocess.run(('patch', '-p1'), stdin=open(
                f'{patchdir}/{patchfile}', 'r'))
        if runres.returncode != 0:
            allok = False
            print(f"Problems applying patch {patchfile}")

    #%% check further changed files
    changed_files = [ item.a_path for item in repo.index.diff(None) ]
    if allok:
        repo.index.add(changed_files)
        repo.index.commit('after applying working patches')
        repo.remote().push()
    else:
        print("Encountered errors in patch application, correct and commit")
print(rundir)
