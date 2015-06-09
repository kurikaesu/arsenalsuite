

-- This will load blurLibrary from the current treeGrunt location
if ( _blurLibrary == undefined ) then
	filein "$max/stdplugs/stdscripts/baselib/blurStartupMaxLib.ms"

_blurLibrary.Load "blurFile"
_blurLibrary.Load "blurString"

-- loads the script from the maxscript job
-- This script must contain a runScript function
-- that takes a task(frame) number
fileIn ("%BURN_DIR%/script.ms")

-- This is used by submitElementsTemplate.ms
global scriptArchiveRoot = "%BURN_DIR%"

function runScriptJob =
(
	setquietmode true
	local fh = openFile "%STATUS_FILE%" mode:"w"
	try
	(
		local framesFile = "c:/temp/runscriptframes.txt"
		
		if( doesFileExist framesFile ) then
		(
			local framearray = #()
			local frameList = _blurFile.readFullFile framesFile
			local framestringarray = filterString frameList ","

			for idx = 1 to framestringarray.count do 
				append framearray (framestringarray[idx] as Integer)

			deleteFile framesFile

			loadMaxFile "%BURN_DIR%/maxhold.mx" quiet:true

			for idx = 1 to framearray.count do 
			(
				local frame = framearray[idx]
				format "starting %\n" frame to:fh
				runScript frame
				format "finished %\n" frame to:fh
			)
		) 
		else 
		(
			format "%\n" "Frames File does not exist" to:fh
			return true
		)
	) 
	catch
	(
		local ce = getCurrentException()
		ce = _blurString.findReplaceString ce "\n" "\\n"
		format "MaxScript Exception: %" ce to:fh
		close fh
		quitMax #noPrompt
		return false
	)
	
	format "%" "success\n" to:fh
	close fh
	quitMax #noPrompt
)

runScriptJob()
