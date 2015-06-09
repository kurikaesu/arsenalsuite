
import initbach
from blur.Stone import *

Database.current().exec_('''
UPDATE bachasset 
SET aperture = COALESCE(
                SUBSTRING(exif from E'\\\nAperture Value\\\\s*:\\\\s*([.\\\\d]+)')::float,
                aperture,
                0),
    focallength = COALESCE(
                SUBSTRING(exif from E'\\\nFocal Length\\\\s*:\\\\s*([.\\\\d]+)')::float,
                focallength,
                0),
    isospeedrating = COALESCE(
                SUBSTRING(exif from E'\\\nISO\\\\s*:\\\\s*([.\\\\d]+)')::int,
                isospeedrating,
                0),
    shutterspeed = COALESCE(
                SUBSTRING(exif from E'\\\nShutter Speed\\\\s*:\\\\s*1/([.\\\\d]+)')::float,
                shutterspeed,
                0)
                WHERE exclude=false AND exif LIKE 'ExifTool%'
                ''')

#    creationdatetime = COALESCE(
#                to_timestamp( SUBSTRING(exif from E'\nModify Date\\\\s*:\\\\s*([^\n+]+)'), 'Y:MM:DD HH24:MI:SS'),
#                creationdatetime
#                )
