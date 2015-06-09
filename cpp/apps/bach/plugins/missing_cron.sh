#!/bin/bash

while read e; do export "$e"; done < <(/drd/software/int/bin/launcher.sh --project hf2 --dept rnd --printEnv bach -x /drd/users/barry.robison/drd/core/launcher_xml/trunk)

cd $EPA_WORKINGDIR
python2.5 plugins/find_missing.py | mail -s '[Bach] assets missing' laurie.assender@drdsutdios.com,barry.robison@drdstudios.com

