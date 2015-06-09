#!/usr/bin/python2.5
import sys
import initbach
from DirectoryImport import DirectoryImport

importTag = sys.argv[2]

di = DirectoryImport()
di.doGather( sys.argv[1], True )
print "found %s files to import" % len(di.files)
di.setVerbose(True)

tags = []
for tag in importTag.split(","):
    if not len(tag) > 1: continue
    bachKeyword = BachKeyword.recordByName(tag)
    if bachKeyword.isRecord():
        tags.append(bachKeyword)
    else:
        keyword = BachKeyword()
        keyword.setName(tag)
        keyword.commit()
        tags.append(keyword)

di.doImport( di.files, tags )

