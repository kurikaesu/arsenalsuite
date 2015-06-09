import os
import sha
import subprocess
import uuid
from PyQt4.QtCore import *

def assetHash(path):
    try: fh=open(path,'rb')
    except IOError: return str()
    except UnicodeDecodeError: return str()
    except MemoryError: return str()
    hash = sha.new()

    size = min(os.path.getsize(path),64000000)
    read = 0
    while read < size:
        fs = fh.read(min(size-read,16384))
        read += len(fs)
        hash.update(fs)

    return hash.hexdigest()

def exifData(path):
    exif = subprocess.Popen(["/drd/software/ext/exiftool/exiftool",path], stdout=subprocess.PIPE).communicate()[0]
    return exif

def getUuid():
    return uuid.uuid1()

def thumbCommand(path, width):
    cachePath = "%s/%s_%sx%s.png" % ("/drd/reference/.thumbnails", path, width,width)
    if not QFile.exists( QFileInfo(cachePath).absolutePath() ):
        QDir().mkpath( QFileInfo(cachePath).absolutePath() )

    # if it's a movie, use ffmpeg
    if( path.toLower().endsWith(".mov") or path.toLower().endsWith(".avi") or path.toLower().endsWith(".mp4") or path.toLower().endsWith(".m4v") ):
        return "ffmpeg.sh '%s' '%s' %s" % (path.toUtf8(), cachePath, str(width))

    # for raw images
    if( path.toLower().endsWith(".nef") or path.toLower().endsWith(".cr2") ):
        return "dcraw.sh '%s' '%s' %s" % (path.toUtf8(), cachePath, str(width))

    # Photoshop files go via Gimp ( yay Scheme )
    if( path.toLower().endsWith(".psd") ):
        return "gimp.sh '%s' %s" % (path.toUtf8(), str(width))

    cmd = QString("convert -flatten -quality 90 -resize %3x%3 \"%1\" \"%2\"")
    return(cmd.arg(path).arg(cachePath).arg(width).toUtf8())

