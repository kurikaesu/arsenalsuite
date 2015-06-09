#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: $"
#

inherits( 'base' )
[
	override( 'base.content' )
	[
		ul( class_='custom keywords' )
		[
			[
				li()
				[
					a( href=c.url( 'keyword', partId=co.keybachkeyword ), target='content' ) [ co.name ]
				] for co in c.vars[ 'keywords' ]
			]
		]
	]
]
