#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: artref.py 9408 2010-03-03 22:35:49Z brobison $"
#

from sevan import controller
from ..config import Session
from ..models.asset import Asset
from ..models.bucket import Bucket
from ..models.keyword import Keyword
from urllib import unquote

#-----------------------------------------------------------------------------
class Artref( controller.Controller ):

#-----------------------------------------------------------------------------
    channel = '/'

#-----------------------------------------------------------------------------
    def __init__( self ):
        super( Artref, self ).__init__()

#-----------------------------------------------------------------------------
    def index( self, *args, **kwargs ):
        return self.to_html( { 'name':'web.bach' }, template='index' )

#-----------------------------------------------------------------------------
    def image( self, uid ):
        sess = Session()
        res = { 'name':'web.bach' }
        queried = sess.query( Asset ).filter_by( keybachasset=uid )
        if queried.count() == 0:
            res[ 'msg' ] = 'No result'
            return self.to_html( res, template=templateName )

        res[ 'asset' ] = queried.one()
        if res[ 'asset' ].cachedkeywords is None:
            res[ 'asset' ].cachedkeywords = "<no keywords>"
        return self.to_html( res, template='image' )

#-----------------------------------------------------------------------------
    def collections( self, *args, **kwargs ):
        sess = Session()
        res = sess.query( Bucket ).order_by( Bucket.name ).all()
        return self.to_html( { 'collections':res, 'name':'web.bach' }, template='collections' )

#-----------------------------------------------------------------------------
    def keywords( self, *args, **kwargs ):
        sess = Session()
        res = sess.query( Keyword ).order_by( Keyword.name ).all()
        return self.to_html( { 'keywords':res, 'name':'web.bach' }, template='keywords' )

#-----------------------------------------------------------------------------
    def returnImages( self, res, queried, partId, start, pp, pv, ts ):
        if queried.count() == 0:
            res[ 'msg' ] = 'No result'
            return self.to_html( res, template=templateName )

        start = int( start )
        pp = int( pp )
        ts = int( ts )
        if ts != 256 and ts != 512:
            ts = 256

        container = queried.one()
        assets = [ a for a in container.assets[ start:start+pp ] if a.exclude == False ]
        for a in assets:
            if a.cachedkeywords is None:
                a.cachedkeywords = "<no keywords>"

        assets_count = len( container.assets )
        prev_page = max( start-pp, 0 )
        next_page = min( start+pp, assets_count )

        res[ 'do_pagination' ] = True
        res[ 'show_prev' ] = start>0
        res[ 'show_next' ] = next_page<assets_count
        res[ 'start' ] = start
        res[ 'pp' ] = pp
        res[ 'prev_page' ] = prev_page
        res[ 'next_page' ] = next_page
        res[ 'assets_count' ] = assets_count
        res[ 'partId' ] = unquote( partId )
        res[ 'name' ] = container.name
        res[ 'assets' ] = assets
        res[ 'route' ] = 'images'
        res[ 'do_print_view' ] = pv == '1'
        res[ 'pv' ] = pv
        res[ 'ts' ] = ts

        result = self.to_html( res, template='images' )
        # Session.remove()
        return result

#-----------------------------------------------------------------------------
    def collection( self, partId=None, start=0, pp=100, pv='0', ts=256 ):
        res = { 'do_pagination':False, 'assets':[], 'name':'web.bach' }

        if partId is None:
            res[ 'msg' ] = 'Empty request'
            return self.to_html( res, template='images' )

        sess = self.db
        queried = None
        try:
            uid = int( partId )
            queried = sess.query( Bucket ).filter_by( keybachbucket=uid ).order_by( Bucket.name )
        except:
            queried = sess.query( Bucket ).filter_by( name=unquote( partId ) ).order_by( Bucket.name )

        return self.returnImages( res, queried, partId, start, pp, pv, ts )


#-----------------------------------------------------------------------------
    def keyword( self, partId=None, start=0, pp=100, pv='0', ts=256 ):
        res = { 'do_pagination':False, 'assets':[], 'name':'web.bach' }

        if partId is None:
            res[ 'msg' ] = 'Empty request'
            return self.to_html( res, template='images' )

        sess = Session()
        queried = None
        try:
            uid = int( partId )
            queried = sess.query( Keyword ).filter_by( keybachkeyword=uid ).order_by( Keyword.name )
        except:
            queried = sess.query( Keyword ).filter_by( name=unquote( partId ) ).order_by( Keyword.name )

        return self.returnImages( res, queried, partId, start, pp, pv, ts )
