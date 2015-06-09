from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *

from verifier_plugin_factory import *

def doThis(job):
    # automatically suspend job if assets use UNC paths or if there are 
    # any mapped drives used besides G:\ (ie, job uses assets from K: or S:)
    badpaths, badpaths2 = [], []
    if QFile.exists(filePathExtra):
        file = open(filePathExtra,'r')

        try:
            # BLUR SPECIFIC HACK
            # Get mappings for G, should be 0 or 1
            gMappings = job.jobType().jobTypeMappings().mappings().filter( "mount", QRegExp("g",Qt.CaseInsensitive) )
            # If we have a G mapping and it's a mirror(uses hostmapping instead of fkeyhost for multiple mirrors)
            usingGMirror = (gMappings.size() == 1 and not gMappings[0].host().isRecord())
            validGPathRegs = []
            if usingGMirror:
                validGPathRegs.append( QRegExp( "/_master/", Qt.CaseInsensitive ) )
                validGPathRegs.append( QRegExp( "/maps/", Qt.CaseInsensitive ) )
                validGPathRegs.append( QRegExp( "/(?:050_)maps/", Qt.CaseInsensitive ) )
                validGPathRegs.append( QRegExp( "/Texture/" ) )
                validGPathRegs.append( QRegExp( "/(?:050_)maps/", Qt.CaseInsensitive ) )
                validGPathRegs.append( QRegExp( "/read/" ) )

                            #rsync -W --numeric-ids -av --progress --stats --timeout=900 --delete 
                            #--dry-run --include '*/' --exclude '*.[Pp][Ss][Dd]' --exclude 'temp/' 
                            #--include '*.rps' --include '*_MASTER.max' --include '*.flw' --include 
                            #'**/_MASTER/**' --include' **/_Master/**' --include 
                            #'**/[Mm][Aa][Pp][Ss]/**' --include '**/050_maps/ **' --include 
                            #'**/Texture/**' --include '**/read/*pc2' --include '**/read/*tmc' 
                            #--include '**/read/*mdd' --include '**/Mesh*bin' --exclude '*' 
        except:
            print "Exception while setting up regexs for valid g maps mirror path tests"
            traceback.print_exc()

        for line in file:
            line = line.strip()
            if re.match('\#',line): continue
            if re.match('mem',line): continue
            if re.match('(map|pc|file)',line):
                try:
                    # Format per line is
                    # filetype,refcount,size,path
                    # Because path can contain commas, we set maxsplit to 3, to avoid splitting path
                    filetype, refcount, size, path = line.split(',',3)
                    if re.match('(\\\\|\/\/)',path,re.I):
                        badpaths.append(path)
                    # Create a RE from the paths, in the format (G:|V:)
                    mapDriveRe = '(%s)' % ('|'.join( [str(s) for s in config.permissableMapDrives.split(',') ] ) )
                    if re.match('([a-z]\:)',path,re.I) and not re.match(mapDriveRe,path,re.I):
                        badpaths.append(path)

                    # BLUR SPECIFIC HACK
                    # If we are using a G: mirror, check if maps are in proper directories
                    try:
                        if usingGMirror:
                            valid = False
                            for re_ in validGPathRegs:
                                if path.contains( re_ ):
                                    valid = True
                                    break
                            if not valid:
                                badpaths2.append(path)
                    except:
                        print "Exception while testing for valid g mirror paths"
                        traceback.print_exc()
                except:
                    print "Invalid Line Format in Stats File:", line

    if badpaths2:
        subject = 'Job contains bad maps, pc2 or xref paths'
        print subject, job.key(), job.name()
        body = "Job: %s\n" % job.name()
        body += "Job ID: %i\n" % job.key()
        body += "User: %s\n" % job.user().displayName()
        body += "This job contains maps, pointcache or xrefs that\n"
        body += "point to locations that can not be reached by render nodes. Please\n"
        body += "make sure your maps are stored with the project on %s.\n"
        body += "Jobs that use maps via UNC paths such as \\\\thor or \\\\snake,"
        body += "or through drive letters other than %s will NOT render on the farm.\n\n"
        body += "List of invalid paths:\n"
        body = body % (config.permissableMapDrives, config.permissableMapDrives)
        for path in badpaths:
            body += path + "\n"
            print path
        notifySend( notifyList = "newellm:j", subject = subject, body = body )

    if badpaths:
        subject = 'Job contains bad maps, pc2 or xref paths'
        print subject, job.key(), job.name()
        body = "Job: %s\n" % job.name()
        body += "Job ID: %i\n" % job.key()
        body += "User: %s\n" % job.user().displayName()
        body += "This job contains maps, pointcache or xrefs that\n"
        body += "point to locations that can not be reached by render nodes. Please\n"
        body += "make sure your maps are stored with the project on %s.\n"
        body += "Jobs that use maps via UNC paths such as \\\\thor or \\\\snake,"
        body += "or through drive letters other than %s will NOT render on the farm.\n\n"
        body += "List of invalid paths:\n"
        body = body % (config.permissableMapDrives, config.permissableMapDrives)
        for path in badpaths:
            body += path + "\n"
            print path

        job.setStatus('suspended')
        job.commit()
        notifylist = "it:e,%s:je" % (job.user().name())
        notifySend( notifyList = notifylist, subject = subject, body = body )
        return False
    return True

VerifierPluginFactory().registerPlugin("bad-paths", doThis)
