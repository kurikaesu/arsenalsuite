#!/bin/bash

files=$(find . -type f | grep -v "installScripts.sh" | grep -v "build.py" | grep -v "__init__.py")

directories=$(find . -type d )

echo "Installing scripts to $DESTDIR/usr/local/share/ab/"
mkdir $DESTDIR/usr/local/share 2> /dev/null
mkdir $DESTDIR/usr/local/share/ab 2> /dev/null

for dir in $directories; do
    if [ $dir != "." ]; then
        mkdir $DESTDIR/usr/local/share/ab/$dir 2> /dev/null
    fi
done

for file in $files; do
    cp -f $file $DESTDIR/usr/local/share/ab/$file
done

configFiles=$(find ../scriptsini -type f -iname "*.ini" | grep -Eo "\w+\.ini$")

echo "Installing configuration files to $DESTDIR/etc/ab/"
mkdir $DESTDIR/etc/ab 2> /dev/null

for file in $configFiles; do
    cp -f ../scriptsini/$file $DESTDIR/etc/ab/$file
done
