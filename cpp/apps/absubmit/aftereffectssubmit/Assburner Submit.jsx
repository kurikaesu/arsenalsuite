
//var submitScript = new File( "C:/blur/absubmit/fusionsubmit/submit.py" )
//submitScript.execute()

function isSecurityPrefSet()
{
	var securitySetting = app.preferences.getPrefAsLong("Main Pref Section",
					"Pref_SCRIPTING_FILE_NETWORK_SECURITY");
	return (securitySetting == 1);
}

function runAbsubmitScript()
{
	var submitScript = new File( "C:/blur/absubmit/aftereffectssubmit/submit.py" );
	submitScript.execute();
}

function writeRenderPasses( optionsFile )
{
	for(i=1; i<=app.project.renderQueue.numItems; ++i ) {
		var renderQueueItem = app.project.renderQueue.item(i);
		if( renderQueueItem instanceof RenderQueueItem ) {
			if( renderQueueItem.status == 2415 ) { // Queued status, Done items cannot be re-rendered
				var comp = renderQueueItem.comp;
				var timeStart = (renderQueueItem.timeSpanStart * comp.frameRate);
				var timeEnd = (renderQueueItem.timeSpanStart + renderQueueItem.timeSpanDuration) * comp.frameRate;
				for(v=1; v<=renderQueueItem.numOutputModules; ++v ) {
					var output = renderQueueItem.outputModules[v];
					var outputFile = output.file;
					optionsFile.writeln(comp.name + "; " + timeStart + "; " + timeEnd + "; " + File.decode(outputFile.fullName));
					//alert( comp.name + " frameStart: " + timeStart + " frameEnd: " + timeEnd + " output path: " + File.decode(outputFile.fullName) );
				}
			}
		}
	}
}

function submitJob()
{
	var optionsFilePath = "C:/blur/absubmit/aftereffectssubmit/current_options.txt";
	var optionsFile = new File( optionsFilePath );
	if( optionsFile.open("w") ) {
		optionsFile.writeln(app.version);
		optionsFile.writeln(app.project.file);
		writeRenderPasses( optionsFile );
		optionsFile.close();
		runAbsubmitScript();
	} else {
		alert( "Unable to open options file for writing at: " + optionsFilePath );
	}
}

if( isSecurityPrefSet() ) {
	submitJob();
} else {
	alert( "Assburner Submission requires ability to write to files.  Please enable the option for scripts to write to files and network in the Main Preferences" );
}





