#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun May  2 17:25:18 2021

@author: repa
"""

#from lxml import etree
from .modules import XML_comment, XML_tag
from .actions import PolicyAction
from .conditions import PolicyCondition
from .modules import Modules
from .policylist import PolicyList
from .commobjects import CommObjectsList
from .verboseprint import dprint
from lxml import etree
from urllib import request
import sys
import os

class Policy:

    def __init__(self, node):
        self.name = node.get('name')
        self.polid = node.get('id')
        self.description = ''
        self.condition = None
        self.actions = []

        # find the conditions for this policy
        for elt in node:

            # skip the comments
            if XML_comment(elt):
                continue

            if XML_tag(elt, 'description'):
                self.description = elt.text

            elif XML_tag(elt, 'condition'):
                self.condition = PolicyCondition.create(elt)

            elif XML_tag(elt, 'action'):
                self.actions.append(PolicyAction.create(elt))

            else:
                raise ValueError("Cannot parse XML tag", elt)

        dprint("Added policy", self.polid)

    def holds(self, p_plist, **kwargs):
        """ Determine if the relevant condition for policy change is present.

        The keyword arguments are:
        - p_path: str         - path of the project folder
        - module: str       - currently considered module, if applicable
        - modules: Modules  - object with available own and borrowed modules
        - commobjects: CommObjectsList - list of comm objects
        - plist: PolicyList - presently known policies

        Returns tuple (bool, list of str)
        - True              - policy conditions detected
        - False             - not detected/ignored

        and a list of arguments on decision details
        """
        res, motivation, newvars = self.condition.holds(p_plist=p_plist, **kwargs)
        motivation = list(motivation)
        if p_plist.status(self.polid) == 'ignore':
            motivation.append(
                f" Status for policy {self.polid} is set to ignore for this project")
            res = False
        elif p_plist.status(self.polid) == 'implemented':
            motivation.append(
                f" This policy was applied on {p_plist.date(self.polid)}")
        else:
            motivation.append(
                f" Policy {self.polid} was tested for application")
        return res, motivation, newvars

    def enact(self, kwargs):

        res = []
        files = []
        for act in self.actions:
            description, f2 = act.enact(**kwargs)
            res.append(description)
            files.extend(f2)
        return '\n'.join(res), files

def _readPolicyFile(fname, openedFiles=None):
    policies = []
    if openedFiles is None:
        openedFiles = set(['fname'])


    try:
        if os.path.isfile(fname):
            f = open(fname, 'rb')
        else:
            f = request.urlopen(fname)

        dprint(f"Reading policies from {fname}")
        parser = etree.XMLParser(remove_blank_text=True)
        xmltree = etree.XML(f.read(), parser=parser)

    except ValueError as e:
        # can happen if URL not valid
        print(f"Cannot read policies from url {fname}: {e}",
             file=sys.stderr)
        raise FileNotFoundError()

    except FileNotFoundError as e:
        print(f"Cannot read policies from file {fname}: {e}",
              file=sys.stderr)
        raise e

    # when here, opened, and have xmltree, try to parse
    try:
        for node in xmltree:
            if XML_comment(node):
                continue

            elif XML_tag(node, 'policy'):
                policies.append(Policy(node))

            elif XML_tag(node, 'policyfile'):
                fname2 = node.text.strip()
                if fname2 in openedFiles:
                    print(
                        f"Detecting file recursion or double use in {fname},"
                        f" ignoring {fname2}")

                # try this first as full filename, and then as a relative
                # file
                try:
                    policies.extend(_readPolicyFile(fname2))
                except FileNotFoundError:
                    # try with a relative url, based on the current filename
                    pdir = os.sep.join(fname.split(os.sep)[:-1])
                    policies.extend(_readPolicyFile(pdir + '/' + fname2))

            else:
                print(f"Unknown tag in policy file {fname}: {node}")
                # probably not a policy file
                return policies

    except ValueError as e:
        print(f"Cannot parse policies from url {fname}: {e}",
             file=sys.stderr)

    return policies

class Policies:

    def __init__(self, path, defaultpol=False, urls=None, explain=False):

        #print("creating Policies", path, defaultpol, urls)

        # assemble applied and known policies in this project
        self.known_policies = {}
        self.policies = []
        self.explain = explain

        # read a user's own policies
        if not urls or defaultpol:
            homedir = os.environ.get('HOME', '/dev/null')

            # user-defined policy file
            if os.path.isfile(f'{homedir}/.config/dueca/policies.xml'):
                self.policies.extend(_readPolicyFile(
                    f'{homedir}/.config/dueca/policies.xml'))
            try:

                # policy files from environment
                for pfile in os.environ['DUECA_POLICIES'].split(';'):
                    self.policies.extend(_readPolicyFile(pfile))

            except KeyError:

                # no environment variable
                pass

            except Exception as e:

                print(f'Error reading policies from environment file {pfile}: {e}')

        if urls is not None:

            for url in urls:
                try:
                    self.policies.extend(_readPolicyFile(url))
                except FileNotFoundError:
                    pass

        # prepare for processing a folder
        self.projectpath = path
        mods = Modules(path)
        self.machine = mods.mclass
        self.machines = [ mods.mclass ]
        self.machines.extend(
            [ m for m in os.listdir(f'{path}/.config/class') if
                str(m) != self.machine and
                os.path.isfile(f'{path}/.config/class/{m}/modules.xml') ])
        self.modules = dict([(m, Modules(path, mclass=m))
                             for m in self.machines])
        self.plist = PolicyList(path)
        self.commobjects = dict([(m, CommObjectsList(f'{path}/{m}'))
                                 for m in os.listdir(path)
                                 if os.path.isfile(f'{path}/{m}/comm-objects.lst')])
        #for m in self.modules.getOwnModules():
        ##    if os.path.isfile(f'{path}/{m}/comm-objects.lst'):
        #        self.commobjects[str(m)] = CommObjectsList(f'{path}/{m}')
        #self.commobjects['comm-objects'] = CommObjectsList(
        #    f'{path}/comm-objects')


    def _prepareArgs(self, policyname, policyid):
        """
        Workspace for evaluating policy conditions

        Parameters
        ----------
        policyname : str
            Name of the considered policy.
        policyid : str
            Policy identification.
        modulename : str
            Name of the module, or of comm-objects folder.

        Returns
        -------
        dict
            Set of variables for policy evaluation.
            p_path : absolute path to the project
            p_project : project name
            p_machine : currently selected machine class
            p_modules : dict, keyed to machine class name, with Modules
                        objects indicating wich modules in a machine class
            p_policy  : str, name of the policy
            p_polid   : str, policy ID
            p_commobjects : dict, keyed to module name, with CommObjectsList
                        objects

        """
        return {
            'p_path' : self.projectpath,
            'p_project' : self.projectpath.split(os.sep)[-1],
            'p_machine' : self.machine,
            'p_modules' : self.modules,
            'p_plist' : self.plist,
            'p_policy': policyname,
            'p_polid': policyid,
            'p_commobjects': self.commobjects}

    def inventory(self):

        result = []
        for p in self.policies:
            args = self._prepareArgs(p.name, p.polid)
            res, mot, newvars = p.holds(**args)
            if res and not self.explain:
                result.append(f"Policy {p.polid}: {p.name}: Applicable")
            if self.explain:
                l = [f"Policy {p.polid}: {p.name}: {((not res) and 'Not a') or 'A'}pplicable"]
                l.extend(mot)
                result.append('\n'.join(l))
        return result

    def apply(self, policylist=None, force=False):

        result = []
        for p in self.policies:

            # skip if not to apply this policy
            if (policylist is not None and p.polid not in policylist) or \
                (not force and self.plist.status(p.polid) == 'implemented'):
                continue

            # set up variable space
            args = self._prepareArgs(p.name, p.polid)

            # test applicability, extends variable space
            res, mot, newvars = p.holds(**args)
            #print(newvars)

            # when applicable, apply the policy
            if res:
                args.update(newvars)
                description, files = p.enact(args)
                result.append(description)

                # mark policy as implemented
                dprint(f"Policy {p.polid} applied to {files}")
                self.plist.implemented(p.polid, files)

        self.plist._sync()
        return result

    def skip(self, policylist=None):

        result = []
        for p in self.policies:
            if p.polid in policylist and self.plist.status(p.polid) == 'new':
                self.plist.skip(p.polid)
                result.append(f"Marking policy {p.polid} as ignored.")
        self.plist._sync()
        return result