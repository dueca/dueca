#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Apr  6 15:57:26 2022

@author: repa
"""

import io
import os
import struct
import crcmod
import msgpack

# The crc function used to check the blocks
crc16 = crcmod.predefined.mkPredefinedCrcFun("crc-ccitt-false")

__verbose = 0


def dprint(*args, **kwargs):
    """Debug print function

    When activated, prints all kinds of debug messages
    """
    if __verbose >= 2:
        print(*args, **kwargs)


def vprint(*args, **kwargs):
    """Debug print function

    When activated, prints all kinds of debug messages
    """
    if __verbose >= 1:
        print(*args, **kwargs)


class DDFFBuffer(io.BytesIO):
    """Buffer back-end, reading and writing a base DDFF structured file"""

    def __init__(self, streamid, offset, block_size, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.block_size = block_size
        self.streamid = streamid
        self.blockcount = 0
        self.offset = offset
        self.object_offset = 0

    def write(self, *argc, **argv):
        dprint("buffer write")
        if self.object_offset == 0:
            self.object_offset = (self.getbuffer().nbytes) % (self.block_size - 28) + 28
        super().write(*argc, **argv)

    def size(self):
        return self.getbuffer().nbytes - self.blockcount * (self.block_size - 28)

    def getblock(self, flush=False):

        while self.size() >= self.block_size or (flush and (self.size() > 0)):

            if flush:
                nzero = self.block_size - self.size() - 28
                if nzero > 0:
                    self.write(b"\0" * nzero)
                    fill = self.block_size - nzero
                else:
                    fill = self.block_size
            else:
                fill = self.block_size

            crc = 0
            if self.size() > self.block_size:
                nextoffset = self.offset + self.block_size
            else:
                nextoffset = -1
            header1 = struct.pack(
                ">HIIII",
                self.streamid,
                self.block_size,
                fill,
                self.object_offset,
                self.blockcount,
            )
            o = self.blockcount * (self.block_size - 28)
            crc = crc16(header1 + self.getbuffer()[o : o + self.block_size - 28])
            header0 = struct.pack(">qH", nextoffset, crc)

            dprint("block at", self.offset, "size", fill)
            yield header0 + header1 + self.getbuffer()[o : o + self.block_size - 28]
            self.offset += self.block_size
            self.object_offset = 0
            self.blockcount += 1


class DDFFBlock:
    """Single block in a DDFF file; reads the header, decodes it, and also reads the tail
    with data.
    """

    def __init__(self, f):
        """Create a DDFF data block from a file

        Parameters
        ----------
        f : file
            File to read from. The current file pointer is used and recorded

        Raises
        ------
        ValueError
            On end-of-file
        ValueError
            On a failure of the CRC
        """
        self.offset = f.tell()
        header = f.read(28)
        if not len(header):
            raise ValueError("File ended")
        (
            self.next_offset,
            crc,
            self.stream_id,
            self.block_size,
            self.block_fill,
            self.object_offset,
            self.block_num,
        ) = struct.unpack(">qHHIIII", header)
        if self.block_num == 0 and self.block_fill and self.object_offset == 0:
            dprint("Correcting missing object offset first block")
            self.object_offset = 28
        dprint(
            f"Block at {self.offset}, stream {self.stream_id}, fill {self.block_fill}"
            f", size {self.block_size}, first object at {self.object_offset}, "
            f"next at {self.next_offset}, block #{self.block_num}"
        )
        if self.block_size:
            self.tail = f.read(self.block_size - 28)
        else:
            self.tail = b""
        if crc != crc16(header[10:] + self.tail):
            raise ValueError("CRC failure in reading ddff block")
        if self.block_size > self.block_fill:
            self.tail = self.tail[: self.block_fill - 28]


class DDFFStream(list):
    """Object representing the data in a DDFF data stream.

    Data (objects) are packed with the msgpack protocol.
    """

    def __init__(
        self,
        file,
        block: DDFFBlock = None,
        stream_id: int | None = None,
        block_size: int = 4096,
    ):
        """Single data stream in a DDFF data file

        Parameters
        ----------
        file : Open file descriptor
            The file descriptor
        block : DDFFBlock, optional
            If given, the stream exists, and the block is the starting block
            of this stream in the current file
        stream_id : int, optional
            If given, the stream is new
        block_size : int
            Determines block size for new streams

        Raises
        ------
        ValueError
            Either block or stream id should be given
        """

        if block is not None:
            self.block0 = block
        elif stream_id is not None:
            self.stream_id = stream_id
        else:
            raise ValueError("Need block or stream id")

        self.file = file
        super().__init__()

        # if not for an existing stream, finished here
        if block is None:
            self.block_size = block_size
            return

        # extract information on the existing stream
        self.stream_id = block.stream_id
        self.block_size = block.block_size

    def reader(self, block0=None):
        """Create a stream reader, which can iterate over the stream's contents

        Returns
        -------
        DDFFReadStream
            Iterable object, returning the decoded stream contents.
        """
        if block0 is None:
            return DDFFReadStream(self.block0, self.file)
        return DDFFReadStream(block0, self.file)

    def readToList(self):
        """Load any data from the file into memory.

        The stream object will function as a list with the loaded data
        """
        for x in self.reader():
            self.append(x)

    def write(self, fd):
        """Write memory data to file

        Parameters
        ----------
        fd : File
            File to write the data to. Its current pointer should be set
        block_size : int, optional
            Size of data blocks to write, by default 4096
        """

        # create a buffer for writing the blocks
        buf = DDFFBuffer(self.stream_id, fd.tell(), self.block_size)

        # run through all objects in the list
        for obj in self:

            msgpack.pack(obj, buf)

            # if this fills one or more blocks, write them out
            for blk in buf.getblock():
                fd.write(blk)

        # write remaining, if applicable
        for blk in buf.getblock(True):
            fd.write(blk)


class DDFFReadStream:
    """Iterator to load data from a file stream

    Returns
    -------
    Any type
        Objects unpacked from msgpack data,

    Raises
    ------
    StopIteration
        After all objects are read
    """

    # buffer size for loading data.
    maxobjectsize = 1024 * 1024

    def __init__(self, block: DDFFBlock, file):
        """Create a stream reader

        Parameters
        ----------
        block : DDFFBlock
            Initial/starting block of the data in the file
        file : BytesIO
            File with further data
        """
        self.block = block
        self.block_size = block.block_size
        self.loaded = block.block_fill
        self.file = file
        self.unpacker = msgpack.Unpacker()
        self.unpacked = []
        self.maxobjectsize = DDFFReadStream.maxobjectsize

    def _topup(self):
        """Feeds data blocks from the file.

            Get data blocks until the unpacker buffer has reached
            the given (1Mb fill size) or the file is exchausted.

        Returns
        -------
        bool
            True if any data was added to the unpacker, False if not.
        """
        topped = False
        while self.block and self.loaded - self.unpacker.tell() < self.maxobjectsize:
            self.unpacker.feed(self.block.tail)
            topped = True
            if self.block.next_offset not in (0x7FFFFFFFFFFFFFFF, -1):
                self.file.seek(self.block.next_offset)
                self.block = DDFFBlock(self.file)
                self.loaded += self.block.block_fill - 28
            else:
                self.block = None
        return topped

    def __iter__(self):
        """Start an iteration

        Returns
        -------
        DDFFReadStream
            Iter object
        """
        return self

    def __next__(self):
        """Obtain the next object from the file/block

        Returns
        -------
        Any
            Object read with msgpack from file

        Raises
        ------
        StopIteration
            No more data
        """
        if self.unpacked:
            return self.unpacked.pop(0)

        while not self.unpacked:
            try:
                # transfer from file to unpacker
                if not self._topup():
                    raise StopIteration

                # fill the unpacked again
                for o in self.unpacker:
                    self.unpacked.append(o)

            except msgpack.OutOfData:
                self.maxobjectsize *= 2
            except msgpack.ExtraData:
                pass
        if self.unpacked:
            return self.unpacked.pop(0)
        raise StopIteration


class DDFF:
    """DelftDataFormat File class

    ## General

    This handles a file in the DDFF format. This format contains multiple,
    numbered, data stream. The DDFF format is intended for real-time
    logging, and its structure is adapted to that.

    A single stream in the file will consist of equally-sized blocks,
    with additional information to interpret these like a linked list.
    This class can both read an existing file, and expose the different
    data streams in a streamio fashion, create a new file in which
    streams may be created, and extend existing files.

    Commonly this class serves as a base class for DDFFInventoried class,
    which connect the stream numbers with names, and add information on
    the data objects within the stream (in that case stream #0 is used
    for the inventory), or for the DDFFSegmented class, which adds
    segments/period information (in which case stream #0 is used
    for the segments/periods)

    ## File format

    Block sizes are a multiple of a base block size, default 4096
    bytes, and for best performance chosen to match (be a multiple of)
    physical device block size.

    Each block has a 28-byte header, indicating next block location,
    block size, fill level, block number, stream id, checksum,
    etc. For the precise format see the description in
    ControlBlock.hxx. The remainder of the block contains data.

    """

    def __init__(
        self, fname, mode: str = "r", block_size: int = 128, nstreams: set | None = None
    ):
        """Access or create a DDFF encoded data file.

        Arguments:
            fname -- file name

        Keyword Arguments:
            mode -- read/write mode (default: {'r'})
            block_size -- default size of data blocks in the file (default: {128})
            nstreams -- streams to pre-load (i.e., find their initial block
                        in the file). If None, and the file exists, the entire
                        file will be scanned, which may be slow
        """

        self.file = open(fname, mode + "b")
        self.streams = dict()
        self.scanpoint = 0
        self._scanStreams(nstreams)
        self.block_size = block_size

    def _scanStreams(self, neededstreams: set | None = None):
        """Internal method to parse data and create streams

        Parameters
        ----------
        nstreams : set|None
            If given, stop scanning after the requested streams are found,
            otherwise read full file to find all streams
        """

        # reset file to zero position
        vprint(f"_scanStreams searching {neededstreams} from {self.scanpoint}")
        self.file.seek(self.scanpoint)
        try:
            while self.file:
                hdr = DDFFBlock(self.file)
                if hdr.stream_id not in self.streams:
                    vprint(
                        "Found data stream",
                        hdr.stream_id,
                        "offset",
                        self.file.tell() - hdr.block_size,
                    )
                    self.streams[hdr.stream_id] = DDFFStream(block=hdr, file=self.file)
                    if (
                        neededstreams is not None
                        and neededstreams <= self.streams.keys()
                    ):
                        self.scanpoint = self.file.tell()
                        return
        except ValueError:
            pass
        self.scanpoint = self.file.tell()

    def _initStreams(self, neededstreams: set, offsets: list | tuple):
        """Improved method to find data streams

            If offset positions of streams are known (typically from ddfftagged file
            and reading the tags), create streams by loading the initial blocks
            at these offset positions.

        Parameters
        ----------
        neededstreams : set
            Stream id's required
        offsets : iterable with file offsets
            Offset locations where the streams start.

        Raises
        ------
        ValueError
            When an offset position is false, and there is no stream starting block
            there
        ValueError
            When not all required streams are found
        """
        vprint(f"_initStreams searching {neededstreams} at {[o for o in offsets]}")
        for o in offsets:
            self.file.seek(o)
            hdr = DDFFBlock(self.file)
            if hdr.block_num != 0:
                raise ValueError(f"_initStreams fails, no new block at {o}")
            if hdr.stream_id not in self.streams:
                self.streams[hdr.stream_id] = DDFFStream(block=hdr, file=self.file)

        if not neededstreams <= self.streams.keys():
            raise ValueError(
                "Cannot find streams", neededstreams.difference(self.streams.keys())
            )

    def createStream(self, block_size=None):
        """Create a new DDFF stream

        Raises:
            ValueError: exception to indicate the previous/read file is not
            correct

        Returns:
            DDFFStream: The new data stream
        """
        if len(self.streams) in self.streams:
            raise ValueError("DDFF streams not contiguous")
        newid = len(self.streams)
        self.streams[newid] = DDFFStream(
            stream_id=newid, file=self.file, block_size=block_size or self.block_size
        )
        return self.streams[newid]

    def write(self, block_size: int = 0) -> None:
        """Write current streams from memory back to file space

        Keyword Arguments:
            block_size -- Size of blocks (default: {0})
        """
        # this writes the streams in order, not randomly scattered like
        # in real-time writing (since we cannot know the timing)
        for ids in sorted(self.streams.keys()):
            self.streams[ids].write(self.file, block_size or self.block_size)
        self.file.flush()


if __name__ == "__main__":

    prj = "DuecaTestCommunication"
    recdata = DDFF(
        os.path.dirname(__file__) + "/../../../test/ddff/recordings-PHLAB-new.ddff",
        mode="r",
    )
    print(recdata.streams[0], recdata.streams[1])

    test = "123456789".encode("ascii")
    print(f"test crc, 0x{crc16(test):4x} should be 0x29b1")

    stuff = DDFF("test.ddff", mode="w")
    st0 = stuff.createStream()
    st0.append((10, ["a"]))
    st0.append((20, ["b"]))
    stuff.write()

    del stuff

    verif = DDFF("test.ddff", mode="r")
    verif.streams[0].readToList()
    print(verif.streams)

    stuff2 = DDFF("test2.ddff", mode="w", block_size=128)
    st0 = stuff2.createStream()
    for i in range(100):
        st0.append((i, f"data point {i}"))
    st1 = stuff2.createStream()
    st1.append((10, ["a"]))
    st1.append((20, ["b"]))

    stuff2.write()

    verif2 = DDFF("test2.ddff", mode="r")
    verif2.streams[0].readToList()
    print(st0 == verif2.streams[0])
    print(verif2.streams[0])
