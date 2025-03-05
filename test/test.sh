#!/bin/sh
export PATH="..:../build:$PATH"
for o in inputs/outlets*.shp; do
	l=$(basename $o | sed 's/outlets/lfp/; s/shp/txt/')
	melfp -l inputs/fdr.tif $o cat outputs/$l
done
