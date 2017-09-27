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
		CC = $CC -m64
		CFLAGS = -c -Wall
		DEFINES = -DLINUX -D_POSIX_PTHREAD_SEMANTICS
		LD = $CC -shared -m64
		LDFLAGS = -fpic
		AR = ar
		ARFLAGS = -r
		DBG = $DEBUG
		INCL = -I`pwd`/include
		LIBCOM = `pwd`/libcom/$BLDPLAT/libcom.a
		LIBRDB = `pwd`/librdb/$BLDPLAT/librdb.a
LINUX_CONFIG

elif [ "$PLAT" = "SunOS" ]; then
	HARDWARE=`isainfo -k`

	BLDPLAT=$PLAT"_"$HARDWARE

	cat > Makefile.constants <<-SOLARIS_CONFIG
		PLAT = $PLAT
		P = $BLDPLAT
		M = $HARDWARE
		INSTDIR = $INSTPATH
		CC = $CC
		CFLAGS = -c -mt
		DEFINES = -DSOLARIS
		LD = $CC -G
		LDFLAGS = -Kpic
		AR = ar
		ARFLAGS = -r
		DBG = -O
		INCL = -I`pwd`/include
		LIBCOM = `pwd`/libcom/$BLDPLAT/libcom.a
		LIBRDB = `pwd`/librdb/$BLDPLAT/librdb.a
SOLARIS_CONFIG

else
	echo "Unsupported platform $P";
	exit 1
fi

echo "Makefile.constants generated successfully"

exit 0
