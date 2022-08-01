#!/bin/bash

if ! [ $# -eq 6 ] ; then
    echo "Usage: obj.sh <objcopy> <output format> <infile> <outfile> <start sym> <size sym>"
    exit 1
fi

OBJCOPY=$1
OFORMAT=$2
FILE=$3
OFILE=$4
START_SYM=$5
SIZE_SYM=$6

FILE_SYM=${FILE//./_}
FILE_SYM=${FILE_SYM//\//_}

START_SYM_DFLT=_binary_${FILE_SYM}_start
SIZE_SYM_DFLT=_binary_${FILE_SYM}_size

echo $FILE_SYM

${OBJCOPY} -I binary -O ${OFORMAT} ${FILE} ${OFILE}  \
    --redefine-sym ${START_SYM_DFLT}=${START_SYM} \
    --redefine-sym ${SIZE_SYM_DFLT}=${SIZE_SYM}