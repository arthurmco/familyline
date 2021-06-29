#!/usr/bin/env python3

#
# Bump version
#  - Update the cmakefile
#  - Create a "Bump version" commit
#  - Create a new tag

import sys
import os
import re

def advance_version(v, bumprule):
    """
    Advance the version based on a version bump rule
    """
    
    vercomps = v.replace('v', '').split('.')

    majv = int(vercomps[0])
    minv = int(vercomps[1])
    patchv = int(vercomps[2])

    if bumprule == "major":
        majv += 1
        minv = 0
        patchv = 0

    if bumprule == "minor":
        minv += 1
        patchv = 0

    if bumprule == "patch":
        patchv += 1

    return "{}.{}.{}".format(majv, minv, patchv)
    

def check_if_staged():
    """
    Check if we have some staged file

    Since this script commits, we cannot run if we have staged files,
    because it would commit something we do not want to commit

    Return True if we have a staged file
    """
    stream = os.popen("git status -s")
    gfiles = stream.readlines()

    return any([f[0] not in ["?", " "] for f in gfiles])


def get_bump_rule():
    """
    Get the bump rule based on the argument
    """
    if len(sys.argv) < 2:
        return "patch"

    valid_bumps = ["major", "minor", "patch"]
    if sys.argv[1] not in valid_bumps:
        return "patch"

    return sys.argv[1]
    

def update_cmakelists(oldver, newver):
    path = "./CMakeLists.txt"
    regex = r"(Familyline VERSION) (\d{1,2}\.\d{1,2}\.\d{1,2})"
    subst = "\\1 {}".format(newver)
    
    with open(path, "r") as cmakefile:
        filedata = cmakefile.read()

    result = re.sub(regex, subst, filedata, 0, re.MULTILINE)

    with open(path, "w") as cmakefile:
        cmakefile.write(result)



if len(sys.argv) >= 2 and sys.argv[1] == "--help":
    print("Usage: tools/bump_version.py [major|minor|patch] \n\nPatch bump is the default")
    sys.exit(2)

bumprule = get_bump_rule()
tagdir = ".git/refs/tags"

try:
    tags = os.listdir(tagdir)
except FileNotFoundError:
    print("cannot find", tagdir)
    print("check if you are in the repository")
    print("you need to run this script from the familyline source root")
    sys.exit(1)
    
# The newest will be first
tags.sort(reverse=True)

if len(tags) == 0:
    print("no tags found, check if you pulled the tags")
    print("or if you are in the correct repository")
    sys.exit(1)

current_ver = tags[0].replace("v", "")
next_ver = advance_version(current_ver, bumprule)

print("\t", current_ver, "->", next_ver)

if check_if_staged():
    print("there are some files that are staged")
    print("since we will commit a file, we might commit something we do not want")
    print("")
    print("please, commit or unstage the file")
    print("")
    print("--------------------------------")
    os.system("git status")
    print("--------------------------------")
    sys.exit(1)

####################

print("\t-> updating CMakeLists.txt")
update_cmakelists(current_ver, next_ver)

####################

os.system("git add CMakeLists.txt")
os.system("git commit -m \"Bump version (v{} -> v{})\"".format(current_ver, next_ver))

####################

os.system("git tag -a v{}".format(next_ver))
