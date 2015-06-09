

; DESCRIPTION: AssFreezer installer script
; (C) Blur Studio 2005

;!include pyqt-svnrev.nsi
!define MUI_PRODUCT "Blur Python Modules"
!define MUI_VERSION "v1.0.X"

Name "${MUI_PRODUCT} ${MUI_VERSION} ${PLATFORM}"

!include "MUI.nsh"

; Name of resulting executable installer
OutFile "blur_pyqt_modules_install_${MUI_VERSION}_${PLATFORM}.exe"
InstallDir "${PYTHON_PATH}\Lib\site-packages\"

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
!define MUI_FINISHPAGE_TITLE "PyQt Installation Complete"
!define MUI_FINISHPAGE_TEXT "\n\r\nClick Finish now to close the installation program."

#SilentInstall silent

!insertmacro MUI_PAGE_INSTFILES ; File installation page

Section "install"
	# nuke old assfreezer installs, if any
	SetOutPath "$INSTDIR\blur"
	File ..\stone\sipStone\Stone.pyd
	File ..\classes\sipClasses\Classes.pyd
	File ..\stonegui\sipStonegui\Stonegui.pyd
	File ..\classesui\sipClassesui\Classesui.pyd
	File ..\absubmit\sipAbsubmit\absubmit.pyd
	SetOutPath $INSTDIR
	File /r "..\..\..\python\blur"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "DisplayName" "${MUI_PRODUCT} (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteUninstaller "$INSTDIR\pyqt_uninstall.exe"
SectionEnd

Section "Uninstall"
	;Delete Files and directory
	Delete $INSTDIR\Stone.pyd
	Delete $INSTDIR\Classes.pyd
	Delete $INSTDIR\Stonegui.pyd
	Delete $INSTDIR\absubmit.pyd
	RMDir $INSTDIR\blur

	;Delete Uninstaller And Unistall Registry Entries
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${MUI_PRODUCT}"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}"  
SectionEnd

