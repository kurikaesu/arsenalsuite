#!/usr/bin/python

import re, sys
import decimal


def indentStr(level):
	return ''.join(['\t' for i in range(0,level)])

# Todo, escape %"\, etc.
def escapeEyeonscriptString( pstr ):
	pstr = pstr.replace('"','\\"')
	pstr = pstr.replace('\\','\\\\')
	return '"' + pstr + '"'

def unescapeEyeonscriptString( pstr ):
	return pstr.replace('\\"','"').replace('\\\\','\\')

def getValueString(val,indentLevel=0,flatMode=None):
	ret =''
	# Have to change E-18 to e-018
	if isinstance(val,decimal.Decimal):
		decstr = str(val)
		reMatch = re.match(r'([-\d\.]+)E([-+])(\d+)',decstr)
		if reMatch:
			decstr = reMatch.group(1) + 'e' + reMatch.group(2) + ('0'*(3-len(reMatch.group(3)))) + reMatch.group(3)
		return decstr
	if (isinstance(val,int) or isinstance(val,float) or isinstance(val,decimal.Decimal) or isinstance(val,long)) and not isinstance(val,bool):
		ret = str(val)
	elif isinstance(val,str) or isinstance(val,unicode):
		ret = escapeEyeonscriptString( val )
	elif isinstance(val,FusionTable):
		ret = val.asString(indentLevel,flatMode)
	elif isinstance(val,bool):
		ret = str(val).lower()
	else:
		raise "Couldnt handle value of type", type(val)
	return ret

def getKeyString(key):
	ret = str(key)
	if re.search('(?:(?:^[-\d])|([\.]))',ret):
		if not (isinstance(key,int) or isinstance(key,float) or isinstance(key,decimal.Decimal) or isinstance(key,long)):
			ret = '"' + ret + '"' 
		ret = '[' + ret + ']'
	return ret

#
#  Each entry in a eyeonscript table can be in on of the following forms
#
#	1. Key = Value
#   2. Key = {...}
#	3. Key = TypedTable {...}
#
#   4. TypedTable {...}
#   5. Value
#
#   In any single table, you can only have 1-3, OR 4-5
#	Basically the table acts as either a dict, or an array, but not both.
#
#	The FusionTable class implements this functionality, including
#	dumping the values to a string that is parsable by eyeonscript.
#
# 	Dict by default
#	Optional TableType
#	Will be neither dict nor array if you pass IsDict=None to the ctor
#	will become a dict or array when you first treat it as such
#	Passing an array or a dict to the ctor overrides IsArray and IsDict
#	IsArray overrides IsDict
#	Dict overrieds Array

def getClassForTypedTable( tableType ):
	nameMatch = 'Fusion' + tableType
	#print 'Searching for class ' + nameMatch
	for t in FusionTypes:
	#	print t.__name__
		if t.__name__ == nameMatch:
			return t
	return FusionTable

class FusionTable(object):
	def __init__(self,tableType = None, Dict = None, Array = None, IsDict = True, IsArray = None):
		# Unknown, will be True or False
		self.IsArray = None
		self.Array = []
		if IsDict:
			self.IsArray=False
		if IsArray and self.IsArray is None:
			self.IsArray = IsArray
		self.Hidden = {}
		if Array:
			self.Array = Array
			self.IsArray = True
		if Dict:
			self.__dict__ = Dict
			self.IsArray = False
		self.Type = tableType
	def append(self,val):
		if self.IsArray == None:
			self.IsArray = True
			self.Array = []
		self.Array.append(val)
	def __getitem__(self,idx):
		if self.__dict__['IsArray']:
			return self.Array[idx]
		return self.__dict__[idx]
	def __setitem__(self,key,val):
		if key in ['IsArray','Hidden','Array','Type','Parent']:
			self.__dict__[key] = val
			return
		if not self.IsArray:
			self.IsArray = False
			if not key in self.Array:
				self.Array.append(key)
			self.__dict__[key] = val
		elif self.IsArray:
			self.Array[key] = val
	def __delitem__(self,key):
		if key in ['IsArray','Hidden','Array','Type','Parent'] or not self.IsArray:
			del self.__dict__[key]
			return
		elif self.IsArray:
			del self.Array[idx]
	def parent(self):
		if hasattr(self,'Parent'):
			return self.Parent
		return None
	def find_key(self,val):
		for k,v in self.__dict__.iteritems():
			if v == val:
				return k
		return None
	def get_key_name(self):
		if self.parent():
			return self.parent().find_key(self)
		return None
	def __iter__(self):
		return iter(self.Array)
	
	def overrideFlatMode(self,key,v,flatMode):
		if key in ['Toolbar','OpToolbar','Proxy']:
			return False
		if isinstance(v,FusionTable) and v.Type == 'FuID':
			return True
		if isinstance(key,FusionTable) and self.parent() and self.parent().parent() and self.parent().parent().find_key(self) == 'Points':
			return True
		if isinstance(v,FusionTable) and v.Type == 'Polyline':
			return None
		if self.get_key_name() == 'KeyFrames':
			return True
		if isinstance(v,FusionTable) and self.get_key_name() == 'Points':
			return True
		if self.parent():
			if self.parent().get_key_name() == 'KeyColorSplines':
				return True
			if self.parent().get_key_name() == 'Layout':
				return False
		return flatMode
	
	def asString(self, indentLevel=0, flatMode=None):
		ret = ''
		if self.Type:
			ret = self.Type + ' '
		ret += '{'
		items = []
		for k in self.Array:
			if k not in ['IsArray','Hidden','Array','Type','Parent']:
				if k in self.__dict__:
					v = self.__dict__[k]
					items.append( getKeyString(k) + ' = ' + getValueString(v,indentLevel+1,self.overrideFlatMode(k,v,flatMode)) )
				else:
					items.append( getValueString(k,indentLevel+1,self.overrideFlatMode(None,k,flatMode)) )
		if flatMode==True or (flatMode==None and not self.Type and len(', '.join(items)) < 35 and len(items) > 1):
			ret += ' ' + ', '.join(items)
			if len(items):
				ret += ', '
			return ret + '}'
		ret += '\n'
		if len(items):
			ret += indentStr(indentLevel+1) + (',\n' + indentStr(indentLevel+1)).join(items) + ',\n'
		return ret + indentStr(indentLevel) + '}'

	def typedChildren( self, fusionType, recursive = False, allowSubclasses = True ):
		ret = []
		fusionTableSubclass = fusionType
		if isinstance(fusionType,str) or isinstance(fusionType,unicode):
			fusionTableSubclass = getClassForTypedTable( fusionType )
			if fusionTableSubclass == FusionTable:
				fusionTableSubclass = None
		for c in iter(self):
			if c in self.__dict__:
				c = self[c]
			if isinstance(c,FusionTable):
				if fusionTableSubclass is None:
					# Compare by type name
					if c.Type == fusionType:
						ret.append( c )
				else:
					# Compare by python type
					if (allowSubclasses and isinstance(c,fusionTableSubclass)) or (not allowSubclasses and c.__class__ == fusionTableSubclass):
						ret.append( c )
				if recursive:
					ret += c.typedChildren( fusionType, True, allowSubclasses )
		return ret
	
def parseValue( script, pos ):
	# All values are either a number(isnumeric), a quoted string("), a typed table(isalpha), or an anon table({)
	# So the first non-space tells us what it is
	while pos < len(script) and script[pos].isspace():
		pos += 1
		
	# Quoted string
	if script[pos] == '"':
		lastCharIsEscape = False
		strEnd = pos+1
		while strEnd < len(script):
			if script[strEnd] == '"' and not lastCharIsEscape:
				break
			if script[strEnd] == '\\':
				lastCharIsEscape = not lastCharIsEscape
			else:
				lastCharIsEscape = False
			strEnd += 1
		return (strEnd+1,unescapeEyeonscriptString(script[pos+1:strEnd]))
	
	# Number
	elif script[pos].isdigit() or script[pos] == '-':
		numEnd = pos + 1
		decFound = False
		while numEnd < len(script) and (script[numEnd].isdigit() or script[numEnd] == '.' or script[numEnd] == 'e' or script[numEnd] == '-'):
			if script[numEnd] == '.':
				decFound = True
			numEnd += 1
		numStr = script[pos:numEnd]
		if decFound:
			return (numEnd,decimal.Decimal(numStr))
		return (numEnd,int(numStr))
	
	# boolean
	elif ( pos + 5 < len(script) and script[pos:pos+5] == 'false' ):
		return (pos+6,False)
	elif ( pos + 4 < len(script) and script[pos:pos+4] == 'true' ):
		return (pos+5,True)
	
	# Table
	elif script[pos].isalpha() or script[pos] == '{':
		return parseTable( script, pos )
	
	raise ("Couldnt figure out the value " + script[pos])

# Returns (pos, key, value)
def parseTableEntry( script, pos ):
	#Scan ahead to see if this is a key/value, or just a value
	isValue = True
	scanpos = pos
	while scanpos < len(script):
#		print 'Scanning:', script[scanpos]
		if script[scanpos] == '=':
			isValue = False
			break
		if script[scanpos] == ',' or script[scanpos] == '}' or script[scanpos] == '{':
			break
		scanpos += 1
	
	if script[scanpos] == '}' and len(script[pos:scanpos].strip()) == 0:
		return (scanpos,None,None)
	
	if isValue:
		(pos,value) = parseValue( script, pos )
		return (pos, None, value)
	else:
		(valueEnd,value) = parseValue( script, scanpos + 1 )
		key = script[pos:scanpos].strip()
		if key.startswith('[') and key.endswith(']'):
			key = key[1:-1]
		if key.startswith('"') and key.endswith('"'):
			key = key[1:-1]
		try:
			decKey = decimal.Decimal(key)
			return (valueEnd, decKey, value)
		except:
			return (valueEnd, key, value)

def parseTable( script, pos ):
	# Find the start of the table and parse the type, if it is typed
	tableStart = pos
	while tableStart < len(script) and script[tableStart] != '{':
		tableStart += 1
	tableType = None
	if tableStart > pos and len(script[pos:tableStart].strip()) > 0:
		tableType = script[pos:tableStart].strip()
			
	#print "Table Start: %i Table Type: %s" % (tableStart, tableType)
	
	table = None
	tableClass = FusionTable
	if tableType is not None:
		tableClass = getClassForTypedTable(tableType)
	if tableClass != FusionTable:
		table = tableClass()
	else:
		table = FusionTable(tableType,None,None,None,None)
	
	pos = tableStart + 1
	# Parse the table entries until the end of the table
	while pos < len(script):
		(pos,entrykey,entryvalue) = parseTableEntry( script, pos )
	#	print "parseTableEntry returned ", pos, entrykey, entryvalue
		if isinstance(entrykey,FusionTable):
			entrykey.Parent = table
		if isinstance(entryvalue,FusionTable):
			entryvalue.Parent = table
		if entrykey is not None:
			table[entrykey] = entryvalue
		elif entryvalue is not None:
			table.append(entryvalue)
		if script[pos] == ',':
			pos += 1
		elif script[pos] == '}':
			break
	
	if script[pos] != '}':
		raise "End of script without end of table!!"
	return (pos+1,table)

def parseEyeonFile( fileName ):
	file = open(fileName)
	script = file.read()
	table = parseTable(script,0)[1]
	return table

def writeEyeonFile( fusionTable, fileName ):
	f = open( fileName, "w" )
	f.write( fusionTable.asString() + "\n\n " )
	f.close()

class RangeFinder:
	def __init__(self):
		self.Start = self.End = None
	def update(self,rng):
		if self.Start==None or rng[0] < self.Start:
			self.Start = rng[0]
		if self.End==None or rng[1] > self.End:
			self.End = rng[1]

class FusionComposition(FusionTable):
	def __init__(self,tableType = None, Dict = None, Array = None, IsDict = True, IsArray = None):
		FusionTable.__init__(self,tableType='Composition')
		self.Tools = FusionTable(IsArray=True)
		self.HiQ = True
		self.MotionBlur = False
		self.SavedOutputs = 1
		self.HeldTools = 0
		self.DisabledTools = 9
		self.LockedTools = 0
		self.AudioOffset = 0
		self.Resumable = True
		self.OutputClips = FusionTable()
		self.Tools = FusionTable()
		self.Version = "Fusion 5.02 build 82"
		self.GlobalRange = FusionTable(Array=[0,0])
		self.RenderRange = FusionTable(Array=[0,0])
	
	def addTool(self, tool):
		self.Tools[tool.name()] = tool
	
	def updateRangeToLoaders(self):
		globalRange = RangeFinder()
		for tool_name in self.Tools:
			tool = self.Tools[tool_name]
			if isinstance(tool,FusionLoader):
				globalRange.update(tool.getGlobalRange())
		self.RenderRange = self.GlobalRange = FusionTable(Array=[globalRange.Start,globalRange.End])

nextToolID = 0
def getUniqueToolName(toolType):
	global nextToolID
	nextToolID += 1
	return toolType + str(nextToolID)

class FusionTool(FusionTable):
	def __init__(self, toolType, toolName=None, pos = None):
		FusionTable.__init__(self, tableType=toolType)
		if not toolName:
			toolName = getUniqueToolName(toolType)
		self.Hidden['Name'] = toolName
		self.ViewInfo = FusionOperatorInfo(pos=pos)
		self.Inputs = FusionTable()
	def setInput(self,name,val):
		self.Inputs[name] = val
	def name(self):
		return self.Hidden['Name']

class FusionInput(FusionTable):
	def __init__(self,value=None,sourceOp=None,source=None):
		FusionTable.__init__(self,tableType='Input')
		self.Value = value
		self.SourceOp = sourceOp
		self.Source = source
	def asString(self,indentLevel=0,flatMode=None):
		if isinstance(self.SourceOp,FusionTool):
			self.SourceOp = self.SourceOp.name()
		if self.SourceOp and not self.Source:
			self.Source = 'Output'
		if not self.SourceOp and not self.Value:
			self.Value = 0
		return FusionTable.asString(self,indentLevel,flatMode)
	
nextOperatorX = 0
def getUniqueOperatorPos():
	global nextOperatorX
	nextOperatorX += 110
	return [nextOperatorX,100]

class FusionOperatorInfo(FusionTable):
	def __init__(self, pos = None):
		FusionTable.__init__(self,tableType='OperatorInfo')
		if not pos:
			pos = FusionTable(Array=getUniqueOperatorPos())
		self.Pos = pos

class FusionLoader(FusionTool):
	def __init__(self, toolName=None, pos = None):
		FusionTool.__init__(self,"Loader", toolName, pos)
		self.Clips = FusionTable(IsArray=True)
		self.setInput('Depth',FusionInput(1))
	def addClip(self, clip):
		self.Clips.append(clip)
	def getRenderRange(self):
		rf = RangeFinder()
		for clip in self.Clips:
			rf.update([clip.RenderStart,clip.RenderStart + clip.Length - 1])
		return (rf.Start,rf.End)
	def getGlobalRange(self):
		rf = RangeFinder()
		for clip in self.Clips:
			rf.update([clip.GlobalStart,clip.GlobalEnd])
		return (rf.Start,rf.End)

class FusionSaver(FusionTool):
	def __init__(self,toolName=None,pos=None):
		FusionTool.__init__(self,"Saver",toolName,pos)
	def setInput(self,name,val):
		FusionTool.setInput(self,name,val)
		if name == 'Clip':
			self.setInput('OutputFormat',FusionInput(FusionTable('FuID',Array=[val.Value.FormatID])))

nextClipID = 0
def getUniqueClipID():
	global nextClipID
	nextClipID += 1
	return 'Clip' + str(nextClipID)

extToFusionFormat = {
	'jpg':'JpegFormat',
	'jpeg':'JpegFormat',
	'tga':'TargaFormat',
	'tiff':'TiffFormat',
	'tif':'TiffFormat',
	'png':'PNGFormat',
	'exr':'OpenExrFormat'
}

def getFusionFormat(ext):
	return extToFusionFormat[str(ext).lower()]

class FusionClip(FusionTable):
	def __init__(self, clipID = getUniqueClipID(), fileName = '', startFrame = 0, length=0, globalStart=0, globalEnd=0):
		FusionTable.__init__(self,"Clip")
		self.ID = clipID
		self.StartFrame = startFrame
		self.Length = length
		self.GlobalStart = globalStart
		self.GlobalEnd = globalEnd
		self.setFileName(fileName)
		self.Multiframe = False
		self.Saving = False
		self.LengthSetManually = True
		self.TrimIn = 0
		self.TrimOut = length-1
		self.ExtendFirst = 0
		self.ExtendLast = 0
		self.Loop = 1
		self.Reverse = False
		self.ImportMode = 0
		self.PullOffset = 0
		self.AspectMode = 0
		self.Depth = 0
		self.TimeCode = 0
		self.KeyCode = ""
		
	def setFileName(self,fileName):
		self.Filename = fileName.replace('/','\\')
		try:
			self.FormatID = getFusionFormat(re.search('\.(\w+)$', fileName).groups()[0])
		except:
			self.FormatID = ""
		
class FusionGradient(FusionTable):
	def __init__(self,colors=[]):
		FusionTable.__init__(self,'Gradient')
		self.Colors = FusionTable()
		for i in range(len(colors)):
			self.Colors[i] = FusionTable(Array=[c for c in colors[i]])

class FusionBackground(FusionTool):
	def __init__(self,name=None,pos=None,size=None):
		FusionTool.__init__(self,'Background',name,pos)
		self.ExtentSet = False
		self.CtrlWShown = False
		self.setInput('Gradient',FusionInput(FusionGradient([(0,0,0,1),(1,1,1,1)])))
		self.setSize(size)
	def setSize(self,size):
		if size!=None and (isinstance(size,list) or isinstance(size,tuple)) and len(size)==2:
			self.setInput('Width',FusionInput(size[0]))
			self.setInput('Height',FusionInput(size[1]))

class FusionMerge(FusionTool):
	def __init__(self,name=None,pos=None,foreground=FusionInput(),background=FusionInput()):
		FusionTool.__init__(self,'Merge',name,pos)
		self.setInput('Foreground',foreground)
		self.setInput('Background',background)
	
class FusionResize(FusionTool):
	def __init__(self,name=None,pos=None,size=None):
		FusionTool.__init__(self,'BetterResize',name,pos)
		self.setSize(size)
	def setSize(self,size):
		if size!=None and (isinstance(size,list) or isinstance(size,tuple)) and len(size)==2:
			self.setInput('Width',FusionInput(size[0]))
			self.setInput('Height',FusionInput(size[1]))
		
class FusionDissolve(FusionTool):
	def __init__(self,name=None,pos=None,foreground=FusionInput(),background=FusionInput(),mix=None):
		FusionTool.__init__(self,'Dissolve',name,pos)
		self.setInput('Foreground',foreground)
		self.setInput('Background',background)
		self.Transitions = FusionTable(Dict={0:"DFTDissolve"})
		if mix:
			self.setInput('Mix',mix)

nextColor = [0.0,1.0,1.0]
def getUniqueColor():
	global nextColor
	nextColor = [ (nextColor[0]+.21) % 1.0, (nextColor[1]+.38) % 1.0, (nextColor[2]+.47) % 1.0]
	return nextColor

class FusionKeyFrame(FusionTable):
	def __init__(self,Value=0,RH=None,LH=None,Flags=None):
		FusionTable.__init__(self)
		self.Hidden['Value']=Value
		self.RH=RH
		self.LH=LH
		self.Flags=Flags
	def asString(self,indentLevel,flatMode=None):
		ret = FusionTable.asString(self,indentLevel,flatMode)
		return '{ ' + str(self.Hidden['Value']) + ', ' + ret[2:]

class FusionBezierSpline(FusionTool):
	def __init__(self,name=None,splineColor=None):
		FusionTool.__init__(self,'BezierSpline',name)
		if not splineColor:
			splineColor = getUniqueColor()
		self.SplineName = self.name()
		self.SplineColor = FusionTable(Array=splineColor)
		self.ViewInfo=None
		self.Inputs=None
		self.NameSet=True
	def createLinearTransition(self,startPos,startVal,endPos,endVal):
		length = endPos-startPos
		self.KeyFrames = FusionTable(
			Dict = {
				startPos : FusionKeyFrame( startVal, RH=FusionTable(Array=[startPos+(length/3.0),0.33333]) ),
				endPos : FusionKeyFrame( endVal, LH=FusionTable(Array=[endPos-(length/3.0),0.66666667]), Flags = FusionTable( Dict = {'Linear':True} ) )
			}
		)

FusionTypes = []
def getSubclasses( fusionClass ):
	subclasses = fusionClass.__subclasses__()
	ret = []
	for sc in subclasses:
		ret += getSubclasses( sc )
	ret += subclasses
	return ret

FusionTypes = getSubclasses( FusionTable ) + [FusionTable]

def rewritePaths( templateFilePath, findRe, replaceString, outputPath ):
	comp = parseEyeonFile( templateFilePath )
	for loader in comp.typedChildren( 'Loader', True ):
		for clip in loader.typedChildren( 'Clip', True ):
			clip['Filename'] = re.sub( findRe, replaceString, clip['Filename'] )
	writeEyeonFile( comp, outputPath )

def replaceCap( pattern, string, replace ):
	replaced = ''
	while True:
		reMatch = re.search( pattern, string )
		if reMatch:
			replaced += string[0:reMatch.start(1)] + replace
			string = string[reMatch.end(1):]
		else:
			break
	return replaced + string

def wwtaChangeShot( templateFilePath, outputPath, shotNumber, frameStart, frameEnd ):
	comp = parseEyeonFile( templateFilePath )
	blur_shotPathSearch = r'[\\/][sS](\d\d\d\d\.\d\d)[\\/]'
	blur_shotPathReplace = '%07.2f' % shotNumber
	client_shotPathSearch = r'_(\d\d\d)_'
	client_shotPathReplace = '%03.0f' % shotNumber
	blur_saverShotSearch = r'[sS](\d\d\d\d)'
	blur_saverShotReplace = '%04.0f' % shotNumber
	for loader in comp.typedChildren( 'Loader', True ):
		for clip in loader.typedChildren( 'Clip', True ):
			print clip['Filename']
			clip['Filename'] = replaceCap( blur_shotPathSearch, clip['Filename'], blur_shotPathReplace )
			clip['Filename'] = replaceCap( client_shotPathSearch, clip['Filename'], client_shotPathReplace )
			clip['StartFrame'] = frameStart
			clip['Length'] = frameEnd - frameStart + 1
			clip['GlobalStart'] = frameStart
			clip['GlobalEnd'] = frameEnd
	for saver in comp.typedChildren( 'Saver', True ):
		for clip in saver.typedChildren( 'Clip', True ):
			print clip['Filename']
			clip['Filename'] = replaceCap( blur_saverShotSearch, clip['Filename'], blur_saverShotReplace )
	comp['RenderRange'].Array[0] = frameStart
	comp['RenderRange'].Array[1] = frameEnd
	comp['GlobalRange'].Array[0] = frameStart
	comp['GlobalRange'].Array[1] = frameEnd
	writeEyeonFile( comp, outputPath )

	
def TransformPremiereDataToFusionComp(premiereData, compSize = (1920,1080)):
	print premiereData

	clips = premiereData['clips']
	transitions = premiereData['transitions']
	
	for t in transitions:
		if t.headNode:
			t.headNode.startTransition = t
		if t.tailNode:
			t.tailNode.endTransition = t
	
	dissolveFadeLength = 1
	posX = 0
	mergePosY = 100
	resizePosY = 140
	loaderPosY = 180
	
	comp = FusionComposition()
	bg = FusionBackground(pos = FusionTable(Array=[posX,100]),size=compSize)
	comp.addTool(bg)
	lastTool = bg
	
	for clip in clips:
		# Position in viewport
		posX += 115
		
		# Create the loader
		loaderName = clip.sequenceID + '_' + clip.shotID.replace('.','_')
		print "Create Loader", loaderName
		loader = FusionLoader(loaderName,pos=FusionTable(Array=[posX,loaderPosY]))
		
		# Adjust frame/global start/end according to the transitions
		if hasattr(clip,'startTransition'):
			diff = clip.globalStart - clip.startTransition.globalStart
			print "Adjusting Clip Start by", diff
			clip.frameStart -= diff
			clip.globalStart = clip.startTransition.globalStart
		if hasattr(clip,'endTransition'):
			diff = clip.endTransition.globalEnd - clip.globalEnd
			print "Adjusting Clip End by", diff
			clip.frameEnd += diff
			clip.globalEnd = clip.endTransition.globalEnd
			
		# Make sure the global length matches the clip length
		if clip.globalEnd - clip.globalStart != clip.frameEnd - clip.frameStart:
			print "Frame Lengths Dont match", clip.fileName, "Clip length", (clip.frameEnd - clip.frameStart + 1), "Global Length", clip.globalEnd - clip.globalStart + 1

		# Create the clip and add it to the loader
		print "Creating Clip", clip.globalStart, clip.globalEnd, clip.frameStart, clip.frameEnd, clip.fileName
		loader.addClip(
			 FusionClip(
				fileName = clip.fileName,
				startFrame = clip.frameStart,
				length = clip.frameEnd - clip.frameStart,
				globalStart = clip.globalStart,
				globalEnd = clip.globalEnd
			)
		)

		# Add the loader to the comp
		comp.addTool( loader )
		
		dissolveInput = loader
		
		# Create a resize tool if needed
		if clip.frameWidth != compSize[0] or clip.frameHeight != compSize[1]:
			print "Creating Resize tool for loader"
			resize = FusionResize(size=compSize,pos=FusionTable(Array=[posX,resizePosY]))
			resize.setInput('Input',FusionInput(sourceOp=loader))
			comp.addTool( resize )
			dissolveInput = resize
		
		dissolve = FusionDissolve(
			pos=FusionTable(Array=[posX,mergePosY]),
			background=FusionInput(sourceOp=lastTool),
			foreground=FusionInput(sourceOp=dissolveInput) )
		
		mix = FusionBezierSpline( name = dissolve.name() + 'ForegroundBackground' )
		if hasattr(clip,'startTransition'):
			trans = clip.startTransition
			print "Using premiere transition data", trans.matchName, trans.globalStart, trans.startPercent, trans.globalEnd, trans.endPercent
			mix.createLinearTransition( trans.globalStart, trans.startPercent, trans.globalEnd, trans.endPercent ) 
		else:
			# Create default 1 frame transition
			mix.createLinearTransition( clip.globalStart - dissolveFadeLength, 0, clip.globalStart, 1 )
			
		dissolve.setInput('Mix',FusionInput(sourceOp=mix,source='Value'))
		
		lastTool = dissolve
		comp.addTool( dissolve )
		comp.addTool( mix )
	posX += 115
	saver = FusionSaver(pos=FusionTable(Array=[posX,mergePosY]))
	saver.setInput('Input',FusionInput(sourceOp=lastTool))
	comp.addTool( saver )
	comp.updateRangeToLoaders()
	return comp.asString()
	
def simpleTest():
	fc = FusionComposition()
	fl = FusionLoader()
	fs = FusionSaver()
	rng = FusionTable(Array=[0,58])
	fc.GlobalRange = rng
	fc.RenderRange = rng
	fs.setInput('Input',FusionInput(sourceOp=fl))
	fl.addClip( FusionClip(fileName="S:\\GentlemensDuel\\Render\\Sc001\\S0009.00\\960_540\\P_Table_Ambient\\P_Table_Ambient_0430.tga",startFrame=430,length=59,globalStart=0,globalEnd=58) )
	fc.addTool( fl )
	fc.addTool( FusionBackground() )
	fc.addTool( fs )
	print fc.asString()

#from blurEdit import GenerateFusionData

def premiereTransformTest(outFile='test.comp'):
	file = open(outFile,'w')
	file.write(TransformPremiereDataToFusionComp(GenerateFusionData('GentlemensDuel',inFileName="G:\\GentlemensDuel\\00_Editorial\\MasterEdit\\Gent'sDuel_Story_reel_V12_12_Network.prproj")))

if __name__ == "__main__":
	#simpleTest()
	premiereTransformTest(sys.argv[1])
