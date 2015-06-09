; DESCRIPTION: pythondotnet [at blur] installer script
; (C) Blur Studio 2008

!include pythondotnet-svnrev.nsi
!define MUI_PRODUCT 'pythondotnet'
!define MUI_VERSION 'v2.0.alpha.24.rev99'

Name '${MUI_PRODUCT} ${MUI_VERSION}'

!include 'MUI.nsh'

; Name of resulting executable installer
OutFile "pythondotnet_${MUI_VERSION}_install_${MUI_SVNREV}.exe"

InstallDir "C:\python24\DLLs\"

; Override the text on the Finish Page
;!define MUI_FINISHPAGE_TITLE 	'pythondotnet Installation Complete'
;!define MUI_FINISHPAGE_TEXT 	'\n\r\nClick Finish now to close the installation program.'
;!insertmacro MUI_PAGE_INSTFILES ; File installation page

#SilentInstall silent

Section "install"
	SetOutPath $INSTDIR
	File pythonnet\clr.pyd
	File pythonnet\Python.Runtime.dll

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "DisplayName" "${MUI_PRODUCT} (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteUninstaller "$INSTDIR\uninstall.exe"	
SectionEnd

Section "Uninstall"
	;Delete Files
	Delete "$INSTDIR\clr.pyd"
	Delete "$INSTDIR\Python.Runtime.dll"

	;Delete Uninstaller And Unistall Registry Entries
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${MUI_PRODUCT}"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}"
	Delete "$INSTDIR\uninstall.exe"
SectionEnd