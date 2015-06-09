
; DESCRIPTION: Qt installer script
; (C) Blur Studio 2005-2012

!define MUI_VERSION "4.8.3"
!define MUI_ITERATION "0"

!define MUI_PRODUCT "Blur Qt Development ${MUI_VERSION}-${MUI_ITERATION} Programs"

!if ${PLATFORM} == "win32-msvc2005_64"
	!define _64_BIT
!endif
!if ${PLATFORM} == "win32-msvc2008_64"
	!define _64_BIT
!endif

!ifdef _64_BIT
	InstallDir "C:\windows\system32\blur64\"
	!define QTDIR "e:\Qt\${MUI_VERSION}_64"
!else
	InstallDir "C:\blur\common\"
	!define QTDIR "e:\Qt\${MUI_VERSION}"
!endif

Name "${MUI_PRODUCT} ${MUI_VERSION} ${PLATFORM}"

!include "addtopath.nsh"
!include "MUI.nsh"
!include "x64.nsh"

; Name of resulting executable installer
OutFile "qt-dev_install_${MUI_VERSION}-${MUI_ITERATION}_${PLATFORM}.exe"


!define MUI_FINISHPAGE
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!define MUI_HEADERBITMAP "assfreezer.bmp"

; The icon for the installer title bar and .exe file
;!define MUI_ICON "myapp.ico"
; I want to use my product logo in the installer
;!define MUI_HEADERIMAGE
; Here is my product logo
;!define MUI_HEADERIMAGE_BITMAP "myapp.bmp"
; I want the logo to be on the right side of the installer, not the left
;!define MUI_HEADERIMAGE_RIGHT
; I've written a function that I want to be called when the user cancels the installation
;!define MUI_CUSTOMFUNCTION_ABORT myOnAbort
; Override the text on the Finish Page
!define MUI_FINISHPAGE_TITLE "${MUI_PRODUCT} Installation Complete"
!define MUI_FINISHPAGE_TEXT "\n\r\nClick Finish now to close the installation program."

#SilentInstall silent



!insertmacro MUI_PAGE_INSTFILES ; File installation page

Section "install"
!ifdef _64_BIT
   ${DisableX64FSRedirection}
!endif

	SetOutPath $INSTDIR
	
	File "${QTDIR}\bin\assistant.exe"
	File "${QTDIR}\bin\designer.exe"
	File "${QTDIR}\bin\idc.exe"
	File "${QTDIR}\bin\linguist.exe"
	File "${QTDIR}\bin\moc.exe"
	File "${QTDIR}\bin\pixeltool.exe"
	File "${QTDIR}\bin\qcollectiongenerator.exe"
	File "${QTDIR}\bin\qhelpgenerator.exe"
	File "${QTDIR}\bin\qhelpconverter.exe"
	File "${QTDIR}\bin\qmake.exe"
	File "${QTDIR}\bin\rcc.exe"
	File "${QTDIR}\bin\uic.exe"
	File "${QTDIR}\bin\xmlpatterns.exe"
	File "${QTDIR}\bin\xmlpatternsvalidator.exe"

	File qt.conf

	SetOutPath $INSTDIR\doc
	File /r "${QTDIR}\doc\qch"
	File /r "${QTDIR}\doc\html"

	SetOutPath $INSTDIR
	WriteUninstaller "$INSTDIR\qt_dev_uninstall.exe"

SectionEnd

Section "Uninstall"

	;Delete Files and directory
	Delete "$INSTDIR\designer.exe"
	Delete "$INSTDIR\assistant.exe"
	Delete "$INSTDIR\qmake.exe"
	Delete "$INSTDIR\rcc.exe"
	Delete "$INSTDIR\linguist.exe"
	Delete "$INSTDIR\idc.exe"
	Delete "$INSTDIR\moc.exe"
	Delete "$INSTDIR\pixeltool.exe"
	Delete "$INSTDIR\uic.exe"
	Delete "$INSTDIR\qhelpgenerator.exe"
	Delete "$INSTDIR\qcollectiongenerator.exe"
	Delete "$INSTDIR\qhelpconverter.exe"

;Delete Uninstaller And Unistall Registry Entries
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${MUI_PRODUCT}"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}"  

SectionEnd

