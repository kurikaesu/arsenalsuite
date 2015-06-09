; DESCRIPTION: Blur MaxScript Extension installer script
; (C) Blur Studios 2006


!define MUI_PRODUCT "Blur MaxScript Extension"
!define MUI_VERSION "v1006"
Name "${MUI_PRODUCT} ${MUI_VERSION}"

# These will change with different releases.
!define MAX_VERSION "8"

# These are all derived from the above.
!define DLX_INSTALLDIR  "C:\max${MAX_VERSION}\plugins\blurbeta\BlurLib_7\"

; Name of resulting executable installer
OutFile "blurDlx_install_${MUI_VERSION}.exe"
InstallDir "${DLX_INSTALLDIR}"

SilentInstall silent

Section "install"
	IfFileExists $INSTDIR MaxPathExists
		Abort ;only install this in the computers that have the related version of max installed

	MaxPathExists:
	DetailPrint "${DLX_INSTALLDIR}"
	SetOverwrite on
	SetOutPath $INSTDIR
	File "BlurDLX.dlx"
	File "BlurDLX_version.txt"
	
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "DisplayName" "${MUI_PRODUCT} (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteUninstaller "$INSTDIR\BlurDLXUninstall.exe"
SectionEnd

Section "Uninstall"
;	;Delete Files
    SetOutPath $INSTDIR
	Delete $INSTDIR\BlurDLX.dlx
    	Delete $INSTDIR\BlurDLX_version.txt
	Delete "$INSTDIR\BlurDLXUninstall.exe"

	;Delete Uninstaller And Unistall Registry Entries
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${MUI_PRODUCT}"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}"  
SectionEnd

