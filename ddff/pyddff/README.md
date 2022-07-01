# Python library for reading ddff files

## Introduction and set-up

DDFF is the DelftDataFileFormat, a convenience, binary data file format,
based on msgpack packing and unpacking of struct-like data with a time or
integer count tag. Data is written as msgpack arrays with two entries, the
first is an unsigned (four byte) time or count tag, the second entry is
the data.

To optimize performance, a suitable block size matching the physical device
block size may be selected for the file. Multiple streams of data can be
written to a file; the information of different streams is packed block-wise.
Each block in the file has a small header, giving information on:

- 2-byte unsigned integer crc checksum for the block; all bytes of the
  block's data area except the present are included in this checksum

- 2-byte unsigned integer, big endian, indicates the stream id

- 4-byte unsigned integer, big endian, indicates this block's size.

- 4-byte unsigned integer, big endian, indicates this block's
  fill level, i.e., number of data bytes, including the 20 control
  bytes.

- 4-byte unsigned integer, big endian, indicates the offset of this
  block's first started write. 0 if no write started in this block;
  for starting reading at any place

- 4-byte unsigned integer, big endian, indicates the block number

This effectively gives a file with multiple streams (like virtual
files) of binary data, with an effective block size of the chosen
physical device block size - 20 bytes.

## Variants

The file format can be used as such, with multiple streams that are
discovered as the file is parsed, but additional variants are possible,
giving the following set of (currently known) variants:

- Base, with each stream (counting from 0), available for data

- With inventory, where stream #0 makes up the inventory. This
  inventory describes and names the remaining streams. It is coded in
  structs, with the following given and user-defined data:

  * key, a unique identifying string for the stream.

  * id, the stream number linked to the key.

  * label, a string with additional information the user wishes to add
    to further describe the stream.

- With inventory and tagged. The inventory in stream #0 is given as
  above for the inventory. The tagging in stream #1 uses the following
  struct-like format:

  * cycle, an unsigned integer, indicating the recording number.

  * index0, a user-defined unsigned integer for the start of the recording.

  * index1, a user-defined unsigned integer for the end of the recording.

  * time, indicating a human readable time for the start of the recording.

  * label, a string label, to be chosen freely

  * offset, a vector of file offsets indicating the first blocks of each
    data stream (streams #2 and beyond), where the tag's first data is
    written.
