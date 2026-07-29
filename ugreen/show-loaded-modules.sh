#!/bin/bash

echo "Modules loaded:"
lsmod | grep -E 'ug_|led'

