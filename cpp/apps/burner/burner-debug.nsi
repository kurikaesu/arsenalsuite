; DESCRIPTION: Burner installer script
; (C) Blur Studion 2005

!include burner-svnrev.nsi
!define MUI_PRODUCT "Burner"
!define MUI_VERSION "v1.3.X"

!define QTDIR "C:\Qt\4.0.1c\"

Name "${MUI_PRODUCT} ${MUI_VERSION}"

#!include "MUI.nsh"

; Name of resulting executable installer
OutFile "ab_install_${MUI_SVNREV}d.exe"
InstallDir "C:\\max5\\burner3\\"

#!define MUI_FINISHPAGE
#!define MUI_FINISHPAGE_NOREBOOTSUPPORT
#!define MUI_HEADERBITMAP "burner.bmp"

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
#!define MUI_FINISHPAGE_TITLE "Burner Installation Complete"
#!define MUI_FINISHPAGE_TEXT "\n\r\nClick Finish now to close the installation program."
#!define MUI_FINISHPAGE_RUN "$INSTDIR\burner.exe"
#!insertmacro MUI_PAGE_INSTFILES ; File installation page
#!insertmacro MUI_PAGE_FINISH
SilentInstall silent


Section "install"
	#Processes::KillProcess "burner.exe"
	#Processes::KillProcess "perl.exe"
	#Sleep 3000
	SetOutPath $INSTDIR
	File burner.exe
	File burner.ini
	CreateShortCut "$DESKTOP\Burner.lnk" "$INSTDIR\burner.exe" ""
	CreateShortCut "$QUICKLAUNCH\Burner.lnk" "$INSTDIR\burner.exe" ""
	File "${QTDIR}\lib\QtGuid4.dll"
	File "${QTDIR}\lib\QtNetworkd4.dll"
	File "${QTDIR}\lib\QtSqld4.dll"
	File "${QTDIR}\lib\QtCored4.dll"
	File "${QTDIR}\lib\QtXmld4.dll"
	File c:\mingw\bin\mingwm10.dll
	File ..\..\..\binaries\libpq.dll
	File ..\..\..\binaries\unzip.exe
	File ..\..\..\binaries\unziplicense.txt
	File ..\..\..\binaries\Tail.exe
	File c:\boost\lib\boost_thread-mgw-mt-d-1_32.dll
	File c:\boost\lib\boost_filesystem-mgw-d-1_32.dll
	File c:\boost\lib\boost_date_time-mgw-d-1_32.dll

	SetOutPath $INSTDIR\sqldrivers
	File ..\..\..\binaries\Qt\plugins\sqldrivers\qsqlpsql.dll

	SetOutPath $INSTDIR\images
	File images\burner.png

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "DisplayName" "${MUI_PRODUCT} (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section "Uninstall"
	;Delete Files and directory
	RMDir /r "$INSTDIR\*.*"
	RMDir "$INSTDIR"
	SetAutoClose true

	Delete "$DESKTOP\Burner.lnk"
	Delete "$QUICKLAUNCH\Burner.lnk"

	;Delete Uninstaller And Unistall Registry Entries
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${MUI_PRODUCT}"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}"  
SectionEnd

Section
	SetOutPath $INSTDIR
	ExecShell "" "$INSTDIR\burner.exe"
SectionEnd
