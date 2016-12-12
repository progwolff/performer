project(performer)
cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
set(PROJECT_VERSION "0.1.0")
set(QT_MIN_VERSION "0.1.0")

SET(CMAKE_CXX_FLAGS "-std=c++0x")

find_package(Qt5 ${QT_MIN_VERSION} 
    CONFIG REQUIRED COMPONENTS 
    Core 
    #Gui 
    Widgets 
    #Designer 
    #X11Extras 
    Quick 
    QuickWidgets
    Concurrent
    )
    
find_package(Qt5 ${QT_MIN_VESION}
    OPTIONAL_COMPONENTS
    WebEngineWidgets
    WebView
)

find_package(ECM NO_MODULE)
if(ECM_FOUND)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
    include(ECMInstallIcons)
    include(FeatureSummary)

    find_package(KF5 COMPONENTS
        CoreAddons
        I18n
        #ConfigWidgets
        KIO
        #XmlGui
        OPTIONAL_COMPONENTS
        Parts
    )

    if(KF5_FOUND)
        option(WITH_KF5 "Build with KDE Frameworks 5" ON)
        if(WITH_KF5)
            add_definitions(-DWITH_KF5) 
            Message("Building with KF5")
        endif()
    endif(KF5_FOUND)

    if(WITH_KF5 AND KF5Parts_FOUND)
        option(WITH_KPARTS "Build with KParts (Support for Okular)" ON)
        if(WITH_KPARTS)
            add_definitions(-DWITH_KPARTS)
            Message("Building with KParts")
        endif()
    else()
        find_package(KF5 COMPONENTS XmlGui)
    endif()

    include(KDEInstallDirs)
    include(KDECMakeSettings)
    include(KDECompilerSettings)
    
    list(APPEND QML_DIRS "${ECM_MODULE_DIR}")
endif(ECM_FOUND)

if(NOT WITH_KPARTS)
    if(Qt5WebEngineWidgets_FOUND)
        option(WITH_QWEBENGINE "Build with QWebEngine (View files without Okular)" ON)
        if(WITH_QWEBENGINE)
            add_definitions(-DWITH_QWEBENGINE)
            Message("Building with QWebEngine")
        endif()
    endif()
endif()

if(NOT WITH_KPARTS AND NOT WITH_QWEBENGINE)
    if(Qt5WebView_FOUND)
        option(WITH_QTWEBVIEW "Build with QtWebView (View files without Okular or QWebEngineView)" ON)
        if(WITH_QTWEBVIEW)
            add_definitions(-DWITH_QTWEBVIEW)
            Message("Building with QtWebView")
        endif()
    endif()
endif()

find_package ( PkgConfig  )
if(PkgConfig_FOUND)
    pkg_check_modules ( JACK jack )
else(PkgConfig_FOUND)
    if(DEFINED JACK_LIBRARIES)
        set(JACK_FOUND 1)
        message("looking for jack headers in ${JACK_INCLUDEDIR}")
        message("using jack lib ${JACK_LIBRARIES}")
    else()
        MESSAGE("pkgconfig not found. please set JACK_INCLUDEDIR and JACK_LIBRARIES.")
    endif()
endif(PkgConfig_FOUND)
if(JACK_FOUND)
    option(WITH_JACK "Build with Jack (enable Carla backend)" ON)
    if(WITH_JACK)
        add_definitions(-DWITH_JACK)
        Message("Building with Jack")
    endif()
endif(JACK_FOUND)

if(NOT WITH_JACK)
    message( "Building without Jack. Carla backend will not work." )
endif()

set(QML_IMPORT_PATH "${QML_DIRS}" CACHE STRING "Qt Creator 4.1 extra qml import paths")

add_definitions(-DTRANSLATION_DOMAIN=\"performer\")
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

FIND_PROGRAM(GETTEXT_MSGFMT_EXECUTABLE msgfmt)

IF(NOT GETTEXT_MSGFMT_EXECUTABLE)
	MESSAGE(
"------
                 NOTE: msgfmt not found. Translations will *not* be installed
------")
ELSE(NOT GETTEXT_MSGFMT_EXECUTABLE)

        SET(catalogname performer)

	MESSAGE("installing localization from ${CMAKE_CURRENT_SOURCE_DIR}/po")
        FILE(GLOB PO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/po/*.po")
        SET(GMO_FILES)

        FOREACH(_poFile ${PO_FILES})
		MESSAGE("found ${_poFile}")
                GET_FILENAME_COMPONENT(_poFileName ${_poFile} NAME)
                STRING(REGEX REPLACE "^${catalogname}_?" "" _langCode ${_poFileName} )
                STRING(REGEX REPLACE "\\.po$" "" _langCode ${_langCode} )

                IF( _langCode )
                        GET_FILENAME_COMPONENT(_lang ${_poFile} NAME_WE)
                        SET(_gmoFile ${CMAKE_CURRENT_BINARY_DIR}/${_lang}.gmo)

                        ADD_CUSTOM_TARGET(${_lang}.gmo)
                        ADD_CUSTOM_COMMAND(TARGET ${_lang}.gmo
                                COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/Messages.sh && ${GETTEXT_MSGFMT_EXECUTABLE} --check -o ${_gmoFile} ${_poFile}
                                DEPENDS ${_poFile})
                        INSTALL(FILES ${_gmoFile} DESTINATION ${KDE_INSTALL_LOCALEDIR}/${_langCode}/LC_MESSAGES/ RENAME ${catalogname}.mo)
                        LIST(APPEND GMO_FILES ${_lang}.gmo)
                ENDIF( _langCode )

        ENDFOREACH(_poFile ${PO_FILES})

        ADD_CUSTOM_TARGET(translations ALL DEPENDS ${GMO_FILES})

ENDIF(NOT GETTEXT_MSGFMT_EXECUTABLE)

add_subdirectory(src)

if(ECM_FOUND)
    install(FILES performer.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})

    ecm_install_icons(
        ICONS
            sc-apps-performer.svg
        sc-mimetypes-application-x-performer-setlist.svg
        DESTINATION ${ICON_INSTALL_DIR}
    )

    install(FILES x-performer-setlist.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
    
    feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif(ECM_FOUND)

find_package(SharedMimeInfo ${SMI_MIN_VERSION})
if(SHARED_MIME_INFO_FOUND)
	update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
endif(SHARED_MIME_INFO_FOUND)

