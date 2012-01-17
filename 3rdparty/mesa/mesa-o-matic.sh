#!/bin/bash
# Mesa-O-Matic
# Copyright 2011, Alexander von Gluck IV

# This script creates an optional package containing the
# the headers and binary code needed by the opengl kit
# to link libGL.so

# This script is run against a *COMPILED* Mesa source tree

echo " Welcome to Mesa-O-Matic!"
echo "-=-=-=-=-=-=-=-=-=-=-=-=-="
echo ""

# These are the Mesa headers and libraries used by the opengl kit
#   Headers are probed for dependencies, only specify ones referenced
#   by the opengl kit.
MESA_PRIVATE_HEADERS="glheader.h glapi.h glapitable.h glapitemp.h glapi_priv.h context.h driverfuncs.h meta.h colormac.h buffers.h framebuffer.h renderbuffer.h state.h version.h swrast.h swrast_setup.h tnl.h t_context.h t_pipeline.h vbo.h common_x86_asm.h common_ppc_features.h extensions.h s_spantemp.h s_renderbuffer.h formats.h cpuinfo.h"

#######################################################################
# END CONFIG DATA, Dragons below!
#######################################################################

if [[ $( uname ) != "Haiku" ]]; then
	echo "*************************************"
	echo " I need to be run on a Haiku system!!"
	echo "*************************************"
	exit 1
fi

if [[ -z $1 ]]; then
	echo ""
	echo "Usage: $0 <compiled mesa location>"
	echo ""
	exit 1
fi

MESA_TOP="$1"

GCC_VER=`gcc -v 2>&1 | tail -1 | awk '{print $3}' | cut -d. -f1`
MESA_VER=`cat $MESA_TOP/Makefile |grep VERSION\= | cut -d= -f2`

echo "Bundling gcc$GCC_VER build of Mesa $MESA_VER..."

cd $MESA_TOP

#######################################################################
# Create Mesa optional pacakge

findInTree() {
	RESULT=`find . -name "$1"`
	if [[ $? -ne 0 || -z $RESULT ]]; then
		echo "$i"
		exit 1
	fi
	echo $RESULT
}

# Directories to search for matching headers
MESA_INCLUDES="-I./include -I./src -I./src/mapi -I./src/mesa"

ZIP_HEADERS=""
echo "Collecting required Mesa private headers..."
for i in $MESA_PRIVATE_HEADERS
do
	FOUND=$(findInTree $i)
	if [[ $GCC_VER -eq 2 ]]; then
		# gcc2 isn't very good at -MM
		setgcc gcc4
	fi
	HEADERS_RAW=`gcc -MM $MESA_INCLUDES $FOUND`
	if [[ $GCC_VER -eq 2 ]]; then
		setgcc gcc2
	fi

	for y in $( echo "$HEADERS_RAW" | cut -d':' -f2 | sed 's/\\//g' | tr -d '\n' )
	do
		CLEAN_HEADER=$( echo "$y" | grep -v "include/GL" )
		ZIP_HEADERS="$ZIP_HEADERS $CLEAN_HEADER"
	done
done

echo "Collecting required Mesa libraries..."
rm -rf lib.haiku
mkdir -p lib.haiku
for i in $( find . -name "*.a" ) 
do
	cp $i lib.haiku/
done

# Disabled as to give the choice to the user ah-la stripOptionalPackageDebugSymbols
#echo "Stripping debug symbols from Mesa libraries..."
#find lib.haiku -exec strip --strip-debug {} \; ;

echo "Creating Mesa OptionalPackage..."
PLATFORM=$( uname -m )
ZIP_FILENAME="../mesa-$MESA_VER-gcc$GCC_VER-x86.zip"
zip -r -9 $ZIP_FILENAME $ZIP_HEADERS ./include/GL/* ./lib.haiku/*

echo "Great Success! $ZIP_FILENAME created."