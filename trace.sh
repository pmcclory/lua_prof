#! /bin/bash

PID=`pidof lua`
ADDR=`./get_globall.sh $PID`

./tracer $PID $ADDR
