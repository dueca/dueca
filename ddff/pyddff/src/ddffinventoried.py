#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Apr  7 19:11:09 2022

@author: repa
"""

from .ddffbase import DDFF, dprint
import json


class DDFFInventoriedStream:
    
    class TimeIt:
        def __init__(self, ddffs):
            self.ddffs = ddffs
            
        def __iter__(self):
            self.it = iter(self.ddffs)
            return self
        
        def __next__(self):
            tmp = next(self.it)
            try:
                return tmp[0]
            except TypeError:
                print("could not get time from", tmp, "stream", self.ddffs)
    
    class ValueIt:
        def __init__(self, ddffs, idx):
            self.ddffs = ddffs
            self.idx = idx
            
        def __iter__(self):
            self.it = iter(self.ddffs)
            return self
        
        def __next__(self):
            return next(self.it)[1][self.idx]           
    
    def __init__(self, ddffs, tag, description):
        self.base = ddffs
        self.tag = tag
        structure = json.loads(description)
        self.klass = structure['class']
        self.members = {}
        for im, m in enumerate(structure['members']):
            self.members[m['name']] = im
            
    def time(self):
        return DDFFInventoriedStream.TimeIt(self.base)
    
    def __getitem__(self, key):
        return DDFFInventoriedStream.ValueIt(self.base, self.members[key])
    
    
class DDFFInventoried(DDFF):
    
    def __init__(self, fname, mode='r', *args, **kwargs):
        
        # analyse with base DDFF read
        super().__init__(fname, mode=mode, *args, **kwargs)
        self.mapping = {}
        dprint("number of streams", len(self.streams), self.streams[0])
        
        # stream #0 should now be the inventory
        for tag, streamid, description in self.streams[0]:
            
            # replace/swap the raw streams with inventoried ones 
            self.streams[streamid] = DDFFInventoriedStream(
                self.streams[streamid], tag, description)
        
            self.mapping[tag] = self.streams[streamid]
        
    def time(self, tag):
        return self.mapping[tag].time()
    
    def __getitem__(self, tagname):
        tag, name = tagname
        return self.mapping[tag].__getitem__(name)
        
if __name__ == '__main__':

    stuff = DDFFInventoried(
        '/home/repa/gdapps/DuecaTestCommunication/DuecaTestCommunication/run/solo/solo/' + 
        'recordings-PHLAB.ddff')
    
    for t in stuff.time('SBlip://PHLAB;'):
        print (t)
        
    for x in stuff[('SBlip://PHLAB;', 'x')]:
        print(x)
