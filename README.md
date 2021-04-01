# Familyline

<p align="center">
	<img src="icons/familyline-logo.png" height="384px" width="384px" />
</p>

<p align="center">
<a href="https://travis-ci.org/arthurmco/familyline" title="travis badge">
  <img src="https://api.travis-ci.org/arthurmco/familyline.svg?branch=master" />
</a>
</p>

Familyline is a real-time strategy game focused on Ancient Era, from since the
neolythic period (~ 10.000 AD) to the death of Christ (~ 30 BD).

In it, you will guide a family, that goes from the end of neolythic until its kingdom
inclusion to the Roman Empire. You will trade with other people, war, mount strategies
and (if the money begin to appear) do a *lot* of quests.

It's made with a homebrew engine, for me to exclusively learn while
bringing joy to its players. The major objective of this game is
fun, to play and develop. 

Expect it to have a lot of good humor!

## Build and Installation

The game runs in Linux and Windows. Other platforms need to be tested.

Before you do anything, you need to pull the dependent repositories.
The only dependency that needs this is the Google Test framework.

Type those commands in the appropriate shell

```
git submodule init
git submodule update
```

and then you are good to go!

### Penguins

On Linux, you will need the softwares below:

 - *GLM*: for vector mathematics
 - *GLEW*: for dealing with OpenGL extensions
 - *SDL*: for dealing with our window. Version 2.0 is required.
 - *devIL*: for loading our textures.
 - *cairo*: for drawing the interfaces
 - *libyaml*: For parsing the `assets.yaml` file, aka the asset list.
 - *nlohmann-json*: For parsing and creating JSON from some network
   messages.
 - *tl-expected*: We are now starting to use the tl-expected header
   files to make error handling nicer
 - *curlpp*: For network communication (pre-game server-client
   communication is HTTP-based)
 - *libflatbuffers*: For serializing the input to a file, and reading
   from it

Please build in a separate directory from the source. For exemple,
when you clone the repository, you can create a directory named
`build` and run cmake from there. Like:

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DFLINE_USE_RELATIVE_PATH=off
-DFLINE_DO_TESTS=off ..
make familyline
```

The relative path tells that some files, like assets, are in the same
directory as the executable rather than the default path (/usr/share)

You will probably have to point to the `flatc` executable.  
This is more frequent in Windows and MacOS, but, in case you do need,
just set the `FLATBUFFERS_FLATC_EXECUTABLE` CMake variable to where it
is.

### Windows

On Windows, you can use `vcpkg`. Use [these
instructions](https://docs.microsoft.com/cpp/vcpkg?view=vs-2017).
You will need to have vcpkg integrated with Visual Studio.

Execute the following command:

`> vcpkg install glm glew sdl2 devil cairo libyaml fmt flatbuffers
nlohmann-json curlpp tl-expected`

Go make a coffee and buy needed things in the market. Order some
food. This will take some time.

When it finishes, add
`-DCMAKE_TOOLCHAIN_FILE={VCPKG_PATH}\scripts\buildsystems\vcpkg.cmake`
to your cmake command line. Replace `{VCPKG_PATH}` to where you
downloaded vcpkg.
If you used that Open Folder thing from Visual Studio -- I used --,
add the following lines into your CMakeSettings.json, inside the
variables key

```json5
{
    "name": "CMAKE_TOOLCHAIN_FILE",
    "value": "{VCPKG_PATH}\\scripts\\buildsystems\\vcpkg.cmake"
}
```

Then, cmake it

You will probably have to point to the `flatc` executable.  
Just set the `FLATBUFFERS_FLATC_EXECUTABLE` CMake variable to where it is.

### Fruits

Follow the same steps as the penguins one, because penguins and fruits are made
by the same God, the God of UNIX compatibility

However, there are some differences:

 - You may want to install your dependencies with homebrew
 - Xcode is needed, because the OpenGL headers come with it.  
   (yes, we still use it, but Vulkan support is coming :wink:)

### Build

To build the game, type `make familyline` to compile the game, `make
familyline-server` to compile the server (that doesn't work yet) or
`make familyline-tests` to make the tests.

(On Windows, inside Visual Studio, the targets might appear as projects
inside the solutions).

You will probably have to point to the `flatc` executable...

## Running

The executables have the same names as the targets. Have fun!

## Packaging

To generate a package, simply run these three commands:

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DFLINE_USE_RELATIVE_PATH=off -DFLINE_DO_TESTS=off ..
make package
```

The first will generate a makefile that don't compile the tests and don't use
relative path builds (they are useful while debugging). The last one installs
the package.

By default, it generates a DEB and a RPM file.

Note that, unless you customize the `CMAKE_INSTALL_PREFIX`, it will
install in the `/usr/local` directory when you run `make install`

## Things to do

Aside from issues, there are things I would like to do very soon

 - Increase test coverage
 - Add Windows test coverage
 - Port and test on FreeBSD
 
## License

Every file in this game (except some inside the cmake directory) is
licensed under the MIT license, the best in existence.

Copyright (C) 2016-2021 Arthur Mendes et al,
