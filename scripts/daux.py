# auxiliary routines
import os
import sys
import re
import string

# exception
class AuxException(Exception):
    """Problem accessing CVS backend"""

# we
fsdp = os.popen('dueca-config --path-datafiles')
sys.path.append(fsdp.readline().strip())
fsdp.close()
import CVS
from modules import *

# an object that remembers the current directory, and changes to another
# its goBack function goes back to its initial position.
class TempCd:
    def __init__(self, tempdir):
        self.olddir = os.path.abspath('.')
        os.chdir(tempdir)

    def goBack(self):
        os.chdir(self.olddir)

def getUserInput(keys, prompts):

    # check this is possible
    if len(keys) != len(prompts):
        raise AuxException('incorrect arguments')

    al = {}
    for i in range(0,len(keys)):
        if len(sys.argv) > 2+i and sys.argv[2+i]:
            al[keys[i]] = sys.argv[2+i]
        else:
            al[keys[i]] = input(prompts[i] + ' : ')

    # if the last key has a plus, accumulate all the remaining
    # arguments in this key.
    if keys[-1][-1:] == '+':
        for i in range(len(keys)+2,len(sys.argv)):
            al[keys[-1]] = al[keys[-1]] + ' ' + sys.argv[i]

    return al

def removeComments(list):

    # take out all returns, and whitespace
    for i in range(0, len(list)):
        list[i] = list[i].strip()

    # filter the list, first take out all comments (starting with #)
    only_comment = re.compile(r"\s*\#")
    for i in list[:]:
        if only_comment.match(i):
            list.remove(i)
        elif not i:
            list.remove(i)
    # print list


    # now strip comments from lines
    for i in range(0,len(list)):
        idx_hash = list[i].find('#')
        if idx_hash != -1:
            list[i] = list[i][0:idx_hash]
    return list

def may_not_exist(dirname):
    if os.access(dirname, os.F_OK):
        print("File or directory", dirname, "already exists")
        sys.exit(1)

class listFiles:
    def __init__(self, dir):

        # create an empty list of entries
        self.list = []

        # find which entries are in this directory
        entries = os.listdir(dir)

        # all stuff that is a file; append it to the list, all stuff that is a
        # directory, recursively append it.
        for e in entries:
            if os.path.isdir(os.path.join(dir,e)):
                lf = listFiles(os.path.join(dir,e))
                self.list.extend(lf.getList())
            else:
                self.list.append(os.path.join(dir,e))

    def getList(self):
        return self.list

def compareAndPatch(odir, ndir, ignore = []):

    # get a list of files from odir, from within odir
    t = TempCd(odir)
    ol = listFiles('.').getList()
    t.goBack()
    t = TempCd(ndir)
    nl = listFiles('.').getList()
    t.goBack()

    # open the patch file
    pfname = os.sep + 'tmp' + os.sep + ndir.replace(os.sep, '.')
    pf = open(pfname + '.patch', 'w')
    pf.close()

    # determine which diff command is available
    if sys.platform[:5] == 'linux':
        diffcmd = 'diff -Naur '
    else:
        diffcmd = 'diff -ur '

    # do all diffs
    removed = ''
    modified = ''
    for i in ol:
        if os.path.exists(ndir + os.sep + i):
            if os.system(diffcmd + odir + os.sep + i + ' ' + \
                         ndir + os.sep + i + ' >>' + pfname + '.patch') != 0:
                modified = modified + ' ' + i
        else:
            removed = removed + ' ' + i

    # now add removed files
    added = ''
    for i in nl:
        if not os.path.exists(odir + os.sep + i):
            # check that these should not be ignored
            ign = 0
            for ireg in ignore:
                ign = ign or re.compile(ireg).match(i)
            if not ign:
                added = added + ' ' + i

    # and tgz these
    if added:
        t = TempCd(odir)
        os.system('tar cvfz ' + pfname + '-added.tgz' + added)
        t.goBack()

    # user feedback
    if removed:
        print("Files locally removed :", removed)
    if modified:
        print("Files locally modified:", modified)
        print("A patch file (repository -> local) is produced:" +
              pfname + '.patch')
    else:
        os.remove(pfname + '.patch')
    if added:
        print("Files locally added   :" + added)
        print("These are saved in: " + pfname + '-added.tgz')

    # return true only if anything changed
    return added or removed or modified

class Platform:

    def __init__(self, platform):

        t = TempCd('run/' + platform)

        # check the machines on this platform
        self.machines = os.listdir('.')
        self.machines.remove('CVS')
        self.machines.remove('report')
        t.goBack()

        # against all modules.machine files in the main directory
        appfiles = os.listdir('.')
        for i in self.machines:
            if not appfiles.count('modules.' + i):
                print("Warning, cannot find file modules." + i)
                self.machines.remove(i)

    def reportWriteExternal(self, fd):

        for m in self.machines:
            ml = ModulesList(m)

            emods = ml.externalModsAndVersions()
            if emods:
                fd.write('\nBorrowed modules in node ' + \
                         m + ', with versions:')
                for i in emods:
                    fd.write('\n'+ i[0] + '  version:' + i[1])
            else:
                fd.write('\nNo external modules borrowed in node:' + m)

class MachineInfo:

    def __init__(self, machine = ''):

        if machine:
            # if a machine name is given, this defines the machine.
            # the .machine file should not exist yet!
            if os.path.exists('.machine'):
                raise AuxException(
                    ".machine already exists, while trying to re-write it" + \
                    " with " + machine)
            fd = open('.machine', 'w')
            fd.write(machine)
            self.machine = machine
        else:
            # if no machine name is given, we must be able to read this
            fd = open('.machine', 'r')
            self.machine = fd.readline().strip()

    def machine(self):
        return self.machine

def CheckInAppDir(mustnotexist=''):

    # check that we have a modules.?? file, that the module directory
    # does not exist yet AND is not listed in the modules.?? file
    files=os.listdir('.')
    modlists=[]
    cvsfound=0
    for i in files:
        base = i[:8]
        if base == 'modules.':
            modlists.append(i)
        if i == mustnotexist:
            print("There is already a file or directory",
                  mustnotexist + ", exiting")
            sys.exit(1)
        if i == 'CVS':
            cvsfound=1
        if i == '.export-only':
            cvsfound=1

    if not modlists:
        print("Found no modules.machine files, exiting")
        sys.exit(1)

    if not cvsfound:
        print("Did not find CVS directory, exiting")
        sys.exit(1)

    # infer the application name
    pnames = os.getcwd().split(os.sep)
    current = pnames[-1]
    if pnames[-2] != current:
        print("probably not in right directory, exiting")
        sys.exit(1)

    return current
