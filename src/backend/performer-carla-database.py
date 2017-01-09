from carla_host import *

if __name__ == '__main__':
    
        from carla_app import CarlaApplication
        from carla_host import initHost, loadHostSettings

        initName, libPrefix = handleInitialCommandLineArguments(__file__ if "__file__" in dir() else None)

        app  = CarlaApplication("Carla2-Database", libPrefix)
        host = initHost("Carla2-Database", libPrefix, False, False, False)
        loadHostSettings(host)

        dialog = PluginDatabaseW(None, host)

        if dialog.exec_():

            btype    = dialog.fRetPlugin['build']
            ptype    = dialog.fRetPlugin['type']
            filename = dialog.fRetPlugin['filename']
            label    = dialog.fRetPlugin['label']
            uniqueId = dialog.fRetPlugin['uniqueId']

            print("selected:"+str(btype)+","+str(ptype)+","+str(filename)+","+str(label)+","+str(uniqueId))
