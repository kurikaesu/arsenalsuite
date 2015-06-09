if [ "v$V" == "v" ]; then
V=`date +%Y%m%d%H%M`
fi

if [ `uname -s` == "Linux" ]; then
ARCH="lin64"
PYPATH="/usr/lib/python2.5/site-packages"
SIPPATH="/usr/share/sip"
else
ARCH="osx"
PYPATH="/Library/Python/2.5/site-packages"
SIPPATH="/System/Library/Frameworks/Python.framework/Versions/2.5/share/sip"
fi

echo '***' release stone libs
STONEDIR=/drd/software/ext/stone/$ARCH/$V
mkdir $STONEDIR
rsync -avc $DESTDIR/usr/local/lib/libfreezer* $STONEDIR/
rsync -avc $DESTDIR/usr/local/lib/libabsubmit* $STONEDIR/
rsync -avc $DESTDIR/usr/local/lib/libstone* $STONEDIR/
rsync -avc $DESTDIR/usr/local/lib/libclasses* $STONEDIR/
rsync -avc $DESTDIR/usr/local/lib/libbrainiac* $STONEDIR/
rsync -rvc --exclude=.svn cpp/lib/stone/include $STONEDIR/
rsync -rvc --exclude=.svn cpp/lib/stone/.out/*.h $STONEDIR/include/
rsync -rvc $DESTDIR/usr/bin/sip $STONEDIR/sip
rsync -rvc --exclude=.svn $SIPPATH/ $STONEDIR/sip.include/
rsync -rvc --exclude=.svn cpp/lib/stonegui/include $STONEDIR/
rsync -rvc --exclude=.svn cpp/lib/stonegui/.out/*.h $STONEDIR/include/

rsync -vc cpp/apps/classmaker/classmaker $STONEDIR/
rsync -vc cpp/apps/classmaker/classmaker.ini $STONEDIR/
mkdir $STONEDIR/templates
rsync -rvc cpp/apps/classmaker/templates/* $STONEDIR/templates/

echo '***' release python libs
rsync -rcv --exclude=wx* $DESTDIR/$PYPATH/blur/ /drd/software/ext/python/$ARCH/2.5/stone/$V/blur/
rsync -rcv $DESTDIR/$PYPATH/sipconfig.py /drd/software/ext/python/$ARCH/2.5/stone/$V/
rsync -rcv $DESTDIR/$PYPATH/stoneconfig.py /drd/software/ext/python/$ARCH/2.5/stone/$V/

echo '***' release applications
DIR=/drd/software/ext/ab/$ARCH/$V
mkdir $DIR
rsync -cv cpp/apps/burner/burner $DIR/ab
rsync -cv cpp/apps/burner/burner.ini $DIR/ab.ini
rsync -cv cpp/apps/burner/ab-offline.py $DIR/
rsync -rvc --exclude=.svn cpp/apps/burner/plugins/ $DIR/plugins/
rsync -cv cpp/apps/freezer/af $DIR/af
rsync -cv cpp/apps/freezer/freezer.ini $DIR/freezer.ini
rsync -rvc --exclude=.svn cpp/apps/freezer/afplugins/ $DIR/afplugins/
rsync -rvc --exclude=.svn cpp/apps/freezer/resources $DIR/

rsync -cv cpp/apps/absubmit/absubmit $DIR/
rsync -cv cpp/apps/absubmit/py2ab.py $DIR/
rsync -cv cpp/apps/absubmit/maya/*py $DIR/
rsync -cv cpp/apps/absubmit/maya/*ui $DIR/
rsync -cv cpp/apps/absubmit/nukesubmit/nuke2AB.py $DIR/nukesubmit/
rsync -cv cpp/apps/absubmit/nukesubmit/*ui $DIR/nukesubmit/

rsync -cv python/scripts/manager.py $DIR/
rsync -cv python/scripts/manager.ini $DIR/
rsync -cv python/scripts/reaper.py $DIR/
rsync -cv python/scripts/verifier.py $DIR/
rsync -cv python/scripts/verifier.ini $DIR/
rsync -cv python/scripts/verifier_plugin_factory.py $DIR/
rsync -rcv python/scripts/verifier_plugins/ $DIR/verifier_plugins/
rsync -cv python/scripts/reclaim_tasks.py $DIR/
rsync -cv python/scripts/initab.py $DIR/

ln -s /drd/software/ext/ab/images $DIR/images

echo '***' released $V

