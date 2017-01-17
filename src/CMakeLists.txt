   
# add_subdirectory(configwidgets)

set(CMAKE_AUTOMOC ON)

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

if(WITH_KF5)
    set(PERFORMER_UI ui/setlist.ui)
    ki18n_wrap_ui(PERFORMER ${PERFORMER_UI})
else(WITH_KF5)
    set(PERFORMER_UI ui/setlist_without_kde.ui)
    qt5_wrap_ui(PERFORMER ${PERFORMER_UI})
	include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif(WITH_KF5)

    
if(ANDROID)
add_library(performer SHARED main.cpp ${PERFORMER_SRCS} ${PERFORMER_UI})
include(../cmake/AddQtAndroidApk.cmake)
add_qt_android_apk(performer_apk performer
	NAME "Performer"
	VERSION_CODE 1
	PACKAGE_NAME "de.julianwolff.performer"
	DEPENDS performer
)


elseif(WIN32)
add_executable(performer WIN32 main.cpp ${PERFORMER_SRCS} ${PERFORMER_UI})
else()
if(NOT WITH_KF5)
include(GNUInstallDirs)
endif()
add_executable(performer main.cpp ${PERFORMER_SRCS} ${PERFORMER_UI})
endif()

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
    )
    
endif(WITH_KF5)

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
    if(NOT DEFINED CMAKE_INSTALL_DATADIR)
        set(CMAKE_INSTALL_DATADIR ${CMAKE_INSTALL_BINDIR}/data)
    endif()
    install( DIRECTORY extern/pdf.js DESTINATION ${CMAKE_INSTALL_DATADIR}/performer )
endif(WITH_QWEBENGINE)

if(WITH_QTWEBVIEW)
    list(APPEND _LINK_LIBRARIES 
        Qt5::WebView
    )
    if(NOT DEFINED CMAKE_INSTALL_DATADIR)
        set(CMAKE_INSTALL_DATADIR ${CMAKE_INSTALL_BINDIR}/data)
    endif()
    install( DIRECTORY extern/pdf.js DESTINATION ${CMAKE_INSTALL_DATADIR}/performer )
endif(WITH_QTWEBVIEW)

if(WITH_JACK)
    list(APPEND _LINK_LIBRARIES 
        ${JACK_LIBRARIES}
    )
endif(WITH_JACK)


target_link_libraries(performer ${_LINK_LIBRARIES})


option(WITH_TESTS "unit tests" ON)
if(WITH_TESTS)
    option(BUILD_TESTING "Build the testing tree." ON)
	add_subdirectory(tests)
	if (EXISTS ${CATCH_INCLUDE_DIR}/catch.hpp)
        add_definitions( -Wall -O2 -fexceptions )
        include_directories(${CATCH_INCLUDE_DIR} ${COMMON_INCLUDES})
        enable_testing()
        add_executable(test_performer tests.cpp ${PERFORMER_SRCS}  ${PERFORMER_UI})
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
