# Performer
Live performance audio session manager using [Carla](https://github.com/falktx/Carla)

![Screenshot](./.screenshot.png "Screenshot")

[![Build Status](https://travis-ci.org/progwolff/performer.svg?branch=master)](https://travis-ci.org/progwolff/performer)

Performer lets you manage all the songs in your setlist as individual carla patches and loads each of them when you need it.
Additionally Performer uses [Okular](https://github.com/KDE/okular) or QWebEngine to display notes and chords of your songs.

Dependencies:
* Carla
* qt5-base
* qt5-declarative
* qt5-tools (make)
* python (make)
* extra-cmake-modules (make, optional: KDE integration)
* kdebase-runtime (optional: KDE integration)
* kparts (optional: display notes or chords with okular)
* okular-git (optional: display notes or chords)
* qt5-webengine (optional: display notes or chords without okular)
