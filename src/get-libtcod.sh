#!/usr/bin/env bash
cd `dirname $0`
wget -c http://doryen.eptalys.net/?file_id=26 -O libtcod-1.5.1-linux.tar.gz
tar -xf libtcod-1.5.1-linux.tar.gz
cp libtcod-1.5.1/libtcod.so ../bin

