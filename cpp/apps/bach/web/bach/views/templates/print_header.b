#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: $"
#

test( c.vars[ 'do_pagination' ] ) and div( class_='pagination' )
[
    div()
	[
         li()
         [
    	     [ 'Showing %s-%s/%d of ' % ( c.vars[ 'start' ]+1,
                                          min( (c.vars[ 'start' ]+c.vars[ 'pp' ]), c.vars[ 'assets_count' ] ),
                                          c.vars[ 'assets_count' ] ) ]
             ,
             span( class_='title' )[ '"%s"' % c.vars[ 'name' ] ]
         ]
         ,
         li()
         [
             [ ' Showing per page: %d'%c.vars[ 'pp' ] ]
         ]
         ,
         li()
         [
             [ 'Thumb Size: ' ]
             ,
             ( test( c.vars[ 'ts' ] != 256 ) and  
                a( href=c.url( c.vars[ 'route' ],
                     partId=c.vars[ 'partId' ],
                     start=c.vars[ 'start' ],
                     pp=c.vars[ 'pp' ],
                     ts=256,
                     pv=c.vars[ 'pv' ] ) ) [ 256 ] 
             or span() [ '>256<' ] )
             ,
             [ ', ' ]
             ,
             ( test( c.vars[ 'ts' ] != 512 ) and  
                a( href=c.url( c.vars[ 'route' ],
                     partId=c.vars[ 'partId' ],
                     start=c.vars[ 'start' ],
                     pp=c.vars[ 'pp' ],
                     ts=512,
                     pv=c.vars[ 'pv' ] ) ) [ 512 ] 
             or span() [ '>512<' ] )

        ]
	]
    ,
    div( class_='break' )[ '' ]
]
