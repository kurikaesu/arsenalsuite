from bachutil import *
import initbach
from Bach import *
from PyQt4.QtCore import *
import os
import sha

assets = BachAsset.select("exclude=false AND hash IS NULL")
print "found %s assets" % assets.size()
for asset in assets:
    path = asset.path()
    asset.setHash(assetHash(path))
    asset.commit()
    try: print "%s %s" % ( asset.hash(), path )
    except UnicodeDecodeError: pass

