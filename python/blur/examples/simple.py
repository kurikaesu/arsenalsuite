#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from blur.Stone import *
from blur.Classes import *
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

#Database.instance().setEchoMode( Database.EchoSelect )

# Now we can use the database classes

# This function updates all timesheets that have fkeyproject=0
# it sets fkeyproject to the same project as the nearest timesheet
# for that user
def timesheet_to_nearest_project():
	for ts in TimeSheet.select( "WHERE fkeyproject=0" ):
		# Find closest timesheet
		print( "TimeSheet " + ts.user().displayName() + ":" + ts.dateTime().toString() + " has 0 fkeyproject" )
		close = TimeSheet.select( "WHERE fkeyproject!=0 AND fkeyemployee=? ORDER BY abs(?::date-datetime::date) ASC LIMIT 1",
			[ QVariant(ts.user().key())
			 ,QVariant(ts.dateTime().date())] )

		if close.size() == 1:
			c = close[0]
			print( "Closest timesheet was " + c.dateTimeSubmitted().toString() + " with project " + c.project().name() )
			ts.setProject( c.project() )
			ts.commit()
		else:
			print( "Couldn't find another timesheet to set fkeyproject" )

def get_email(user):
	email = user.email()
	if not '@' in email:
		return email + '@blur.com'
	return email

def get_jabber(user):
	jabb = user.jid()
	if not '@' in jabb:
		return jabb + '@jabber.blur.com'
	return jabb

def print_all_users_email_and_jabber():
	for user in Employee.select("disabled=0"):
		print user.displayName(), get_email(user), get_jabber(user)

#timesheet_to_nearest_project()
#print_all_users_email_and_jabber()

def createAssetsFromRender( maxJob ):
	
	maxFilePath = maxJob.fileOriginal().replace('\\','/')
	
	if maxFilePath.isEmpty():
		return
	
	# This looks for existing filetracker record for this path
	# It will match different versions of the same file
	# as long as they have the same base name and follow
	# the version format (_[vV]\d+_\d+.ext)
	maxFileTracker = FileTracker.fromPath( maxFilePath )
	shot = None
	project = None
	
	if maxFileTracker.isRecord():
		print "Found existing filetracker for scene assembly file"
		# Get the project and shot assets from the file tracker
		shot = maxFileTracker.element()
		project = shot.project()
	else:
		# If we aren't already tracking this file(or version), create a new tracker
		print "Trying to locate shot asset to create new filetracker"
		
		# First find the project
		project = Project.findProjectFromPath( maxFilePath )
		if not project.isRecord():
			raise "Unable to find project from path ", maxFilePath
		
		# Then lookup the shot
		shot = project.findShotFromPath( maxFilePath )
		
		if not shot.isRecord():
			# If we can't find the shot, then try to find the sequence
			p = Path(maxFilePath)
			print "Trying to locate scene asset"
			seq = project.findSequence( p[3] )
			
			if not seq.isRecord():
				# If the path matches Sc001 format, then create the sequence asset
				if re.match( 'sc\\d\\d\\d', str(p[3]), re.I ):
					print "Creating Sequence", p[3]
					# Find Shots top level asset
					shotGroup = None
					shots = project.children().filter( "name", "Shots" )
					if shots.size() == 1:
						shotGroup = shots[0]
					else:
						raise "Unable to Create Sequence, Can't find top-level Shots asset"
					
					at = AssetType.recordByName( 'Scene' )
					if not at.isRecord():
						raise "Unable to Create Sequence, Can't find 'Scene' AssetType"
					
					template = at.findDefaultTemplate( project )
					
					if template.isRecord():
						seq = template.create( p[3], shotGroup, project )
					else:
						seq = ShotGroup()
						seq.setName( p[3] )
						seq.setAssetType( at )
						seq.setParent( shotGroup )
						seq.setProject( project )
						seq.commit()
			
			print "Trying to create shot automatically from path: ", maxFilePath
			shot = Shot()
			shotNumber = None
			shotNumberMatch = re.match( '\\D+(\\d+\\.\\d+)', str(p[4]) )
			if shotNumberMatch:
				shotNumber = float( shotNumberMatch.group(1) )
			else:
				raise str("Couldn't parse shot number from: ", p[4])
			at = AssetType.recordByName( 'Shot' )
			if not at.isRecord():
				raise "Unable to Create Shot, Can't find 'Shot' AssetType"
			
			template = at.findDefaultTemplate( project )
			
			if template.isRecord():
				shot = Shot(template.create( p[4], seq, project ))
			else:
				shot.setParent( seq )
				shot.setName( p[4] )
				shot.setProject( project )
			
			shot.setShotNumber( shotNumber )
			shot.commit()
		
		fi = QFileInfo( maxFilePath )
		maxFileTracker = VersionFileTracker()
		maxFileTracker.setPath( fi.filePath() )
		maxFileTracker.setFileName( fi.fileName() )
		maxFileTracker.setName( "max_scene_assembly" )
		maxFileTracker.setElement( shot )
		maxFileTracker.commit()

	# Get Dependencies from the Stats File
	statsFile = '/var/spool/assburner/' + str(maxJob.fileName().replace('\\','/'))[2:]
	if QFile.exists(statsFile):
		# We should generate the stats file automatically on file save
		# So it is available on G:, instead of needing to read it from stryfe
		# and only being able to read it for rendered files
		class MaxFileDep:
			def __init__(self,path,deptype):
				self.path = path
				self.deptype = deptype
		deps = []
		print "Attempting to open ", statsFile, " to read max file dependencies"
		file = open(statsFile)
		for line in file:
			line = line.strip()
			if re.match('\#',line): continue
			if re.match('mem',line): continue
			if re.match('(map|pc|file)',line):
				try:
					# Format per line is
					# filetype,refcount,size,path
					# Because path can contain commas, we set maxsplit to 3, to avoid splitting path
					filetype, refcount, size, path = line.split(',',maxsplit=3)
					deps.append( MaxFileDep(path,filetype) )
				except:
					print "Invalid Line Format in Stats File:", line

		for dep in deps:
			ft = FileTracker.fromPath( dep.path )
			
			# Create FileTracker for the dependency, if it is not present
			if not ft.isRecord():
				print "Creating new filetracker ", dep.deptype, dep.path
				fi = QFileInfo(dep.path.replace('\\','/'))
				ft.setPath(fi.filePath())
				ft.setFileName(fi.fileName())
				if dep.deptype == 'map':
					ft.setName( 'map' )
				elif dep.deptype == 'pc':
					ft.setName( 'pointcache' )
				elif dep.deptype == 'file':
					if dep.path.endswith('.max'):
						ft.setName( 'max_model' )
					else:
						ftp.setName( dep.path[-3:] )
				ft.setElement( shot )
				ft.commit()
			
			dep = FileTrackerDep.recordByInputAndOutput( ft, maxFileTracker )
			
			# Create the dependency if it does not exist
			if not dep.isRecord():
				dep.setInput( ft )
				dep.setOutput( maxFileTracker )
				dep.commit()
		
	passName = None
	if not maxJob.elementFile().isEmpty():
		fi = QFileInfo(maxJob.elementFile().replace('\\','/'))
		passName = fi.completeBaseName()
		print "Pass Name is ", passName
	
	# Create the RangeFileTracker for tracking the output frames
	frameFileTracker = FileTracker.fromPath( maxJob.outputPath() )
	if not frameFileTracker.isRecord():
		fi = Path(maxJob.outputPath())
		print "Creating RangeFileTracker ", fi.dirPath(), fi.fileName()
		frameFileTracker = RangeFileTracker()
		frameFileTracker.setPath(fi.dirPath())
		frameFileTracker.setFileNameTemplate(fi.fileName())
		frameFileTracker.setFrameStart(maxJob.frameStart())
		frameFileTracker.setFrameEnd(maxJob.frameEnd())
		frameFileTracker.setRenderElement(passName)
		frameFileTracker.setElement( shot )
		frameFileTracker.commit()
		print "Frame FileTracker Created for ", FileTracker(frameFileTracker).filePath()
	
	# Create a dependency from the frames to the max file
	maxDep = FileTrackerDep.recordByInputAndOutput( maxFileTracker, frameFileTracker )
	if not maxDep.isRecord():
		maxDep.setInput( maxFileTracker )
		maxDep.setOutput( frameFileTracker )
		maxDep.commit()
		print "Dependency Created from output frames to the max file"
	
jobs = JobMax8.select( "fileOriginal is not null ORDER BY keyjob desc limit 1" )

if jobs.size() == 1:
	createAssetsFromRender( jobs[0] )
	
