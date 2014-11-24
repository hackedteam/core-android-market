#!/bin/bash -
#===============================================================================
#
#          FILE: build.sh
#
#         USAGE: ./build.sh
#
#   DESCRIPTION:
#
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: zad (), e.placidi@hackingteam.com
#  ORGANIZATION: ht
#       CREATED: 16/10/2014 09:34:23 CEST
#      REVISION:  ---
#===============================================================================
START_DIR=$PWD
KEY="ciao mondo"
./createHeader.sh
echo "utils/encript.py encstring src/libbson/ src/libbson/preprocessed \"$KEY\" utils/tfc"
python utils/encript.py encstring src/libbson/ src/libbson/preprocessed "$KEY" utils/tfc
cd jni
ndk-build
cd -

