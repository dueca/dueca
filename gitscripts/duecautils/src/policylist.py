#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Jun 21 20:58:07 2021

@author: repa
"""

from lxml import etree
from .xmlutil import XML_tag, XML_comment
import datetime
import os

class PolicyStatus:

    def __init__(self, polid=None, files=None, node=None, 
                 tree=None, ignored=False):
        self.node = node

        # read from the node
        self.files = set()
        if tree is None:    
            self.status = (
                node.get('ignored', False) and 'ignored') or 'implemented'
            self.date = node.get('date', 'unknown')
            self.polid = node.get('id')
            for fnode in self.node:
                if XML_comment(node):
                    pass
                elif XML_tag(fnode, 'file'):
                    self.files.add(fnode.text.strip())
                else:
                    raise ValueError("Cannot interpret tag {fnode.tag")

        # create a new node
        else:
            if not self.node:
                self.node = etree.SubElement(tree, 'policy')

            self.update(polid, files, ignored)

    def update(self, polid, files=None, ignored=None):
        self.node.attrib['date'] = \
            datetime.datetime.now().strftime("%Y-%m-%d %H:%M")
        if ignored:
            self.node.attrib['ignored'] = '1'
        elif ignored == False and 'ignored' in self.node:
            del self.node.attrib['ignored']
        self.status = (ignored and 'ignored') or 'implemented'
        self.node.attrib['id'] = polid
        if files is not None:
            for f in files:
                if f not in self.files:
                    self.files.add(f)
                    fnode = etree.SubElement(self.node, 'file')
                    fnode.text = f
                    
class PolicyList:

    def __init__(self, path):
        self.projectdir = path
        self.clean = None
        self.fname = f'{self.projectdir}/.config/policylist.xml'
        self._sync()

    def _sync(self):

        if self.clean:
            return

        if self.clean is None:

            # initially no policies
            self.policies = dict()

            # try to read from file
            try:
                parser = etree.XMLParser(remove_blank_text=True)
                with open(self.fname, 'rb') as pl:
                    self.xmltree = etree.XML(pl.read(), parser=parser)
                    for node in self.xmltree:

                        if XML_comment(node):
                            pass

                        elif XML_tag(node, 'policy'):
                            self.policies[node.get('id')] = \
                                PolicyStatus(node=node)

                        else:
                            raise ValueError("Cannot interpret tag {node.tag}")
                            
                # success, return
                self.clean = True
                return 
            
            except FileNotFoundError:
                # when no file, create an emoty tree
                self.xmltree = etree.Element('policies')

        # try a rename of the current file
        try:
            os.rename(self.fname, self.fname + '~')
        except FileNotFoundError:
            pass

        # write the tree
        etree.ElementTree(self.xmltree).write(
                self.fname, pretty_print=True, encoding='utf-8',
                xml_declaration=True)
        self.clean = True

    def status(self, polid):
        if polid in self.policies:
            return self.policies.get(polid).status
        return 'new'

    def date(self, polid):
        if polid in self.policies:
            return self.policies.get(polid).date
        return 'new'

    def implemented(self, polid, files):
        if polid in self.policies:
            # remove any ignored & new date
            self.policies[polid].update(polid, files=files, ignored=False)
        else:
            # new policy
            self.policies[polid] = PolicyStatus(
                polid=polid, files=files, tree=self.xmltree)
        self.clean = False
            
    def skip(self, polid):
        if polid in self.policies:
            print(f"Policy {polid} already marked, cannot skip now")
            return
        self.policies[polid] = PolicyStatus(
            polid=polid, tree=self.xmltree, ignored=True)
        self.clean = False