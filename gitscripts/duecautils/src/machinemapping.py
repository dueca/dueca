#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Feb 22 15:45:52 2021

@author: repa
"""

import os
import sys
from lxml import etree
from .verboseprint import dprint
from .xmlutil import XML_comment, XML_tag, XML_TagUnknown

class Platform:

    def __init__(self, name):
        self.nodes = dict()
        self.pname = name

    def addNode(self, nname, machineclass, sparse_checkout):
        if nname in self.nodes:
            raise Exception(
                f"Repeated node {nname} for platform {self.pname},"
                "malformed .config/machinemapping.xml file")
        self.nodes[nname] = (machineclass, sparse_checkout)



class NodeMachineMapping:

    def __init__(self, projectdir):
        self.clean = None
        self.projectdir = projectdir
        self._sync()

    def _sync(self):

        if self.clean:
            return

        fname = f'{self.projectdir}/.config/machinemapping.xml'
        if self.clean is None:

            dprint(f"reading mapping {fname}")

            # clear dictionary
            self.nodes = dict()

            parser = etree.XMLParser(remove_blank_text=True)
            with open(fname, 'rb') as mm:
                self.xmltree = etree.XML(mm.read(), parser=parser)
            for ino, node in enumerate(self.xmltree):

                # skip comment tags
                if XML_comment(node):
                    continue

                if not XML_tag(node, 'node'):
                    raise XML_TagUnknown(node)

                nname = node.get('name', None)
                mclass = node.get('machineclass', None)
                sparse = node.get('sparse-checkout', False)
                if not nname or not mclass:
                    raise Exception(
                        f"Node without name or machine class,"
                        f" malformed file {fname}")

                self.nodes[nname] = (mclass, sparse, node)

                if not os.path.isdir(
                        f'{self.projectdir}/.config/class/{mclass}'):
                    print("Missing machine class folder for class {mclass}",
                          file=sys.stderr)
            self.clean = True

        else:
            # clean is False, write the file
            dprint(f"writing mapping {fname}")
            os.rename(fname, fname + '~')
            etree.ElementTree(self.xmltree).write(
                fname, pretty_print=True, encoding='utf-8',
                xml_declaration=True)

            self.clean = True

    def getClass(self, node: str) -> str:
        if node in self.nodes:
            return self.nodes.get(node)[0]

        # failed, warn and return a harmless default
        print(f"Cannot find class for node {node} in mapping,"
                  " defaulting to solo")
        return 'solo'

    def newMapping(self, nname, mclass, sparse_checkout=False, force=False):

        self._sync()

        if nname in self.nodes:
            if self.nodes[nname][:2] == (mclass, sparse_checkout):
                # no change in mapping
                return
            elif force:
                print(f"Forcing change in mapping for node {nname}, from"
                      f"{self.nodes[nname][0]}"
                      f" {self.nodes[nname][1] and ' (sparse)' or ''}"
                      f"to {mclass}{sparse_checkout and ' (sparse)' or ''}")

                self.nodes[nname][2]['machineclass'] = mclass
                self.nodes[nname][2]['sparse-checkout'] = sparse_checkout
            else:
                raise Exception(
                    f"Conflicting mapping for node {nname}, from"
                    f"{self.nodes[nname][0]}"
                    f" {self.nodes[nname][1] and ' (sparse)' or ''}"
                    f"to {mclass}{sparse_checkout and ' (sparse)' or ''}")
            self.clean = False
        else:
            self.nodes[nname] = (mclass, sparse_checkout)
            self.xmltree.append(
                etree.Element(
                    'node', attrib={
                        'name': nname, 'machineclass': mclass,
                        'sparse-checkout':
                             (sparse_checkout and 'true') or 'false' }))
            self.clean = False

        self._sync()
