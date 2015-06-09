import initbach
import bachutil
from Bach import *
from PyQt4.QtCore import *
import os

def listMissingThumbs(width):
    assets = BachAsset.select("exclude=false")
    for asset in assets:
        path = asset.path()
        cachePath = "%s/%s_%sx%s.png" % ("/drd/reference/.thumbnails", path, width,width)

        if os.path.exists(cachePath):
            continue

        if not QFile.exists( QFileInfo(cachePath).absolutePath() ):
            QDir().mkpath( QFileInfo(cachePath).absolutePath() )

        print bachutil.thumbCommand(path, width)

listMissingThumbs(256)
listMissingThumbs(512)

