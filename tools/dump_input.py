#!/usr/bin/env python

# Read a Familyline input file (the one with extension *.frec) and dump a JSON
#
# We could use `flatc` directly, but, since the file will be written in chunks, to ease
# recovering in case of a crash, we have smaller flatbuffer chunks, instead of a big
# flatbuffer file.
#
# You need to run `flatc --python ../src/common/input_serialize.fbs` before running
# this script, to generate the familyline.* module, a series of flatbuffers accessors.

import sys
import struct
from datetime import datetime

import familyline.RecordHeader as rh
import familyline.InputElement as ie
import familyline.InputType as it

import familyline.CommandInput as ci
import familyline.SelectAction as sa
import familyline.ObjectMove as om
import familyline.CameraMove as cm
import familyline.CameraRotate as cr
import familyline.CreateEntity as ce

import flatbuffers
import json

def parse_type(itype, typecode):
    """
    Parse type code information, return a dictionary with the data on the file
    """
    if typecode == it.InputType().cmd:
        tval = ci.CommandInput()
        tval.Init(itype.Bytes, itype.Pos)

        clen = tval.Args().ArgsLength()
        cargs = [tval.Args().Args(i) for i in range(clen)]

        return dict(
            type="command-input",
            command=tval.Command().decode("utf-8"),
            args=cargs
        )

    elif typecode == it.InputType().sel:
        tval = sa.SelectAction()
        tval.Init(itype.Bytes, itype.Pos)

        clen = tval.Objects().ValuesLength()
        cargs = [tval.Objects().Values(i) for i in range(clen)]

        return dict(
            type="select-action",
            values=cargs
        )

    elif typecode == it.InputType().obj_move:
        tval = om.ObjectMove()
        tval.Init(itype.Bytes, itype.Pos)

        return dict(
            type="object-move",
            xpos=tval.XPos(),
            ypos=tval.YPos()
        )

    elif typecode == it.InputType().cam_move:
        tval = cm.CameraMove()
        tval.Init(itype.Bytes, itype.Pos)

        return dict(
            type="camera-move",
            xdelta=tval.XDelta(),
            ydelta=tval.YDelta(),
            zoomdelta=tval.ZoomDelta()
        )

    elif typecode == it.InputType().cam_rotate:
        tval = cr.CameraRotate()
        tval.Init(itype.Bytes, itype.Pos)

        return dict(
            type="camera-rotate",
            radians=tval.Radians()
        )

    elif typecode == it.InputType().create:
        tval = ce.CreateEntity()
        tval.Init(itype.Bytes, itype.Pos)

        return dict(
            type="create-entity",
            name=tval.Type().decode("utf-8"),
            xpos=tval.XPos(),
            ypos=tval.YPos()
        )

    else:
        return dict(type="unknown", code=typecode)

def parse_input_segment(ifile, player):
    """
    Parse an input declaration.

    Returns a dictionary with its properties, or None if it could not parse

    We convert the timestamp to a date; the game timestamp is counted as microseconds,
    """
    magic, len = struct.unpack("4sI", ifile.read(8))

    if magic != b"FINP":
        return None

    inputbuf = bytearray(ifile.read(len))
    vinput = ie.InputElement.GetRootAsInputElement(inputbuf, 0)

    timestamp = datetime.fromtimestamp(vinput.Timestamp() / 1000000)
    typecode = vinput.TypeType()
    return dict(
        tick=vinput.Tick(),
        timestamp=str(timestamp),
        playercode=vinput.Playercode(),
        player=player.get(vinput.Playercode(), {"name": None})["name"],
        typeinfo=parse_type(vinput.Type(), typecode)

    )


def get_player_list(rheader, version):
    """
    Get the player list on this input

    Return a dictionary; the player ID is the key and the name is the value
    """
    playerdict = {}
    plen = rheader.PlayersLength()
    for i in range(plen):
        player = rheader.Players(i)

        if version == 1:
            id = player.Id() & 0xffffffff

        playerdict[id] = dict(name=player.Name().decode("utf-8"), color=player.Color())

    return playerdict

##########################################################
##########################################################

if len(sys.argv) < 2:
    print(f"Usage: {sys.argv[0]} <input_rec_file>\n")
    exit(1)

filename = sys.argv[1]
supported_version = [1]

objcount = 0
inputs = []
with open(filename, 'rb') as ifile:
    magic, version, headerlen = struct.unpack("4sII", ifile.read(12))
    if magic != b"FREC":
        print("error: invalid magic number", file=sys.stderr)
        exit(1)

    if version == 0:
        print("error: invalid version", file=sys.stderr)
        exit(1)

    if version not in supported_version:
        print(f"error: file version {version} not supported", file=sys.stderr)
        exit(1)

    if headerlen == 0:
        print("error: invalid length", file=sys.stderr)
        exit(1)

    headerbuf = bytearray(ifile.read(headerlen))
    rheader = rh.RecordHeader.GetRootAsRecordHeader(headerbuf, 0)

    if rheader.PlayersIsNone():
        print(f"error: no players", file=sys.stderr)
        exit(1)

    if rheader.Checksums() is None:
        print(f"error: no object checksums", file=sys.stderr)
        exit(1)

    players = get_player_list(rheader, version)

    iseg = parse_input_segment(ifile, players)
    lastpos = ifile.tell()
    while iseg is not None:
        inputs.append(iseg)
        lastpos = ifile.tell()
        iseg = parse_input_segment(ifile, players)

    ifile.seek(lastpos)

    try:
        footer, objcount, checksum = struct.unpack("4sII", ifile.read(12))
        if footer != b"FEND":
            raise RuntimeError("invalid footer magic value")

        if objcount != len(inputs):
            raise RuntimeError("input count does not match what was in the file. {} vs {}".format(
                objcount, len(inputs)))
        
    except RuntimeError as e:
        print("error: failure to read the footer, the file might be corrupted: ", e, file=sys.stderr)

print(json.dumps(dict(count=objcount, inputs=inputs), indent=2))
