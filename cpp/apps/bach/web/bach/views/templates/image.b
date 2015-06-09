#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: $"
#
# BASEDIR = /drd/reference/.thumbnails/

inherits( 'base' )
[
	override( 'base.content' )
	[
        (   test( c.vars.has_key( 'msg' ) ) and div( class_='msg' )
            [
                c.vars[ 'msg' ]
            ]
        or
            img( class_='bach_image', 
                 src=c.url( 'thumb', path=c.vars[ 'asset' ].path+'_512x512.png' ), 
                 title='Keywords: '+( c.vars[ 'asset' ].cachedkeywords if c.vars[ 'asset' ].cachedkeywords else "{no keywords}") )
            ,

            br(),span()[ 'Path: ' ],a( href='http://drddocs.drd.int'+c.vars[ 'asset' ].path, class_='bolder' )[ c.vars[ 'asset' ].path ],
            br(),span()[ 'Path: ' ],input( type='text', value=c.vars[ 'asset' ].path, size='80' ),
            br(),span()[ 'Dimensions: '],span( class_='bolder' )['%dx%d'%(c.vars[ 'asset' ].width,c.vars[ 'asset' ].height) ],
            br(),span()[ 'Keywords: '],span( class_='bolder' )[(', '.join( [ kw for kw in c.vars[ 'asset' ].cachedkeywords.split( ',' ) if kw != "" ] ) ) ],
            br(),a( href='javascript:history.back(1);' )[ 'Back to list' ],

         )
	]
]
