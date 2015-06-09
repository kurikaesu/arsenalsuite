

; DESCRIPTION: AssFreezer installer script
; (C) Blur Studio 2005

!define MUI_VERSION "4.3.0-0"
!define MUI_PRODUCT "Blur Qt ${MUI_VERSION} DLLs"

!define QTDIR "C:\Qt\4.3.0"

Name "${MUI_PRODUCT} ${MUI_VERSION}"

!include "addtopath.nsh"

!include "MUI.nsh"

; Name of resulting executable installer
OutFile "qt-debug_install_${MUI_VERSION}.exe"
InstallDir "C:\blur\common\"

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
	File "${QTDIR}\bin\QtGuid4.dll"
	File "${QTDIR}\bin\QtOpenGLd4.dll"
	File "${QTDIR}\bin\QtNetworkd4.dll"
	File "${QTDIR}\bin\QtSqld4.dll"
	File "${QTDIR}\bin\QtCored4.dll"
	File "${QTDIR}\bin\QtXmld4.dll"
	SetOutPath $INSTDIR\sqldrivers
	File "${QTDIR}\plugins\sqldrivers\qsqlpsqld4.dll"
	SetOutPath $INSTDIR\imageformats
	File "${QTDIR}\plugins\imageformats\qjpegd4.dll"
;	File "${QTDIR}\plugins\imageformats\qgifd4.dll"
	File ..\qimageio\tga\tgad.dll
	Push $INSTDIR
	Call AddToPath
	ReadEnvStr $R0 "PATH"
	StrCpy $R0 "$R0;$INSTDIR}"
	System::Call 'Kernel32::SetEnvironmentVariableA(t, t) i("PATH", R0).r0'
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "DisplayName" "${MUI_PRODUCT} (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteUninstaller "$INSTDIR\uninstall.exe"

SectionEnd

Section "Uninstall"

	;Delete Files and directory
	Delete $INSTDIR\QtGui4.dll
	Delete $INSTDIR\QtNetwork4.dll
	Delete $INSTDIR\QtSql4.dll
	Delete $INSTDIR\QtCore4.dll
	Delete $INSTDIR\QtXml4.dll
	Delete $INSTDIR\QtOpenGL4.dll
	Delete $INSTDIR\mingwm10.dll
	Delete $INSTDIR\libpq.dll
	Delete $INSTDIR\mingwm10.dll
	Delete $INSTDIR\zlib1.dll
	Delete $INSTDIR\jpeg62.dll
	Delete $INSTDIR\libimage.dll
	Delete $INSTDIR\libpng13.dll
	Delete $INSTDIR\libtiff3.dll
	Delete $INSTDIR\uninstall.exe
	RMDir /r $INSTDIR\sqldrivers
	RMDir /r $INSTDIR\imageformats

;Delete Uninstaller And Unistall Registry Entries
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${MUI_PRODUCT}"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}"  

SectionEnd

