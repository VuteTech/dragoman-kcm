# SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
# SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
# SPDX-License-Identifier: GPL-3.0-or-later

# The logic behind the page, free of QML and of KCMUtils: the module
# compiles it in, and the autotests link it as a plain library.
set(dragomankcm_core_sources
    daemonconfig.cpp
    daemonconfig.h
    installedpairs.cpp
    installedpairs.h
    servicestatus.cpp
    servicestatus.h)
set(dragomankcm_core_libraries
    DragomanQt::DragomanQt
    Qt6::Core
    Qt6::DBus
    KF6::CoreAddons
    KF6::I18n)

# Installed into plasma/kcms/systemsettings; the QML files come from ui/.
kcmutils_add_qml_kcm(kcm_dragomand SOURCES kcm.cpp kcm.h ${dragomankcm_core_sources})
target_compile_definitions(kcm_dragomand PRIVATE TRANSLATION_DOMAIN="kcm_dragomand")
target_link_libraries(kcm_dragomand PRIVATE
    ${dragomankcm_core_libraries}
    Qt6::Qml
    Qt6::Quick
    KF6::KCMUtilsQuick
    KF6::KIOGui
    KF6::Service)

if(BUILD_TESTING)
    add_library(dragomankcmcore STATIC ${dragomankcm_core_sources})
    target_compile_definitions(dragomankcmcore PUBLIC TRANSLATION_DOMAIN="kcm_dragomand")
    target_include_directories(dragomankcmcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(dragomankcmcore PUBLIC ${dragomankcm_core_libraries})
endif()
