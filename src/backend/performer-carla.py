#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla plugin host
# Copyright (C) 2011-2015 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the doc/GPL.txt file.

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)


import os,io

out = io.open("/proc/"+str(os.getpid())+"/fd/1", 'w')

from carla_host import *


class CallbackHandler(QObject):
    
    def __init__(self):
        super().__init__()
    
    @pyqtSlot(int, str)
    def slot_pluginAdded(self, pluginId, pluginName):
        out.write("Added plugin: "+pluginName+"\n")
        out.flush()
        
       
# ------------------------------------------------------------------------------------------------------------
# Main

if __name__ == '__main__':
    # -------------------------------------------------------------
    # Read CLI args
    
    initName, libPrefix = handleInitialCommandLineArguments(__file__ if "__file__" in dir() else None)

    # -------------------------------------------------------------
    # App initialization

    app = CarlaApplication("Carla2-Patchbay", libPrefix)

    # -------------------------------------------------------------
    # Set-up custom signal handling

    setUpSignals()
    # -------------------------------------------------------------
    # Init host backend

    host = initHost(initName, libPrefix, False, False, True)
    host.processMode       = ENGINE_PROCESS_MODE_PATCHBAY
    host.processModeForced = True
    
    printErr = False

    # Find port setup argument
    for arg in sys.argv[1:]:
        if arg.startswith("--port-setup="):
            tryPortSetup = arg.replace("--port-setup=", "").split(":")
            if len(tryPortSetup) == 4:
                try:
                    tryPortSetup = tuple(int(p) for p in tryPortSetup)
                except:
                    printErr = True
                else:
                    host.patchbayPortSetup = tryPortSetup
            else:
                printErr = True
            break
    
    handler = CallbackHandler()
    
    host.PluginAddedCallback.connect(handler.slot_pluginAdded)

    loadHostSettings(host)    
    # -------------------------------------------------------------
    # Create GUI
    
    gui = HostWindow(host, True)

    # -------------------------------------------------------------
    # Show GUI

    gui.show()
    
    # -------------------------------------------------------------
    # App-Loop

    app.exit_exec()
    
    os.close()
