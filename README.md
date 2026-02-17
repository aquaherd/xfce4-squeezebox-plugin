# Squeezebox XFCE4 Panel Plugin

This plugin remote controls some media players from your panel. It is in no way derived from the mpc plugin (although similar) or from the gnome media applet (although inspired).

Its goal is to control virtually all available audio players as soon as they allow for remote controlling by dbus, pipe or whatever. Required minimal functionality would be play/pause, skip/rewind and fetch track info, album/artist/title at least. Rating, album art display and toggling the visibility of the main UI would be optional.

Naming conflicts with existing gadgetery are coincidental. I was listening to a song of the Who when the idea to this plugin came to me.

Track change notification is made by means of libnotify only if libnotify is available during build.

Media Keys can be grabbed and ungrabbed with the modified mmkeys.c glory, as seen here and there. (See also [README.xubuntu](#readmexubuntu) below.)

Embedded album art extraction is under research (for iTunes switchers like me). It also considers `[/.]` `(folder|cover|front).jpg` in the file's folder if the backend supports file names.

Automatic detection of a started player and attaching to it is a long term goal. Currently, one can select one in the plugin properties.

## Currently Available Backends

In order of appearance:

**Active/Maintained:**
- Rhythmbox (via D-Bus)
- Music Player Daemon (network protocol)
- QuodLibet (via pipe and D-Bus)
- Audacious (via D-Bus)
- Pragha (via D-Bus)

**Deprecated (player discontinued):**
- Banshee (via D-Bus) - *discontinued 2014*
- Muine (via D-Bus) - *discontinued ~2010*
- Exaile (via D-Bus) - *last release 2019, effectively abandoned*
- RadioTray (via D-Bus) - *original discontinued; RadioTray-NG is the maintained fork*

## Dropped Backends

- BMPx (defunct)
- Consonance (defunct)
- Listen (defunct)
- Decibel Audio Player (defunct)

## Remotely Possible Backends

- mplayer (via -silent -idle switches with own playlist management)
- lua scripted with lua-dbus bindings
- SlimServer (as a nod to aforesaid naming issues)
- Shuffle anything audio from a tracker query (should be nice)
- amarok (won't introduce DCOP dependency, awaiting DBUS interface)
- built-in (via gstreamer)

## Other Backends

Anything GTK+-ish that plays audio and has a D-Bus interface or features any way of being controlled is welcome.
MPRIS2-compatible players can be added via the plugin settings.

---

*Originally by Hakan Erduman, 2010 — Updated 2025*

## README.exaile

**Note:** Exaile is effectively abandoned (last release December 2019).
The current backend may work with newer versions if they support MPRIS.

## README.bmpx

**Note:** BMPx has been discontinued. The backend was dropped.

## README.xubuntu

On some systems, media keys may not work out of the box. If your media keys aren't working:

1. **Check keyboard layout settings** in your desktop environment
2. **Create `~/.Xmodmap`** if needed (use `xev` to find your keycodes):
   ```
   keycode 160=XF86AudioMute
   keycode 162=XF86AudioPlay
   keycode 176=XF86AudioRaiseVolume
   keycode 174=XF86AudioLowerVolume
   keycode 153=XF86AudioNext
   keycode 144=XF86AudioPrev
   keycode 164=XF86AudioStop
   ```
3. **Run xmodmap at startup:** `xmodmap ~/.Xmodmap`

## Build Dependencies

### Debian/Ubuntu
```bash
cmake libgtk-3-dev libxfce4panel-2.0-dev libxfce4ui-2-dev libxfconf-0-dev \
libexo-2-dev libdbus-glib-1-dev libid3tag0-dev keybinder-3.0-dev \
libxfce4windowing-0-dev
```

### Fedora
```bash
cmake gtk3-devel xfce4-panel-devel libxfce4ui-devel xfconf-devel \
exo-devel dbus-glib-devel libid3tag-devel keybinder3-devel
```

## Building

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```
