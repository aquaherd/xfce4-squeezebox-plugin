#!/bin/sh
for pl in *-player-binding.xml; do
    plx=$(echo $pl|cut -d- -f1)
    echo $plx
    dbus-binding-tool --mode=glib-client --prefix=$plx --ignore-unsupported ./$plx-player-binding.xml --output=$plx-player-binding.h
done
