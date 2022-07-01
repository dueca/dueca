'''     item            : SVN.py
        made by         : René van Paassen
        date            : 070404
        category        : python class
        description     : implements interface to SVN repositories
        changes         : 070404 first version
        language        : python
'''

# Design of the SVN repository
# apps                    application directory
# apps/MyApp              base directory for an application
# apps/MyApp/trunk        main directory for an application. It may
#                         include extras such as README files etc.
# apps/MyApp/tags         base directory for tagged copies
# apps/MyApp/tags/rx      main directory for a tagged copy of the application
# apps/MyApp/<t-or-t>/run       run directory, in trunk or tagged
# apps/MyApp/<t-or-t>/.config   config files in the main directory; modules.*,
#                               Makefile.*
# apps/MyApp/<t-or-t>/modules   directory with "own" modules
# apps/MyApp/<t-or-t>/comm-objects communication objects

import os
import string
import pysvn
from daux import *

def get_login(realm, username, may_save):
    """ callback function for login information """

    print("Login information for accessing " + realm)
    nuser = raw_input("Username (" + username "):")
    if not nuser: nuser = username
    password = getpass("Password:")

    if nuser and password:
        return True, nuser, password, False

    return False, nuser, password, False

class ClientError:
    """ Exception class """

    def __init__(self, reason=''):
        """Creates an instance of ClientError

        reason - exception message"""
        self.reason = "SVN error " + reason

    def __str__(self):
        return self.reason


class SVN(pysvn.Client):
    """ Class for dueca-project level interaction with SVN repositories.

    DAPPS_SVNROOT - Environment variable that is investigated to produce the
                    primary subversion repository. Here any new projects will be
                    added.
    DAPPS_SVNEXTRAROOT - Environment variable with ;-separated additional
                    subversion repositories. Modules can be borrowed from here
                    too.
    """

    def __init__(self):

        # base class init
        pysvn.Client.__init__(self)
        self.callback_get_login = get_login

        # find the svn root for dueca applications
        if os.environ.has_key('DAPPS_SVNROOT'):
            self.svnroot=os.environ['DAPPS_SVNROOT']
        else:
            self.svnroot=os.environ['DAPPS_SVNROOT']

        # find any additional SVN repositories
        if os.environ.has_key('DAPPS_SVNEXTRAROOT'):
            allextra = os.environ['DAPPS_SVNEXTRAROOT']
            self.svnextra = string.split(allextra, ';')
        else:
            self.svnextra = [ ]

    def checkForApp(self, appname):
        """ Check whether an application is present in the repository

        appname - name of the application
        returns True if application found"""

        return pysvn.Client.is_url(self, self.svnroot + '/dapps/' +
                                   appname + '/trunk/.config')


    def checkForSingleSVNModule(self, svnrep, module):
        """Checks that an SVN module is present.

        svnrep - url to the repository
        module - location of the module, in the form of appname/module"""

        module = string.split(module, '/')
        appname = module[0]
        module = module[1]
        return self.is_url(svnrep + '/dapps/'+ appname + '/trunk/' + module)

    def checkForSVNModules(self, module):
        """Checks that an SVN module is present in either the primary svn
        repository or one of the auxiliary ones.

        module - module name, in the form of appname/module"""

        if self.checkForSingleSVNModule(self.cvsroot, module):
            return self.svnroot

        for r in self.svnextra:
            if self.checkForSingleCVSModule(r, module):
                return r

        return None

    def add(self, object, recurse=False):
        """Add a single object or a directory to the repository

        object   - file to be added
        recurse  - if True, add recursively"""

        pysvn.Client.add(self, object)

    def recursiveAdd(self, dir):
        """Recursively add a directory to the repository

        dir - directory to add"""

        self.add(self, dir, recurse=True)

    def createApp(self, appname):
        """Create the directory structure for a new application.

        There may not be an existing file or directory with appname, and
        the application should not exist in the repository

        appname  - string with name of the application
        returns True if all OK"""

        try:
            # first check that this is clean
            if self.is_url(self.svnroot + '/dapps/' + appname):
                raise ClientError("Application already exists in repository")

            # ensure there is no project assembly directory
            if os.access(appname, os.F_OK):
                raise ClientError(appname + " already exists as file")

            # create base structure
            baseurl = self.svnroot + '/dapps/' + appname
            d = [baseurl + '/trunk',
                 baseurl + '/tags',
                 baseurl + 'trunk/.config',
                 baseurl + 'trunk/run'
                 baseurl + 'trunk/run/solo',
                 baseurl + 'trunk/run/solo/solo',
                 baseurl + 'trunk/run/run-data',
                 baseurl + 'trunk/modules',
                 baseurl + 'trunk/comm-objects']
            pysvn.Client.mkdir(self, d, 'new project')

        except pysvn.ClientError, e:
            print("Problem SVN" + e)
            return False

        except ClientError, e:
            print(e)
            return False

        return True

    def checkoutAppBase(self, appname):
        """Checks out the base of an application, meaning comm-objects dir,
        configuration files and run files

        appname - name of the application"""

        try:
            # (try to) create the project assembly directory
            os.mkdir(appname)

            # check out base dirs, main dir non-recursive
            self.checkout(self.svnroot+'/dapps/'+appname+'/trunk',
                          appname+os.sep+appname, recurse = False)
            self.checkout(self.svnroot+'/dapps/'+appname+'/trunk/comm-objects'
                          appname+os.sep+appname+os.sep+'comm-objects')
            self.checkout(self.svnroot+'/dapps/'+appname+'/trunk/.config'
                          appname+os.sep+appname+os.sep+'.config')
            self.checkout(self.svnroot+'/dapps/'+appname+'/trunk/run'
                          appname+os.sep+appname+os.sep+'run')

        except pysvn.ClientError, e:
            print("Problem SVN" + e)
            return False

        except os.OSError, e:
            print(e)
            return False

    def inAppDir(self):
        """Verify, as far as possible, that we are in an application directory

        A file .machine should exist, and directories .config, comm-objects and
        run"""

        # check expected files
        if not os.access('.machine', os.F_OK):
            return False
        if not os.access('comm-objects', os.F_OK):
            return False
        if not os.access('run', os.F_OK):
            return False

        # check that up dirs ok
        path = os.path.abspath('.')
        path = string.split(path, os.sep)
        if len(path) < 2 or path[-1] != path[-2]:
            return False

        # all passed
        return True

    def checkoutModule(self, modname, tag=None):
        """Check out a module

        module - location of the module, in the form of appname/module"""

        try:
            # check we are at the right spot
            if not self.inAppDir():
                raise ClientError("This must be done from application dir")

            # figure out where the module is
            svnroot = checkForSVNModules(self, module)

            if not svnroot:
                raise ClientError('Module ' + module + ' not found')

            # split up compound module name
            module = string.split(module, '/')
            appname = module[0]
            module = module[1]

            # go up one dir
            cd = TempCd('..')

            # extract from SVN
            if tag:
                pysvn.Client.checkout(svnroot + '/dapps/' + appname +
                                      '/tags/' + tag + '/' + module,
                                      appname + os.sep + module)
            else:
                pysvn.Client.checkout(svnroot + '/dapps/' + appname +
                                      '/trunk/' + module,
                                      appname + os.sep + module)

        except pysvn.ClientError e:
            print("Problem SVN" + e)
            return False

        except ClientError, e:
            print(e)
            return False


    def exportModule(self, modname, tag=None):
        """Borrow/export a module, no version control

        module - location of the module, in the form of appname/module"""

        try:
            # check we are at the right spot
            if not self.inAppDir():
                raise ClientError("This must be done from application dir")

            # figure out where the module is
            svnroot = checkForSVNModules(self, module)

            if not svnroot:
                raise ClientError('Module ' + module + ' not found')

            # split up compound module name
            module = string.split(module, '/')
            appname = module[0]
            module = module[1]

            # go up one dir
            cd = TempCd('..')

            # extract from SVN
            if tag:
                pysvn.Client.export(svnroot + '/dapps/' + appname +
                                    '/tags/' + tag + '/' + module,
                                    appname + os.sep + module)
            else:
                pysvn.Client.export(svnroot + '/dapps/' + appname +
                                    '/trunk/' + module,
                                    appname + os.sep + module)

        except pysvn.ClientError e:
            print("Problem SVN" + e)
            return False

        except ClientError, e:
            print(e)
            return False


    def checkout(self, object, version=None):
        """ Checks out a single file

        file    - file name, (relative path)
        version - a tag
        """
        try:
            if not version or version == 'HEAD':
                pysvn.Client.checkout(self, url = self.svnroot + '/trunk',
                                      path = object)

            else:
                pysvn.Client.checkout(self, url = self.svnroot + '/trunk/'
                                      + version, path = object)
        except (pysvn.ClientError e):
            print("Error from SVN connection "+e)
            sys.exit(1)

    def commit(self, message='', object=None):

        try:
            if object:
                rev = pysvn.Client.checkin(self, object, message)
            else:
                rev = pysvn.Client.checkin(self, '.', message)

            print("Checked in revision: "+rev)

        except (pysvn.ClientError e):
            print("Error from SVN connection "+e)
            sys.exit(1)

    def exportNoRecurse(self, object, version = 'HEAD'):

        # check availability in different CVS repositories
        cvsrep = self.checkForCVSModules(object)

        if cvsrep:
            self.os_system('cvs -d ' + cvsrep + \
                           ' export -r ' + version + ' -l ' + object)
        else:
            print("Cannot find module " + object)

