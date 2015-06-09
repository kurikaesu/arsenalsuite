
import initbach
from blur.Stone import *

Database.current().exec_('''
UPDATE bachasset 
SET width = COALESCE(
                SUBSTRING(exif from E'Image Width\\\\s*:\\\\s*(\\\\d+)')::int, 
                width, 
                0),
    height = COALESCE(
                SUBSTRING(exif from E'Image Height\\\\s*:\\\\s*(\\\\d+)')::int, 
                height, 
                0)
                WHERE exclude=false AND exif LIKE 'ExifTool%'
                ''')

