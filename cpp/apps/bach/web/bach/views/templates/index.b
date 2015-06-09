#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: $"
#

inherits( 'base' )
[
	override( 'base.title' )
	[
		title()[ 'web.bach' ]
	],
	override( 'base.frameset' )
	[
	    frameset( cols='15%,*,15%' )
	   	[
		    frame( name='collections', src=c.url( 'collections' ) ),
		    frame( name='content', src=c.url( 'collection' ) ),
		    frame( name='keywords', src=c.url( 'keywords' ) )
		]
	]
]
