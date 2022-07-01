import sys
import os
import string
import re

# extend the search path
fsdp = os.popen('dueca-config --path-datafiles')
sys.path.append(fsdp.readline()[:-1])
fsdp.close()
import CVS
from daux import *

class DCOException(Exception):
    def __init__(self, msg=""):
        self.message = msg

class CommPair:

    def __init__(self, line, file):

        # split the line into space-separated parts
        parts = line.split()

        # parts[0] should exist and be a non-empty string
        if len(parts) == 0 or len(parts) > 2 or \
               not len(parts[0]) or \
               parts[0][-4:] != '.dco' or \
               len(parts[0].split(os.sep)) != 3 or \
               parts[0][0] == os.sep:
            raise DCOException("no valid comm object in file " + file + \
                    "\nline: \"" + line + '"')

        self.object = parts[0]

        if len(parts) == 2:
            self.tag = parts[1]
        else:
            self.tag = ''

    # this function has serious side effects, but is the most effective way
    # of filtering the list and helping with tag policies
    def inList(self, list):
        count = 0
        for other in list:
            if other.object == self.object:

                # yes, there is another object with the same base name
                count = count + 1

                # if only one of us has a non-zero tag, assume the non-zero
                # tag is good, and take it both
                if not self.tag and other.tag:
                    self.tag = other.tag
                elif not other.tag and self.tag:
                    other.tag = self.tag

                # now check whether the two tags are the same. Give a
                # warning when not
                if other.tag != self.tag and count == 1:
                    print("dueca-project found different revisions for " +
                          self.object +
                          ", '" + self.tag + "'/'" + other.tag +"'\n")
        return count

    def toString(self):
        return '['+self.object+'/'+self.tag+']'

    def dir(self):
        parts = self.object.split(os.sep)
        return parts[0] + os.sep + parts[1]

class CommDirPair:

    def __init__(self, line):

        # split the line into space-separated parts
        parts = line.split()

        # parts[0] should exist and be a non-empty string
        if len(parts) == 0 or len(parts) > 2 or \
               not len(parts[0]) or \
               len(parts[0].split(os.sep)) != 2 or \
               parts[0][0] == '/':
            print('Line "' + line + '" no valid comm object dir')
            raise DCOException("no valid comm object dir")

        self.object = parts[0]

        if len(parts) == 2:
            self.tag = parts[1]
        else:
            self.tag = ''

    # this function has serious side effects, but is the most effective way
    # of filtering the list and helping with tag policies
    def inList(self, list):
        count = 0
        for other in list:
            if other.object == self.object:

                # yes, there is another object with the same base name
                count = count + 1

                # if only one of us has a non-zero tag, assume the non-zero
                # tag is good, and take it both
                if not self.tag and other.tag:
                    print("dueca-project, found a non-tagged and tagged" +
                          " borrow for " + self.object +
                          ' using tag "' + other.tag + '"')
                    self.tag = other.tag
                elif not other.tag and self.tag:
                    print("dueca-project, found a non-tagged and tagged" +
                          " borrow for " + self.object +
                          ' using tag "' + self.tag + '"')
                    other.tag = self.tag

                # now check whether the two tags are the same. Give a
                # warning when not
                if other.tag != self.tag and count == 1:
                    print("dueca-project found revisions tags for " + \
                          self.object + ', using "' + self.tag + \
                          '", dropping "' + other.tag + '"')
                    other.tag = self.tag

        return count

    def toString(self):
        return '['+self.object+'/'+self.tag+']'

class CommObjectsList:

    def __init__(self, modlist, appname):

        if CheckInAppDir() != appname:
            raise DCOException('not in application directory')

        self.appname = appname

        # use the modlist to find all comm objects.lst files
        t = TempCd('..')

        # the future list with comm objects
        self.cobjects = []

        for i in modlist.getModules():
            if not os.path.exists(i+'/comm-objects.lst'):
                #sys.stderr.write("Warning: cannot find the",
                #                 "comm-objects.lst in"+i)
                pass
            else:
                fd = open(i+'/comm-objects.lst', 'r')
                for cp in removeComments(fd.readlines()):
                    self.cobjects.append(CommPair(cp, i+'/comm-objects.lst'))

        # now remove all doubles
        maxidx = len(self.cobjects) - 1
        for i in range(maxidx, -1, -1):
            if self.cobjects[i].inList(self.cobjects) > 1:
                # print "found duplicate object", self.cobjects[i].toString()
                del self.cobjects[i]

        # now create a list of borrowed directories
        self.borrowdir = []
        for i in self.cobjects:
            cdir = i.dir() + ' ' + i.tag
            if cdir.split(os.sep)[0] != self.appname:
                self.borrowdir.append(CommDirPair(cdir))

        # and again remove the doubles
        maxidx = len(self.borrowdir) - 1
        for i in range(maxidx, -1, -1):
            if self.borrowdir[i].inList(self.borrowdir) > 1:
                del self.borrowdir[i]

        t.goBack()

    def reconstruct(self, appversion, checkout = 1):
        # checkout or export all needed comm objects as given in the
        # list that was prepared at class construction
        # formerly:
        # updateCommObjects(modlist, appname, appversion, checkout=1):

        # get a unique extension for renaming directories
        ext = '.' + str(os.getpid())

        # move up to above the project-specific directories
        t = TempCd('..')

        # need a cvs connection
        cvs = CVS.CVS()

        # always check out our 'own' comm-objects directory, if only as a
        # hint that stuff can be put in there
        if not os.path.exists(self.appname+os.sep+'comm-objects'):
            if checkout:
                cvs.checkout(self.appname+os.sep+'comm-objects', appversion)
            else:
                cvs.export(self.appname+os.sep+'comm-objects', appversion)
        else:
            cvs.update(self.appname+os.sep+'comm-objects')

        # iterate over all comm objects directories
        for c in self.borrowdir:

            # check whether there is a version requested
            if c.tag:
                version = c.tag
            else:
                version = 'HEAD'

            if os.path.exists(c.object):
                # clean this one first
                tx = TempCd(c.object)
                os.system('make mrproper')
                tx.goBack()
                # now rename the module directory
                os.rename(c.object, c.object + ext)

            # export the stuff
            print("borrowing " + c.toString())
            cvs.export(c.object, version)

            # if no module now, warn
            if not os.path.exists(c.object):
                print("warning, did not succeed to get " + c.object)
            elif os.path.exists(c.object + ext):
                os.system('rm -rf ' + c.object + ext)


        # back to original directory
        t.goBack()

    def allFromProject(self, project):

        # return a list of all simple files in a certain project
        res = ''
        for i in self.cobjects:
            parts = i.object.split(os.sep)
            if parts[0] == project:
                if res:
                    res = res + ' ' + parts[2]
                else:
                    res = parts[2]
        return res


