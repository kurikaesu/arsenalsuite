# PyQt4 NSIS installer script.
#
# Copyright (c) 2006
# 	Riverbank Computing Limited <info@riverbankcomputing.co.uk>
# 
# This file is part of PyQt.
# 
# This copy of PyQt is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2, or (at your option) any later
# version.
# 
# PyQt is supplied in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
# 
# You should have received a copy of the GNU General Public License along with
# PyQt; see the file LICENSE.  If not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# Defines expected from the cmd line /DDEFINE=VALUE
# PYTHON_PATH
# PYTHON_VERSION
# PLATFORM

!if ${PLATFORM} == "win32-msvc2005_64"
	!define _64_BIT
!endif
!if ${PLATFORM} == "win32-msvc2008_64"
	!define _64_BIT
!endif

!ifdef _64_BIT
	!define DESIGNER_PATH	"C:\windows\system32\blur64\designer\"
	!define BIT_NAME ' x64'
!endif
!ifndef _64_BIT
	!define DESIGNER_PATH	"C:\blur\common\designer\"
	!define BIT_NAME ''
!endif

# These will change with different releases.
!define PYQT_VERSION        "4.9.5-0"
!define PYQT_LICENSE        "GPL"
!define PYQT_LICENSE_LC     "gpl"
!define PYQT_QT_VERS        "4.8.3"

# These are all derived from the above.
!define PYQT_NAME           "PyQt4 ${PYQT_LICENSE} v${PYQT_VERSION} ${PLATFORM}"
!define PYQT_INSTALLDIR     "${PYTHON_PATH}\"
!define PYQT_PYTHON_HKLM    "Software\Python\PythonCore\${PYTHON_VERSION}\InstallPath"
!define PYQT_QT_HKLM        "Software\Trolltech\Versions\${PYQT_QT_VERS}"


# Tweak some of the standard pages.
!define MUI_WELCOMEPAGE_TEXT \
"This wizard will guide you through the installation of ${PYQT_NAME}.\r\n\
\r\n\
This copy of PyQt has been built with the ${PLATFORM} compiler against \
Python v${PYTHON_VERSION}.x and Qt v${PYQT_QT_VERS}.\r\n\
\r\n\
Any code you write must be released under a license that is compatible with \
the GPL.\r\n\
\r\n\
Click Next to continue."

!define MUI_FINISHPAGE_LINK "Get the latest news of PyQt here"
!define MUI_FINISHPAGE_LINK_LOCATION "http://www.riverbankcomputing.co.uk"


# Include the tools we use.
!include MUI.nsh
!include LogicLib.nsh
!include FileFunc.nsh
!include "x64.nsh"
!include "..\..\..\nsis\FileSystem.nsh"
!include "..\..\..\nsis\ReplaceSubStr.nsh"
!include "..\..\..\nsis\GetParent.nsh"
!include "..\..\..\nsis\LastInstall.nsh"
!include "..\..\..\nsis\SoftwareINI.nsh"


# Define the product name and installer executable.
Name "PyQt4"
Caption "${PYQT_NAME} Setup"
!define OUT_FILE "PyQt4_${PYQT_LICENSE_LC}_${PYQT_VERSION}-Py${PYTHON_VERSION}-${PLATFORM}.exe"
OutFile "${OUT_FILE}"


# Set the install directory, from the registry if possible.
InstallDir "${PYQT_INSTALLDIR}"

# The different installation types.  "Full" is everything.  "Minimal" is the
# runtime environment.
InstType "Full"
InstType "Minimal"


# Maximum compression.
SetCompressor /SOLID lzma


# We want the user to confirm they want to cancel.
!define MUI_ABORTWARNING


Function .onInit

#    ${If} $0 == ""
#        MessageBox MB_YESNO|MB_ICONQUESTION \
#"This copy of PyQt has been built against Python v${PYTHON_VERSION}.x which \
#doesn't seem to be installed.$\r$\n\
#$\r$\n\
#Do you with to continue with the installation?" IDYES GotPython
#            Abort
GotPython:
    #${Endif}

!ifdef _64_BIT
	SetRegView 64
!endif

    # Check the right version of Qt has been installed.  Note we can't check if
    # the right compiler was used.  This key seems to be present in the
    # commercial version and missing in the GPL version.
    ReadRegStr $0 HKLM "${PYQT_QT_HKLM}" "InstallDir"

    ${If} $0 != ""
               StrCpy $0 $INSTDIR
    ${Endif}

#    ${If} $0 != ""
#        MessageBox MB_YESNO|MB_ICONQUESTION \
#"This copy of PyQt has been built with the ${PLATFORM} compiler against \
#Qt v${PYQT_QT_VERS} which doesn't seem to be installed.$\r$\n\
#$\r$\n\
#Do you with to continue with the installation?" IDYES GotQt
#            Abort
GotQt:
#    ${Endif}
FunctionEnd


# Define the different pages.
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
  
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

  
# Other settings.
!insertmacro MUI_LANGUAGE "English"


# Installer sections.

Section "Extension modules" SecModules
    SectionIn 1 2 RO

!ifdef _64_BIT
    ${DisableX64FSRedirection}
!endif

    # Make sure this is clean and tidy.
    RMDir /r $PROGRAMFILES\PyQt4
    CreateDirectory $PROGRAMFILES\PyQt4

    SetOverwrite on

    # We have to take the SIP files from where they should have been installed.
    SetOutPath $INSTDIR\Lib\site-packages
    File "${PYQT_INSTALLDIR}Lib\site-packages\sip.pyd"

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4
    File .\LICENSE.GPL2
    File .\LICENSE.GPL3
    File .\GPL_EXCEPTION.TXT
    File .\GPL_EXCEPTION_ADDENDUM.TXT
    File .\OPENSOURCE-NOTICE.TXT
    File .\__init__.py
    File .\Qt\Qt.pyd
    File .\pyqtconfig.py
    File .\QAxContainer\QAxContainer.pyd
    File .\QtAssistant\QtAssistant.pyd
    File .\QtCore\QtCore.pyd
    File .\QtDeclarative\QtDeclarative.pyd
    File .\QtDesigner\QtDesigner.pyd
    File .\QtGui\QtGui.pyd
    File .\QtHelp\QtHelp.pyd
    File .\QtMultimedia\QtMultimedia.pyd
    File .\QtNetwork\QtNetwork.pyd
    File .\QtOpenGL\QtOpenGL.pyd
    File .\QtScript\QtScript.pyd
    File .\QtScriptTools\QtScriptTools.pyd
    File .\QtSql\QtSql.pyd
    File .\QtSvg\QtSvg.pyd
    File .\QtTest\QtTest.pyd
    File .\QtWebKit\QtWebKit.pyd
    File .\QtXml\QtXml.pyd
    File .\QtXmlPatterns\QtXmlPatterns.pyd
    File .\phonon\phonon.pyd
    #File ..\pyqtwinmigrate\sipQtWinMigrate\QtWinMigrate.pyd
    #File ..\qscintilla\Python\QSci.pyd
SectionEnd

Section "Developer tools" SecTools
    SectionIn 1

    SetOverwrite on

    SetOutPath $INSTDIR
    File .\pylupdate\pylupdate4.exe
    File .\pyrcc\pyrcc4.exe
    File .\pyuic\pyuic4.bat

    SetOutPath "${DESIGNER_PATH}"
	File .\designer\release\pythonplugin.dll
	SetOutPath "${DESIGNER_PATH}\python"
    #File ..\qt_installer\designer\python\blurdev_plugin.py

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4
    File /r .\pyuic\uic
    File /r .\elementtree
SectionEnd

Section "Examples and tutorial" SecExamples
    SectionIn 1

    SetOverwrite on

    SetOutPath $PROGRAMFILES\PyQt4
    File /r .\examples
SectionEnd

Section "Start Menu shortcuts" SecShortcuts
    SectionIn 1

    # Make sure this is clean and tidy.
    RMDir /r "$SMPROGRAMS\${PYQT_NAME}"
    CreateDirectory "$SMPROGRAMS\${PYQT_NAME}"

    IfFileExists "$PROGRAMFILES\PyQt4\examples" 0 +2
        CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Examples.lnk" "$PROGRAMFILES\PyQt4\examples"

    CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Uninstall PyQt.lnk" "$INSTDIR\Lib\site-packages\PyQt4\Uninstall.exe"
SectionEnd

Section -post
	# Register the uninstaler only if a cmd line arg was not passed in
	ClearErrors
	${GetParameters} $R1
	${GetOptions} $R1 "/noUninstallerReg" $R2
	IfErrors 0 makeInstaller
		# build a registry safe install path key
		!insertmacro ReplaceSubStr $INSTDIR "\" "/"
		
		# Tell Windows about the package.
		WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PyQt4-$MODIFIED_STR" "UninstallString" '"$INSTDIR\Lib\site-packages\PyQt4\Uninstall.exe"'
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PyQt4-$MODIFIED_STR" "DisplayName" "${PYQT_NAME} ($INSTDIR)"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PyQt4-$MODIFIED_STR" "DisplayVersion" "${PYQT_VERSION}"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PyQt4-$MODIFIED_STR" "Publisher" "Blur Studio"
		WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PyQt4-$MODIFIED_STR" "NoModify" "1"
		WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PyQt4-$MODIFIED_STR" "NoRepair" "1"
	makeInstaller:
		# Create the uninstaller.
		WriteUninstaller "$INSTDIR\Lib\site-packages\PyQt4\Uninstall.exe"
		!insertmacro SetLastInstall "$INSTDIR\Lib\site-packages\PyQt4\Uninstall.exe"
	# Update software.ini so we can track what software is installed on each system
	!insertmacro UpdateSettingsINI ${BLUR_SOFTWARE_INI} "PyQt4 Python${PYTHON_VERSION}${BIT_NAME}" "${PYQT_VERSION}" "${OUT_FILE}"
SectionEnd


# Section description text.
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SecModules} \
"The PyQt4 extension modules: sip, QtCore, QtGui, QtAssistant, QtNetwork, \
QtOpenGL, QtSql, QtSvg and QtXml."
!insertmacro MUI_DESCRIPTION_TEXT ${SecTools} \
"The PyQt4 developer tools: pyuic4, pyrcc4 and pylupdate4."
!insertmacro MUI_DESCRIPTION_TEXT ${SecExamples} \
"Ports to Python of the standard Qt v4 examples and tutorial."
!insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts} \
"This adds shortcuts to your Start Menu."
!insertmacro MUI_FUNCTION_DESCRIPTION_END


Section "Uninstall"
	!ifdef _64_BIT
		SetRegView 64
		${DisableX64FSRedirection}
	!endif

	Push $INSTDIR
	Call un.GetParent
	Call un.GetParent
	Call un.GetParent
	Pop $INSTDIR

    # The modules section.
    Delete $INSTDIR\Lib\site-packages\sip.pyd
    RMDir /r $INSTDIR\Lib\site-packages\PyQt4

	# Remove the designer plugin
	Delete "${DESIGNER_PATH}\python\blurdev_plugin.py"
	# delete python if empty
	#!insertmacro removeEmptyDirectory "${DESIGNER_PATH}\python"
	Delete "${DESIGNER_PATH}\pythonplugin.dll"
	# delete python if empty
	#!insertmacro removeEmptyDirectory "${DESIGNER_PATH}"

    # The shortcuts section.
    RMDir /r "$SMPROGRAMS\${PYQT_NAME}"

    # The tools section.
    Delete $INSTDIR\pylupdate4.exe
    Delete $INSTDIR\pyrcc4.exe
    Delete $INSTDIR\pyuic4.bat

    # The examples section and the installer itself.
	MessageBox MB_YESNO "Do you want to remove the example files? This will remove them for all other installations of ${PYQT_NAME}" /SD IDYES IDNO AfterExamples
		DetailPrint "Removing the example files"
		RMDir /r "$PROGRAMFILES\PyQt4"
		Goto Next
	AfterExamples:
		DetailPrint "Not removing the example files in $PROGRAMFILES\PyQt4"
	Next:

	# Can't store \'s as a registry key so replace with /
	!insertmacro un.ReplaceSubStr $INSTDIR "\" "/"

    # Clean the registry.
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PyQt4-$MODIFIED_STR"
SectionEnd
