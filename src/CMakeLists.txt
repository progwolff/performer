   
# add_subdirectory(configwidgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
#QT5_WRAP_UI(setlistheader setlist_without_kde.ui)

include_directories ( ${JACK_INCLUDEDIR} ${JACK_INCLUDE_DIRS} )
link_directories ( ${JACK_LIBDIR} ${JACK_LIBRARY_DIRS} )


#include_directories(configwidgets)
set(PERFORMER_SRCS
    performer.cpp
    util.cpp
    midi.cpp
    setlistmodel.cpp
    setlistmetadata.cpp
    abstractpatchbackend.hpp
    carlapatchbackend.cpp
    setlistview.cpp
    okulardocumentviewer.cpp
    qwebenginedocumentviewer.cpp
    qtwebviewdocumentviewer.cpp
    fallback.cpp
)
   
if(ANDROID)
set(ANDROID_SRCS androidfiledialog.cpp)
add_library(performer SHARED main.cpp ${ANDROID_SRCS} ${PERFORMER_SRCS} ${PERFORMER_UI})
include(../cmake/AddQtAndroidApk.cmake)
add_definitions(-DANDROID)

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/package_sources/assets)
if(WITH_QTWEBVIEW)
    FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/qml DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/package_sources/assets)
    FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/extern/pdf.js DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/package_sources/assets)
    FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/AndroidManifest.xml DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/package_sources)
endif()

add_qt_android_apk(performer_apk performer
	NAME "Performer"
	VERSION_CODE 1.1
	PACKAGE_NAME "de.julianwolff.performer"
	DEPENDS performer
	PACKAGE_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/package_sources
)
list(APPEND _LINK_LIBRARIES
    Qt5::AndroidExtras
)


elseif(WIN32)
add_executable(performer main.cpp ${PERFORMER_SRCS} ${PERFORMER_UI} ${CMAKE_CURRENT_SOURCE_DIR}/../performer.rc)
else()
add_executable(performer main.cpp ${PERFORMER_SRCS} ${PERFORMER_UI} ${setlistheader})
endif()



#target_compile_features(performer PRIVATE cxx_auto_type)
list(APPEND _LINK_LIBRARIES 
    Qt5::Widgets
    #Qt5::X11Extras
    Qt5::Quick
    Qt5::QuickWidgets
    Qt5::Concurrent
)


if(LINK_STATIC)

set_target_properties(performer PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(performer PROPERTIES LINK_SEARCH_END_STATIC 1)

if(UNIX)
if(NOT DEFINED Qt5_LIBDIR)
set(Qt5_LIBDIR "${Qt5_DIR}/../../..")
endif()

list(APPEND _LINK_LIBRARIES 
    
    Qt5::Gui
    Qt5::Core
    ${Qt5_LIBDIR}/plugins/platforms/libqxcb.a
    ${Qt5_LIBDIR}/lib/libQt5XcbQpa.a
    ${Qt5_LIBDIR}/lib/libqtharfbuzzng.a
    ${Qt5_LIBDIR}/lib/libqtpcre.a
    ${Qt5_LIBDIR}/lib/libxcb-static.a
    ${Qt5_LIBDIR}/lib/libQt5PlatformSupport.a
    Qt5::DBus
    xcb X11 Xi X11-xcb ICE SM
    pthread dl 
    glib-2.0 dl icui18n icuuc 
    freetype fontconfig dbus-1
    png jpeg
    z
)
endif(UNIX)

endif(LINK_STATIC)



if(WITH_JACK)
    list(APPEND _LINK_LIBRARIES 
        ${JACK_LIBRARIES}
    )
endif(WITH_JACK)

if(WITH_KF5)
    set(PERFORMER_UI setlist.ui)
    ki18n_wrap_ui(ui_setlist.h ${PERFORMER_UI})
else(WITH_KF5)
    set(PERFORMER_UI setlist_without_kde.ui)
    qt5_wrap_ui(ui_setlist_without_kde.h ${PERFORMER_UI})
	include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif(WITH_KF5)

target_compile_definitions(performer PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")


if(WITH_KF5)
    kde_enable_exceptions()

    list(APPEND _LINK_LIBRARIES 
        #KF5::ConfigWidgets
        KF5::KIOWidgets
        KF5::I18n
        #KF5::XmlGui
        KF5::CoreAddons
        KF5::GlobalAccel
        KF5::Notifications
    )
    
endif(WITH_KF5)

if(WITH_KCRASH)
    list(APPEND _LINK_LIBRARIES
        KF5::Crash
    )
endif(WITH_KCRASH)

if(NOT WITH_KPARTS AND WITH_KF5)
    list(APPEND _LINK_LIBRARIES 
        KF5::XmlGui
    )
endif(NOT WITH_KPARTS AND WITH_KF5)

if(WITH_KPARTS)
    list(APPEND _LINK_LIBRARIES 
        KF5::Parts
    )
endif(WITH_KPARTS)

if(WITH_QWEBENGINE)
    list(APPEND _LINK_LIBRARIES  
        Qt5::WebEngineWidgets
    )
    if(NOT DEFINED CMAKE_INSTALL_BINDIR)
        set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
    endif()
    if(NOT DEFINED CMAKE_INSTALL_DATADIR)
        set(CMAKE_INSTALL_DATADIR ${CMAKE_INSTALL_BINDIR}/data)
    endif()
    if(NOT WITH_KPARTS)
        install( DIRECTORY extern/pdf.js DESTINATION ${CMAKE_INSTALL_DATADIR}/performer )
    endif()
endif(WITH_QWEBENGINE)

if(WITH_QTWEBVIEW)
    list(APPEND _LINK_LIBRARIES 
        Qt5::WebView
    )
    if(NOT DEFINED CMAKE_INSTALL_BINDIR)
        set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
    endif()
    if(NOT DEFINED CMAKE_INSTALL_DATADIR)
        set(CMAKE_INSTALL_DATADIR ${CMAKE_INSTALL_BINDIR}/data)
    endif()
    if(NOT WITH_KPARTS)
        install( DIRECTORY extern/pdf.js DESTINATION ${CMAKE_INSTALL_DATADIR}/performer )
    endif()
endif(WITH_QTWEBVIEW)


target_link_libraries(performer ${_LINK_LIBRARIES})

add_definitions( -fexceptions )

option(WITH_TESTS "unit tests" OFF)
if(WITH_TESTS)
    option(BUILD_TESTING "Build the testing tree." ON)
	add_subdirectory(tests)
	if (EXISTS ${CATCH_INCLUDE_DIR}/catch.hpp)
        add_definitions( -Wall -O2 -fexceptions )
        include_directories(${CATCH_INCLUDE_DIR} ${COMMON_INCLUDES})
        enable_testing()
        add_executable(test_performer tests.cpp ${ANDROID_SRCS} ${PERFORMER_SRCS}  ${PERFORMER_UI})
        add_dependencies(test_performer catch) 
        target_link_libraries(test_performer ${_LINK_LIBRARIES})
        add_test(NAME "test_performer" COMMAND test_performer)
    else()
        set(WITH_TESTS 0)
    endif()
endif()


if(NOT DEFINED CMAKE_INSTALL_BINDIR)
  set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "user executables (bin)")
endif()
install(TARGETS performer DESTINATION ${CMAKE_INSTALL_BINDIR})

#install(FILES qml/main.qml DESTINATION ${CMAKE_INSTALL_DATADIR}/performer)
#install(FILES qml/TooltipArea.qml DESTINATION ${CMAKE_INSTALL_DATADIR}/performer)
#install(FILES qml/behavior.qml DESTINATION ${CMAKE_INSTALL_DATADIR}/performer)
#install( FILES okularui.rc DESTINATION  ${CMAKE_INSTALL_DATADIR}/performer )
#install( FILES Performerui.rc DESTINATION ${CMAKE_INSTALL_DATADIR}/performer )
if(ECM_FOUND)
    install( FILES okularui.rc DESTINATION  ${KXMLGUI_INSTALL_DIR}/performer )
    install( FILES Performerui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/performer )
    install( FILES performer.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR} )
endif(ECM_FOUND)
#kcoreaddons_desktop_to_json(performer ../performer.desktop)

if(WIN32)
	get_target_property(location Qt5::Core LOCATION)
	list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${location})
	get_target_property(location Qt5::Gui LOCATION)
	list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${location})
	get_target_property(location Qt5::Network LOCATION)
	list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${location})
	get_target_property(location Qt5::Positioning LOCATION)
	list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${location})
	get_target_property(location Qt5::PrintSupport LOCATION)
	list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${location})
	get_target_property(location Qt5::Qml LOCATION)
	list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${location})
	get_target_property(location Qt5::Quick LOCATION)
	list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${location})
	get_target_property(location Qt5::QuickWidgets LOCATION)
	list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${location})
	get_target_property(location Qt5::WebChannel LOCATION)
	list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${location})
	get_target_property(location Qt5::WebEngineCore LOCATION)
	list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${location})
#	get_target_property(location Qt5::WebEngine LOCATION)
#	list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${location})
	get_target_property(location Qt5::WebEngineWidgets LOCATION)
	list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${location})
	get_target_property(location Qt5::Widgets LOCATION)
	list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${location})
endif(WIN32)

if(WITH_QTWEBVIEW)
    install(FILES qml/webview.qml DESTINATION ${CMAKE_INSTALL_DATADIR}/performer)
endif()

if(WITH_JACK)
    install(FILES backend/performer-carla.py DESTINATION ${CMAKE_INSTALL_DATADIR}/carla RENAME performer-carla)
    install(FILES backend/performer-carla-database.py DESTINATION ${CMAKE_INSTALL_DATADIR}/carla RENAME performer-carla-database)
    install(
        FILES backend/performer-carla DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME performer-carla 
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
    )
    install(
        FILES backend/performer-carla-database DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME performer-carla-database 
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
    )

	if(WIN32)
		install( DIRECTORY $ENV{LOCALAPPDATA}/Temp/Carla DESTINATION ${CMAKE_INSTALL_DATADIR}/performer )
		install( FILES $ENV{LOCALAPPDATA}/Temp/Carla/Debug/Carla.exe DESTINATION ${CMAKE_INSTALL_DATADIR}/performer/Carla/ )
	endif(WIN32)
endif(WITH_JACK)



INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Audio session manager for live music performances")
SET(CPACK_PACKAGE_VENDOR "Julian Wolff")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
SET(CPACK_PACKAGE_NAME "Performer")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "1")
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "Performer")
IF(WIN32 AND NOT UNIX)
  SET(CPACK_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}\\\\..\\\\cmake)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\..\\\\sc-apps-performer.ico")
  SET(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\..\\\\sc-apps-performer.ico")
  SET(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\..\\\\sc-apps-performer.ico")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME  "bin\\\\performer.exe")
  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "CreateShortCut '$desktop\\\\Performer.lnk' '\$INSTDIR\\\\bin\\\\performer.exe'") 
  set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete '$desktop\\\\Performer.lnk'")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "Performer.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "Performer")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\github.com\\\\progwolff\\\\performer")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\github.com\\\\progwolff\\\\performer")
  SET(CPACK_NSIS_CONTACT "wolff@julianwolff.de")
  SET(CPACK_NSIS_MODIFY_PATH ON)
ELSE(WIN32 AND NOT UNIX)
  SET(CPACK_STRIP_FILES "performer")
  SET(CPACK_SOURCE_STRIP_FILES "")
  SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Julian Wolff <wolff at julianwolff dot de>")
  SET(CPACK_DEBIAN_PACKAGE_SECTION "sound")
  SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS "carla-git (>= 1.5)")
  SET(CPACK_RPM_PACKAGE_REQUIRES "python3-qt4, libKF5Crash5, libKF5GuiAddons5, libKF5Notifications5, libKF5QuickAddons5, libKF5WidgetsAddons5, libKF5XmlGui5, jack >= 1.9, carla >= 1.5")
ENDIF(WIN32 AND NOT UNIX)
SET(CPACK_PACKAGE_EXECUTABLES "Performer" "Performer")
INCLUDE(CPack)
