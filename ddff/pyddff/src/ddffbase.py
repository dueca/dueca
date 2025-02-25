#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Apr  6 15:57:26 2022

@author: repa
"""

import struct
import crcmod
import msgpack
import io
import os

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

    def __init__(self, streamid, offset, blocksize, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.blocksize = blocksize
        self.streamid = streamid
        self.blockcount = 0
        self.offset = offset
        self.object_offset = 0

    def write(self, *argc, **argv):
        dprint("buffer write")
        if self.object_offset == 0:
            self.object_offset = (self.getbuffer().nbytes) % (self.blocksize - 28) + 28
        super().write(*argc, **argv)

    def size(self):
        return self.getbuffer().nbytes - self.blockcount * (self.blocksize - 28)

    def getblock(self, flush=False):

        while self.size() >= self.blocksize or (flush and (self.size() > 0)):

            if flush:
                nzero = self.blocksize - self.size() - 28
                if nzero > 0:
                    self.write(b"\0" * nzero)
                    fill = self.blocksize - nzero
                else:
                    fill = self.blocksize
            else:
                fill = self.blocksize

            crc = 0
            if self.size() > self.blocksize:
                nextoffset = self.offset + self.blocksize
            else:
                nextoffset = -1
            header1 = struct.pack(
                ">HIIII",
                self.streamid,
                self.blocksize,
                fill,
                self.object_offset,
                self.blockcount,
            )
            o = self.blockcount * (self.blocksize - 28)
            crc = crc16(header1 + self.getbuffer()[o : o + self.blocksize - 28])
            header0 = struct.pack(">qH", nextoffset, crc)

            dprint("block at", self.offset, "size", fill)
            yield header0 + header1 + self.getbuffer()[o : o + self.blocksize - 28]
            self.offset += self.blocksize
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
        offset = f.tell()
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
        dprint(
            f"Block at {offset}, stream {self.stream_id}, fill {self.block_fill}"
            f", size {self.block_size}, first object at {self.object_offset}, "
            f"next at {self.next_offset}, block #{self.block_num}"
        )
        if self.block_size:
            self.tail = f.read(self.block_size - 28)
        else:
            self.tail = b''
        dprint("crc", crc, "from data", crc16(header[10:] + self.tail))
        if crc != crc16(header[10:] + self.tail):
            raise ValueError("CRC failure in reading ddff block")
        if self.block_size > self.block_fill:
            self.tail = self.tail[: self.block_fill - 28]


class DDFFStream(list):

    def __init__(self, file, *args, block=None, stream_id=None, **kwargs):
        """Single stream in a DDFF data file, kept as list in memory"""

        if block is not None:
            self.block0 = block
        elif stream_id is not None:
            self.stream_id = stream_id
        else:
            raise ValueError("Need block or stream id")

        self.file = file
        super().__init__(*args, **kwargs)

        # if not for an existing stream, finished here
        if block is None:
            return

        # extract information on the existing stream
        self.stream_id = block.stream_id

    def reader(self):
        """Create a stream reader, which can iterate over the stream's contents

        Returns
        -------
        DDFFReadStream
            Iterable object, returning the decoded stream contents.
        """
        return DDFFReadStream(self.block0, self.file)

    def readToList(self):
        for x in self.reader():
            self.append(x)

    def write(self, fd, blocksize=4096):
        """Write memory data to file

        Parameters
        ----------
        fd : File
            File to write the data to. Its current pointer should be set
        blocksize : int, optional
            Size of data blocks to write, by default 4096
        """

        # create a buffer for writing the blocks
        buf = DDFFBuffer(self.stream_id, fd.tell(), blocksize)

        # create a packer

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

    # buffer size for loading data.
    maxobjectsize = 1024 * 1024

    def __init__(self, block: DDFFBlock, file):
        self.block = block
        self.loaded = block.block_fill
        self.file = file
        self.unpacker = msgpack.Unpacker()
        self.unpacked = []
        self.maxobjectsize = DDFFReadStream.maxobjectsize

    def topup(self):
        """Feeds data blocks from the file until the unpacker buffer has reached
            the given (1Mb fill size) or the file is exchausted

        Returns
        -------
        bool
            True if any data was added to the unpacker, False if not.
        """
        topped = False
        while self.block and self.loaded - self.unpacker.tell() < self.maxobjectsize:
            self.unpacker.feed(self.block.tail)
            topped = True
            if self.block.next_offset != 0x7FFFFFFFFFFFFFFF:
                self.file.seek(self.block.next_offset)
                self.block = DDFFBlock(self.file)
                self.loaded += self.block.block_fill - 28
            else:
                self.block = None
        return topped

    def __iter__(self):
        return self

    def __next__(self):
        """Extract data objects from the stream"""
        if self.unpacked:
            return self.unpacked.pop(0)

        while not self.unpacked:
            try:
                # transfer from file to unpacker
                if not self.topup():
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

    def __init__(self, fname, mode="r", blocksize=128, nstreams=None):
        """Access a DDFF encoded data file

        Arguments:
            fname -- file name

        Keyword Arguments:
            mode -- read/write mode (default: {'r'})
            blocksize -- size of data blocks in the file (default: {128})
        """

        self.file = open(fname, mode + "b")
        self.streams = dict()
        self.scanpoint = 0
        self._scanStreams(nstreams)
        self.blocksize = blocksize

    def _scanStreams(self, neededstreams: set | None):
        """Internal method to parse data and create streams

        Parameters
        ----------
        nstreams : int|None
            If given, stop scanning after the requested number of streams,
            otherwise read full file to find all streams
        """

        # reset file to zero position
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
                        return
        except ValueError:
            pass
        self.scanpoint = self.file.tell()

    def _initStreams(self, neededstreams: set, offsets):
        for o in offsets:
            self.file.seek(o)
            hdr = DDFFBlock(self.file)
            if hdr.stream_id in self.streams or hdr.block_num != 0:
                raise ValueError(f"Init stream failse, no new block at {o}")
            self.streams[hdr.stream_id] = DDFFStream(block=hdr, file=self.file)

        if not neededstreams <= self.streams.keys():
            raise ValueError(
                "Cannot find streams", neededstreams.difference(self.streams.keys())
            )

    def createStream(self):
        """Create a new DDFF stream

        Raises:
            ValueError: exception to indicate the previous/read file is not
            correct

        Returns:
            The new data stream
        """
        if len(self.streams) in self.streams:
            raise ValueError("DDFF streams not contiguous")
        newid = len(self.streams)
        self.streams[newid] = DDFFStream(stream_id=newid, file=self.file)
        return self.streams[newid]

    def write(self, blocksize: int = 0) -> None:
        """Write current streams from memory back to file space

        Keyword Arguments:
            blocksize -- Size of blocks (default: {0})
        """
        # this writes the streams in order, not randomly scattered like
        # in real-time writing (since we cannot know the timing)
        for ids in sorted(self.streams.keys()):
            self.streams[ids].write(self.file, blocksize or self.blocksize)
        self.file.flush()


if __name__ == "__main__":

    prj = "DuecaTestCommunication"
    recdata = DDFF("../recordings-PHLAB-new.ddff", mode="r")
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
    print(verif.streams)

    stuff2 = DDFF("test2.ddff", mode="w", blocksize=128)
    st0 = stuff2.createStream()
    for i in range(100):
        st0.append((i, f"data point {i}"))
    st1 = stuff2.createStream()
    st1.append((10, ["a"]))
    st1.append((20, ["b"]))

    stuff2.write()

    verif2 = DDFF("test2.ddff", mode="r")
    print(st0 == verif2.streams[0])
    print(verif2.streams[0])
