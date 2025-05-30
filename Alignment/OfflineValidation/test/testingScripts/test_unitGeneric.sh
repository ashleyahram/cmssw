#! /bin/bash

function die { echo $1: status $2 ; exit $2; }

echo "TESTING Alignment/Generic single configuration with json..."
pushd test_yaml/GenericV/single/testUnits/unitTest/1/
./cmsRun validation_cfg.py config=validation.json || die "Failure running Generic single configuration with json" $?

echo "TESTING Alignment/Generic single configuration standalone..."
./cmsRun validation_cfg.py || die "Failure running PV single configuration standalone" $?
popd

echo "TESTING SplotV merge step"
pushd test_yaml/GenericV/merge/testUnits/1/
./GenericVmerge validation.json --verbose || die "Failure running PV merge step" $?
popd
