#!/bin/bash

make
RES=$?
if [ $RES -eq 0 ]; then
	cp screen.uf2 /media/weetsy/RPI-RP2/
else
	echo "Failed compile!"
fi
