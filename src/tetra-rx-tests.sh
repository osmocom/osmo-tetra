#!/bin/bash

if [ $# -lt 1 ]; then
	echo "$0"
	echo "Runs tetra-rx on bit files provided as args, prints the number of correct frames"
	echo " and the time it took and compares it to previous runs."
	echo ""
	echo "Extra options:"
	echo "	-n <experiment name> (default: git head id) (no spaces and \"-\" please)"
	echo "	-o overwrite previous measurement"
	echo "	-t parameters for tetra-rx"
	echo ""
	echo "Examples:"
	echo "$0 path/to/file.bits"
	echo "$0 -n \"something changed\" path/to/file.bits path/to/file2.bits"
	exit 1
fi

if ! command -v bc > /dev/null; then
	echo "Please install \"bc\""
fi

TESTS_DIR="tests_data/"
mkdir -p "$TESTS_DIR"

n=`git rev-parse HEAD`
o=0
t=" "
while getopts ":n:ot:" opt; do
	case $opt in
	n)
		n="$OPTARG"
		;;
	o)
		o=1
		;;
	t)
		t="$OPTARG"
		;;
	\?)
		echo "Unknown option $OPTARG" >&2
		exit 1
		;;
	:)
		echo "-$OPTARG requires argument" >&2
		exit 1
		;;
	esac
done

shift $(( $OPTIND - 1 ))

tmpdir=`mktemp -d /tmp/tetraXXX`

for f in $@; do
	corrects=`"time" -o "$tmpdir/time" ./tetra-rx "$f" $t 2>/dev/null | grep -E "^CRC COMP: 0x.+ OK" | wc -l`
	tt=`grep user "$tmpdir/time" | head -n 1 | cut -d u -f 1`
	echo "$f: $corrects frames, $tt s"
	hash=`sha256sum "$f" | cut -c 1-20`
	fnb="$TESTS_DIR/rx-$hash-"
	for meas in "$fnb"*; do
		if ! [ -s "$meas" ]; then
			continue
		fi
		tag=`echo "$meas" | rev | cut -d - -f 1 | rev`
		if [ "$tag" = "$n" ]; then
			continue
		fi
		pf=`cat "$meas" | cut -d " " -f 1`
		pt=`cat "$meas" | cut -d " " -f 2`
		deltaf=`echo "scale=7; $corrects/$pf" | bc -l`
		deltat=`echo "scale=3; $tt/$pt" | bc -l`
		echo "... ${deltaf}x frames than $tag"
		echo "... ${deltat}x time than $tag"
	done
	fn="$fnb$n"
	if [ "$o" -eq 1 -o ! -s "$fn" ]; then
		echo "$corrects $tt" > "$fn"
	fi
done

rm -r "$tmpdir"
