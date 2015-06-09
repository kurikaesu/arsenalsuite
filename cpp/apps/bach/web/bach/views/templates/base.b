#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: $"
#

html [

    head [
		link( type='text/css', rel='stylesheet', href='/styles/site.css' ),
		test( c.vars.has_key( 'do_print_view' ) and c.vars[ 'do_print_view' ] ) and link( type='text/css', rel='stylesheet', href='/styles/print.css' ),
        slot( 'base.head' ),
        slot( 'base.title' )
    ],

    slot( 'base.frameset' ),

    body [
        slot( 'base.content' )
    ]

]