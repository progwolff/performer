   
# add_subdirectory(configwidgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

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
add_executable(performer WIN32 main.cpp ${PERFORMER_SRCS} ${PERFORMER_UI})
else()
if(NOT WITH_KF5)
include(GNUInstallDirs)
endif()
add_executable(performer main.cpp ${PERFORMER_SRCS} ${PERFORMER_UI})
endif()



if(WITH_JACK)
    list(APPEND _LINK_LIBRARIES 
        ${JACK_LIBRARIES}
    )
endif(WITH_JACK)

if(LINK_STATIC)

set_target_properties(performer PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(performer PROPERTIES LINK_SEARCH_END_STATIC 1)

if(UNIX)
find_library (QXcb_LIBRARY NAMES qxcb)
find_library (QXcbQpa_LIBRARY NAMES Qt5XcbQpa)
find_library (QXcb_glxintegration_LIBRARY NAMES libqxcb-glx-integration)
find_library (Qt5GlxSupport_LIBRARY NAMES Qt5GlxSupport)
find_library (Qt5EventDispatcherSupport_LIBRARY NAMES Qt5EventDispatcherSupport)
find_library (Qt5AccessibilitySupport_LIBRARY NAMES Qt5AccessibilitySupport)
find_library (Qt5LinuxAccessibilitySupport_LIBRARY NAMES Qt5LinuxAccessibilitySupport)
find_library (Qt5ThemeSupport_LIBRARY NAMES Qt5ThemeSupport)
find_library (Qt5FontDatabaseSupport_LIBRARY NAMES Qt5FontDatabaseSupport)
find_library (Qt5EglFSDeviceIntegration_LIBRARY NAMES Qt5EglFSDeviceIntegration)
find_library (Qt5ServiceSupport_LIBRARY NAMES Qt5ServiceSupport)
list(APPEND _LINK_LIBRARIES 
    ${QXcb_LIBRARY} 
    ${QXcbQpa_LIBRARY} 
    ${Qt5ThemeSupport_LIBRARY} 
    ${Qt5FontDatabaseSupport_LIBRARY}
    ${Qt5ServiceSupport_LIBRARY} 
    ${Qt5LinuxAccessibilitySupport_LIBRARY} 
    ${Qt5AccessibilitySupport_LIBRARY} 
    ${Qt5EventDispatcherSupport_LIBRARY} 
    xcb X11 xcb-xkb xcb-render xcb-randr xcb-xinerama xcb-xfixes xcb-shape xcb-shm xcb-sync xcb-keysyms xcb-image 
    xcb-ewmh xkbcommon xcb-icccm xkbcommon-x11 xcb-render-util Xi X11-xcb ICE SM Xft fontconfig
    pthread dl dbus-1 
    png freetype 
    z 
    harfbuzz 
    glib-2.0 icui18n icuuc pcre16
    Qt5::DBus
)
endif(UNIX)

endif(LINK_STATIC)

if(WITH_KF5)
    set(PERFORMER_UI setlist.ui)
    ki18n_wrap_ui(ui_setlist.h ${PERFORMER_UI})
else(WITH_KF5)
    set(PERFORMER_UI setlist_without_kde.ui)
    qt5_wrap_ui(ui_setlist_without_kde.h ${PERFORMER_UI})
	include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif(WITH_KF5)

target_compile_definitions(performer PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")

#target_compile_features(performer PRIVATE cxx_auto_type)
list(APPEND _LINK_LIBRARIES 
    Qt5::Widgets
    #Qt5::X11Extras
    Qt5::Quick
    Qt5::QuickWidgets
    Qt5::Concurrent
)


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

option(WITH_TESTS "unit tests" ON)
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
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}\\\\sc-apps-performer.svg")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "Performer.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} Performer")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\github.com\\\\progwolff\\\\performer")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\github.com\\\\progwolff\\\\performer")
  SET(CPACK_NSIS_CONTACT "wolff@julianwolff.de")
  SET(CPACK_NSIS_MODIFY_PATH ON)
ELSE(WIN32 AND NOT UNIX)
  SET(CPACK_STRIP_FILES "performer")
  SET(CPACK_SOURCE_STRIP_FILES "")
  SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Julian Wolff <wolff at julianwolff.de>")
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5gui5 (>= 5.5), libqt5quickwidgets5 (>= 5.5), libqt5quick5 (>= 5.5), libqt5qml5 (>= 5.5), libqt5xml5 (>= 5.5), libqt5widgets5 (>= 5.5), libqt5dbus5 (>= 5.5), libqt5core5a (>= 5.5), libqt5concurrent5 (>= 5.5), libjack-jackd2-0, carla-git (>= 1.5)")
  SET(CPACK_RPM_PACKAGE_REQUIRES "okular5, kparts,  libqt5gui5 >= 5.6, libqt5quickwidgets5 >= 5.6, libqt5quick5 >= 5.6, libqt5qml5 >= 5.6, libqt5xml5 >= 5.6, libqt5widgets5 >= 5.6, libqt5dbus5 >= 5.6, libqt5core5a >= 5.6, libqt5concurrent5 >= 5.6, libjack-jackd2-0, carla-git >= 1.5")
ENDIF(WIN32 AND NOT UNIX)
SET(CPACK_PACKAGE_EXECUTABLES "Performer" "Performer")
INCLUDE(CPack)
