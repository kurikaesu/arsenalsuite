# PyQt4 NSIS installer script.
# 
# Copyright (c) 2009 Riverbank Computing Limited <info@riverbankcomputing.com>
# 
# This file is part of PyQt.
# 
# This file may be used under the terms of the GNU General Public
# License versions 2.0 or 3.0 as published by the Free Software
# Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
# included in the packaging of this file.  Alternatively you may (at
# your option) use any later version of the GNU General Public
# License if such license has been publicly approved by Riverbank
# Computing Limited (or its successors, if any) and the KDE Free Qt
# Foundation. In addition, as a special exception, Riverbank gives you
# certain additional rights. These rights are described in the Riverbank
# GPL Exception version 1.1, which can be found in the file
# GPL_EXCEPTION.txt in this package.
# 
# Please review the following information to ensure GNU General
# Public Licensing requirements will be met:
# http://trolltech.com/products/qt/licenses/licensing/opensource/. If
# you are unsure which license is appropriate for your use, please
# review the following information:
# http://trolltech.com/products/qt/licenses/licensing/licensingoverview
# or contact the sales department at sales@riverbankcomputing.com.
# 
# This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
# WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


# These will change with different releases.
!define PYQT_VERSION        "4.6.1"
!define PYQT_INSTALLER      "1"
!define PYQT_LICENSE        "GPL"
!define PYQT_LICENSE_LC     "gpl"
!define PYQT_QT_VERS        "4.5.2"
!define PYQT_QT_DOC_VERS    "4.5"

# These are all derived from the above.
!define PYQT_PYTHON_DIR     "C:\Python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}"
!define PYQT_PYTHON_VERS    "${PYTHON_VERSION}"
!define PYQT_PYTHON_HK      "Software\Python\PythonCore\${PYQT_PYTHON_VERS}\InstallPath"
!define PYQT_NAME           "PyQt ${PYQT_LICENSE} v${PYQT_VERSION} for Python v${PYQT_PYTHON_VERS}"
!define PYQT_FULL_VERSION   "${PYQT_VERSION}-${PYQT_INSTALLER}"
!define PYQT_HK_ROOT        "Software\PyQt4\Py${PYQT_PYTHON_VERS}"
!define PYQT_HK             "${PYQT_HK_ROOT}\InstallPath"
!define PYQT_QT_DIR         "C:\Qt\${PYQT_QT_VERS}"


# Include the tools we use.
!include MUI2.nsh
!include LogicLib.nsh
!include AddToPath.nsh
!include StrSlash.nsh


# Tweak some of the standard pages.
!define MUI_WELCOMEPAGE_TEXT \
"This wizard will guide you through the installation of ${PYQT_NAME}.$\r$\n\
$\r$\n\
This copy of PyQt includes Qt v${PYQT_QT_VERS} Open Source Edition.$\r$\n\
$\r$\n\
Any code you write must be released under a license that is compatible with \
the GPL.$\r$\n\
$\r$\n\
Click Next to continue."

!define MUI_FINISHPAGE_LINK "Get the latest news of PyQt here"
!define MUI_FINISHPAGE_LINK_LOCATION "http://www.riverbankcomputing.com"


# Define the product name and installer executable.
Name "PyQt"
Caption "${PYQT_NAME} Setup"
OutFile "PyQt-Py${PYQT_PYTHON_VERSION}-${PYQT_LICENSE_LC}-${PYQT_FULL_VERSION}.exe"


# The different installation types.  "Full" is everything.  "Minimal" is the
# runtime environment.
InstType "Full"
InstType "Minimal"


# Maximum compression.
SetCompressor /SOLID lzma


# We want the user to confirm they want to cancel.
!define MUI_ABORTWARNING

Function .onInit
    # Check if there is already a version of PyQt installed for this version of
    # Python.
    ReadRegStr $0 HKCU "${PYQT_HK}" ""

    ${If} $0 == ""
        ReadRegStr $0 HKLM "${PYQT_HK}" ""
    ${Endif}

    ${If} $0 != ""
        MessageBox MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION \
"A copy of PyQt for Python v${PYQT_PYTHON_VERS} is already installed in $0 \
and should be uninstalled first.$\r$\n \
$\r$\n\
Do you wish to continue with this installation?" IDYES Overwrite
            Abort
Overwrite:
    ${Endif}

    # Check the right version of Python has been installed.
    ReadRegStr $INSTDIR HKCU "${PYQT_PYTHON_HK}" ""

    ${If} $INSTDIR == ""
        ReadRegStr $INSTDIR HKLM "${PYQT_PYTHON_HK}" ""
    ${Endif}

    ${If} $INSTDIR == ""
        MessageBox MB_YESNO|MB_ICONQUESTION \
"This copy of PyQt has been built against Python v${PYQT_PYTHON_VERS} which \
doesn't seem to be installed.$\r$\n\
$\r$\n\
Do you wish to continue with the installation?" IDYES GotPython
            Abort
GotPython:
        StrCpy $INSTDIR "${PYQT_PYTHON_DIR}"
    ${Endif}
FunctionEnd


# Define the different pages.
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE ".\LICENSE-MERGED-GPL2-GPL3"
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

    SetOverwrite on

    # We have to take the SIP files from where they should have been installed.
    SetOutPath $INSTDIR\Lib\site-packages
    File "${PYQT_PYTHON_DIR}\Lib\site-packages\sip.pyd"

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4
    File .\LICENSE.GPL2
    File .\LICENSE.GPL3
    File .\GPL_EXCEPTION.TXT
    File .\GPL_EXCEPTION_ADDENDUM.TXT
    File .\OPENSOURCE-NOTICE.TXT
    File .\__init__.py
    File .\Qt\Qt.pyd
    File .\QtAssistant\QtAssistant.pyd
    File .\QtCore\QtCore.pyd
    File .\QtDesigner\QtDesigner.pyd
    File .\QtGui\QtGui.pyd
    File .\QtHelp\QtHelp.pyd
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
    File .\QAxContainer\QAxContainer.pyd
    File .\phonon\phonon.pyd
    File /r .\pyuic\uic

    # QScintilla.
    #File "${PYQT_PYTHON_DIR}\Lib\site-packages\PyQt4\Qsci.pyd"
    #File /r "${PYQT_QT_DIR}\qsci"
    #File "${PYQT_QT_DIR}\lib\qscintilla2.dll"
SectionEnd

Section "Qt runtime" SecQt
    SectionIn 1 2

    SetOverwrite on

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4
    File "${PYQT_QT_DIR}\bin\QtAssistantClient4.dll"
    File "${PYQT_QT_DIR}\bin\QtCLucene4.dll"
    File "${PYQT_QT_DIR}\bin\QtCore4.dll"
    File "${PYQT_QT_DIR}\bin\QtDesigner4.dll"
    File "${PYQT_QT_DIR}\bin\QtDesignerComponents4.dll"
    File "${PYQT_QT_DIR}\bin\QtGui4.dll"
    File "${PYQT_QT_DIR}\bin\QtHelp4.dll"
    File "${PYQT_QT_DIR}\bin\QtNetwork4.dll"
    File "${PYQT_QT_DIR}\bin\QtOpenGL4.dll"
    File "${PYQT_QT_DIR}\bin\QtScript4.dll"
    File "${PYQT_QT_DIR}\bin\QtScriptTools4.dll"
    File "${PYQT_QT_DIR}\bin\QtSql4.dll"
    File "${PYQT_QT_DIR}\bin\QtSvg4.dll"
    File "${PYQT_QT_DIR}\bin\QtTest4.dll"
    File "${PYQT_QT_DIR}\bin\QtWebKit4.dll"
    File "${PYQT_QT_DIR}\bin\QtXml4.dll"
    File "${PYQT_QT_DIR}\bin\QtXmlPatterns4.dll"
    File "${PYQT_QT_DIR}\bin\phonon4.dll"

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4\plugins\codecs
    File "${PYQT_QT_DIR}\plugins\codecs\*.dll"

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4\plugins\designer
    File "${PYQT_QT_DIR}\plugins\designer\*.dll"

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4\plugins\graphicssystems
    File "${PYQT_QT_DIR}\plugins\graphicssystems\*.dll"

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4\plugins\iconengines
    File "${PYQT_QT_DIR}\plugins\iconengines\*.dll"

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4\plugins\imageformats
    File "${PYQT_QT_DIR}\plugins\imageformats\*.dll"

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4\plugins\phonon_backend
    File "${PYQT_QT_DIR}\plugins\phonon_backend\*.dll"

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4\plugins\sqldrivers
    File "${PYQT_QT_DIR}\plugins\sqldrivers\*.dll"

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4\translations
    File "${PYQT_QT_DIR}\translations\*.qm"

    # Tell the Python and Qt executables where to find Qt.
    Push $INSTDIR
    Push "\"
    Call StrSlash
    Pop $R0

    FileOpen $0 $INSTDIR\qt.conf w
    FileWrite $0 "[Paths]$\r$\n"
    FileWrite $0 "Prefix = $R0/Lib/site-packages/PyQt4$\r$\n"
    FileWrite $0 "Binaries = .$\r$\n"
    FileClose $0

    FileOpen $0 $INSTDIR\Lib\site-packages\PyQt4\qt.conf w
    FileWrite $0 "[Paths]$\r$\n"
    FileWrite $0 "Prefix = $R0/Lib/site-packages/PyQt4$\r$\n"
    FileWrite $0 "Binaries = .$\r$\n"
    FileClose $0
SectionEnd

Section "Developer tools" SecTools
    SectionIn 1

    SetOverwrite on

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4
    File .\pylupdate\pylupdate4.exe
    File .\pyrcc\pyrcc4.exe

    FileOpen $0 $INSTDIR\Lib\site-packages\PyQt4\pyuic4.bat w
    FileWrite $0 "@$\"$INSTDIR\python$\" $\"$INSTDIR\Lib\site-packages\PyQt4\uic\pyuic.py$\" %1 %2 %3 %4 %5 %6 %7 %8 %9$\r$\n"
    FileClose $0
SectionEnd

Section "Qt developer tools" SecQtTools
    SectionIn 1

    SetOverwrite on

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4
    File "${PYQT_QT_DIR}\bin\assistant.exe"
    File "${PYQT_QT_DIR}\bin\designer.exe"
    File "${PYQT_QT_DIR}\bin\linguist.exe"
    File "${PYQT_QT_DIR}\bin\lrelease.exe"
    File "${PYQT_QT_DIR}\bin\xmlpatterns.exe"
SectionEnd

Section "SIP developer tools" SecSIPTools
    SectionIn 1

    SetOverwrite on

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4
    File "${PYQT_PYTHON_DIR}\Lib\site-packages\PyQt4\sip.exe"
    File /r "${PYQT_PYTHON_DIR}\Lib\site-packages\PyQt4\sip"
    File .\pyqtconfig.py

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4\include
    File "${PYQT_PYTHON_DIR}\Lib\site-packages\PyQt4\include\sip.h"

    SetOutPath $INSTDIR\Lib\site-packages
    File "${PYQT_PYTHON_DIR}\Lib\site-packages\sipconfig.py"
    File "${PYQT_PYTHON_DIR}\Lib\site-packages\sipdistutils.py"
SectionEnd

Section "Documentation" SecDocumentation
    SectionIn 1

    SetOverwrite on

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4
    File /r .\doc
SectionEnd

Section "Examples and tutorial" SecExamples
    SectionIn 1

    SetOverwrite on

    SetOutPath $INSTDIR\Lib\site-packages\PyQt4
    File /r .\examples
SectionEnd

Section "Start Menu shortcuts" SecShortcuts
    SectionIn 1

    # Make sure this is clean and tidy.
    RMDir /r "$SMPROGRAMS\${PYQT_NAME}"
    CreateDirectory "$SMPROGRAMS\${PYQT_NAME}"

    IfFileExists "$INSTDIR\Lib\site-packages\PyQt4\assistant.exe" 0 +4
        CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Assistant.lnk" "$INSTDIR\Lib\site-packages\PyQt4\assistant.exe"
        CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Designer.lnk" "$INSTDIR\Lib\site-packages\PyQt4\designer.exe"
        CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Linguist.lnk" "$INSTDIR\Lib\site-packages\PyQt4\linguist.exe"

    IfFileExists "$INSTDIR\Lib\site-packages\PyQt4\doc" 0 +5
        CreateDirectory "$SMPROGRAMS\${PYQT_NAME}\Documentation"
        CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Documentation\PyQt Documentation.lnk" "$INSTDIR\Lib\site-packages\PyQt4\doc\pyqt4ref.html"
        CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Documentation\PyQt API Reference.lnk" "$INSTDIR\Lib\site-packages\PyQt4\doc\html\classes.html"
	CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Documentation\Qt Documentation.lnk" "http://doc.trolltech.com/${PYQT_QT_DOC_VERS}/"

    IfFileExists "$INSTDIR\Lib\site-packages\PyQt4\examples" 0 +6
        CreateDirectory "$SMPROGRAMS\${PYQT_NAME}\Examples"
	SetOutPath $INSTDIR\Lib\site-packages\PyQt4\examples\demos\qtdemo
        CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Examples\PyQt Examples and Demos.lnk" "$INSTDIR\Lib\site-packages\PyQt4\examples\demos\qtdemo\qtdemo.pyw"
	SetOutPath $INSTDIR
        CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Examples\PyQt Examples Source.lnk" "$INSTDIR\Lib\site-packages\PyQt4\examples"

    CreateDirectory "$SMPROGRAMS\${PYQT_NAME}\Links"
    CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Links\PyQt Book.lnk" "http://www.qtrac.eu/pyqtbook.html"
    CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Links\PyQt Homepage.lnk" "http://www.riverbankcomputing.com/software/pyqt/"
    CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Links\Qt Homepage.lnk" "http://www.qtsoftware.com/"
    CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Links\QScintilla Homepage.lnk" "http://www.riverbankcomputing.com/software/qscintilla/"
    CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Links\PyQwt Homepage.lnk" "http://pyqwt.sourceforge.net/"
    CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Links\Qwt Homepage.lnk" "http://qwt.sourceforge.net/"
    CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Links\eric Homepage.lnk" "http://eric-ide.python-projects.org/index.html"

    CreateShortCut "$SMPROGRAMS\${PYQT_NAME}\Uninstall PyQt.lnk" "$INSTDIR\Lib\site-packages\PyQt4\Uninstall.exe"
SectionEnd

Section -post
    # Add the bin directory to PATH.
    Push $INSTDIR\Lib\site-packages\PyQt4
    Call AddToPath

    # Tell Windows about the package.
    WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PYQT_NAME}" "UninstallString" '"$INSTDIR\Lib\site-packages\PyQt4\Uninstall.exe"'
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PYQT_NAME}" "DisplayName" "${PYQT_NAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PYQT_NAME}" "DisplayVersion" "${PYQT_FULL_VERSION}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PYQT_NAME}" "NoModify" "1"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PYQT_NAME}" "NoRepair" "1"

    # Save the installation directory for the uninstaller.
    ClearErrors
    WriteRegStr HKLM "${PYQT_HK}" "" $INSTDIR
    IfErrors 0 +2
        WriteRegStr HKCU "${PYQT_HK}" "" $INSTDIR

    # Create the uninstaller.
    WriteUninstaller "$INSTDIR\Lib\site-packages\PyQt4\Uninstall.exe"
SectionEnd


# Section description text.
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SecModules} \
"The PyQt, QScintilla and sip extension modules."
!insertmacro MUI_DESCRIPTION_TEXT ${SecQt} \
"The Qt DLLs, plugins and translations."
!insertmacro MUI_DESCRIPTION_TEXT ${SecTools} \
"The PyQt developer tools: pyuic4, pyrcc4 and pylupdate4."
!insertmacro MUI_DESCRIPTION_TEXT ${SecQtTools} \
"The Qt developer tools: QtAssistant, Qt Designer and Qt Linguist."
!insertmacro MUI_DESCRIPTION_TEXT ${SecSIPTools} \
"The SIP developer tools and .sip files."
!insertmacro MUI_DESCRIPTION_TEXT ${SecDocumentation} \
"The PyQt and related documentation."
!insertmacro MUI_DESCRIPTION_TEXT ${SecExamples} \
"Ports to Python of the standard Qt v4 examples and tutorial."
!insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts} \
"This adds shortcuts to your Start Menu."
!insertmacro MUI_FUNCTION_DESCRIPTION_END


Function un.onInit
    # Get the installation directory.
    ReadRegStr $INSTDIR HKCU "${PYQT_HK}" ""

    ${If} $INSTDIR == ""
        ReadRegStr $INSTDIR HKLM "${PYQT_HK}" ""

        ${If} $INSTDIR == ""
            # Try where Python was installed.
            ReadRegStr $INSTDIR HKCU "${PYQT_PYTHON_HK}" ""

            ${If} $INSTDIR == ""
                ReadRegStr $INSTDIR HKLM "${PYQT_PYTHON_HK}" ""

                ${If} $INSTDIR != ""
                    # Default to where Python should be installed.
                    StrCpy $INSTDIR "${PYQT_PYTHON_DIR}\"
                ${Endif}
            ${Endif}
        ${Endif}
    ${Endif}
FunctionEnd


Section "Uninstall"
    # Remove the bin directory from PATH.
    Push $INSTDIR\Lib\site-packages\PyQt4
    Call un.RemoveFromPath

    # The Qt path file.
    Delete $INSTDIR\qt.conf

    # The modules section.
    Delete $INSTDIR\Lib\site-packages\sip.pyd
    RMDir /r $INSTDIR\Lib\site-packages\PyQt4

    # The shortcuts section.
    RMDir /r "$SMPROGRAMS\${PYQT_NAME}"

    # The SIP tools section.
    Delete $INSTDIR\Lib\site-packages\sipconfig.py
    Delete $INSTDIR\Lib\site-packages\sipconfig.pyc
    Delete $INSTDIR\Lib\site-packages\sipconfig.pyo
    Delete $INSTDIR\Lib\site-packages\sipdistutils.py
    Delete $INSTDIR\Lib\site-packages\sipdistutils.pyc
    Delete $INSTDIR\Lib\site-packages\sipdistutils.pyo

    # Clean the registry.
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PYQT_NAME}"
    DeleteRegKey HKLM "${PYQT_HK_ROOT}"
    DeleteRegKey HKCU "${PYQT_HK_ROOT}"
SectionEnd
