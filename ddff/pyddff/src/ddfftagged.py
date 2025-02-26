#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Apr  8 17:16:10 2022

@author: repa
"""
try:
    from .ddffinventoried import DDFFInventoried, DDFFInventoriedStream
    from .ddffbase import vprint, DDFFStream, DDFFBlock
except ImportError:
    from ddffinventoried import DDFFInventoried, DDFFInventoriedStream
    from ddffbase import vprint, DDFFStream, DDFFBlock
import itertools

class DDFFTag:

    def __init__(self, *args):
        """Decoded tag information

        Arguments:
            offset -- array with offset indices for all named streams
            inblock_offset -- array with the in-block start offsets
            cycle -- integer cycle
            index0 -- start tick for data period
            index1 -- end tick for data period
            time -- wall time string
            name -- name of the period
            inco -- initial condition matching the period
        """
        self.offset, self.inblock_offset, self.cycle, self.index0, self.index1, \
            self.time, self.name, self.inco = args

    def __str__(self):
        return f"Period(n={self.name},cycle={self.cycle},at=\"{self.time}\"," \
               f"span={self.index0}-{self.index1},offb={self.offset},offo={self.inblock_offset},ic=\"{self.inco}\")"

    def __repr__(self):
        return f"DDFFTag({self.offset},{self.cycle},{self.index0},{self.index1}," \
            f'"{self.time}","{self.name}","{self.inco}")'

class DDFFTagStream(dict):

    def __init__(self, ddffs, *args, **kwargs):
        """ Converts stream 1 to time period information dictionary

        Arguments:
            ddffs -- raw base stream with DDFFTag objects
        """
        super(DDFFTagStream).__init__(*args, **kwargs)
        self.base = ddffs
        for st in self.base:
            self[st[-2]] = DDFFTag(*st)

    def offsets(self, period: int|str=0):
        if isinstance(period, int):
            return self.base[period][0]
        elif isinstance(period, str):
            return self[period][0]

    class BaseIt:
        """ Base iterator for tagged streams """

        def __init__(self, ddffs: DDFFStream, tag: DDFFTag):
            ids = ddffs.stream_id
            ddffs.file.seek(tag.offset[ids-2])
            block = DDFFBlock(ddffs.file)
            block.tail = block.tail[tag.inblock_offset[ids-2]-28:]
            self.reader = ddffs.reader(block)
            self.t0 = tag.index0
            self.t1 = tag.index1

        def __iter__(self):
            return self

    class TimeIt(BaseIt):

        def __init__(self, ddffs: DDFFStream, tag: DDFFTag):
            super().__init__(ddffs, tag)

        def __next__(self):
            tmp = next(self.reader)
            while tmp[0] + tmp[1] < self.t0:
                tmp = next(self.reader)
            if tmp[0] > self.t1:
                raise StopIteration()
            return tmp[0]

    class ValueIt(BaseIt):
        def __init__(self, ddffs: DDFFStream, tag: DDFFTag, member: int|None):

            super().__init__(ddffs, tag)
            self.idx = member

        def __next__(self):
            tmp = next(self.reader)
            while tmp[0] + tmp[1] < self.t0:
                tmp = next(self.reader)
            if tmp[0] > self.t1:
                raise StopIteration()
            if self.idx is None:
                return tmp[-1]
            return tmp[-1][self.idx]

    def getData(self, period:int|str|None=None, icount:int=100):

        if period is None:
            return self.base.getData(icount)
        
        if isinstance(period, str):
            tag = self[period]
        else:
            tag = [t for t in self.values() if t.cycle == period]
        
        mappers = self.base._getMappers(icount)
        
        i = -1
        for i, d in enumerate(DDFFTagStream.ValueIt(self.base, tag)):
            if i == icount:
                icount = icount*2
                time0.resize((icount, *time0.shape[1:]))
                time1.resize((icount, *time1.shape[1:]))
                for v in result.values():
                    v.resize((icount, *v.shape[1:]), refcheck=False)
            for m in mappers:
                m(d[2], i)
            time0[i] = d[0]
            time1[i] = d[1]

        if i != icount:
            icount = i + 1
            time0.resize((icount, *time0.shape[1:]))
            time1.resize((icount, *time1.shape[1:]))
            for v in result.values():
                v.resize((icount, *v.shape[1:]), refcheck=False)

        return time0, time1, result



class DDFFTagged(DDFFInventoried):
    """DDFF file with inventory and stretch/time tags.

    Handles datafiles with an inventory in stream 0, describing the datatype
    written in each stream (from 2 onwards), and a time section tagging in
    stream 1, describing named periods, and the link to the time in each
    stream.
    """

    def __init__(self, name: str, mode='r', nstreams=frozenset((0, 1)),
                 *args, **kwargs):
        """Open a tagged stream datafile

        Arguments:
            name -- filename to open

        Keyword Arguments:
            mode -- open mode, read or write (default: {'r'})
        """

        # analyse with base DDFF read
        super().__init__(name, *args, mode=mode, nstreams=nstreams, **kwargs)

        # replace/parse stream 1, to get a tagstream
        self.streams[1].readToList()

        # compatibility with the one or two old datafiles lying around
        # the conversion assumes all blocksized are default 4096-bytes
        if len(self.streams[1][0]) == 7:
            vprint("Converting old 7-index tags to new 8-index")
            for tags in self.streams[1]:
                tags.insert(1, [o % 4096 for o in tags[0]])
                for i, _ in enumerate(tags[0]):
                    tags[0][i] -= tags[1][i]
        print(self.streams[1])
        self.streams[1] = DDFFTagStream(self.streams[1])

        # Scan initial blocks for the inventory streams
        self._doInitialScan(self.streams[1].offsets())

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
        if isinstance(period, int):
            it = [ itx for itx in self.streams[1].values() if itx.cycle == period][0]
        else:
            # get start and end time corresponding to the tag
            it = self.streams[1][period]

        # use this to return an iterator
        return DDFFTagStream.ValueIt(stream.base, it, member)


    def time(self, streamid, period):
        """Access time stamps, for a specific period and named stream

        Arguments:
            streamid -- Name or number of requested stream
            period -- Name or number of period

        Returns:
            Iterator for time ticks
        """

        # if no period given, treat as inventoried stream
        if period is None:
            return DDFFInventoriedStream.TimeIt(stream.base)

        # get data on the period
        if isinstance(period, int):
            it = [ itx for itx in self.streams[1].values() if itx.cycle == period][0]
        else:
            it = self.streams[1][period]

        # get stream number corresponding to the key (or int index)
        if isinstance(streamid, int):
            stream = self.streams[streamid+2]
        else:
            stream = self.mapping[streamid]

        # return a time tick iterator
        return DDFFTagStream.TimeIt(stream.base, it)


if __name__ == '__main__':
    import os

    f2 = DDFFTagged(os.path.dirname(__file__) +
            '/../../../test/ddff/recordings-PHLAB-new.ddff')
    keys = [ k for k in f2.keys() ]
    nameds = [ n for n in f2.periods() ]
    for key in keys:
        for n in nameds:
            print("tag", key, "key", n)
            ticks = [t for t in f2.time(key, n)]
            rx = [x for x in f2[key,n,'rx']]
            ry = [y for y in f2[key,n,'ry']]

    print(f2.index()['one'])

    for key in range(len(keys)):
        for n in range(len(nameds)):
            print("tag", key, "key", n)
            ticks = [t for t in f2.time(key, n)]
            rx = [x for x in f2[key,n,'rx']]
            ry = [y for y in f2[key,n,'ry']]
            alld = [z for z in f2[key,n]]
    print(alld)

    t0, t1, data = f2.stream("WriteUnified:first blip").getData(0)
    print(t0, t1, data)