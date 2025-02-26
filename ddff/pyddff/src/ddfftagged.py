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
import numpy as np

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
        (
            self.offset,
            self.inblock_offset,
            self.cycle,
            self.index0,
            self.index1,
            self.time,
            self.name,
            self.inco,
        ) = args

    def __str__(self):
        return (
            f'Period(n={self.name},cycle={self.cycle},at="{self.time}",'
            f'span={self.index0}-{self.index1},offb={self.offset},offo={self.inblock_offset},ic="{self.inco}")'
        )

    def __repr__(self):
        return (
            f"DDFFTag({self.offset},{self.cycle},{self.index0},{self.index1},"
            f'"{self.time}","{self.name}","{self.inco}")'
        )


class DDFFTagIndex:
    """Index dictionary of the tags."""

    def __init__(self, ddffs, *args, **kwargs):
        """Converts stream 1 to time period information dictionary

        Arguments:
            ddffs -- raw base stream with DDFFTag objects
        """
        super(DDFFTagStream).__init__(*args, **kwargs)
        self.base = ddffs
        self.taglist = list()
        self.tagdict = dict()
        for st in self.base:
            t = DDFFTag(*st)
            self.tagdict[st[-2]] = t
            self.taglist.append(t)

    def offsets(self, period: int | str = 0):
        """Offsets of the different starting blocks

        Parameters
        ----------
        period : int | str, optional
            Tag period, by default 0

        Returns
        -------
        list of int
            Offset positions in the file for the different streams
        """
        if isinstance(period, int):
            return self.taglist[period].offset
        elif isinstance(period, str):
            return self.tagdict[period].offset

    def __getitem__(self, period: int | str = 0):
        """Return tag index item, either from string or int

        Parameters
        ----------
        period : int | str, optional
            Tag period, by default 0

        Returns
        -------
        DDFFTag
            Tag matching the period
        """
        if isinstance(period, int):
            return self.taglist[period]
        elif isinstance(period, str):
            return self.tagdict[period]

    def __len__(self):
        """ Return number of tags

        Returns
        -------
        int
            Number of available tags
        """
        return len(self.taglist)

    def keys(self):
        """ Return available tag keys.

        Returns
        -------
        keydict
            Keys/periods available
        """
        return self.tagdict.keys()


class DDFFTagStream:

    """ Tagged data stream
    """

    def __init__(self, ddffs: DDFFInventoriedStream, tags: DDFFTagIndex):
        """Converts stream 1 to time period information dictionary

        Arguments:
            ddffs -- raw base stream with DDFFTag objects
        """
        self.base = ddffs
        self.tags = tags

    class BaseIt:
        """Base iterator for tagged streams"""

        def __init__(self, ddffs: DDFFStream, tag: DDFFTag):
            ids = ddffs.stream_id
            ddffs.file.seek(tag.offset[ids - 2])
            block = DDFFBlock(ddffs.file)
            block.tail = block.tail[tag.inblock_offset[ids - 2] - 28 :]
            self.reader = ddffs.reader(block)
            self.t0 = tag.index0
            self.t1 = tag.index1

        def __iter__(self):
            return self

    class TimeIt(BaseIt):

        def __init__(self, ddffs: DDFFStream, tag: DDFFTag):
            super().__init__(ddffs, tag)

        def __next__(self):
            """ Return next time point

            Returns
            -------
            int
                Time/index value

            Raises
            ------
            StopIteration
                End of period
            """
            tmp = next(self.reader)
            while tmp[0] + tmp[1] < self.t0:
                tmp = next(self.reader)
            if tmp[0] > self.t1:
                raise StopIteration()
            return tmp[0]

    class ValueIt(BaseIt):
        def __init__(self, ddffs: DDFFStream, tag: DDFFTag, member: int | None):

            super().__init__(ddffs, tag)
            self.idx = member

        def __next__(self):
            """ Return next data point

            Returns
            -------
            list with data
                Unpacked data from msgpack

            Raises
            ------
            StopIteration
                End of period
            """
            tmp = next(self.reader)
            while tmp[0] + tmp[1] < self.t0:
                tmp = next(self.reader)
            if tmp[0] > self.t1:
                raise StopIteration()
            if self.idx is None:
                return tmp[-1]
            return tmp[-1][self.idx]

    class ObjectIt(BaseIt):
        def __init__(self, ddffs: DDFFStream, tag: DDFFTag):

            super().__init__(ddffs, tag)

        def __next__(self):
            """ Return complete time + data object

            Returns
            -------
            list with time + data object
                Time tag, span, data

            Raises
            ------
            StopIteration
                End of period
            """
            tmp = next(self.reader)
            while tmp[0] + tmp[1] < self.t0:
                tmp = next(self.reader)
            if tmp[0] > self.t1:
                raise StopIteration()
            return tmp

    def time(self, period: int | str | None = None):
        """ Access time stamps, for a specific period and named stream

        Arguments:
            period -- Name or number of period

        Returns:
            Iterator for time ticks
        """

        # if no period given, treat as inventoried stream
        if period is None:
            return self.base.time()

        # get data on the period
        tag = self.tags[period]

        # return a time tick iterator
        return DDFFTagStream.TimeIt(self.base.base, tag)

    def __getitem__(self, arg):
        """ Access data, for a specific period in this stream

        Parameters
        ----------
        arg : tuple(str|int, str|int) or str|int
            Indicated period (None=total data) + data member (None=all)

        Returns
        -------
        ValueIt
            Data iterator
        """
        try:
            period, key = arg
        except TypeError:
            period, key = arg, None
        if period is None:
            return self.base[key]

        if isinstance(key, int) or key is None:
            return DDFFTagStream.ValueIt(self.base.base, self.tags[period], key)
        else:
            return DDFFTagStream.ValueIt(
                self.base.base, self.tags[period], self.base.members[key]
            )

    def getData(self, period: int | str | None = None, icount: int = 100):
        """ Assemble stream data in numpy arrays

        Parameters
        ----------
        period : int | str | None, optional
            Chosen period, if None, return all data
        icount : int, optional
            Default length allocation numpy arrays, by default 100

        Returns
        -------
        (np.array, np.array, dict(str,np.array))
            Time arrays (start time, span), and dictionary with data arrays
            for all object members.
        """
        if period is None:
            return self.base.getData(icount)

        tag = self.tags[period]

        time0 = np.zeros(shape=(icount,), dtype=np.uint32)
        time1 = np.zeros(shape=(icount,), dtype=np.uint32)
        result, mappers = self.base._getMappers(icount)

        i = -1
        for i, d in enumerate(DDFFTagStream.ObjectIt(self.base.base, tag)):
            if i == icount:
                icount = icount * 2
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

    def __init__(
        self, name: str, mode="r", nstreams=frozenset((0, 1)), *args, **kwargs
    ):
        """Open a tagged stream datafile

        Arguments:
            name -- filename to open

        Keyword Arguments:
            mode -- open mode, read or write (default: {'r'})
        """

        # analyse with base DDFF read
        super().__init__(name, *args, mode=mode, nstreams=nstreams, **kwargs)

        # load all data from the tags stream
        self.streams[1].readToList()

        # compatibility with the one or two old datafiles lying around
        # the conversion assumes all blocksized are default 4096-bytes
        if len(self.streams[1][0]) == 7:
            vprint("Converting old 7-index tags to new 8-index")
            for tags in self.streams[1]:
                tags.insert(1, [o % 4096 for o in tags[0]])
                for i, _ in enumerate(tags[0]):
                    tags[0][i] -= tags[1][i]

        # replace the tags stream with an index
        self.streams[1] = DDFFTagIndex(self.streams[1])

        # Scan initial blocks for the inventory streams
        self._doInitialScan(self.streams[1].offsets())

        # replace further streams
        # Use the inventory to enhance the streams
        self.tagmapping = dict()
        for tag, streamid, description in self.streams[0]:
            self.streams[streamid] = DDFFTagStream(
                self.streams[streamid], self.streams[1]
            )
            self.tagmapping[tag] = self.streams[streamid]

    def __getitem__(self, key):
        """Access a specific data stream in the file

        Parameters
        ----------
        key : str|int
            Stream id or number to access

        Returns
        -------
        DDFFInventoriedStream
            Data access
        """
        if isinstance(key, int):
            return self.streams[self.streams[0][key][1]]
        return self.tagmapping[key]

    def tags(self):
        """Access the time period index

        Returns:
            A DDFFTagStream (dict) object, with mapping from period names
            to details.
        """
        return self.streams[1]

    def __len__(self):
        """Return number of available periods

        Returns:
            Integer
        """
        return len(self.streams[1])


if __name__ == "__main__":
    import os

    f2 = DDFFTagged(
        os.path.dirname(__file__) + "/../../../test/ddff/recordings-PHLAB-new.ddff"
    )
    keys = [k for k in f2.keys()]
    nameds = [n for n in f2.tags().keys()]
    for key in keys:
        for n in nameds:
            print("tag", key, "key", n)
            ticks = [t for t in f2[key].time(n)]
            rx = [x for x in f2[key][n, "rx"]]
            ry = [y for y in f2[key][n, "ry"]]

    print(f2.tags()["one"])

    for key in range(len(keys)):
        for n in range(len(nameds)):
            print("tag", key, "key", n)
            ticks = [t for t in f2[key].time(n)]
            rx = [x for x in f2[key][n, "rx"]]
            ry = [y for y in f2[key][n, "ry"]]
            alld = [z for z in f2[key][n]]
    print(alld)

    t0, t1, data = f2["WriteUnified:first blip"].getData(0)
    print(t0, t1, data)
