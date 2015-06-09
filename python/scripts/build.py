from blur.build import *
import os

path = os.path.dirname(os.path.abspath(__file__))

if sys.platform != 'win32':
    abscriptInstallCmd = "bash " + path + "/installScripts.sh"
else:
    abscriptInstallCmd = " " # to do

managerIniTarget = IniConfigTarget("managerini",path + "/../scriptsini",'manager.ini.template','manager.ini')
reaperIniTarget = IniConfigTarget("reaperini",path + "/../scriptsini",'reaper.ini.template','reaper.ini')
verifierIniTarget = IniConfigTarget("verifierini",path + "/../scriptsini",'verifier.ini.template','verifier.ini')
abscriptTarget = StaticTarget("abscripts",path,abscriptInstallCmd,['managerini','reaperini','verifierini'],shell=True)

if __name__ == "__main__":
    build()
