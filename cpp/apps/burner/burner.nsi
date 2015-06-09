; DESCRIPTION: Burner installer script
; (C) Blur Studio 2005

!define MUI_PRODUCT "Burner"
!define MUI_VERSION "v1.3.X"


Name "${MUI_PRODUCT} ${MUI_VERSION} ${PLATFORM}"

#!include "MUI.nsh"

; Name of resulting executable installer
OutFile "ab_install_${PLATFORM}.exe"
InstallDir "C:\arsenalsuite\burner\"

Page directory
Page instfiles

#SilentInstall silent

Section "install"
	StrCmp $R0 "0" skipsleep
	Sleep 3000
	skipsleep:
    Delete $INSTDIR\*.dll
    SetOutPath $INSTDIR
	File burner.exe
	File psmon\abpsmon.exe
	File burner.ini
	File runScriptJob.ms
    File ..\..\lib\stone\stone.dll
    File ..\..\lib\stonegui\stonegui.dll
    File ..\..\lib\classes\classes.dll
	CreateShortCut "$DESKTOP\Burner.lnk" "$INSTDIR\burner.exe" ""
	CreateShortCut "$QUICKLAUNCH\Burner.lnk" "$INSTDIR\burner.exe" ""
    SetOutPath $INSTDIR\plugins
    File /r "plugins\*.*"
	; Delete everything from spool directory, then re-create it
	RMDir /r "$INSTDIR\spool"
	CreateDirectory "$INSTDIR\spool"
	; Disable windows error reporting for burner
	DeleteRegKey HKLM "SOFTWARE\Microsoft\PCHealth\ErrorReporting\ExclusionList\burner.exe"
	WriteRegDWORD HKLM "SOFTWARE\Microsoft\PCHealth\ErrorReporting\ExclusionList" "burner.exe" 1
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "DisplayName" "${MUI_PRODUCT} (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section "Uninstall"
	;Delete Files and directory
	RMDir /r "$INSTDIR\*.*"
	RMDir "$INSTDIR"
	
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
