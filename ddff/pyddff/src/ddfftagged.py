#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Apr  8 17:16:10 2022

@author: repa
"""
try:    
    from .ddffinventoried import DDFFInventoried, DDFFInventoriedStream
except ImportError:
    from ddffinventoried import DDFFInventoried, DDFFInventoriedStream
import itertools

class DDFFTag:

    def __init__(self, *args):
        """Decoded tag information

        Arguments:
            offset -- array with offset indices for all named streams
            cycle -- integer cycle
            index0 -- start tick for data period
            index1 -- end tick for data period
            time -- wall time string
            name -- name of the period
            inco -- initial condition matching the period
        """
        self.offset, self.cycle, self.index0, self.index1, \
            self.time, self.name, self.inco = args

    def __str__(self):
        return f"Period(n={self.name},cycle={self.cycle},at=\"{self.time}\"," \
               f"span={self.index0}-{self.index1},off={self.offset},ic=\"{self.inco}\")"

    def __repr__(self):
        return f"DDFFTag({self.offset},{self.cycle},{self.index0},{self.index1}," \
            f'"{self.time}","{self.name}","{self.inco}")'

class DDFFTagStream(dict):

    def __init__(self, ddffs, *args, **kwargs):
        """Convert stream to time period information dictionary

        Arguments:
            ddffs -- raw base stream with DDFFTag objects
        """
        super(DDFFTagStream).__init__(*args, **kwargs)
        self.base = ddffs
        for st in self.base.reader():
            self[st[-2]] = DDFFTag(*st)

    class TimeIt:
        def __init__(self, ddffs, t0, t1):
            self.iter0 = iter(ddffs.base)
            if ddffs.base[0][0] < t0:
                tmp = next(self.iter0)
                while tmp[0] < t0:
                    tmp = next(self.iter0)
            self.t1 = t1
            
        def __iter__(self):
            self.iter0, self.it = itertools.tee(self.iter0)
            return self
        
        def __next__(self):
            tmp = next(self.it)
            if tmp[0] > self.t1:
                raise StopIteration()
            return tmp[0]
    
    class ValueIt:
        def __init__(self, ddffs, idx, t0, t1):
            self.idx = idx
            self.iter0 = iter(ddffs.base)
            if ddffs.base[0][0] < t0:
                tmp = next(self.iter0)
                while tmp[0] < t0:
                    tmp = next(self.iter0)
            self.t1 = t1

        def __iter__(self):
            self.iter0, self.it = itertools.tee(self.iter0)
            return self
        
        def __next__(self):
            tmp = next(self.it)
            if tmp[0] > self.t1:
                raise StopIteration()
            if self.idx is None:
                return tmp[-1]
            return tmp[-1][self.idx]


class DDFFTagged(DDFFInventoried):
    """DDFF file with inventory and stretch/time tags. 

    Handles datafiles with an inventory in stream 0, describing the datatype
    written in each stream (from 2 onwards), and a time section tagging in
    stream 1, describing named periods, and the link to the time in each
    stream.
    """

    def __init__(self, name: str, mode='r', nstreams=frozenset((0, 1)), *args, **kwargs):
        """Open a tagged stream datafile

        Arguments:
            name -- filename to open

        Keyword Arguments:
            mode -- open mode, read or write (default: {'r'})
        """

        # analyse with base DDFF read
        super().__init__(name, *args, mode=mode, nstreams=nstreams, **kwargs)

        # replace/parse stream 1, to get a tagstream
        self.streams[1] = DDFFTagStream(self.streams[1])

        # keep the conventional inventoried streams

    def inventory(self):
        """Access the base DDFFInventory class

        Returns:
            Same object, as DDFFInventory
        """
        return super(DDFFTagged,self)

    def index(self):
        """Access the time period index

        Returns:
            A DDFFTagStream (dict) object, with mapping from period names
            to details.
        """
        return self.streams[1]

    def periods(self):
        """List the keys of the available periods

        Returns:
            Iterable of key names
        """
        return self.streams[1].keys()

    def __len__(self):
        """Return number of available periods

        Returns:
            Integer
        """
        return len(self.streams[1])

    def __getitem__(self, key):
        """Access data, from a specific period and a specific named stream

        Arguments:
            key -- tuple(period,streamid,member) or tuple(period,streamid)
            defining the requested period, and which data stream should
            be returned. The member string indicates which data member to
            return

        Raises:
            KeyError: key not found, either period, stream or member name 
            missing.

        Returns:
            Iterator for data, either for a single member, or the whole data
            list/struct
        """
        if isinstance(key, str|int):
            streamid = key
            period = None
            member = None
        elif len(key) == 3:
            streamid, period, member = key
        elif len(key) == 2:
            streamid, period = key
            member = None
        else:
            raise KeyError("Supply 2 or 3 elements for key")

        # get stream number corresponding to the key (or int index)
        if isinstance(streamid, int):
            stream = self.streams[streamid+2]
        else:
            stream = self.mapping[streamid]

        if isinstance(member, str):
            member = stream.members[member]

        # if no period given, treat as inventoried stream
        if period is None:
            return DDFFInventoriedStream.ValueIt(stream.base, member)

        # get start and end time corresponding to the tag
        it = self.streams[1][period]
        idx0, idx1 = it.index0, it.index1

        # use this to return an iterator
        return DDFFTagStream.ValueIt(stream, member, idx0, idx1)


    def time(self, streamid, period):
        """Access time stamps, for a specific period and named stream

        Arguments:
            streamid -- Name or number of requested stream
            period -- Name or number of period

        Returns:
            Iterator for time ticks
        """

        # get start and end time corresponding to the tag
        it = self.streams[1][period]
        idx0, idx1 = it.index0, it.index1
        
        # get stream number corresponding to the key (or int index)
        if isinstance(streamid, int):
            stream = self.streams[streamid+2]
        else:
            stream = self.mapping[streamid]

        # return a time tick iterator
        return DDFFTagStream.TimeIt(stream, idx0, idx1)


if __name__ == '__main__':
    import os

    f2 = DDFFTagged(os.path.dirname(__file__) + 
            '/../../../test/ddff/recordings-PHLAB-new.ddff')
    keys = [ k for k in f2.keys() ]
    nameds = [ n for n in f2.inventory().keys() ]
    for key in keys:
        for n in nameds:
            print("tag", key, "key", n)
            ticks = [t for t in f2.time(key, n)]
            rx = [x for x in f2[key,n,'rx']]
            ry = [y for y in f2[key,n,'ry']]

    for key in keys:
        for n in range(2):
            print("tag", key, "key", n)
            ticks = [t for t in f2.time(key, n)]
            rx = [x for x in f2[key,n,'rx']]
            ry = [y for y in f2[key,n,'ry']]

    print(f2.index()['one'])
