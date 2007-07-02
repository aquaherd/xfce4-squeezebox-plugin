#! /bin/sh
#This is only some mockup until I understand the original, really

echo "Running aclocal............................(1/6)"
aclocal -I /usr/share/xfce4/dev-tools/m4macros

echo "Running autoheader.........................(2/6)"
autoheader -f

echo "Running libtoolize.........................(3/6)"
libtoolize --automake

echo "Running automake ..........................(4/6)"
automake --add-missing

echo "Running autoconf...........................(5/6)"
autoconf -f

echo "Running configure with no parameters.......(6/6)"
./configure
