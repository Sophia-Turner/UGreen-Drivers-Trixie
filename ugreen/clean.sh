#!/bin/bash

find . -type f -name '*.o'  -print -exec rm {} \;
find . -type f -name '*.ko' -print -exec rm {} \;
find . -type f -name '.Mod*' -print -exec rm {} \;
find . -type f -name '.it5*' -print -exec rm {} \;
find . -type f -name '.led*' -print -exec rm {} \;
find . -type f -name '.mod*' -print -exec rm {} \;
find . -type f -name '.ug_*' -print -exec rm {} \;
find . -type f -name '.ug-*' -print -exec rm {} \;
find . -type f -name '..mod*' -print -exec rm {} \;
find . -type f -name '*.mod' -print -exec rm {} \;
find . -type f -name 'modules.order' -print -exec rm {} \;
rm Module.symvers
