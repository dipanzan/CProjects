#!/bin/bash
cd /tmp/WRF
echo "Staring Download"
wget http://www2.mmm.ucar.edu/wrf/src/wps_files/geog_complete.tar.gz
cd ./geog
./configure
./compile 
