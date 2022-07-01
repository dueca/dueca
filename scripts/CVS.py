import os
import string
import sys
from daux import *

class CVSException(Exception):
    """Problem accessing CVS backend"""

class CVS:
    def __init__(self):

        # find the cvs root for dueca applications
        if 'DAPPS_CVSROOT' in os.environ:
            self.cvsroot=os.environ['DAPPS_CVSROOT']
        elif 'CVSROOT' in os.environ:
            self.cvsroot=os.environ['CVSROOT']
        else:
            self.cvsroot=input("Supply CVS root:")

        if self.cvsroot=='':
            self.cvslater = 1
            self.cvsroot='@cvsroot@'
        else: self.cvslater = 0

        # make certain that cvs will use ssh
        os.environ['CVS_RSH'] = 'ssh'

        # find any additional CVS repositories
        if 'DAPPS_CVSEXTRAROOT' in os.environ:
            allextra = os.environ['DAPPS_CVSEXTRAROOT']
            self.cvsextra = allextra.split(',')
        else:
            self.cvsextra = [ ]

        if self.cvsroot:
            print("Main CVS repository   "+self.cvsroot)
        for r in self.cvsextra:
            print("Additional repository "+r)

    def findAppDir(self):

        # find the application directory, either as this one or one above
        olddir = os.path.abspath('.')

        dirlist = olddir.split(os.sep)
        reldir = []
        while len(dirlist) >= 2 and \
              ( dirlist[-1] != dirlist[-2] or \
                not os.path.exists('.machine') or \
                not os.path.exists('CVS')):
            os.chdir('..')
            reldir.insert(0, dirlist.pop())

        # if we at up our dirlist, return false
        if len(dirlist) < 2:
            os.chdir(olddir)
            return 0

        # does the rememberfile exist?
        self.appdir = os.path.abspath('.')
        self.rememberfile = self.appdir + os.sep + 'stored-commands.cvs'

        # the relative path is the path from the app dir to the dir the
        # command is scheduled in
        reldir.insert(0, '.')
        self.relpath = os.sep.join(reldir)
        os.chdir(olddir)
        return 1

    def checkForSingleCVSModule(self, cvsrep, module):

        # pry apart the cvs repository definition
        repparts = cvsrep.split(':')
        if len(repparts) == 4 and repparts[1] == 'ext':

            # external repository, assume ssh, check for a directory and
            # for a file
            res = os.system('ssh ' + repparts[2] + " test -d '" +
                            repparts[3]+'/'+module + "' -o -f '" +
                            repparts[3]+'/'+module+",v'")
            if (res & 0xffff) == 0:

                return 1

            # failed, not there
            return 0

        elif len(repparts) == 1:

            # repository on the local system, just ls
            return os.path.exists(cvsrep + '/' + module) or \
                   os.path.exists(cvsrep + '/' + module + ',v')

        # at this point, we could not successfully access an external
        # CVS dir, nor one on the system here. Admit failure
        raise CVSException('Cannot analyse cvs path ' + cvsrep)


    def checkForCVSModules(self, module):
        # this function checks whether a CVS module is present on one
        # of the cvs repositories. It returns the repository where the
        # module is found.


        # start with own
        if self.checkForSingleCVSModule(self.cvsroot, module):
            return self.cvsroot

        for r in self.cvsextra:
            if self.checkForSingleCVSModule(r, module):
                return r

        # no such module
        return ''


    def runStacked(self):

        if not self.findAppDir():
            raise CVSException("Problem in finding application directory")

        if self.cvslater:
            print("Cannot commit without connection to CVS server")
            sys.exit(1)

        if os.path.exists(self.rememberfile):

            # move there
            t = TempCd(self.appdir)

            # open the file with commands
            fs = open(self.rememberfile, 'r')
            commands = fs.readlines()
            while len(commands):
                t2 = TempCd(commands.pop(0)[:-1])
                os.system(commands.pop(0).replace \
                          ('@cvsroot@', self.cvsroot, 1))
                t2.goBack()

            # remove the file and set back again
            os.remove(self.rememberfile)
            t.goBack()

    def os_system(self, a, can_postpone=0):
        # print("CVS line", a)

        if self.cvslater:
            if self.findAppDir() and can_postpone:

                # schedule the command for later, because now we don't have
                # connection
                print("Scheduling the following command for later execution:",
                '\n' + a + '\n')
                fs = open(self.rememberfile, 'a')
                fs.write(self.relpath + '\n')
                fs.write(a + '\n')
                fs.close()

            else:
                print("Cannot schedule cvs command\n",a)
                sys.exit(1)

        elif self.findAppDir() and os.path.exists(self.rememberfile):

            # should first run a commit
            print("You have pending cvs commands, run a commit first")
            sys.exit(1)

        else:
            res = os.system(a)
            if res & 0xffff != 0:
                print("Error encountered by cvs command:\n" + a)
            return res

    def os_popen(self, a):
        print("CVS line", a)

        if self.cvslater:
            print("Cannot schedule cvs command\n",a)
            sys.exit(1)

        elif self.findAppDir() and os.path.exists(self.rememberfile):

            # should first run a commit
            print("You have pending cvs commands, run a commit first")
            sys.exit(1)
        else:
            return os.popen(a)

    def add(self, object):
        self.os_system('cvs -d ' + self.cvsroot + ' add '+object, 1)

    def recursiveAdd(self, dir):

        # directory must have been added
        t = TempCd(dir)
        names = os.listdir('.')
        if names.count('CVS'):
            names.remove('CVS')

        # add all this
        allnames = ''
        for i in names:
            allnames = allnames + "'" + i + "' "

        # cvs add of all dirs and files, except CVS itself
        self.add(allnames)

        # walk through, and recursively call directories
        for i in names:
            if os.path.isdir(i):
                self.recursiveAdd(i)

        # return to main dir
        t.goBack()

    def cimport(self, object, message):
        if message: m2 = '-m "' + message + '" '
        else: m2 = ''
        self.os_system('cvs -d ' + self.cvsroot + \
                       ' import ' + m2 + object + \
                       ' ' + os.environ['USER'] + ' initial_version')

    # check out an object from cvs. Version may contain versioning info,
    # e.g. -r tag or -D date
    def checkout(self, object, version = ''):

        if version: version2 = '-r ' + version
        else: version2 = ''
        self.os_system('cvs -d  ' + self.cvsroot + \
                       ' checkout -P ' + version2 + ' ' + object)

    # export, so get without version info
    def export(self, object, version = ''):

        # check availability in different CVS repositories
        cvsrep = self.checkForCVSModules(object)

        if cvsrep:
            self.os_system('cvs -d  ' + cvsrep + \
                           ' export -r ' + version + ' ' + object)
        else:
            print("Cannot find module " + object)

    def commit(self, message = '', object = ''):
        # do we have stacked cvs commands? if so, run these first
        self.runStacked()

        if message: message2 = '-m \"' + message + '"'
        else: message2 = ''
        self.os_system('cvs -d  ' + self.cvsroot + ' commit ' + message2 + \
                       ' ' + object)

    def exportChangeName(self, object, appname, version):
        if version: version2 = '-r ' + version
        else: version2 = ''

        # check availability in different CVS repositories
        cvsrep = self.checkForCVSModules(object)

        if cvsrep:
            self.os_system('cvs -d ' + cvsrep + \
                           ' export ' + version2 + ' -d ' + appname + ' '
                           + object)
        else:
            print("Cannot find module " + object)

    def exportNoRecurse(self, object, version = 'HEAD'):

        # check availability in different CVS repositories
        cvsrep = self.checkForCVSModules(object)

        if cvsrep:
            self.os_system('cvs -d ' + cvsrep + \
                           ' export -r ' + version + ' -l ' + object)
        else:
            print("Cannot find module " + object)

    def checkoutNoRecurse(self, object, version = ''):
        if version: version2 = '-r ' + version
        else: version2 = ''
        self.os_system('cvs -d ' + self.cvsroot + \
                       ' checkout ' + version2 + ' -l ' + object)

    def updateLocal(self):
        # get all files in the local directory. The -l prevents
        # checking out all directories
        print("updating files in the project directory")
        self.os_system('cvs -d ' + self.cvsroot + ' update -d -l')

    def update(self, dir = ''):
        # complete update, including recursion
        if dir:
            x = 1 #nonsensw
            # print("updating " + dir)
        else:
            print("updating in " + os.path.abspath('.'))
        self.os_system('cvs -d ' + self.cvsroot + ' update -d ' + dir)

    def rtag(self, tagname, object):
        self.os_system('cvs -d ' + self.cvsroot + ' rtag ' + \
                       tagname + ' ' + object)

    def showtags(self, object = 'modules.solo'):
        # tell cvs to return the status of modules.solo
        # (must be in from the start)
        fd = self.os_popen('cvs -d ' + self.cvsroot + \
                           ' status -v ' + object)

        # read until the magic line
        lines = fd.readlines()

        magicword = 0
        tags = []
        for i in lines:
            if magicword and i and i.find('No Tags Exist') == -1 \
                   and len(i.split()):
                tags.append(i.split()[0])
            elif i.find('Existing Tags') != -1:
                magicword = 1

        return tags

    def isUpToDate(self):

        # run a general status report
        fd = self.os_popen('cvs -d ' + self.cvsroot + ' status')

        # status lines can be:
        # Up-to-datw
        # Locally Modified
        # Locally Added
        # Locally Removed
        # Needs Checkout
        line = fd.readline()
        up_to_date = 1
        while line:
            if line.find('File:') and line.find('Status:') \
               and not line.find('Up-to-date'): up_to_date = 0
            line = fd.readline()

        return up_to_date

    def findTags(self, object):
        fd = self.os_popen('cvs -d ' + self.cvsroot + \
                           ' rlog -r ' + object)

        # read until the magic line "symbolic names:"
        lines = fd.readlines()
        magicword = 0
        tags = []
        for i in lines:
            if i:
                if i.find('keyword substitution:') != -1:
                    magicword = 0
                elif i.find('symbolic names:') != -1:
                    magicword = 1
                elif magicword and len(i.split(':')):
                    tags.append(i.split(':')[0].strip())

        return tags

    def release(self, object):
        self.os_system('cvs -d ' + self.cvsroot + \
                       ' release ' + object)

    def remove(self, objects):
        self.os_system('cvs -d ' + self.cvsroot + \
                       ' remove -l ' + objects)
