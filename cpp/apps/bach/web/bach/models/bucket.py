#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: bucket.py 9408 2010-03-03 22:35:49Z brobison $"
#

from sqlalchemy import Column, Table, types, ForeignKey, Index
from sqlalchemy.orm import relation, backref
from ..config import mapper, metadata
from .asset import Asset

class Bucket( object ):
    def __init__( self ):
        self.keybachbucket = None
        self.name = None

    @property
    def asset_count(self):
        return len(self.assets)

    def __repr__( self ):
        return '<%s:%s:%s>' % ( self.__class__.__name__, self.keybachbucket, self.name )

table = Table( 'bachbucket', metadata,
               Column( 'keybachbucket', types.Integer, primary_key=True ),
               Column( 'name', types.String, nullable=False ) )

join_table = Table( 'bachbucketmap', metadata,
                    Column( 'fkeybachbucket', types.Integer, ForeignKey( 'bachbucket.keybachbucket' ) ),
                    Column( 'fkeybachasset',  types.Integer, ForeignKey( 'bachasset.keybachasset' ) ),
                    Column( 'position', types.Integer) )

mapper( Bucket, table,
        properties={
                    'assets':relation( Asset,
                                       secondary=join_table,
                                      # backref='buckets'
                                      order_by='position'
                                     ),


                    } )
