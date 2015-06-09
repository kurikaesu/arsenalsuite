#
#	__PYDOC__
#
#	[TITLE]
#	TODO:Add Document Title
#
#	[DESCRIPTION]
#	TODO: Add Document Description
#
#	[CREATION INFO]
#	Author: Eric Hulser
#	Email: eric@blur.com
#	Company: Blur Studios
#	Date: 06/30/06
#
#	[HISTORY]
#	--1.0	- Created
#
#	[DEPENDENCIES]
#
#	__END__
#

import ConfigParser, os.path, sys;

configFile = 'c:/blur/config.ini'

#-------------------------------------------------------------------------------------------------------------
#											INI FUNCTIONS
#-------------------------------------------------------------------------------------------------------------

def __getINISetting( inFileName, inSection, inKey = "" ):
	if ( os.path.isfile( inFileName ) ):
		tParser = ConfigParser.ConfigParser();
		tParser.read( inFileName );
		inSection 	= str( inSection );
		inKey		= str( inKey );
		if ( tParser.has_section( inSection ) ):
			if ( inKey ):
				if ( tParser.has_option( inSection, inKey ) ):
					return ( tParser.get( inSection, inKey ) );
			else:
				tItemList = tParser.items( inSection );
				return [ tItem[0] for tItem in tItemList ];
	return "";

if ( os.path.exists( configFile ) ):
	tEnvironment = __getINISetting( configFile, 'GLOBALS', 'environment' )
	
	tStartupLib = os.path.normpath( __getINISetting( configFile, tEnvironment, 'startupPath' ) );
	if ( not tStartupLib in sys.path ):
		sys.path.append( tStartupLib );
	from blurGlobals import *;
else:
	print ( "Blur Library Error: Could not find 'c:/blur/config.ini'" );