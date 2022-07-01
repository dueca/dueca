import sys
import os
import string
import re

# extend the search path
fsdp = os.popen('dueca-config --path-datafiles')
sys.path.append(fsdp.readline().strip())
fsdp.close()
import CVS
import daux

class ModException(Exception):
    """Problem accessing CVS backend"""

class ModulesList:
    def __init__(self, mach=''):

        if not mach:
            mi = daux.MachineInfo()
            machine = mi.machine
        else:
            machine = mach

        self.has_cvs = 0
        self.modlist = 'modules.' + machine
        if not os.path.exists(self.modlist[:]):
            raise ModException('Cannot find modules.'+ machine)

        # remember the absolute path to the modules.machine file
        # open it and read the modules
        self.modlist = os.path.abspath(self.modlist)
        fd = open(self.modlist, 'r')
        self.modules = fd.readlines()
        fd.close()

        # also find out and remember the application name
        self.appname = os.getcwd().split(os.sep)[-1]

        # remove comments from the modules file
        self.modules = daux.removeComments(self.modules)

        # split into module and revision (if applicable)
        self.revisions = []
        for i in range(0,len(self.modules)):
            parts = self.modules[i].split()
            self.modules[i] = parts[0]
            if len(parts) > 1:
                self.revisions.append(parts[1])
            else:
                self.revisions.append('')

    def has(self, name):
        return self.modules.count(name)

    def withCvs(self):
        return self.has_cvs

    def getModules(self):
        return self.modules

    def modlistname(self):
        return self.modlist

    def externalModsAndVersions(self):
        extmods = []
        for i in range(0,len(self.modules)):
            if self.modules[i].split(os.sep)[0] != self.appname:
                extmods.append([self.modules[i], self.revisions[i]])
        return extmods

    def add(self, name, version=''):

        # check we don't have this module
        if self.has(name):
            raise ModException(name + ' already in modules.lst')

        # check that there is a modules list
        if not self.modlist:
            raise ModException('no modules list')

        # open the file and add it
        fd = open(self.modlist, 'a')
        fd.write('\n'+name+' '+version)
        fd.close()

        # directly add it to the list
        self.modules.append(name)
        self.revisions.append('')

        # update the list
        # need a cvs connection
        cvs = CVS.CVS()
        cvs.commit('added module '+name, \
                   self.modlist.split(os.sep)[-1])

    def reconstruct(self, version='', checkout=1):

        # move one directory up
        t = daux.TempCd('..')

        print(self.revisions)

        # need a cvs connection
        cvs = CVS.CVS()

        # check existence of all modules, checkout own, export others,
        # if chechout is false, always export
        for i in range(0, len(self.modules)):
            if self.modules[i].split(os.sep)[0] == self.appname:
                if checkout:
                    cvs.checkout(self.modules[i], version)
                else:
                    if self.revisions[i]:
                        cvs.export(self.modules[i], self.revisions[i])
                    else:
                        cvs.export(self.modules[i], version)
            else:
                # for externals, always use the revision listed in the
                # modules list
                if self.revisions[i]:
                    cvs.export(self.modules[i], self.revisions[i])
                else:
                    cvs.export(self.modules[i], 'HEAD')

        # return to the old directory
        t.goBack()

    def updateModules(self):

        # move one directory up
        t = daux.TempCd('..')

        # get a unique extension for renaming directory
        ext = '.' + str(os.getpid())

        # need a cvs connection
        cvs = CVS.CVS()

        # export foreign modules again
        for i in range(0, len(self.modules)):
            if self.modules[i].split(os.sep)[0] != self.appname:

                # a borrowed module. Move an existing one (if it exists)
                # out of the way
                if os.path.exists(self.modules[i]):
                    # clean this one first
                    if os.path.exists(self.modules[i] + '/Makefile') :
                        tx = daux.TempCd(self.modules[i])
                        os.system('make mrproper')
                        tx.goBack()
                    # now rename the module directory
                    os.rename(self.modules[i], self.modules[i] + ext)

                # export the stuff from repository
                print('found borrowed', self.modules[i])
                if self.revisions[i]:
                    cvs.export(self.modules[i], self.revisions[i])
                else:
                    print('dueca-project warning, no revision for ',
                          self.modules[i])
                    cvs.export(self.modules[i], 'HEAD')

                # if there is no module now, warn
                if not os.path.exists(self.modules[i]):
                    print('dueca-project warning, did not succeed to get ',
                          self.modules[i])
                elif os.path.exists(self.modules[i] + ext):

                    # compare and produce patches, stand in the common base
                    # directory for that
                    borrowed_from = self.modules[i].split(os.sep)[0]
                    modname = self.modules[i].split(os.sep)[1]
                    t2 = daux.TempCd(borrowed_from)
                    to_ignore = ['.+\.o', '.*\.depend', \
                                 '.*comm-objects\.h', '.*\~']
                    ismodified = daux.compareAndPatch(modname, modname+ext, \
                                                      to_ignore)
                    t2.goBack()
                    if ismodified:
                        choice = input('use modified [m] or update [u]?')
                        if choice == 'm':
                            # remove the new dir, and rename the old one again
                            os.system('rm -rf ' + self.modules[i])
                            os.rename(self.modules[i] + ext, self.modules[i])
                        else:
                            # remove the old dir
                            os.system('rm -rf ' + self.modules[i] + ext)
                    else:
                        # both are the same. More efficient to use the old one
                        os.system('rm -rf ' + self.modules[i])
                        os.rename(self.modules[i] + ext, self.modules[i])

            else:
                if os.path.exists(self.modules[i]):
                    # own module, and developing, existing, update
                    #t2 = daux.TempCd(self.modules[i])
                    cvs.update(self.modules[i])
                    #t2.goBack()
                else:
                    # the module was not checked out yet
                    print('found new module', self.modules[i])
                    cvs.checkout(self.modules[i])

        # return to the old directory
        t.goBack()
