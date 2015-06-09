mkdir snapshot
rsync --delete --exclude=*.o --exclude=ffmpeg --exclude=ImageMagick* --exclude=.out --exclude=*pyc --exclude=*.so* --exclude=.svn --exclude=trax --exclude=headshots --exclude=autocore --exclude=**cpp/plugins** --exclude=**apps/dist** --exclude=**apps/activex** --exclude=**apps/blurt** --exclude=**cpp/docs/html** --exclude=annotate --exclude=annotateserver --exclude=**apps/assburner/** --exclude=**apps/qp** --exclude=**apps/epa** --exclude=resin --exclude=siren --exclude=**cpp/docs/** -rtv ./cpp snapshot/
rsync -rtv --delete --exclude=trax --exclude=.svn --exclude=*pyc ./python snapshot/
rsync -rtv --delete --exclude=.svn ./binaries/mingw-* snapshot/binaries/
rsync -rtv --delete --exclude=.svn ./sql snapshot/
rsync -rtv --delete --exclude=.svn ./bash snapshot/
rsync -rtv --delete --exclude=.svn COPYING snapshot/
rsync -rtv --delete --exclude=.svn build.py snapshot/

tar -cvf snapshot.tar snapshot/
gzip snapshot.tar
