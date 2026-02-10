#!/bin/bash
cd ..
cd build
cmake ..
make
cd ..
cd scripts
../build/main
