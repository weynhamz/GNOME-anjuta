#!/bin/bash
TEST_DIR=./test-dir
rm -fr $TEST_DIR
mkdir $TEST_DIR
for i in `find ../../../ -name "*.[c|h]"`; do 
cp $i $TEST_DIR
done;

