

; DESCRIPTION: Freezer installer script
; (C) Blur Studio 2005

!define MUI_PRODUCT "Freezer"
!define MUI_VERSION "v1.0.X"

Name "${MUI_PRODUCT} ${MUI_VERSION} ${PLATFORM}"

!include "MUI.nsh"

; Name of resulting executable installer
OutFile "af_install_${PLATFORM}.exe"
InstallDir "C:\\arsenalsuite\\freezer\\"

!define MUI_FINISHPAGE
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!define MUI_HEADERBITMAP "freezer.bmp"

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
!define MUI_FINISHPAGE_TITLE "Freezer Installation Complete"
!define MUI_FINISHPAGE_TEXT "\n\r\nClick Finish now to close the installation program."

#SilentInstall silent

!insertmacro MUI_PAGE_INSTFILES ; File installation page

Section "install"
	# nuke old freezer installs, if any
	RMDir /r "C:\freezer\*.*"
	RMDIR /r "C:\freezer"
	Delete "$DESKTOP\short*freezer*lnk"
	Delete "$QUICKLAUNCH\short*freezer*lnk"
	SetOutPath $INSTDIR
	File af.exe
  	CreateShortCut "$DESKTOP\Freezer 1.0.lnk" "$INSTDIR\freezer.exe" ""
	CreateShortcut "$QUICKLAUNCH\Freezer 1.0.lnk" "$INSTDIR\freezer.exe" ""
	File freezer.ini
    File ..\..\lib\stone\stone.dll
    File ..\..\lib\stonegui\stonegui.dll
    File ..\..\lib\classes\classes.dll
    File ..\..\lib\freezer\freezer.dll
	File ..\..\lib\classesui\classesui.dll
	File ..\..\lib\absubmit\absubmit.dll
	File /r afplugins
	SetOutPath $INSTDIR\images
	File "images\*.*"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "DisplayName" "${MUI_PRODUCT} (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteUninstaller "$INSTDIR\uninstall.exe"

SectionEnd

Section "Uninstall"

	;Delete Files and directory
	RMDir /r "$INSTDIR\*.*"
	RMDir "$INSTDIR"
	Delete "$DESKTOP\Freezer 1.0.lnk"
	Delete "$QUICKLAUNCH\Freezer 1.0.lnk"

	;Delete Uninstaller And Unistall Registry Entries
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${MUI_PRODUCT}"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}"  

SectionEnd

