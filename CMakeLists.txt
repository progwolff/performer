project(performer)
cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
set(PROJECT_VERSION "0.1.0")
set(QT_MIN_VERSION "0.1.0")

find_package(Qt5 ${QT_MIN_VERSION} 
    CONFIG REQUIRED COMPONENTS 
    Core 
    #Gui 
    Widgets 
    #Designer 
    #X11Extras 
    Quick 
    QuickWidgets
    Concurrent)

find_package(ECM NO_MODULE)
if(ECM_FOUND)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
    include(ECMInstallIcons)
    include(FeatureSummary)

    find_package(KF5 REQUIRED COMPONENTS
        CoreAddons
        I18n
        #ConfigWidgets
        KIO
        XmlGui
        #OPTIONAL_COMPONENTS
        #Parts
    )

    if(KF5_FOUND)
        add_definitions(-DWITH_KF5) 
    endif(KF5_FOUND)

    if(KF5Parts_FOUND)
        add_definitions(-DWITH_KPARTS)
    else(KF5Parts_FOUND)
        find_package(KF5 REQUIRED COMPONENTS XmlGui)
    endif(KF5Parts_FOUND)

    include(KDEInstallDirs)
    include(KDECMakeSettings)
    include(KDECompilerSettings)
    
    list(APPEND QML_DIRS "${ECM_MODULE_DIR}")
endif(ECM_FOUND)

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

