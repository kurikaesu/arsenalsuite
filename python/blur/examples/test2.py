#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from blur.Stone import *
from blur.Classes import *
from blur.Classesui import *
import sys
import re

# First Create a Qt Application
app = QApplication(sys.argv)

# Load database config
if sys.platform=='win32':
	initConfig("c:\\blur\\resin\\resin.ini")
else:
	initConfig("/etc/db.ini")

blurqt_loader()

def showProjects():
	ps = ProjectStatus.select("projectstatus='Production'")

	charType = AssetType.recordByName( 'Character' )

	for p in Project.select("fkeyprojectstatus=?", [QVariant(ps[0].key())] ):
		print p.name()
		
		sl = Shot.select( "fkeyproject=?", [QVariant(p.key())] )
		sl = sl.sorted( "shotNumber" )
		for s in sl:
			print dir(s)
			print s.displayName()
	

def assetFromPath( path ):
	asset = Element.fromPath( path, True )
	if ( asset.assetType().name() == "Shot" ):
		return Shot( asset )
	elif ( asset.assetType().name() == "Scene" ):
		return ShotGroup( asset )
	return asset

def adfsdafd():
	ass = assetFromPath( "G:/Transformers/01_Shots/Sc001/S0012.00/SceneAssembly/" )
	print ass.displayPath()

	print ass.project().name()
	shot = Shot( ass.anscestorByAssetType( AssetType.recordByName("Shot") ) )

	print shot.displayPath()

	scene = shot.sequence()
	print scene.displayPath()

def testDialogs():
	ass = assetFromPath( "G:/Transformers/01_Shots/Sc001/" )

	df = DialogFactory.instance()
	df.newShot( ass )
	df.newAsset(ass, AssetType.recordByName( "Character" ) )
	df.newProject()
	df.newScene(ass)
	df.editAssetTemplates()
	df.editPathTemplates()
	df.editStatusSets()
	df.editAssetTypes()
	df.enterTimeSheetData( ass )
	df.editPermissions()

def testProperties():
	ass = assetFromPath( "G:/Transformers/" )
	ass.setProperty( "test", QVariant(1) )
	print ass.getProperty( "test", QVariant() ).toInt()

def testDotPaths():
	ass = assetFromPath( "G:/Transformers/" )
	shot = ass.fromRelativeDotPath( "Scene.sc001.s0012_00" )
	print shot.dotPath()
	
	ass = assetFromPath( "g:/Transformers/01_Characters/Optimus/Modeling/" )
	print ass.dotPath()
	
	ass = Element.fromDotPath( "Transformers.Scene.Sc001.S0012_00.Scene_Assembly" )
	print ass.displayPath()

def testChildren():
	ass = Element.fromDotPath( "Transformers" )
	shots = ass.children( AssetType.recordByName( "Shot" ), True )
	# Group by their parent(Sequence)
	bySequence = shots.groupedBy( "fkeyelement" )
	for shots in bySequence.values():
		for s in shots.sorted("shotNumber"):
			print Shot(s).displayPath()

testChildren()
