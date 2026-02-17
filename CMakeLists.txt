cmake_minimum_required(VERSION 3.10)
project(xfce4-squeezebox-plugin VERSION 1.0.0 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(GNUInstallDirs)
include(FindPkgConfig)

# Options for backends
option(ENABLE_RHYTHMBOX "Enable Rhythmbox backend" ON)
option(ENABLE_MPD "Enable MPD backend" ON)
option(ENABLE_QUODLIBET "Enable QuodLibet backend" ON)
option(ENABLE_AUDACIOUS "Enable Audacious backend" ON)
option(ENABLE_EXAILE "Enable Exaile backend" ON)
option(ENABLE_PRAGHA "Enable Pragha backend" ON)
option(ENABLE_MUINE "Enable Muine backend" ON)
option(ENABLE_BANSHEE "Enable Banshee backend" ON)
option(ENABLE_RADIOTRAY "Enable RadioTray backend" ON)
option(ENABLE_DEBUG_TRACE "Enable debug trace output" OFF)

# Required packages
pkg_check_modules(GTK3 REQUIRED gtk+-3.0>=3.20.0)
pkg_check_modules(LIBXFCE4PANEL REQUIRED libxfce4panel-2.0>=4.12.0)
pkg_check_modules(LIBXFCE4UTIL REQUIRED libxfce4util-1.0>=4.12.0)
pkg_check_modules(LIBXFCE4UI REQUIRED libxfce4ui-2>=4.12.0)
pkg_check_modules(LIBXFCE4WINDOWING REQUIRED libxfce4windowing-0>=4.19.3)
pkg_check_modules(DBUS REQUIRED dbus-1>=1.0.0)
pkg_check_modules(DBUS_GLIB REQUIRED dbus-glib-1>=0.73)
pkg_check_modules(LIBXFCONF REQUIRED libxfconf-0>=4.12.0)
pkg_check_modules(LIBEXO REQUIRED exo-2>=0.11.0)
pkg_check_modules(GLIB REQUIRED glib-2.0>=2.40.0)
pkg_check_modules(GMODULE REQUIRED gmodule-2.0)
pkg_check_modules(KEYBINDER REQUIRED keybinder-3.0>=0.3.0)
pkg_check_modules(GTHREAD REQUIRED gthread-2.0)

# Optional packages
pkg_check_modules(LIBGIO gio-unix-2.0>=2.16.6)
pkg_check_modules(LIBID3TAG id3tag>=0.15)
pkg_check_modules(LIBINDICATE indicate-0.5>=0.5.0)
pkg_check_modules(LIBZEITGEIST zeitgeist-2.0>=0.9.0)

# X11
find_package(X11 REQUIRED)

# Map ENABLE_* options to HAVE_BACKEND_* for config.h
if(ENABLE_RHYTHMBOX)
    set(HAVE_BACKEND_RHYTHMBOX 1)
endif()
if(ENABLE_MPD)
    set(HAVE_BACKEND_MPD 1)
endif()
if(ENABLE_QUODLIBET)
    set(HAVE_BACKEND_QUODLIBET 1)
endif()
if(ENABLE_AUDACIOUS)
    set(HAVE_BACKEND_AUDACIOUS 1)
endif()
if(ENABLE_EXAILE)
    set(HAVE_BACKEND_EXAILE 1)
endif()
if(ENABLE_PRAGHA)
    set(HAVE_BACKEND_PRAGHA 1)
endif()
if(ENABLE_MUINE)
    set(HAVE_BACKEND_MUINE 1)
endif()
if(ENABLE_BANSHEE)
    set(HAVE_BACKEND_BANSHEE 1)
endif()
if(ENABLE_RADIOTRAY)
    set(HAVE_BACKEND_RADIOTRAY 1)
endif()

# Map optional library findings
if(LIBGIO_FOUND)
    set(HAVE_GIO 1)
endif()
if(LIBID3TAG_FOUND)
    set(HAVE_LIBID3TAG 1)
endif()
if(LIBINDICATE_FOUND)
    set(HAVE_LIBINDICATE 1)
endif()
if(LIBZEITGEIST_FOUND)
    set(HAVE_LIBZEITGEIST 1)
endif()
if(ENABLE_DEBUG_TRACE)
    set(DEBUG_TRACE 1)
endif()

# Generate config.h
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# Gettext / i18n
set(GETTEXT_PACKAGE "xfce4-squeezebox-plugin")
add_definitions(-DGETTEXT_PACKAGE="${GETTEXT_PACKAGE}")

# Installation paths
set(PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/xfce4/panel/plugins")
set(BACKEND_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/xfce4/xfce4-squeezebox-plugin/backends")
set(GLADE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/xfce4/xfce4-squeezebox-plugin/glade")
set(DESKTOP_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/xfce4/panel/plugins")

add_definitions(
    -DBACKENDDIR="${CMAKE_INSTALL_FULL_LIBDIR}/xfce4/xfce4-squeezebox-plugin/backends"
    -DGLADEDIR="${CMAKE_INSTALL_FULL_LIBDIR}/xfce4/xfce4-squeezebox-plugin/glade"
    -DLOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}"
    -DDATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/xfce4/xfce4-squeezebox-plugin"
    -DG_LOG_DOMAIN="SBP"
    -DLIBXFCE4WINDOWING_COMPILATION
)

if(ENABLE_DEBUG_TRACE)
    add_definitions(-DDEBUG_TRACE=1)
endif()

# Common include/link flags for all targets
set(COMMON_INCLUDE_DIRS
    ${GTK3_INCLUDE_DIRS}
    ${LIBXFCE4PANEL_INCLUDE_DIRS}
    ${LIBXFCE4UTIL_INCLUDE_DIRS}
    ${LIBXFCE4UI_INCLUDE_DIRS}
    ${LIBXFCE4WINDOWING_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
    ${DBUS_GLIB_INCLUDE_DIRS}
    ${LIBXFCONF_INCLUDE_DIRS}
    ${LIBEXO_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GMODULE_INCLUDE_DIRS}
    ${KEYBINDER_INCLUDE_DIRS}
    ${X11_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(COMMON_LINK_LIBS
    ${GTK3_LIBRARIES}
    ${LIBXFCE4UTIL_LIBRARIES}
    ${LIBXFCE4UI_LIBRARIES}
    ${LIBXFCE4WINDOWING_LIBRARIES}
    ${LIBXFCONF_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GMODULE_LIBRARIES}
    ${X11_LIBRARIES}
)

# Backend common flags
set(BACKEND_INCLUDE_DIRS
    ${COMMON_INCLUDE_DIRS}
    ${LIBGIO_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/panel-plugin
)

set(BACKEND_LINK_LIBS
    ${GTK3_LIBRARIES}
    ${LIBXFCE4UTIL_LIBRARIES}
    ${LIBXFCE4UI_LIBRARIES}
    ${LIBXFCE4WINDOWING_LIBRARIES}
    ${LIBXFCONF_LIBRARIES}
    ${DBUS_LIBRARIES}
    ${DBUS_GLIB_LIBRARIES}
)

if(LIBGIO_FOUND)
    list(APPEND BACKEND_LINK_LIBS ${LIBGIO_LIBRARIES})
endif()

# Subdirectories
add_subdirectory(panel-plugin)
add_subdirectory(glade)

# Backend subdirectories
if(ENABLE_RHYTHMBOX)
    add_subdirectory(backends/rhythmbox)
endif()
if(ENABLE_MPD)
    add_subdirectory(backends/mpd)
endif()
if(ENABLE_QUODLIBET)
    add_subdirectory(backends/quodlibet)
endif()
if(ENABLE_AUDACIOUS)
    add_subdirectory(backends/audacious)
endif()
if(ENABLE_EXAILE)
    add_subdirectory(backends/exaile)
endif()
if(ENABLE_PRAGHA)
    add_subdirectory(backends/pragha)
endif()
if(ENABLE_MUINE)
    add_subdirectory(backends/muine)
endif()
if(ENABLE_BANSHEE)
    add_subdirectory(backends/banshee)
endif()
if(ENABLE_RADIOTRAY)
    add_subdirectory(backends/radiotray)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "########### SqueezeBox CONFIGURATION ############")
message(STATUS "")
message(STATUS " ID3 tag support ...............${LIBID3TAG_FOUND}")
message(STATUS " Sound menu support ............${LIBINDICATE_FOUND}")
message(STATUS " Zeitgeist support .............${LIBZEITGEIST_FOUND}")
message(STATUS "")
message(STATUS "----------- SqueezeBox Backends -----------------")
message(STATUS "")
message(STATUS " rhythmbox support .............${ENABLE_RHYTHMBOX}")
message(STATUS " MPD support ...................${ENABLE_MPD}")
message(STATUS " QuodLibet support .............${ENABLE_QUODLIBET}")
message(STATUS " audacious support .............${ENABLE_AUDACIOUS}")
message(STATUS " exaile support ................${ENABLE_EXAILE}")
message(STATUS " Muine support .................${ENABLE_MUINE}")
message(STATUS " Banshee support ...............${ENABLE_BANSHEE}")
message(STATUS " pragha support ................${ENABLE_PRAGHA}")
message(STATUS " radiotray support .............${ENABLE_RADIOTRAY}")
message(STATUS "")

# Debian package target
add_custom_target(deb
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_SOURCE_DIR}/debian/compat
    COMMAND dpkg-buildpackage -us -uc -b
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building Debian package"
)

add_custom_target(deb-clean
    COMMAND dh_clean
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Cleaning Debian build artifacts"
)
