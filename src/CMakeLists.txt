   
# add_subdirectory(configwidgets)

find_package ( PkgConfig REQUIRED )
pkg_check_modules ( JACK jack )
if(JACK_FOUND)
    add_definitions(-DWITH_JACK)
endif(JACK_FOUND)

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

set(PERFORMER_UI
     ui/setlist.ui
#    qml/main.qml
#    qml/TooltipArea.qml
#    ui/behaviorconfig.ui
#    qml/behavior.qml
)

if(KF5_FOUND)
    ki18n_wrap_ui(PERFORMER ${PERFORMER_UI})
endif(KF5_FOUND)

add_executable(performer ${PERFORMER_SRCS} ${PERFORMER_UI})

target_compile_definitions(performer PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")

target_link_libraries(performer
    Qt5::Widgets
    Qt5::X11Extras
    Qt5::Quick
    Qt5::QuickWidgets
    Qt5::Concurrent
)

if(KF5_FOUND)
    target_link_libraries(performer
    KF5::ConfigWidgets
    KF5::KIOWidgets
    KF5::I18n
    KF5::XmlGui
    KF5::CoreAddons
    )
endif(KF5_FOUND)

if(KF5Parts_FOUND)
    target_link_libraries(performer
    KF5::Parts
    )
endif(KF5Parts_FOUND)

if(JACK_FOUND)
    target_link_libraries(performer
    ${JACK_LIBRARIES}
    )
endif(JACK_FOUND)

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


