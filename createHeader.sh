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
JAVAC=javac
JAVAH=javah
LIBDIR=libbson
WORKING_PATH=src/main/java/
WRAPPER_PATH=org/benews/
BUILD_TYPE=debug
WRAPPER_CLASS_PATH=build/intermediates/classes/$BUILD_TYPE/
WRAPPER_PKG="${WRAPPER_PATH//\//.}"
WRAPPER_NAME=BsonBridge
WRAPPER_HEADER_SFX="${WRAPPER_PATH//\//_}"
WRAPPER_HEADER=$WRAPPER_HEADER_SFX$WRAPPER_NAME.h
WRAPPER_CLASS_JAVA="$WRAPPER_NAME.java"
WRAPPER_CLASS_SRC=$WRAPPER_CLASS_JAVA
JNI_INCLUDE=src/$LIBDIR/include
S=$(pwd)
cd $WORKING_PATH
#$JAVAC -d . $WRAPPER_PATH/$WRAPPER_CLASS_JAVA
echo $JAVAH -classpath $S/$WRAPPER_CLASS_PATH -jni $WRAPPER_PKG$WRAPPER_NAME/
$JAVAH -classpath $S/$WRAPPER_CLASS_PATH -jni $WRAPPER_PKG$WRAPPER_NAME
mv $WRAPPER_HEADER  $S/$JNI_INCLUDE/
cd $S
echo "#include \"$WRAPPER_HEADER_SFX$WRAPPER_NAME.h\"" > $JNI_INCLUDE/bson.h
echo "HEADER CREATED!!:"

