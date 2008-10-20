#!/bin/sh
for pl in *-player-binding.xml; do
    plx=$(echo $pl|cut -d- -f1)
    echo $plx
    dbus-binding-tool --prefix=client_object --mode=glib-client ./$plx-player-binding.xml>./$plx-player-binding.h
done
