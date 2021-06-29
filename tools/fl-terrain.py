#!/usr/bin/env python3

# Familyline terrain creator
#
# From an image, create a terrain, with appropriate terrain height and type, depending on the
# pixel color.
# Each pixel will be a point in the map, with the R and G channels being the height and B and A channels
# being the type. Since both channel and types are 16 bits in the terrain format, no problem.
# The R will be the lower 8 bits for the size and the G channel will be the higher 16 ones. Same with B 
# and A for the type: B will be the lower bits and A will be the upper bits.

from dataclasses import dataclass
from enum import Enum, unique
from PIL import Image, UnidentifiedImageError
from io import BytesIO
import sys
import struct


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

@unique
class TerrainType(Enum):
    GRASS = 0
    DIRT = 10
    WATER = 20
    MOUNTAIN = 30

@dataclass
class TerrainData():
    """
    The terrain data.

    Each terrain height point is 1cm higher than the previous one, so the maximum height is 655.35cm.
    The height, however, might be capped to 64m more or less so we do not have a height difference
    too big.

    Also, the camera stays at a fixed height, it does not change according to the height of the terrain
    below it *yet*, so it might enter inside the terrain sometimes.
    """
    height: int
    terrain_type: TerrainType


@dataclass
class Terrain():
    """
    The terrain object.

    Usually, the terrain file can fit multiple terrains but, for now, we only need one per file.
    """
    name: str
    width: int
    height: int
    authors: [str]
    description: str 
    data: [TerrainType]


@dataclass
class TerrainFile():
    """
    The terrain file object.
    """
    terrain: Terrain
    version: int = 3


def open_image(imagepath):
    """
    Receives an image file, opens it and returns a PIL header 
    """
    im = Image.open(imagepath)

    return im

def pixel_to_terrain_height(pixel):
    """
    Convert a RGBA pixel to a terrain height.

    The R channel composes the lower 8 bits, the G channel composes the upper 8 bits.
    """
    return pixel[0] & 0xff |( (pixel[1] & 0xff) << 8)

def pixel_to_terrain_type(pixel):
    """
    Convert a RGBA pixel to a terrain type.

    The B channel composes the lower 8 bits, the A channel composes the upper 8 bits.
    Note that some images do not have an alpha channel.
    """
    if len(pixel) > 3:
        return pixel[2] & 0xff | ((pixel[3] & 0xff) << 8)
    else:
        return pixel[2] & 0xff


def convert_image_data_to_terrain_data(imdata) -> TerrainData:
    return [TerrainData(pixel_to_terrain_height(p), pixel_to_terrain_type(p))
            for p in imdata]


def convert_image_to_terrain(im) -> Terrain:
    width, height = im.size

    data = im.getdata()
    terrain_data = convert_image_data_to_terrain_data(data)

    return Terrain(None, width, height, [], "", terrain_data)
    
def parse_authors(authorstr: str):
    """
    Create an array of authors from an author list (in the name <email> format) separated
    by comma
    """
    return [a for a in authorstr.split(",") if a.strip() != "" ]

def calculate_crc(data: BytesIO) -> int:
    import binascii

    return binascii.crc32(data.getvalue())

def write_terrain_data(data: [TerrainData]):
    buf = (BytesIO(), BytesIO())

    for d in data:
        buf[0].write(struct.pack("H", d.height))
        buf[1].write(struct.pack("H", d.terrain_type))

    return buf[0].getvalue(), buf[1].getvalue()

def write_name_data(name, align=True):
    data = name.encode("utf-8")
    buf = BytesIO(struct.pack("B", len(data)))
    buf.seek(1)
    buf.write(data)

    # Ensure correct offset alignment for the next offset.
    if align:
        while buf.tell() % 4 != 0:
            buf.write(struct.pack("B", 0))

    return buf.getvalue()

def write_authors_data(authors):
    buf = BytesIO(struct.pack("B", len(authors)))
    buf.seek(1)

    # It is the same format as the name, so...
    for a in authors:
        buf.write(write_name_data(a, False))

    while buf.tell() % 4 != 0:
        buf.write(struct.pack("B", 0))

    return buf.getvalue()

def write_description_data(description):
    description = description.replace("\\n", "\n")
    description = description.replace("\\t", "\t")

    data = description.encode("utf-8")
    buf = BytesIO(struct.pack("H", len(data)))
    buf.seek(2)
    buf.write(data)

    while buf.tell() % 4 != 0:
        buf.write(struct.pack("B", 0))

    return buf.getvalue()

def write_terrain(terrain: Terrain, header_off: int = 16):
    data = write_terrain_data(terrain.data)

    magic = 0x45454554
    header_len = 36
    terrain_height_off = header_off + header_len
    crc_offset = 12

    name_data = None
    author_data = None
    description_data = None

    if terrain.name is None:
        name_off = 0
    else:
        name_data = write_name_data(terrain.name)
        name_off = terrain_height_off
        terrain_height_off += len(name_data)

    if len(terrain.authors) <= 0:
        author_off = 0
    else:
        author_data = write_authors_data(terrain.authors)
        author_off = terrain_height_off
        terrain_height_off += len(author_data)

    if len(terrain.description) <= 0:
        description_off = 0
    else:
        description_data = write_description_data(terrain.description)
        description_off = terrain_height_off
        terrain_height_off += len(description_data)

    terrain_height_len = len(data[0])
    # do not put this variable above those IFs above
    terrain_type_off = terrain_height_off + terrain_height_len

    buf = BytesIO(struct.pack("IIII", magic, terrain.width, terrain.height, 0))
    buf.seek(16)
    buf.write(struct.pack("IIIII", name_off, author_off, description_off, 
        terrain_height_off, terrain_type_off))    
    if name_off > 0:
        buf.write(name_data)

    if author_off > 0:
        buf.write(author_data)

    if description_off > 0:
        buf.write(description_data)

    buf.write(data[0])
    buf.write(data[1])
    crc32 = calculate_crc(buf)

    print(f"terrain crc32 is {crc32:04x}")

    buf.seek(crc_offset)
    buf.write(struct.pack("I", crc32))

    return buf.getvalue()

def write_terrain_file(terrain_file: TerrainFile):
    """
    Write terrain file data here.

    The format for the terrain follows here:

     - Terrain File header:

       offset | size | description
       -------+------+------------
             0|     4| signature bytes 0x45544c46 (FLTE in ascii)
             4|     4| CRC32 of the whole file, including the header. Zeroed when
              |      | calculating the checksum
             8|     4| map file version, currently 3q
            12|     4| offset, in the file, to the first terrain.             

     - Terrain header
       Usually at offset 16 decimal, 0x10 hexadecimal

       offset | size | description
       -------+------+------------
             0|     4| terrain signature bytes, 0x45454554 (TEEE in ascii)
              |      | Might be useful to fix corrupted terrains
             4|     4| terrain width
             8|     4| terrain height
            12|     4| terrain CRC32 (this bytes are zeroed when calculating this value)
              |      | Obviously the crc includes the header, the name, description and author list
              |      | and the terrain data, in this order.
            16|     4| file offset to the name (this is relative to the file start)
            20|     4| file offset to the author list (same as above)
            24|     4| file offset to the description (same as above)
            28|     4| file offset to the terrain height data
            32|     4| file offset to the terrain type data

       Both name, description and author offset, when 0, means that that field
       does not exist. A null terrain data offset also means that the terrain data
       does not exist, but this would not make sense.

       The offsets need to be multiples of four, to ensure a correct
       alignment of the strings and terrain data

     - Name and description
       The map name is a pascal-like string, the string length, in bytes, comes first, and
       then the string data. The string is UTF-8. 
       Description is the same thing, but the length field has 2 bytes, instead of one byte.

     - Author list
       First byte is the author count.
       For each author we have a string like the name string, 1 byte for the length, the remaining for the string.
       Author data is on the `name <email>` format, much like git

     - Terrain height data
       An array of words (2 bytes) for the terrain height. The array size is `width`*`height`
       
     - Terrain type data
       An array of words (2 bytes) for the terrain type code. The array size is `width`*`height`
       
     TODO: support compression if terrain sizes go too big (> 500 MB +/-).
     First RLE, then deflate if RLE also starts being too big

    """
    magic = 0x45544c46
    buf = BytesIO(struct.pack("III", magic, 0, terrain_file.version))
    
    buf.seek(12)
    toff = buf.tell() + 4
    buf.write(struct.pack("I", toff))

    crc_offset = 4
    
    buf.write(write_terrain(terrain_file.terrain))
    crc32 = calculate_crc(buf)

    buf.seek(crc_offset)
    buf.write(struct.pack("I", crc32))

    print(f"terrain file crc32 is {crc32:04x}")

    return buf.getvalue()


import argparse
parser = argparse.ArgumentParser(
    description="familyline image to terrain file converter"
)
parser.add_argument("image_in", help="The image file for input")
parser.add_argument("outfile", help="The terrain file that will be written on output", default="terrain.flte")
parser.add_argument("--name", help="The map name", type=str)
parser.add_argument("--description", help="The map description", type=str)
parser.add_argument("--authors", 
    help="The map authors, comma separated, following the `name <email>` syntax, if possible.", type=str)
args = parser.parse_args()

print(f"reading {args.image_in}")

im = None
try:
    im = open_image(args.image_in)
except UnidentifiedImageError as e:
    eprint(f"This file is not an image")
    exit(1)
except FileNotFoundError as e:
    eprint(f"Image not found")
    exit(1)

print(f"terrain size will be {im.width}x{im.height}")

if im.width != im.height:
    print("warning: terrain is not a square, this might be unsupported in the future.")

if im.mode != "RGB":
    eprint("only RGB images are supported")
    exit(1)

print(im.format, im.size, im.mode)
terrain = convert_image_to_terrain(im)

if args.name is not None:
    terrain.name = args.name

if args.authors is not None:
    terrain.authors = parse_authors(args.authors)
    
if args.description is not None:
    terrain.description = args.description

tfile = TerrainFile(terrain)


authorstr = ",".join(terrain.authors)
print("writing terrain")
print(f"\tsize:        {terrain.width}x{terrain.height}")
print(f"\tname:        {terrain.name}")
print(f"\tauthors:     {authorstr}")
print(f"\tdescription: {terrain.description}")

data = write_terrain_file(tfile)

outfile = open(args.outfile, "wb")
outfile.write(data)
outfile.close()