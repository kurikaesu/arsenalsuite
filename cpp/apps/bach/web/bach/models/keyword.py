#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: keyword.py 9408 2010-03-03 22:35:49Z brobison $"
#

from sqlalchemy import Column, Table, types, ForeignKey, Index
from sqlalchemy.orm import relation, backref
from ..config import mapper, metadata
from .asset import Asset

class Keyword( object ):
    def __init__( self ):
        self.keybachkeyword = None
        self.name = None

    @property
    def asset_count(self):
        return 0 #len(self.assets)

    def __repr__( self ):
        return '<%s:%s:%s>' % ( self.__class__.__name__, self.keybachkeyword, self.name )

table = Table( 'bachkeyword', metadata,
               Column( 'keybachkeyword', types.Integer, primary_key=True ),
               Column( 'name', types.String, nullable=False ) )

join_table = Table( 'bachkeywordmap', metadata,
                    Column( 'fkeybachkeyword', types.Integer, ForeignKey( 'bachkeyword.keybachkeyword' ) ),
                    Column( 'fkeybachasset',  types.Integer, ForeignKey( 'bachasset.keybachasset' ) ) )

mapper( Keyword, table,
        properties={
                    'assets':relation( Asset,
                                       secondary=join_table,
                                      # backref='buckets'
                                     ),


                    } )
