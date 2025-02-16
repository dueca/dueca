#!@Python_EXECUTABLE@
# -*-python-*-
"""     item            : ddff-convert
        made by         : RvP
        date            : 2023
        category        : python program
        description     : Conversion of ddff data
        language        : python
        changes         :
        copyright       : 2023 Rene van Paassen
        license         : EUPL-1.2
"""
try:
    from pyddff import DDFFTagged, DDFFInventoried
except ModuleNotFoundError:
    # debug/test?
    from src.ddffinventoried import DDFFInventoried
    from src.ddfftagged import DDFFTagged
import numpy as np
import h5py
import argparse
import os
import sys
from functools import partial

helptext = """
Conversion script for ddff files
"""

__verbose = False


def vprint(*args, **kwargs):
    if __verbose:
        print(*args, **kwargs)


parser = argparse.ArgumentParser(description="Convert or inspect DDFF data")
parser.add_argument(
    "--verbose", action="store_true", help="Verbose run with information output"
)
subparsers = parser.add_subparsers(help="commands", title="commands")


class Info:
    """List properties of DDFF files"""

    command = "info"

    @classmethod
    def args(cls, subparsers):
        parser = subparsers.add_parser(cls.command, help="Inspect a ddff file")
        parser.add_argument(
            "--period", type=str, help="Inspect a specific recording period"
        )
        parser.add_argument(
            "--inventory",
            action="store_true",
            help="Inventory-only, no time period inspection",
        )
        parser.add_argument("--streamid", type=str, help="Inspect a specific stream")
        parser.add_argument("filename", type=str, help="File name to be analysed")
        parser.set_defaults(handler=Info)

    def __call__(self, ns: argparse.Namespace):

        # open the file as tagged
        try:
            if ns.inventory:
                f = DDFFInventoried(ns.filename)
            else:
                f = DDFFTagged(ns.filename)
        except Exception as e:
            print(f"Cannot open file {ns.filename}, error {e}", file=sys.stderr)
            sys.exit(-1)

        if ns.inventory and ns.period:
            print(
                f"--inventory and --period options are not compatible", file=sys.stderr
            )
            sys.exit(-1)

        if ns.period:
            try:
                print(f"Details for period {ns.period}:\n", str(f.index()[ns.period]))
            except KeyError:
                print(f"Cannot find period {ns.period}", file=sys.stderr)
            except Exception as e:
                print(f"Cannot read index, error {e}")

        if ns.streamid:
            try:
                print(
                    f"Details for stream {ns.streamid}\n", str(f.mapping[ns.streamid])
                )
            except KeyError:
                print(f"Cannot find stream {ns.streamid}", file=sys.stderr)
            except Exception as e:
                print(f"Cannot read streamid, error {e}", file=sys.stderr)

        if not ns.streamid and not ns.period:
            try:
                if not ns.inventory:
                    print('Available periods:\n"', '", "'.join(f.keys()), '"', sep="")
                print(
                    'Available streams:\n"',
                    '", "'.join(f.inventory().keys()),
                    '"',
                    sep="",
                )
            except Exception as e:
                print(f"Cannot read periods/inventory, error {e}", file=sys.stderr)


Info.args(subparsers)


def doExclude(x: list, idxes: list):
    for i in reversed(idxes):
        del x[i]
    return tuple(x)


class ToHdf5:

    command = "hdf5"

    typemap = {
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

    @classmethod
    def shapeAndType(cls, count, info):
        exd = dict()

        shape = info.get("size", False) and (count, info.get("size")) or (count,)
        if info["type"] == "primitive":
            btype = cls.typemap.get(info["class"], None)
        elif info["type"] == "object":

            # nested object, run through the members
            excluded = []
            mtypes =[]
            for im, m in enumerate(info["members"]):
                if m["type"] == "primitive":
                    if m.get("container", None) is None:
                        mtypes.append((m["name"], ToHdf5.typemap[m["class"]]))
                    elif m.get("container") == "array" and m.get("size", None):
                        mtypes.append(
                            (m["name"], ToHdf5.typemap[m["class"]], m["size"])
                        )
                    else:
                        excluded.append(im)
            btype = np.dtype(mtypes)
            exd = dict(excluded=excluded)

        elif info["type"] == "enum":
            ebasetype = ToHdf5.typemap.get(info.get("enumint", ""), np.uint32)
            if info.get("enumvalues", False):
                btype = h5py.enum_dtype(info["enumvalues"], basetype=ebasetype)
            else:
                # old recordings, just ints
                btype = ebasetype
        else:
            raise ValueError(f"Wrong info specification {info}")

        ktype = cls.typemap.get(info.get("key_class", ""), None)
        if ktype:
            dtype = h5py.vlen_dtype(np.dtype([("key", ktype), ("val", dtype)]))
        elif "size" in info:
            dtype = btype
        elif info.get("container", "") == "array":
            dtype = h5py.vlen_dtype(btype)
        else:
            dtype = btype
        vprint(f"{info['name']} shape {shape} from {info['class']} type {dtype}")

        return dict(shape=shape, dtype=dtype, **exd)

    @classmethod
    def args(cls, subparsers):
        parser = subparsers.add_parser(cls.command, help="Convert a ddff file to hdf5")
        parser.add_argument(
            "--period", type=str, help="Convert a specific recording period"
        )
        parser.add_argument(
            "--compress",
            type=str,
            choices=("gzip", "lzf", ""),
            default="",
            help="Specify a compression method",
        )
        parser.add_argument(
            "--streamids", nargs="+", default=[], help="Convert specific stream(s)"
        )
        parser.add_argument(
            "--outfile",
            type=str,
            default="",
            help="Filename for output file, if not specified, created from\n"
            "the input filename",
        )
        parser.add_argument("filename", type=str, help="File name to be analysed")
        parser.set_defaults(handler=ToHdf5)

    def __call__(self, ns: argparse.Namespace):

        if ns.compress:
            compressargs = {"compression": ns.compress}
        else:
            compressargs = {}

        # open the file
        try:
            if not ns.period:
                f = DDFFInventoried(ns.filename)
            else:
                f = DDFFTagged(ns.filename)
        except Exception as e:
            print(f"Cannot open file {ns.filename}, error {e}", file=sys.stderr)
            sys.exit(-1)
        vprint("Opened file", ns.filename)

        # either all stream id's, or just the selected ones
        if not ns.streamids:
            ns.streamids = [i for i in f.keys()]

        # hdf5 file name
        if not ns.outfile:
            if ns.filename.endswith(".ddff"):
                ns.outfile = os.path.basename(ns.filename[:-4] + "hdf5")
            else:
                ns.outfile = os.path.basename(ns.filename + ".hdf5")
            vprint("output file", ns.outfile)

        # create the file
        hf = h5py.File(ns.outfile, "w")

        for streamid in ns.streamids:
            vprint("Processing stream", streamid)
            gg = hf.create_group(streamid)
            dg = gg.create_group("data")

            # first the matching time
            d = np.fromiter(f.time(streamid, ns.period), dtype=np.uint64)
            count = d.shape[0]
            gg.create_dataset("tick", data=d, **compressargs)

            for m, im in f.mapping[streamid].members.items():

                try:
                    info = f.stream(streamid).getMeta(im)

                    if info["type"] == "object":

                        vprint("processing member object", m)

                        res = self.shapeAndType(count, info)
                        shape, dtype, excluded = res['shape'], res['dtype'], res['excluded']
                        if len(excluded) == info["members"]:
                            print("Cannot nest-code member", m)
                            continue

                        # fixed size dataset array
                        _d = np.zeros(shape, dtype)
                        fxit = partial(doExclude, idxes=excluded)

                        if info.get("size", None):

                            # array size, data will be lists of lists of data
                            for i, x in enumerate(f[streamid, ns.period, im]):
                                _d[i] = [fxit(_x) for _x in x]

                        elif info.get("container", "") == "array":

                            # variable size array, don't know if this works
                            for i, x in enumerate(f[streamid, ns.period, im]):
                                _d[i] = tuple((fxit(_x) for _x in x))

                        else:

                            # single object member
                            for i, x in enumerate(f[streamid, ns.period, im]):
                                _d[1] = fxit(x)

                        # with that, create the dataset
                        d = dg.create_dataset(m, data=_d, **compressargs)
                        continue

                    if info.get("container", None) == "map":

                        vprint("processing member map", m)
                        d = dg.create_dataset(
                            m, **self.shapeAndType(count, info), **compressargs
                        )
                        for i, x in enumerate(f[streamid, ns.period, im]):
                            d[i] = x.items()  # maybe it is an object in the msgpack?
                        continue

                    # though numpy array iteration for non-complex members, fixed
                    # size arrays and straight
                    if not info.get("container", False):
                        vprint("quick processing default member", m)
                        _d = np.fromiter(
                            f[streamid, ns.period, im],
                            dtype=self.shapeAndType(count, info)["dtype"],
                            count=count,
                        )
                        d = dg.create_dataset(m, data=_d, **compressargs)
                        continue

                    elif info.get("size", False):
                        vprint("processing fixed-size member", m)
                        _d = np.zeros(**self.shapeAndType(count, info))
                        for i, x in enumerate(f[streamid, ns.period, im]):
                            _d[i, :] = x
                        d = dg.create_dataset(m, data=_d, **compressargs)
                        continue

                    # otherwise straight up?
                    vprint("processing default member", m)
                    d = dg.create_dataset(
                        m, **self.shapeAndType(count, info), **compressargs
                    )
                    for i, x in enumerate(f[streamid, ns.period, im]):
                        d[i] = x

                except Exception as e:
                    print(
                        f"Cannot convert data {streamid}, {m} with {info}, to numpy array, problem {e}"
                    )

        hf.close()


ToHdf5.args(subparsers)

if __name__ == "__main__":

    # parse arguments
    pres = parser.parse_args(sys.argv[1:])

    # verbose output, todo
    if pres.verbose:
        __verbose = True

    # extract the handler
    try:
        hclass = pres.handler
    except AttributeError:
        parser.print_usage()
        sys.exit(-1)

    # create and run the handler
    handler = hclass()
    handler(pres)
