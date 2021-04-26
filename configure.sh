#!/bin/sh

# Generates Makefile.constants

ME=`basename $0`
USAGE="Usage: $ME [-cc <c++ compiler>] [-debug] [-install <install_path>] [-h]" 
CC='g++'
INSTPATH=
PLAT=`uname -s`
HARDWARE=
BLDPLAT=
DEBUG='-O'

while [ $# -gt 0 ];
do
	case $1 in
		"-cc")
			shift
			if [ $# -gt 0 ]; then
				CC=$1
			else
				echo $USAGE
				exit 1
			fi
			;;

		"-debug")
			DEBUG='-g'
			;;

		"-install")
			shift
			if [ $# -gt 0 ]; then
				INSTPATH=$1
			else
				echo $USAGE
				exit 1
			fi
			;;

		*)
			echo $USAGE
			exit 1
			;;
	esac
	shift
done

if [ "$PLAT" = "Linux" ]; then
	case `uname -m` in
		*x86_64*)
			HARDWARE="x64"
			;;

		*amd64*)
			HARDWARE="x64"
			;;

		*)
			echo "Unsupported hardware $H";
			exit 1
	esac

	BLDPLAT=$PLAT"_"$HARDWARE

	cat > Makefile.constants <<-LINUX_CONFIG
		PLAT = $PLAT
		P = $BLDPLAT
		M = $HARDWARE
		INSTDIR = $INSTPATH
		CC = $CC
		CFLAGS = -c -Wall -std=c++17
		DEFINES = -D_FILE_OFFSET_BITS=64
		LD = $CC -shared
		LDFLAGS =
		AR = ar
		ARFLAGS = -r
		DBG = $DEBUG
		INCLCOM = -I`pwd`/libcom/include
		INCLRDB = -I`pwd`/librdb/include
		INCLJSON = -I`pwd`/libjson/include
		INCLLOG = -I`pwd`/liblog/include
		INCLNET = -I`pwd`/libnet/include
		INCLHTTPCMN = -I`pwd`/http/include/common
		INCLHTTPSRVR = -I`pwd`/http/include/server
		INCLHTTPCLNT = -I`pwd`/http/include/client
		INCLTF = -I`pwd`/tf
		INCLSSL = -I`pwd`/ssl/$BLDPLAT/include -I`pwd`/libsslwrapper/include
		LIBCOM = `pwd`/libcom/src/$BLDPLAT/libcom.a
		LIBRDB = `pwd`/librdb/src/$BLDPLAT/librdb.a
		LIBJSON = `pwd`/libjson/src/$BLDPLAT/libjson.a
		LIBLOG = `pwd`/liblog/src/$BLDPLAT/liblog.a
		LIBSSLWRAPPER = `pwd`/libsslwrapper/src/$BLDPLAT/libsslwrapper.a
		LIBNET = `pwd`/libnet/src/$BLDPLAT/libnet.a
		LIBHTTPCMN = `pwd`/http/src/common/$BLDPLAT/libhttpcmn.a
		LIBHTTPSRVR = `pwd`/http/src/server/$BLDPLAT/libhttpsrvr.a
		LIBHTTPCLNT = `pwd`/http/src/client/$BLDPLAT/libhttpclnt.a
LINUX_CONFIG

else
	echo "Unsupported platform $P";
	exit 1
fi

echo "Makefile.constants generated successfully"

exit 0
