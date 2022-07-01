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
crc16 = crcmod.predefined.mkPredefinedCrcFun('crc-ccitt-false')

def dprint(*args, **kwargs):
    print(*args, **kwargs)


class DDFFBuffer(io.BytesIO):

    def __init__(self, streamid, offset, blocksize):
        self.blocksize = blocksize
        self.streamid = streamid
        self.blockcount = 0
        self.offset = offset
        self.object_offset = 0

    def write(self, *argc, **argv):
        dprint("buffer write")
        if self.object_offset == 0:
            self.object_offset = \
                (self.getbuffer().nbytes) % (self.blocksize - 28) + 28
        super().write(*argc, **argv)
        
    def size(self):
        return self.getbuffer().nbytes - \
            self.blockcount * (self.blocksize - 28)
        
    def getblock(self, flush=False):

        while self.size() >= self.blocksize or \
            (flush and (self.size() > 0)):
            
            if flush:
                nzero = self.blocksize - self.size() - 28
                if nzero > 0:
                    self.write(b'\0'*nzero)
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
                ">HIIII", self.streamid,
                self.blocksize, fill, self.object_offset, 
                self.blockcount)
            o = self.blockcount * (self.blocksize - 28)
            crc = crc16(header1 + 
                        self.getbuffer()[o:o+self.blocksize-28])
            header0 = struct.pack('>qH', nextoffset, crc)
            
            dprint("block at", self.offset, "size", fill)
            yield header0 + header1 + self.getbuffer()[o:o+self.blocksize-28]
            self.offset += self.blocksize
            self.object_offset = 0
            self.blockcount += 1

class DDFFBlock:

    def __init__(self, f):
        offset = f.tell()
        header = f.read(28)
        if not len(header):
            raise ValueError("File ended")
        (self.next_offset, crc, self.stream_id, self.block_size, 
         self.block_fill, self.object_offset, self.block_num) = \
            struct.unpack(">qHHIIII", header)
        dprint(f"Block at {offset}, stream {self.stream_id}, fill {self.block_fill}"
               f", size {self.block_size}, first object at {self.object_offset}, block #{self.block_num}")
        self.tail = f.read(self.block_size - 28)
        dprint("crc", crc, "from data",
               crc16(header[10:] + self.tail))
        if crc != crc16(header[10:] + self.tail):
            raise ValueError("CRC failure in reading ddff block")

class DDFFStream(list):

    def __init__(self, *args, **kwargs):

        block = None
        if len(args) > 0:
            if isinstance(args[0], DDFFBlock):
                block = args[0]
            else:
                self.stream_id = int(args[0])
            args = args[1:]

        elif 'block' in kwargs:
            block = kwargs['block']
            del kwargs['block']
        else:
            self.stream_id = kwargs['stream_id']
            del kwargs['stream_id']

        self.unpacker = msgpack.Unpacker()
        super().__init__(*args, **kwargs)

        # if not for an existing stream, finished here
        if block is None:
            return

        # extract information on the existing stream
        self.stream_id = block.stream_id
        self.unpacker.feed(block.tail[:block.block_fill-28])

        try:
            for unpacked in self.unpacker:
                self.append(unpacked)
                #dprint("unpacked object", unpacked)
        except ValueError:
            dprint("Unpack fails, object number", len(self))
            pass

    def readBlock(self, block):
        self.unpacker.feed(block.tail[:block.block_fill-28])
        try:
            for unpacked in self.unpacker:
                self.append(unpacked)
                #print(unpacked)
        except ValueError:
            dprint("Unpack fails, object number", len(self))
            pass

    def write(self, fd, blocksize=4096):

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

class DDFF:

    def __init__(self, fname, mode='r', blocksize=128):

        self.file = open(fname, mode+'b')
        self.streams = None
        self._scanStreams()
        self.blocksize = blocksize

    def _scanStreams(self):

        if self.streams is not None:
            return
        self.streams = {}

        try:
            while self.file:
                hdr = DDFFBlock(self.file)
                if hdr.stream_id in self.streams:
                    dprint("adding block to stream", hdr.stream_id)
                    self.streams[hdr.stream_id].readBlock(hdr)
                else:
                    dprint("creating new stream", hdr.stream_id)
                    self.streams[hdr.stream_id] = DDFFStream(hdr)
        except ValueError:
            pass

    def createStream(self):
        if len(self.streams) in self.streams:
            raise ValueError("DDFF streams not contiguous")
        newid = len(self.streams)
        self.streams[newid] = DDFFStream(newid)
        return self.streams[newid]

    def write(self, blocksize:int=0) -> None:
        # this writes the streams in order, not randomly scattered like
        # in real-time writing (since we cannot know the timing)
        for ids in sorted(self.streams.keys()):
            self.streams[ids].write(
                self.file, blocksize or self.blocksize)
        self.file.flush()
        
if __name__ == '__main__':

    prj = 'DuecaTestCommunication'
    recdata = DDFF(os.getenv('HOME') + 
                   f'/gdapps/{prj}/{prj}/run/solo/solo' + 
                   '/recordings-PHLAB.ddff', mode='r')
    print(recdata.streams[0], recdata.streams[1])


    test = "123456789".encode("ascii")
    print(f"test crc, 0x{crc16(test):4x} should be 0x29b1")

    stuff = DDFF('test.ddff', mode='w')
    st0 = stuff.createStream()
    st0.append((10, ['a']))
    st0.append((20, ['b']))
    stuff.write()

    del stuff
    
    verif = DDFF('test.ddff', mode='r')
    print(verif.streams)
    
    stuff2 = DDFF('test2.ddff', mode='w', blocksize=128)
    st0 = stuff2.createStream()
    for i in range(100):
        st0.append((i, f"data point {i}"))
    st1 = stuff2.createStream()
    st1.append((10, ['a']))
    st1.append((20, ['b']))
    
    stuff2.write()
    
    verif2 = DDFF('test2.ddff', mode='r')
    print(st0 == verif2.streams[0])
    print(verif2.streams[0])

    
    