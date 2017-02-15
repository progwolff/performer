# Performer

Live performance audio session manager using [Carla](https://github.com/falktx/Carla)

![Screenshot](./.screenshot.png "Screenshot")

[![Build Status](https://travis-ci.org/progwolff/performer.svg?branch=master)](https://travis-ci.org/progwolff/performer)

Performer lets you manage all the songs in your setlist as individual carla patches and loads each of them when you need it.
Additionally Performer uses [Okular](https://github.com/KDE/okular) or QWebEngine to display notes and chords of your songs.

## Features
* Loads carla patches for songs and connects them when they are active
* Displays notes or chords for songs
* Detects crashed instances of Carla or the Jack server and restarts them
* MIDI controllable
* Qt style selectable

## Dependencies
* Carla
* python
* qt5-base
* qt5-declarative
* qt5-tools (make)
* extra-cmake-modules (make, optional: KDE integration)
* kdebase-runtime (optional: KDE integration)
* kparts (optional: display notes or chords with okular)
* okular-git (optional: display notes or chords)
* qt5-webengine (optional: display notes or chords without okular)
* jackman (optional: automatically restart unresponsive jack server)

## Install
### Arch Linux
Install performer-git from AUR

### KXStudio / Ubuntu 14.04

To build Performer with a document viewer on KXStudio or Ubuntu 14 run:
```
$ sudo apt-get install build-essential
$ sudo apt-get install git
$ sudo apt-get install cmake
$ sudo apt-get install libjack-jackd2-dev
$ wget http://download.qt.io/official_releases/qt/5.7/5.7.0/qt-opensource-linux-x64-5.7.0.run
$ chmod +x qt-opensource-linux-x64-5.7.0.run
$ ./qt-opensource-linux-x64-5.7.0.run
```

Install Qt to ~/Qt5.7.0, skip login, make sure to install Qt WebEngine


Build and install Performer:
```
$ git clone git@github.com:progwolff/performer.git
$ mkdir build && cd build
$ cmake -DLINK_STATIC=1 -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_DATADIR=/usr/share -DQt5_DIR=~/Qt5.7.0/5.7/gcc_64/lib/cmake/Qt5 -DJACK_LIBRARIES=/usr/lib/x86_64-linux-gnu/libjack.so -DJACK_INCLUDEDIR=/usr/include ..
$ make
$ sudo make install
```

### Other Linux Distros
```
$ mkdir build && cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr ..
$ make
# make install
```
### Windows [Experimental]

Download and extract the zip file from Releases to C:\Program Files (x86).

To update to the latest git version:

In cmd.exe type:
```
mkdir build
cd build
cmake -DQt5_DIR="C:\Qt\5.7\msvc2015\lib\cmake\Qt5" -DJACK_INCLUDEDIR="C:\Program Files (x86)\Jack\includes" -DJACK_LIBRARIES="C:\Program Files (x86)\Jack\lib\libjack.lib" -DWITH_TESTS=0 ..
cmake --build .
```
In cmd.exe with admin privileges:
```
cmake --build . --target install
```

### Android [Experimental]

```
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DANDROID_TOOLCHAIN_ROOT=arm-linux-androideabi -DANDROID_ABI="armeabi-v7a" -DCMAKE_FIND_ROOT_PATH="/opt/android-qt5;/opt/android-qt5/5.7.0/armeabi-v7a/lib/cmake/Qt5" -DANDROID_TOOLCHAIN_NAME=arm-linux-androideabi-4.9 -DANDROID_TOOLCHAIN_MACHINE_NAME=arm-linux-androideabi -DANDROID_COMPILER_VERSION=4.9 -DWITH_JACK=0 ..
cmake --build .
```

Install the apk in src/bin/ to your android device.

## Usage
[usage](usage)
