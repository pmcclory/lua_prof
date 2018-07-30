#! /bin/bash

gdb -p $1 -ex 'set confirm off' -ex 'p globalL' -ex 'quit' | grep '^$1' | awk '{print $5}'
