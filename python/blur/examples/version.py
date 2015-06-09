#!/usr/bin/python

import re, sys

versionPattern = "[vV]_?(\d\d?)[-_](\d\d?)"

def parseVersion( filename ):
	return [int(i) for i in re.search(versionPattern,filename).groups()]

def replaceVersion( filename, version, iteration ):
	match = re.search(versionPattern,filename)
	if len(match.groups()) == 2:
		return filename[:match.start(1)] + '%02i' % int(version) + filename[match.end(1):match.start(2)] + '%02i' % int(iteration) + filename[match.end(2):]
	return None

def addVersionIteration( filename, versionDelta, iterationDelta ):
	v, i = parseVersion( filename )
	return replaceVersion( filename, v + versionDelta, i + iterationDelta )

incrementVersion = lambda filename: addVersionIteration( filename, 1, 0 )
incrementIteration = lambda filename: addVersionIteration( filename, 0, 1 )

print parseVersion( sys.argv[1] )
print replaceVersion( sys.argv[1], 2, 4 )
print incrementVersion( sys.argv[1] )
print incrementIteration( sys.argv[1] )
