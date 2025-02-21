#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Apr  7 19:11:09 2022

@author: repa
"""
try:
    from .ddffbase import DDFF, DDFFStream, dprint, vprint
except:
    from ddffbase import DDFF, DDFFStream, dprint, vprint
import json
import numpy as np
import h5py
from functools import partial

# map with common conversions
_typemap = {
    "int32_t": np.int32,
    "int64_t": np.int64,
    "int16_t": np.int16,
    "int8_t": np.int8,
    "uint32_t": np.uint32,
    "uint64_t": np.uint64,
    "uint16_t": np.uint16,
    "uint8_t": np.uint8,
    "float": np.float32,
    "double": np.float64,
    "std::string": h5py.string_dtype(encoding="utf-8"),
    "string8": h5py.string_dtype(encoding="utf-8", length=8),
    "string16": h5py.string_dtype(encoding="utf-8", length=16),
    "string32": h5py.string_dtype(encoding="utf-8", length=32),
    "string64": h5py.string_dtype(encoding="utf-8", length=64),
    "string128": h5py.string_dtype(encoding="utf-8", length=128),
    "smartstring": h5py.string_dtype(encoding="utf-8"),
    "LogString": h5py.string_dtype(encoding="utf-8", length=236),
    "bool": bool,
}


def shapeAndType(count: int, info: dict):
    """ Given type information, return the array shape and numpy/hdf5 type information

    Parameters
    ----------
    count : int
        Array length
    info : dict
        Type information

    Returns
    -------
    dict
        Dictionary with a 'shape' tuple, a 'dtype' type information object, and optionally
        'excluded' for the data members that cannot be packed.

    Raises
    ------
    ValueError
        info dict not correct.
    """
    exd = dict()

    shape = info.get("size", False) and (count, info.get("size")) or (count,)
    if info["type"] == "primitive":
        btype = _typemap.get(info["class"], None)
    elif info["type"] == "object":

        # nested object, run through the members
        excluded = []
        mtypes =[]
        for im, m in enumerate(info["members"]):
            if m["type"] == "primitive":
                if m.get("container", None) is None:
                    mtypes.append((m["name"], _typemap[m["class"]]))
                elif m.get("container") == "array" and m.get("size", None):
                    mtypes.append(
                        (m["name"], _typemap[m["class"]], m["size"])
                    )
                else:
                    excluded.append(im)
        btype = np.dtype(mtypes)
        exd = dict(excluded=excluded)

    elif info["type"] == "enum":
        ebasetype = _typemap.get(info.get("enumint", ""), np.uint32)
        if info.get("enumvalues", False):
            btype = h5py.enum_dtype(info["enumvalues"], basetype=ebasetype)
        else:
            # old recordings, just ints
            btype = ebasetype
    else:
        raise ValueError(f"Wrong info specification {info}")

    ktype = _typemap.get(info.get("key_class", ""), None)
    if ktype:
        dtype = h5py.vlen_dtype(np.dtype([("key", ktype), ("val", dtype)]))
    elif "size" in info:
        dtype = btype
    elif info.get("container", "") == "array":
        dtype = h5py.vlen_dtype(btype)
    else:
        dtype = btype
    dprint(f"{info['name']} shape {shape} from {info['class']} type {dtype}")

    return dict(shape=shape, dtype=dtype, **exd)


class DDFFInventoriedStream:
    """Datastream in an inventory

    Such a datastream is timed or tagged (usually with an integer),
    and has a series of data members all of the same type.
    """

    class TimeIt:
        """Iterator for time or tag values."""

        def __init__(self, ddffs: DDFFStream):
            """Create an iterator for time values on an inventoried stream

            Parameters
            ----------
            ddffs : DDFFStream
                Base data stream
            """
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
        """Iterator for data values"""

        def __init__(self, ddffs: DDFFStream, idx: int | None):
            """Create an iterator for data values on an inventoried stream

            Parameters
            ----------
            ddffs : DDFFStream
                Base data stream
            idx : int
                Data member to return, or None for returning the complete
                data structure
            """
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

    def __init__(self, ddffs: DDFFStream, tag: str, description: str):
        """Create a parsed inventory of stream id's and contents

        Arguments:
            ddffs -- Raw stream
            tag -- Name of the stream, id
            description -- JSON encoded string with a description of the
            stream contents.
        """
        self.base = ddffs
        self.tag = tag
        self.structure = json.loads(description)
        self.klass = self.structure["class"]
        self.members = {}
        for im, m in enumerate(self.structure["members"]):
            self.members[m["name"]] = im

    def time(self):
        """Create an iterator to run through all time values in the
            data stream.

        Returns
        -------
        TimeIt
            Iterator to get all time values.
        """
        return DDFFInventoriedStream.TimeIt(self.base)

    def __getitem__(self, key: int | str | None):
        """Obtain an iterator on the data

        Parameters
        ----------
        key : int|str|None
            Which part of the data to access, None gives whole structure

        Returns
        -------
        ValueIt
            Iterator on values.
        """
        if key is None:
            return DDFFInventoriedStream.ValueIt(self.base, None)
        elif isinstance(key, int):
            return DDFFInventoriedStream.ValueIt(self.base, key)
        else:
            return DDFFInventoriedStream.ValueIt(self.base, self.members[key])

    def __str__(self):
        return f'Object(class="{self.klass}",members={", ".join(self.members.keys())})'

    def getMeta(self, key: int | str | None = None):
        """ Metadata description of the data in this stream

        Parameters
        ----------
        key : int | str | None, optional
            access either metadata for a single member (give by key) or for the whole 
            stream, by default None

        Returns
        -------
        dict
            dictionary with data on the stream or member.
        """
        if isinstance(key, str):
            key = self.members[key]
        if key is None:
            return self.structure["members"]
        return self.structure["members"][key]

    def getData(self, icount=100):
        """ Return data from the stream as a dictionary of numpy arrays

        Parameters
        ----------
        icount : int, optional
            size to initially reserve for the data, by default 100

        Returns
        -------
        (np.array, np.array, dict())
            Time points, time spans, and a dictionary with all object member data in arrays
        """

        time0 = np.zeros(shape=(icount,), dtype=np.uint32)
        time1 = np.zeros(shape=(icount,), dtype=np.uint32)
        result = dict()
        mappers = list()

        for m, midx in self.members.items():
            meminfo = self.getMeta(midx)
            res = shapeAndType(icount, meminfo)

            # create empty default array
            result[m] = np.zeros(**res)

            if 'excluded' in res:
                # object with excluded members
                if meminfo.get("container", "") == "array":
                    if 'size' in meminfo:
                        mappers.append(partial(copyObjectFixedArrayExclude, res=result[m], midx=midx, excluded=excluded))
                    else:
                        mappers.append(partial(copyObjectArrayExclude, res=result[m], midx=midx, excluded=excluded))
                else:
                    mappers.append(partial(copyObjectExclude, res=result[m], midx=midx, excluded=excluded))
            elif meminfo['type'] == 'object':
                # object, all members can be included
                if meminfo.get("container", "") == "array":
                    if 'size' in meminfo:
                        mappers.append(partial(copyObjectFixedArray, res=result[m], midx=midx))
                    else:
                        mappers.append(partial(copyObjectArray, res=result[m], midx=midx))
                else:
                    mappers.append(partial(copyObject, res=result[m], midx=midx))
            elif meminfo['type'] == 'map':
                # map object
                mappers.append(partial(copyMap, res=result[m], midx=midx))
            elif 'container' not in meminfo:
                mappers.append(partial(copyDefault, res=result[m], midx=midx))
            elif 'size' in meminfo:
                mappers.append(partial(copyFixedArray, res=result[m], midx=midx))
            else:
                mappers.append(partial(copyDefault, res=result[m], midx=midx))


        i = -1
        for i, d in enumerate(self.base.reader()):
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

def copyObjectFixedArrayExclude(obj, i, res, midx, excluded):
    # array of tuples
    res[i] = [(x for ix, x in enumerate(obj[midx]) if ix not in excluded)]

def copyObjectArrayExclude(obj, i, res, midx, excluded):
    # nested tuples
    res[i] = tuple((x for ix, x in enumerate(obj[m]) if ix not in excluded))

def copyObjectExclude(obj, i, res, midx, excluded):
    # single tuple
    res[i] = (x for ix, x in enumerate(obj[m]) if ix not in excluded)

def copyObjectFixedArray(obj, i, res, midx):
    # array of tuples
    res[i] = ([(x for x in obj[m])])

def copyObjectArray(obj, i, res, midx):
    # nested tuples
    res[i] = tuple(obj[midx])

def copyObject(obj, i, res, m, midx):
    # single tuple
    res[i] = tuple(obj[midx])

def copyMap(obj, i, res, midx):
    res[i] = obj[midx].items()

def copyFixedArray(obj, i, res, midx):
    res[i] = obj[midx]

def copyDefault(obj, i, res, midx):
    res[i] = obj[midx]



class DDFFInventoried(DDFF):
    """Inventoried file representation.

    The inventory functions as a dictionary with "streams",
    partial data in the file of a specific type. Each stream
    is named, however, you can also access the stream with their
    integer id. Stream 0 for such an inventoried file is by
    definition the inventory itself.

    The dictionary members are data streams of DDFFInventoriedStream
    type. These are tagged with a time or index, and are all of the same
    data type.
    """

    def __init__(self, fname, mode="r", nstreams=1, *args, **kwargs):
        """Open a DDFF datafile with stream inventory

        Arguments:
            fname -- filename to open

        Keyword Arguments:
            mode -- open mode, read or write (default: {'r'})
        """

        # analyse with base DDFF read
        super().__init__(fname, *args, mode=mode, nstreams=nstreams, **kwargs)
        self.mapping = {}

        # complete finding the descriptions stream, & possibly additional ones
        self._scanStreams(nstreams)

        # Read how many data streams are there
        descriptions = [ d for d in self.streams[0].reader() ]

        # also scan these streams
        self._scanStreams(len(descriptions) + nstreams)

        # Use the inventory to enhance the streams
        for tag, streamid, description in descriptions:

            # replace/swap the raw streams with inventoried ones
            self.streams[streamid] = DDFFInventoriedStream(
                self.streams[streamid], tag, description
            )

            self.mapping[tag] = self.streams[streamid]

    def inventory(self):
        """Return the inventory itself, for compatibility purposes

        Returns
        -------
        DDFFInventoried
            Object itself.
        """
        return self

    def time(self, key, period=None):
        """Access time stamps for a specific named stream

        Arguments:
            period -- Optional, ignored
            key -- Name or number of requested stream

        Returns:
            Iterator for time ticks
        """
        if isinstance(key, int):
            return self.streams[self.streams[0][key][1]]
        return self.mapping[key].time()

    def __getitem__(self, keyname):
        """Access data, from a specific named stream

        Arguments:
            keyname -- tuple(streamid,member), tuple(streamid,period,member) or only streamid
            Defines which data stream should be returned. The member string
            indicates which data member to return.

        Returns:
            Iterator for data, either for a single member, or the whole data
            list/struct
        """
        try:
            if isinstance(keyname, str | int):
                key = keyname
                varname = None
            elif len(keyname) == 2:
                key, varname = keyname
            elif len(keyname) == 3:
                key, _, varname = keyname
        except ValueError:
            key = keyname
            varname = None

        if isinstance(key, int):
            stream = self.streams[self.streams[0][key][1]]
        else:
            stream = self.mapping[key]

        # return an iterator on the data (or all), corresponding to
        # this stream
        return stream[varname]

    def stream(self, key: str|int):
        """ Access a specific data stream in the file

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
        return self.mapping[key]

    def keys(self):
        """Return an overview of available named streams

        Returns:
            Iterator of strings
        """
        return [k[0] for k in self.streams[0]]


if __name__ == "__main__":

    stuff = DDFFInventoried("../recordings-PHLAB-new.ddff")

    # known entries
    print(stuff.keys())

    for t in stuff.time("WriteUnified:first blip"):
        print(t)

    for x in stuff["WriteUnified:first blip", "rx"]:
        print(x)
    for x in stuff["WriteUnified:first blip", "ry"]:
        print(x)
