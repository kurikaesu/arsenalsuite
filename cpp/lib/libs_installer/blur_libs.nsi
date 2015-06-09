

; DESCRIPTION: AssFreezer installer script
; (C) Blur Studio 2005

!include blur-svnrev.nsi

!define MUI_PRODUCT "Blur Libs"
!define MUI_VERSION "1.0.0"

Name "${MUI_PRODUCT} ${MUI_VERSION} ${PLATFORM}"

!include "MUI.nsh"
!include "x64.nsh"

; Name of resulting executable installer
OutFile "blur_libs_install_${MUI_SVNREV}_${PLATFORM}.exe"

!if ${PLATFORM} == "win32-msvc2005_64"
	InstallDir "C:\windows\system32\"
!else
	InstallDir "C:\blur\common\"
!endif

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
	SetOutPath $INSTDIR
!if ${PLATFORM} == "win32-msvc2005_64"
   ${DisableX64FSRedirection}
!else
!endif
	File ..\stone\stone.dll
	File ..\classes\classes.dll
	File ..\stonegui\stonegui.dll
	File ..\classesui\classesui.dll
	File ..\classes\ng_schema.xml
	File ..\absubmit\absubmit.dll
	File blur_libs_version.txt
#	File ..\assfreezer\assfreezer.dll
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "DisplayName" "${MUI_PRODUCT} (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteUninstaller "$INSTDIR\uninstall.exe"

SectionEnd

Section "Uninstall"

	;Delete Files and directory
	Delete $INSTDIR\stone.dll
	Delete $INSTDIR\classes.dll
	Delete $INSTDIR\stonegui.dll
#	Delete $INSTDIR\assfreezer.dll
	Delete $INSTDIR\ng_schema.xml
	Delete $INSTDIR\absubmit.dll
;Delete Uninstaller And Unistall Registry Entries
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${MUI_PRODUCT}"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}"  

SectionEnd

