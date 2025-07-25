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
import os
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
    "Dstring": h5py.string_dtype(encoding="utf-8"),  # temporary
    "string8": h5py.string_dtype(encoding="utf-8", length=8),
    "string16": h5py.string_dtype(encoding="utf-8", length=16),
    "string32": h5py.string_dtype(encoding="utf-8", length=32),
    "string64": h5py.string_dtype(encoding="utf-8", length=64),
    "string128": h5py.string_dtype(encoding="utf-8", length=128),
    "smartstring": h5py.string_dtype(encoding="utf-8"),
    "LogString": h5py.string_dtype(encoding="utf-8", length=236),
    "bool": bool,
}


def singleType(info: dict):
    if info["type"] == "primitive":
        return _typemap.get(info["class"], None)
    if info["type"] == "enum":
        ebasetype = _typemap.get(info.get("enumint", ""), np.uint32)
        if info.get("enumvalues", False):
            return h5py.enum_dtype(info["enumvalues"], basetype=ebasetype)
        else:
            # old recordings, just ints
            return ebasetype


def shapeTypeExclude(count: int, info: dict):
    """Given type information, return the array shape and numpy/hdf5 type information

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
    excluded = []

    shape = info.get("size", False) and (count, info.get("size")) or (count,)
    if info["type"] == "primitive":
        btype = _typemap.get(info["class"], None)
    elif info["type"] == "object":

        # nested object, run through the members
        mtypes = []
        for im, m in enumerate(info["members"]):
            if m["type"] in ("primitive", "enum"):
                if m.get("container", None) is None:
                    mtypes.append((m["name"], singleType(m)))
                elif m.get("container") == "array" and m.get("size", None):
                    mtypes.append((m["name"], singleType(m), m["size"]))
                else:
                    excluded.append(im)
        btype = np.dtype(mtypes)

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
        dtype = h5py.vlen_dtype(np.dtype([("key", ktype), ("val", btype)]))
    elif "size" in info:
        dtype = btype
    elif info.get("container", "") == "array":
        dtype = h5py.vlen_dtype(btype)
    else:
        dtype = btype
    dprint(f"{info['name']} shape {shape} from {info['class']} type {dtype}")

    return dict(shape=shape, dtype=dtype), excluded


class Objecter:
    """Helper class, to create structs matching the data content description.
    Typically used for non-time (index) data."""

    def __init__(self, info: dict):
        """Create objecter

        Parameters
        ----------
        info : dict
            Data description
        """
        self.info = info

    def __call__(self, data):
        """Convert data object into a structure matching data type description

        Parameters
        ----------
        data : Typically array with nested data, structs, parsed with msgpack
            Raw parsed data

        Returns
        -------
        struct
            Structure matching the info dict
        """
        res = dict()
        for ix, midx in enumerate(self.info["members"]):
            m = midx["name"]
            if midx.get("container", "") == "":
                if midx["type"] == "primitive":
                    res[m] = data[ix]
                elif midx["type"] == "object":
                    res[m] = Objecter(midx)(data[ix])
                elif midx["type"] == "enum":
                    try:
                        res[m] = midx["enummapper"][data[ix]]
                    except KeyError:
                        midx["enummapper"] = dict(
                            [(k, v) for v, k in midx["enumvalues"].items()]
                        )
                        res[m] = midx["enummapper"][data[ix]]

            if midx.get("container", "") == "array":
                if midx["type"] == "primitive":
                    res[m] = data[ix]
                elif midx["type"] == "object":
                    res[m] = list(map(Objecter(midx), data[ix]))
                elif midx["type"] == "enum":
                    try:
                        res[m] = list(map(midx["enummapper"].get, data[ix]))
                    except KeyError:
                        midx["enummapper"] = dict(
                            [(k, v) for v, k in midx["enumvalues"].items()]
                        )
                        res[m] = list(map(midx["enummapper"].get, data[ix]))
            elif midx.get("container", "") == "map":
                res[m] = dict()
                if midx["type"] == "primitive":
                    for k, v in data[ix].items():
                        res[m][k] = v
                elif midx["type"] == "object":
                    for k, v in data[ix].items():
                        res[m][k] = Objecter(midx)(v)
                elif midx["type"] == "enum":
                    try:
                        for k, v in data[ix].items():
                            res[m][k] = midx["enummapper"][data[ix]]
                    except KeyError:
                        midx["enummapper"] = dict(
                            [(k, v) for v, k in midx["enumvalues"].items()]
                        )
                        for k, v in data[ix].items():
                            res[m][k] = midx["enummapper"][data[ix]]
        return res


class DDFFInventoriedStream:
    """Datastream in an inventory

    Such a datastream is timed or tagged (usually with an integer),
    and has a series of data members all of the same type.
    """

    class BaseIt:
        """Base iterator"""

        def __init__(self, ddffs: DDFFStream):
            self.reader = ddffs.reader()

        def __iter__(self):
            return self

        def __next__(self):
            raise StopIteration

    class TimeIt(BaseIt):
        """Iterator for time or tag values."""

        def __init__(self, ddffs: DDFFStream):
            """Create an iterator for time values on an inventoried stream

            Parameters
            ----------
            ddffs : DDFFStream
                Base data stream
            """
            super().__init__(ddffs)

        def __next__(self):
            """Return next time point

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
            return tmp[0]

    class ValueIt(BaseIt):
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
            super().__init__(ddffs)
            self.idx = idx

        def __next__(self):
            """Return next data point

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
            if self.idx is None:
                return tmp[-1]
            return tmp[-1][self.idx]

    class TimeAndObjectIt(BaseIt):
        """Iterator for data values"""

        def __init__(self, ddffs: DDFFStream, objecter: Objecter):
            """Create an iterator for data values on an inventoried stream

            Parameters
            ----------
            ddffs : DDFFStream
                Base data stream
            idx : int
                Data member to return, or None for returning the complete
                data structure
            """
            super().__init__(ddffs)
            self.objecter = objecter

        def __next__(self):
            """Return next data point

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
            return (tmp[:-1], self.objecter(tmp[-1]))

    def __init__(self, ddffs: DDFFStream, tag: str, description: str):
        """Create a pared inventory of stream ids and content metadata.

        Parameters
        ----------
        ddffs : DDFFStream
            Raw stream data, in msgpack format
        tag : str
            Name of the stream, id
        description : str
            JSON encoded string with a description of the contents
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
        """Metadata description of the data in this stream

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

    def _getMappers(self, icount):
        """Helper to obtain mapping functions from msgpack object to numpy arrays

        Returns
        -------
        tuple(list, mappers)
            Result struct and list of mapping functions
        """
        result = dict()
        mappers = list()

        for m, midx in self.members.items():
            meminfo = self.getMeta(midx)
            res, excluded = shapeTypeExclude(icount, meminfo)

            # create empty default array
            result[m] = np.zeros(**res)

            if excluded:
                # object with excluded members
                if meminfo.get("container", "") == "array":
                    if "size" in meminfo:
                        mappers.append(
                            partial(
                                copyObjectFixedArrayExclude,
                                res=result[m],
                                midx=midx,
                                excluded=excluded,
                            )
                        )
                    else:
                        mappers.append(
                            partial(
                                copyObjectArrayExclude,
                                res=result[m],
                                midx=midx,
                                excluded=excluded,
                            )
                        )
                else:
                    mappers.append(
                        partial(
                            copyObjectExclude,
                            res=result[m],
                            midx=midx,
                            excluded=excluded,
                        )
                    )
            elif meminfo["type"] == "object":
                # object, all members can be included
                if meminfo.get("container", "") == "array":
                    if "size" in meminfo:
                        mappers.append(
                            partial(copyObjectFixedArray, res=result[m], midx=midx)
                        )
                    else:
                        mappers.append(
                            partial(copyObjectArray, res=result[m], midx=midx)
                        )
                else:
                    mappers.append(partial(copyObject, res=result[m], midx=midx))
            elif meminfo["type"] == "map":
                # map object
                mappers.append(partial(copyMap, res=result[m], midx=midx))
            elif "container" not in meminfo:
                mappers.append(partial(copyDefault, res=result[m], midx=midx))
            elif "size" in meminfo:
                mappers.append(partial(copyFixedArray, res=result[m], midx=midx))
            else:
                mappers.append(partial(copyDefault, res=result[m], midx=midx))
        return result, mappers

    def getData(self, icount=100):
        """Return data from the stream as a dictionary of numpy arrays

        For "numeric" data, this way of obtaining the data is often much more
        efficient than iterating over the stream, which returns a sequence of objects with
        the data.
        This returns a dictionary of objects with numpy arrays, which can then directly
        be used in plotting, etc.

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
        result, mappers = self._getMappers(icount)

        i = -1
        for i, d in enumerate(self.base.reader()):
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

    def items(self):
        """Create an iterator on the stream objects, returns time,object tuples.

        Use this to read a typically limited number of items, returned as
        python struct, and following the definition of data in the inventory.
        This is less efficient than using the getData function.

        Returns
        -------
        Iterator on tuple(int, object)
            Runs through data. Each iteration returns the next data object as
            a dictionary.
        """
        return DDFFInventoriedStream.TimeAndObjectIt(
            self.base, Objecter(self.structure)
        )


# support routines, extracting different types of objects from
# data structures
def copyObjectFixedArrayExclude(obj, i, res, midx, excluded):
    # array of tuples
    res[i] = [(x for ix, x in enumerate(obj[midx]) if ix not in excluded)]


def copyObjectArrayExclude(obj, i, res, midx, excluded):
    # nested tuples
    res[i] = tuple((x for ix, x in enumerate(obj[midx]) if ix not in excluded))


def copyObjectExclude(obj, i, res, midx, excluded):
    # single tuple
    res[i] = (x for ix, x in enumerate(obj[midx]) if ix not in excluded)


def copyObjectFixedArray(obj, i, res, midx):
    # array of tuples
    res[i] = [tuple(x) for x in obj[midx]]


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
    is named, however, you can also access the streams with their
    integer id. Stream 0 for such an inventoried file is by
    definition the inventory itself.

    The dictionary members are data streams of DDFFInventoriedStream
    type. These are tagged with a time or index, and are all of the same
    data type.
    """

    def __init__(self, fname: str, *args, mode="r", nstreams=frozenset((0,)), **kwargs):
        """Open or create an inventoried DDFF file

        Parameters
        ----------
        fname : str
            File name.
        mode : str, optional
            Reading or writing mode, by default "r".
        nstreams : iterable with integers, optional
            Streams to fully scan, by default frozenset((0,)), the stream with the inventory
        """

        # analyse with base DDFF read, scans my requested base streams
        super().__init__(fname, *args, mode=mode, nstreams=nstreams, **kwargs)
        self.mapping = {}

        # read the inventory into the stream as list
        self.streams[0].readToList()

        if len(nstreams) == 1:
            self._doInitialScan()

    def _doInitialScan(self, offsets=None):

        # Read how many data streams are there
        descriptions = self.streams[0]
        neededstreams = frozenset((d[1] for d in descriptions))

        # find the initial blocks for streams with data
        if offsets:
            self._initStreams(neededstreams, offsets)
        else:
            self._scanStreams(neededstreams)

        # Use the inventory to enhance the streams
        for tag, streamid, description in descriptions:

            try:
                # replace/swap the raw streams with inventoried ones
                self.streams[streamid] = DDFFInventoriedStream(
                    self.streams[streamid], tag, description
                )
                self.mapping[tag] = self.streams[streamid]
            except KeyError:
                print(f"Cannot find data for stream {streamid}/{tag}, create empty")
                base = DDFFStream(self.file, stream_id=streamid)
                self.streams[streamid] = DDFFInventoriedStream(base, tag, description)

    def __getitem__(self, key: str | int):
        """Access a specific data stream in the file

        Parameters
        ----------
        key : str|int
            Stream id or number to access.

        Returns
        -------
        DDFFInventoriedStream
            Data access.
        """
        if isinstance(key, int):
            return self.streams[self.streams[0][key][1]]
        return self.mapping[key]

    def keys(self):
        """Return an overview of available named streams.

        Returns:
            Iterator of strings
        """
        return [k[0] for k in self.streams[0]]

    def __len__(self):
        """Return number of data streams.

        Returns:
            Integer
        """
        return len(self.streams[0])


if __name__ == "__main__":

    stuff = DDFFInventoried(
        os.path.dirname(__file__) + "/../../../test/ddff/recordings-PHLAB-new.ddff"
    )

    # known entries
    print(stuff.keys())

    for t in stuff["WriteUnified:first blip"].time():
        print(t)

    for x in stuff["WriteUnified:first blip"]["rx"]:
        print(x)
    for x in stuff["WriteUnified:first blip"]["ry"]:
        print(x)

    t0, t1, data = stuff["WriteUnified:first blip"].getData()
    print(t0, t1, data)
