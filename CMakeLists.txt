project(performer)
cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
set(PROJECT_VERSION "1.0.2")
set(QT_MIN_VERSION "0.1.0")

SET(CMAKE_CXX_FLAGS "-std=c++0x")

add_definitions(-DTRANSLATION_DOMAIN=\"performer\")
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

option(LINK_STATIC "Force link static" OFF)


if(LINK_STATIC)
add_definitions(-DLINK_STATIC) 
if(UNIX)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
endif(UNIX)

endif(LINK_STATIC)

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

if(LINK_STATIC)
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS DBus Gui)
endif(LINK_STATIC)


if(ANDROID)
find_package(Qt5 ${QT_MIN_VERSION}
    CONFIG REQUIRED COMPONENTS
    AndroidExtras
)
endif(ANDROID)

option(SKIP_ECM "Do not search for extra cmake modules" OFF)
if(NOT SKIP_ECM)
find_package(ECM NO_MODULE)
endif()
if(ECM_FOUND)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
    include(ECMInstallIcons)
    include(FeatureSummary)

    find_package(KF5 REQUIRED COMPONENTS
        CoreAddons
        I18n
        #ConfigWidgets
        KIO
        GlobalAccel
        #XmlGui
        Notifications
    )
    
    find_package(KF5 
        OPTIONAL_COMPONENTS
        Parts
        Crash
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
    
    if(WITH_KF5 AND KF5Crash_FOUND)
        option(WITH_KCRASH "Build with KCrash" ON)
        if(WITH_KCRASH)
            add_definitions(-DWITH_KCRASH) 
            Message("Building with KCRASH")
        endif()
    endif()

    include(KDEInstallDirs)
    include(KDECMakeSettings)
    include(KDECompilerSettings)
    
    list(APPEND QML_DIRS "${ECM_MODULE_DIR}")
endif(ECM_FOUND)

if(NOT WITH_KF5)
include(GNUInstallDirs)
endif()

#if(NOT WITH_KPARTS)
    if(Qt5WebEngineWidgets_FOUND)
        option(WITH_QWEBENGINE "Build with QWebEngine (View files without Okular and view html docs with or without Ocular)" ON)
        if(WITH_QWEBENGINE)
            add_definitions(-DWITH_QWEBENGINE)
            Message("Building with QWebEngine")
        endif()
    endif()
#endif()

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

option(LL_DEBUG "Print debug messages" ON)
if(NOT LL_DEBUG)
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()


FIND_PROGRAM(GETTEXT_MSGFMT_EXECUTABLE msgfmt)
FIND_PROGRAM(BASH_EXECUTABLE bash)

get_cmake_property(_variableNames VARIABLES)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

IF(NOT WITH_KF5 OR NOT GETTEXT_MSGFMT_EXECUTABLE OR NOT BASH_EXECUTABLE)

    FIND_PROGRAM(LCONVERT_EXECUTABLE lconvert)
    FIND_PROGRAM(LRELEASE_EXECUTABLE lrelease)
	
	IF(NOT LCONVERT_EXECUTABLE OR NOT LRELEASE_EXECUTABLE)
        MESSAGE(
    "------
                    NOTE: msgfmt and lconvert or lrelease not found. Translations will *not* be installed
    ------")
    ELSE()
        
        MESSAGE("installing qm localization from ${CMAKE_CURRENT_SOURCE_DIR}/po")
        FILE(GLOB PO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/po/*.po")
        SET(QM_FILES)
        
        if(NOT DEFINED CMAKE_INSTALL_DATADIR)
            set(CMAKE_INSTALL_DATADIR ${CMAKE_INSTALL_BINDIR}/data)
        endif()
        if(NOT QT_INSTALL_LOCALEDIR)
            set(QT_INSTALL_LOCALEDIR "${CMAKE_INSTALL_DATADIR}/performer")
        endif()

        FOREACH(_poFile ${PO_FILES})
		MESSAGE("found ${_poFile}")
                GET_FILENAME_COMPONENT(_poFileName ${_poFile} NAME)
                STRING(REGEX REPLACE "\\.po$" "" _langCode ${_poFileName} )

                IF( _langCode )
                    
                        GET_FILENAME_COMPONENT(_lang ${_poFile} NAME_WE)

                        ADD_CUSTOM_TARGET(performer_${_langCode}.qm)
                        ADD_CUSTOM_COMMAND(TARGET performer_${_langCode}.qm
                                COMMAND ${LCONVERT_EXECUTABLE} -locations relative ${_poFile} -o ${CMAKE_CURRENT_BINARY_DIR}/performer_${_langCode}.ts && ${LRELEASE_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/performer_${_langCode}.ts -qm ${CMAKE_CURRENT_BINARY_DIR}/performer_${_langCode}.qm
                                DEPENDS ${_poFile})
                                
                        INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/performer_${_langCode}.qm DESTINATION ${QT_INSTALL_LOCALEDIR})
                        LIST(APPEND QM_FILES performer_${_langCode}.qm)
                ENDIF( _langCode )

        ENDFOREACH(_poFile ${PO_FILES})

        ADD_CUSTOM_TARGET(translations ALL DEPENDS ${QM_FILES})
        
    ENDIF()
ELSE(NOT GETTEXT_MSGFMT_EXECUTABLE OR NOT BASH_EXECUTABLE)

        SET(catalogname performer)

        MESSAGE("installing localization from ${CMAKE_CURRENT_SOURCE_DIR}/po")
        FILE(GLOB PO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/po/*.po")
        SET(GMO_FILES)
        
        if(NOT KDE_INSTALL_LOCALEDIR)
            set(KDE_INSTALL_LOCALEDIR "${CMAKE_INSTALL_PREFIX}/share/locale")
        endif()

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

ENDIF(NOT GETTEXT_MSGFMT_EXECUTABLE OR NOT BASH_EXECUTABLE)

add_subdirectory(src)

if(ECM_FOUND)
find_package(KF5DocTools)
endif()

FILE(GLOB_RECURSE DOC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/po/docs/*")
FILE(GLOB_RECURSE IMAGE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/.images/*")

SET(DOC_DIR ${KDE_INSTALL_DOCBUNDLEDIR})
if(NOT DOC_DIR)
    SET(DOC_DIR ${CMAKE_INSTALL_PREFIX}/share/doc/HTML)
endif()


SET(DOC_TARGETS)
FOREACH(_docFile ${DOC_FILES})
MESSAGE("found ${_docFile}")
        STRING(REGEX REPLACE "\\/index.docbook$" "" _langCode ${_docFile} )
        GET_FILENAME_COMPONENT(_langCode ${_langCode} NAME)
        
        MESSAGE("langcode ${_langCode}")
        
        IF( _langCode )
                if(KF5DocTools_FOUND)
                    kdoctools_create_handbook(${_docFile} INSTALL_DESTINATION ${DOC_DIR}/${_langCode} SUBDIR performer)
                else()
                    FIND_PROGRAM(MEINPROC_EXECUTABLE meinproc5)
                    if(NOT MEINPROC_EXECUTABLE)
                        MESSAGE("NOTE: meinproc5 not found. Documentation will *not* be installed")
                    else()
                        ADD_CUSTOM_TARGET(${_langCode}_docs)
                        make_directory(${CMAKE_CURRENT_BINARY_DIR}/${_langCode}/performer)
                        ADD_CUSTOM_COMMAND(TARGET ${_langCode}_docs
                                COMMAND ${MEINPROC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/po/docs/${_langCode}/index.docbook
                                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${_langCode}/performer)
                        LIST(APPEND DOC_TARGETS ${_langCode}_docs)
                        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${_langCode}/performer DESTINATION ${DOC_DIR}/${_langCode})
                        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/po/docs/${_langCode}/index.docbook DESTINATION ${DOC_DIR}/${_langCode}/performer)
                    endif()
                endif()
                install(FILES ${IMAGE_FILES} DESTINATION ${DOC_DIR}/${_langCode}/performer)
                install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/.screenshot.png DESTINATION ${DOC_DIR}/${_langCode}/performer)

        ENDIF( _langCode )

ENDFOREACH(_docFile ${DOC_FILES})
ADD_CUSTOM_TARGET(documentation ALL DEPENDS ${DOC_TARGETS})

    
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
elseif(UNIX)
    install(FILES performer.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
    install(FILES sc-apps-performer.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons RENAME performer.svg)
endif(ECM_FOUND)

find_package(SharedMimeInfo ${SMI_MIN_VERSION})
if(SHARED_MIME_INFO_FOUND)
	update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
endif(SHARED_MIME_INFO_FOUND)

