#!/bin/bash
while read e; do export "$e"; done < <(/drd/software/int/bin/launcher.sh --project hf2 --dept rnd --printEnv bach)

cd /drd/software/int/apps/bach/tn_requests
for uuid in `find /drd/software/int/apps/bach/tn_requests -type f -mmin +2 -not -size 0`; do
echo $uuid
LINES=`wc -l $uuid | cut -f1 -d' '`
/drd/software/int/bin/launcher.sh -p hf2 -d rnd --launchBlocking farm -o EPA_WORKINGDIR $EPA_WORKINGDIR -o EPA_CMDLINE python2.5 --arg plugins/submit_thumb_gen.py --arg $uuid --arg $LINES
done

