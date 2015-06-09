#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: http://svn/drd/apps/bach/trunk/web/bach/models/asset.py $"
# SVN_META_ID = "$Id: asset.py 9408 2010-03-03 22:35:49Z brobison $"
#

from sqlalchemy import Column, Table, types, ForeignKey, Index
from ..config import mapper, metadata

class Asset( object ):
	def __init__( self ):
		self.keybachasset = None
		self.path = None
		self.cachedkeywords = None
		self.directory = None
		self.exclude = False
		
		self.width = -1
		self.height = -1
		self.creationdatetime = None
		self.importeddatetime = None

	def __repr__( self ):
		justfilename = self.path[ len( self.directory ) : ]
		return '<%s:%s:%s>' % ( self.__class__.__name__, self.keybachasset, justfilename )


table = Table( 'bachasset', metadata,
				Column( 'keybachasset', types.Integer, primary_key=True ),
				Column( 'path', types.String, nullable=False ),
				Column( 'cachedkeywords', types.String, nullable=False ),
				Column( 'directory', types.String, nullable=False ),
				Column( 'exclude', types.Boolean, nullable=False ),
				
				Column( 'width', types.Integer, nullable=False ),
				Column( 'height', types.Integer, nullable=False ),
				
				Column( 'creationdatetime', types.TIMESTAMP, nullable=False ),
				Column( 'importeddatetime', types.TIMESTAMP, nullable=False )
				)

mapper( Asset, table )
