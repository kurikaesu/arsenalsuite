import initbach
from Bach import *
import os
import sys

# get the collection for assets gone missing
missingList = BachBucket.select("name='ZZZ_Missing'")
if missingList.size() > 0:
    missing = missingList[0]
else:
    missing = BachBucket()
    missing.setName("ZZZ_Missing")
    missing.commit()

all = BachAsset.select("exclude=false")
for asset in all:
    try:
        if not os.path.exists( asset.path() ):
            print "%s" % asset.path()
            bbm = BachBucketMap()
            bbm.setBachAsset( asset )
            bbm.setBachBucket( missing )
            bbm.commit()
            asset.setExclude(True)
            asset.commit()
    except UnicodeDecodeError:
        bbm = BachBucketMap()
        bbm.setBachAsset( asset )
        bbm.setBachBucket( missing )
        bbm.commit()
        asset.setExclude(True)
        asset.commit()
        pass

