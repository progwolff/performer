   
# add_subdirectory(configwidgets)

set(CMAKE_AUTOMOC ON)

include_directories ( ${JACK_INCLUDEDIR} ${JACK_INCLUDE_DIRS} )
link_directories ( ${JACK_LIBDIR} ${JACK_LIBRARY_DIRS} )

#include_directories(configwidgets)
set(PERFORMER_SRCS
    performer.cpp
    main.cpp
    setlistmodel.cpp
    setlistmetadata.cpp
    abstractpatchbackend.hpp
    carlapatchbackend.cpp
    setlistview.cpp
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
add_library(performer ${PERFORMER_SRCS} ${PERFORMER_UI})
elseif(WIN32)
add_executable(performer WIN32 ${PERFORMER_SRCS} ${PERFORMER_UI})
else()
add_executable(performer ${PERFORMER_SRCS} ${PERFORMER_UI})
endif()

target_compile_definitions(performer PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")

#target_compile_features(performer PRIVATE cxx_auto_type)

target_link_libraries(performer
    Qt5::Widgets
    #Qt5::X11Extras
    Qt5::Quick
    Qt5::QuickWidgets
    Qt5::Concurrent
)


if(WITH_KF5)
    kde_enable_exceptions()

    target_link_libraries(performer
        #KF5::ConfigWidgets
        KF5::KIOWidgets
        KF5::I18n
        #KF5::XmlGui
        KF5::CoreAddons
    )
endif(WITH_KF5)

if(NOT WITH_KPARTS AND WITH_KF5)
    target_link_libraries(performer
        KF5::XmlGui
    )
endif(NOT WITH_KPARTS AND WITH_KF5)

if(WITH_KPARTS)
    target_link_libraries(performer
        KF5::Parts
    )
else(WITH_KPARTS)
    if(WITH_QWEBENGINE)
        target_link_libraries(performer 
            Qt5::WebEngineWidgets
        )
		if(NOT DEFINED CMAKE_INSTALL_DATADIR)
		    set(CMAKE_INSTALL_DATADIR ${CMAKE_INSTALL_BINDIR}/data)
		endif()
        install( DIRECTORY pdf.js DESTINATION ${CMAKE_INSTALL_DATADIR}/performer )
    endif(WITH_QWEBENGINE)
endif(WITH_KPARTS)

if(WITH_JACK)
    target_link_libraries(performer
        ${JACK_LIBRARIES}
    )
endif(WITH_JACK)

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


