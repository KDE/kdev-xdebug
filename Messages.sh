#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.rc` `find . -name \*.ui` >> rc.cpp
$XGETTEXT `find . -name \*.cpp -o -name \*.h` -o $podir/kdevxdebug.pot
rm -f rc.cpp
