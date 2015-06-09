#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: $"
#

inherits( 'base' )
[
	override( 'base.content' )
	[
		ul( class_='custom collections' )
		[
			[
				li()
				[
					a( href=c.url( 'collection', partId=co.name ), target='content' ) [ co.name, " : ", co.asset_count ]
				] for co in c.vars[ 'collections' ]
			]
		]
	]
]
