#!/bin/bash
# Make curlew.pot file from python source files.
mkdir -p locale
make -C po update-po
make -C po
make -C po install
rm -f po/*.mo
