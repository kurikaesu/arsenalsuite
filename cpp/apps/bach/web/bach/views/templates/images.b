#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: $"
#
# BASEDIR = /drd/reference/.thumbnails/

inherits( 'base' )
[
    override( 'base.title' )
    [
        title()[ c.vars[ 'name' ] ]
    ]
    ,
	override( 'base.content' )
	[
		test( c.vars.has_key( 'msg' ) ) and div( class_='msg' )
		[
			c.vars[ 'msg' ]
		]
		,
		test( c.vars.has_key( 'do_print_view' ) and c.vars[ 'do_print_view' ] ) and
            include( 'print_header' )
        or
            include( 'pagination' )
		,
		div( class_='images' )
		[
			[
				test( c.vars.has_key( 'do_print_view' ) and c.vars[ 'do_print_view' ]  ) and
					[
                        [
                            div( class_='image_container_print_view' )
                            [
                                [ '\n\n\n' ]
                                ,
                                div(style='height: %dpx; '%(c.vars[ 'ts' ]))
                                [
                                    img( src=c.url( 'thumb', path=im.path+'_%dx%d.png'%( c.vars[ 'ts' ], c.vars[ 'ts' ] ) ), 
                                         title='Keywords: '+im.cachedkeywords )
                                ]
                                ,
                                div( )
                                [
                                    br(),span()[ 'Path:' ],
                                    br(),span()[ im.path ],
                                    br(),span()[ 'Dimensions: %dx%d'%(im.width,im.height) ],
                                    br(),span()[ 'Keywords: '+(', '.join( [ kw for kw in im.cachedkeywords.split( ',' ) if kw != "" ] ) ) ],
                                ]
                                ,
                                [ '\n\n\n' ]
                            ] for im in c.vars[ 'assets' ]
						]
					]
				or
					[
						div( class_='image_container', style='height: %dpx ; width: %dpx ; line-height: %dpx ;' % (c.vars[ 'ts' ],c.vars[ 'ts' ],c.vars[ 'ts' ] ) )
						[
							[ '\n\n\n' ]
							,
							a( href=c.url( 'image', uid=im.keybachasset ) )
							[
								img(	src=c.url( 'thumb', path=im.path+'_%dx%d.png'%( c.vars[ 'ts' ], c.vars[ 'ts' ] ) ), 
									title='Keywords: '+im.cachedkeywords )
							]
							,
							[ '\n\n\n' ]
						] for im in c.vars[ 'assets' ]
					]
			]
		]
		,
		test( not( c.vars.has_key( 'do_print_view' ) and c.vars[ 'do_print_view' ] ) ) and
            include( 'pagination' )
	]
]
