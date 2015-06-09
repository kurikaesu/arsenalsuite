#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: $"
#

test( c.vars[ 'do_pagination' ] ) and div( class_='pagination' )
[
    div()
	[
	     # for debugging:
	     #[ div()[ '%s:%s::'%(k,c.vars[k]) ] for k in c.vars.keys() ],
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
             ( test( c.vars[ 'show_prev' ] ) and span( class_='prev_page' )
             [
                 a( href=c.url( c.vars[ 'route' ],
                                 partId=c.vars[ 'partId' ],
                                 start=c.vars[ 'prev_page' ],
                                 pp=c.vars[ 'pp' ],
                                 pv=c.vars[ 'pv' ],
                                 ts=c.vars[ 'ts' ] ) ) [ 'Previous Page' ],
             ] or span( class_='strikeout' ) [ 'Previous Page' ] )
             ,
             [ ':' ]
             ,
             ( test( c.vars[ 'show_next' ] )
                 and span( class_='next_page' )
             [
                 a( href=c.url( c.vars[ 'route' ],
                                 partId=c.vars[ 'partId' ],
                                 start=c.vars[ 'next_page' ],
                                 pp=c.vars[ 'pp' ],
                                 pv=c.vars[ 'pv' ],
                                 ts=c.vars[ 'ts' ] ) ) [ 'Next Page' ],
             ] or span( class_='strikeout' ) [ 'Next Page' ] )
         ]
         ,
         li()
         [
             [ ' Show per page:' ],
             [
                 span()
                 [
                     ( test( num != c.vars[ 'pp' ] and num < c.vars[ 'assets_count' ] ) and
                         a( href=c.url( c.vars[ 'route' ],
                                     partId=c.vars[ 'partId' ],
                                     start=c.vars[ 'start' ],
                                     pp=num,
                                     pv=c.vars[ 'pv' ],
                                     ts=c.vars[ 'ts' ] ) ) [ num ]
                     or [ '>%d<'%num if num == c.vars[ 'pp' ] else num ] ), [ ', ' ]
                 ] for num in ( 10, 50, 100, 1000 )
              ],
              ( test( c.vars[ 'assets_count' ] != c.vars[ 'pp' ] ) and
                  a( href=c.url( c.vars[ 'route' ],
                        partId=c.vars[ 'partId' ],
                        start=c.vars[ 'start' ],
                        pp=c.vars[ 'assets_count' ],
                        ts=c.vars[ 'ts' ],
                        pv=c.vars[ 'pv' ] ) ) [ 'All (%s)'%c.vars[ 'assets_count' ] ]
                or [ 'All (%s)'%c.vars[ 'assets_count' ] ] )
         ]
         ,
         li()
         [
             a( href=c.url( c.vars[ 'route' ],
                partId=c.vars[ 'partId' ],
                start=c.vars[ 'start' ],
                pp=c.vars[ 'pp' ],
                pv=c.vars[ 'pv' ],
                ts=c.vars[ 'ts' ] ), target='_blank' ) [ 'Open in new window' ]
         ]
         ,
         li()
         [
             span( class_='sep' )[ 'View: ' ]
             ,
             ( test( c.vars[ 'pv' ] == '1' ) and  
                a( href=c.url( c.vars[ 'route' ],
                     partId=c.vars[ 'partId' ],
                     start=c.vars[ 'start' ],
                     pp=c.vars[ 'pp' ],
                     ts=c.vars[ 'ts' ],
                     pv=0 ) ) [ 'Normal' ] 
             or span() [ '>Normal<' ] )
             ,
             [ ', ' ]
             ,
             ( test( c.vars[ 'pv' ] == '0' ) and  
                a( href=c.url( c.vars[ 'route' ],
                     partId=c.vars[ 'partId' ],
                     start=c.vars[ 'start' ],
                     pp=c.vars[ 'pp' ],
                     ts=c.vars[ 'ts' ],
                     pv=1 ), target='bach_print' ) [ 'Print' ] 
             or span() [ '>Print<' ] )

		 ]
         
         ,
         li()
         [
             span( class_='sep' )[ 'Thumb Size: ' ]
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
