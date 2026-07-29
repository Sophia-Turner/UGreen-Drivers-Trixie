#!/bin/bash

make -C /lib/modules/$(uname -r)/build M=$(pwd) modules_install

depmod -a

