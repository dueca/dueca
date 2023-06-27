#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Apr  7 19:11:09 2022

@author: repa
"""
try:
    from .ddffbase import DDFF, dprint
except:
    from ddffbase import DDFF, dprint
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
            tmp = next(self.it)
            try:
                return tmp[-1][self.idx]
            except IndexError as e:
                print(f"error {e}")
    
    def __init__(self, ddffs, tag, description):
        """Create a parsed inventory of stream id's and contents

        Arguments:
            ddffs -- Raw stream
            tag -- Name of the stream, id
            description -- JSON encoded string with a description of the 
            stream contents. 
        """
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
        if key is None:
            return DDFFInventoriedStream.ValueIt(self.base, None)
        elif isinstance(key, int):
            return DDFFInventoriedStream.ValueIt(self.base, key)
        else:
            return DDFFInventoriedStream.ValueIt(self.base, self.members[key])
    
    def __str__(self):
        return f'Object(class="{self.klass}",members={", ".join(self.members.keys())})'
    
class DDFFInventoried(DDFF):
    
    def __init__(self, fname, mode='r', *args, **kwargs):
        """Open a DDFF datafile with stream inventory

        Arguments:
            fname -- filename to open

        Keyword Arguments:
            mode -- open mode, read or write (default: {'r'})
        """
        
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
    
    def inventory(self):
        return self

    def time(self, key):
        """Access time stamps for a specific named stream

        Arguments:
            key -- Name or number of requested stream

        Returns:
            Iterator for time ticks
        """
        return self.mapping[key].time()
    
    def __getitem__(self, keyname):
        """Access data, from a specific named stream

        Arguments:
            keyname -- tuple(streamid,member) or only streamid
            Defines which data stream should be returned. The member string 
            indicates which data member to return.

        Returns:
            Iterator for data, either for a single member, or the whole data
            list/struct
        """
        try:
            key, varname = keyname
        except ValueError:
            key = keyname
            namename = None

        if isinstance(key, int):
            stream = self.streams[self.streams[0][key][2]]
        else:
            stream = self.mapping[key]

        # return an iterator on the data (or all), corresponding to
        # this stream
        return stream[varname]

    def keys(self):
        """Return an overview of available named streams

        Returns:
            Iterator of strings
        """
        return [ k[0] for k in self.streams[0]]

if __name__ == '__main__':

    stuff = DDFFInventoried('../recordings-PHLAB-new.ddff')
    
    # known entries
    print(stuff.keys())

    for t in stuff.time('WriteUnified:first blip'):
        print (t)
        
    for x in stuff['WriteUnified:first blip', 'rx']:
        print(x)
    for x in stuff['WriteUnified:first blip', 'ry']:
        print(x)
