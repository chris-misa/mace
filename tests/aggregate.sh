#!/bin/bash

#
# Per-file merge on given argument
#
# Assumes all given directory structures are identical
#

DEST="aggregated_results"

mkdir $DEST || { echo "$DEST already exists!"; exit; }

for path in $@
do

  echo Reading from $path

  cp ${path}/manifest ${DEST}/manifest

  for i in `cat ${path}/manifest`
  do
    mkdir -p ${DEST}/${i}
    cp ${path}/${i}/manifest ${DEST}/${i}/manifest

    for file in `cat ${path}/${i}/manifest`
    do
      cat ${path}/${i}/${file} >> ${DEST}/${i}/${file}
    done
  done
done
