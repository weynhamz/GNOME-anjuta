#!/bin/sh
for file in *.png; do
  name=`echo $file | sed 's/\.png$//'`
  pngtopnm $file >$name.pnm && pnmtops -nocenter -nosetpage $name.pnm >$name.eps && rm $name.pnm
done

