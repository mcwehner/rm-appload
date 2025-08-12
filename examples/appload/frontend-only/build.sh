#!/bin/bash
rm -rf output
mkdir output
cp icon.png manifest.json output
rcc --binary -o output/resources.rcc application.qrc
