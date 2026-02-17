# CLAUDE.md - xfce4-squeezebox-plugin

## Project Overview
XFCE4 panel plugin that controls various music players (Rhythmbox, MPD,
Audacious, Exaile, Banshee, Muine, Pragha, QuodLibet, RadioTray) via
D-Bus or network protocols. Displays playback controls in the panel with
album art tooltips and desktop notifications.

## Build System
GNU Autotools (autoconf + automake + libtool).

### Build Commands
```sh
./autogen.sh        # generate configure script (requires xfce4-dev-tools)
./configure --prefix=/usr
make
sudo make install
```

### Dependencies (GTK3 port)
- gtk+-3.0 >= 3.20
- libxfce4panel-2.0 >= 4.12.0
- libxfce4util-1.0 >= 4.12.0
- libxfce4ui-2 >= 4.12.0
- libxfconf-0 >= 4.12.0
- exo-2 >= 0.11.0
- dbus-1 >= 1.0.0
- dbus-glib-1 >= 0.73
- glib-2.0 >= 2.40.0
- libwnck-3.0 >= 3.4
- keybinder-3.0 >= 0.3.0

### Optional Dependencies
- id3tag >= 0.15 (embedded album art from MP3 files)
- zeitgeist-2.0 (activity logging)

## Project Structure
```
panel-plugin/          # Core panel plugin (UI frontend)
  squeezebox.c         # Main plugin: create, events, backend mgmt
  squeezebox.h         # Public API, Backend struct, SPlayer interface
  squeezebox-private.h # Private data structures, D-Bus helpers
  settings.c           # Settings dialog (GtkBuilder based)
  notifications.c      # Desktop notification via D-Bus
backends/              # Loadable player backends (GModule plugins)
  mpd/                 # Music Player Daemon (network backend)
  rhythmbox/           # Rhythmbox (D-Bus backend)
  audacious/           # Audacious (D-Bus backend)
  exaile/              # Exaile (D-Bus backend)
  banshee/             # Banshee (D-Bus backend)
  muine/               # Muine (D-Bus backend)
  pragha/              # Pragha (D-Bus backend)
  quodlibet/           # QuodLibet (D-Bus backend)
  radiotray/           # RadioTray (D-Bus backend)
glade/                 # GtkBuilder UI definitions
themes/                # Theme UI definitions for panel widget
po/                    # Translations (de, fr, tr)
```

## Architecture Notes
- Backends are compiled as shared libraries loaded via GModule at runtime.
- Each backend exports a `backend_info()` function returning a `Backend` struct.
- The `SPlayer` struct acts as the API contract between frontend and backends.
- D-Bus integration uses dbus-glib (legacy; consider GDBus for future work).
- Settings stored via xfconf.
- Global keyboard shortcuts via keybinder.

## Coding Conventions
- C99 with GLib/GObject conventions
- Logging via LOG/LOGERR/LOGWARN macros (enabled with --enable-debug_trace)
- EXPORT macro for public symbols
- MKTHIS macro to cast thsPlayer pointer to SqueezeBoxData*
