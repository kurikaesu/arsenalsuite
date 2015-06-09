

; DESCRIPTION: Qt installer script
; (C) Blur Studio 2005-2012

!define MUI_VERSION "4.8.3"
!define MUI_ITERATION "0"
!define MUI_PRODUCT "Blur Qt ${MUI_VERSION}-${MUI_ITERATION} DLLs"
!define UNINSTALL_PATH "C:\blur\uninstall"

!if ${PLATFORM} == "win32-msvc2005_64"
	!define _64_BIT
!endif
!if ${PLATFORM} == "win32-msvc2008_64"
	!define _64_BIT
!endif

!ifdef _64_BIT
	!define INSTALL_PATH 'C:\windows\system32\blur64'
	!define QTDIR "$%QTDIR%"

	;!define QTDIR "C:\Qt\${MUI_VERSION}_64"
	!define MUI_UNINSTALL_NAME "Blur Qt ${MUI_VERSION}-${MUI_ITERATION} 64Bit DLLs"
	!define UNINSTALL_FILE "${UNINSTALL_PATH}\qt_uninstall_64.exe"
!else
	!define INSTALL_PATH 'C:\blur\common'
	!define QTDIR "$%QTDIR%"

	;!define QTDIR "C:\Qt\${MUI_VERSION}"
	!define MUI_UNINSTALL_NAME "Blur Qt ${MUI_VERSION}-${MUI_ITERATION} 32Bit DLLs"
	!define UNINSTALL_FILE "${UNINSTALL_PATH}\qt_uninstall.exe"
!endif

InstallDir ${INSTALL_PATH}

Name "${MUI_PRODUCT} ${MUI_VERSION} ${PLATFORM}"

!include "addtopath.nsh"
!include "MUI.nsh"
!include "x64.nsh"
!include FileFunc.nsh	; Provide GetOptions and GetParameters
!include "..\..\..\nsis\LastInstall.nsh"
!include "..\..\..\nsis\FileSystem.nsh"

; Name of resulting executable installer
OutFile "qt_install_${MUI_VERSION}-${MUI_ITERATION}_${PLATFORM}.exe"


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
#	Delete "${INSTDIR}\QtAssistantClient4.dll"
	Delete "${INSTDIR}\QtCLucene4.dll"
	Delete "${INSTDIR}\QtCore4.dll"
	Delete "${INSTDIR}\QtDeclarative4.dll"
	Delete "${INSTDIR}\QtDesigner4.dll"
	Delete "${INSTDIR}\QtDesignerComponents4.dll"
	Delete "${INSTDIR}\QtGui4.dll"
	Delete "${INSTDIR}\QtHelp4.dll"
	Delete "${INSTDIR}\QtMultimedia4.dll"
	Delete "${INSTDIR}\QtNetwork4.dll"
	Delete "${INSTDIR}\QtOpenGL4.dll"
	Delete "${INSTDIR}\QtScript4.dll"
	Delete "${INSTDIR}\QtScriptTools4.dll"
	Delete "${INSTDIR}\QtSql4.dll"
	Delete "${INSTDIR}\QtSvg4.dll"
	Delete "${INSTDIR}\QtTest4.dll"
	Delete "${INSTDIR}\QtWebKit4.dll"
	Delete "${INSTDIR}\QtXml4.dll"
	Delete "${INSTDIR}\QtXmlPatterns4.dll"
	Delete "${INSTDIR}\QtTest4.dll"
	Delete "${INSTDIR}\phonon4.dll"
	Delete "${INSTDIR}\qscintilla2.dll"
	RMDir /r "${INSTDIR}\sqldrivers"
	RMDir /r "${INSTDIR}\imageformats"
!endif

	SetOutPath $INSTDIR
#	File "${QTDIR}\bin\QtAssistantClient4.dll"
	File "${QTDIR}\bin\QtCLucene4.dll"
	File "${QTDIR}\bin\QtCore4.dll"
	File "${QTDIR}\bin\QtDeclarative4.dll"
	File "${QTDIR}\bin\QtDesigner4.dll"
	File "${QTDIR}\bin\QtDesignerComponents4.dll"
	File "${QTDIR}\bin\QtGui4.dll"
	File "${QTDIR}\bin\QtHelp4.dll"
	File "${QTDIR}\bin\QtMultimedia4.dll"
	File "${QTDIR}\bin\QtNetwork4.dll"
	File "${QTDIR}\bin\QtOpenGL4.dll"
	File "${QTDIR}\bin\QtScript4.dll"
	File "${QTDIR}\bin\QtScriptTools4.dll"
	File "${QTDIR}\bin\QtSql4.dll"
	File "${QTDIR}\bin\QtSvg4.dll"
	File "${QTDIR}\bin\QtTest4.dll"
	File "${QTDIR}\bin\QtWebKit4.dll"
	File "${QTDIR}\bin\QtXml4.dll"
#	File "${QTDIR}\bin\QtXmlPatterns4.dll"
	File "${QTDIR}\bin\QtTest4.dll"
	File "${QTDIR}\bin\phonon4.dll"
	File ..\qscintilla\Qt4\qscintilla2.dll

!ifdef _64_BIT
!endif

!ifndef _64_BIT
;	File ..\..\..\binaries\mingwm10.dll
;	File ..\..\..\binaries\jpeg62.dll
;	File ..\..\..\binaries\zlib1.dll
!endif
;	File ..\..\..\binaries\krb5_32.dll
	File ..\..\..\binaries\Half.dll
	File ..\..\..\binaries\IlmThread.dll
	File ..\..\..\binaries\Iex.dll
	File ..\..\..\binaries\IlmImf.dll
	File ..\..\..\binaries\Imath.dll
	File ..\..\..\binaries\zlibwapi.dll
;	File ..\..\..\binaries\comerr32.dll
;	File ..\..\..\binaries\libtiff3.dll
;	File ..\..\..\binaries\libimage.dll
;	File ..\..\..\binaries\libpng13.dll
;	File ..\..\..\binaries\libiconv-2.dll

;;	Needed for qt with -openssl-linked, provides https support
!ifdef _64_BIT
	File e:\openssl-win64\ssleay32.dll
	File e:\openssl-win64\libeay32.dll
!else
	File e:\openssl-win32\ssleay32.dll
	File e:\openssl-win32\libeay32.dll
!endif

	RMDir /r $INSTDIR\sqldrivers
	SetOutPath $INSTDIR\sqldrivers

	File "${QTDIR}\plugins\sqldrivers\qsqlpsql4.dll"
	File "${QTDIR}\plugins\sqldrivers\qsqlite4.dll"

;; Needed for libpq.dll
	SetOutPath $INSTDIR
!ifdef _64_BIT
	File e:\postgresql-9.0.3-x86_64-bin\bin\libpq.dll
	File e:\postgresql-9.0.3-x86_64-bin\bin\libeay32.dll
	File e:\postgresql-9.0.3-x86_64-bin\bin\ssleay32.dll
	File e:\postgresql-9.0.3-x86_64-bin\bin\libintl.dll
	File e:\postgresql-9.0.3-x86_64-bin\bin\libiconv.dll
!endif

!ifndef _64_BIT
	File e:\postgresql-9.0.3-bin\bin\libpq.dll
	File e:\postgresql-9.0.3-bin\bin\libeay32.dll
	File e:\postgresql-9.0.3-bin\bin\ssleay32.dll
	File e:\postgresql-9.0.3-bin\bin\libintl-8.dll
	File e:\postgresql-9.0.3-bin\bin\libiconv-2.dll
!endif


	SetOutPath $INSTDIR\phonon_backend
	File /r "${QTDIR}\plugins\phonon_backend\*.dll"

	SetOutPath $INSTDIR\imageformats
	Delete $INSTDIR\imageformats\qjpeg1.dll
	Delete $INSTDIR\imageformats\tif.dll
	File "${QTDIR}\plugins\imageformats\qjpeg4.dll"
	File "${QTDIR}\plugins\imageformats\qgif4.dll"
	File "${QTDIR}\plugins\imageformats\qico4.dll"
	File "${QTDIR}\plugins\imageformats\qmng4.dll"
	File "${QTDIR}\plugins\imageformats\qsvg4.dll"
	File "${QTDIR}\plugins\imageformats\qtiff4.dll"

	File ..\qimageio\exr\exr.dll
	File ..\qimageio\tga\tga.dll
	File ..\qimageio\sgi\sgi.dll
	File ..\qimageio\rla\rla.dll

	Push $INSTDIR
	Call AddToPath
	ReadEnvStr $R0 "PATH"
	StrCpy $R0 "$R0;$INSTDIR}"
	System::Call 'Kernel32::SetEnvironmentVariableA(t, t) i("PATH", R0).r0'
	
	# Register the uninstaler only if a cmd line arg was not passed in
	ClearErrors
	${GetParameters} $R0
	${GetOptions} $R0 "/noUninstallerReg" $R1
	IfErrors 0 makeInstaller
		!ifdef _64_BIT
			SetRegView 64
		!endif
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_UNINSTALL_NAME}" "DisplayName" "${MUI_UNINSTALL_NAME} (remove only)"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_UNINSTALL_NAME}" "UninstallString" ${UNINSTALL_FILE}
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_UNINSTALL_NAME}" "Publisher" "Blur Studio"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_UNINSTALL_NAME}" "DisplayVersion" "${MUI_VERSION}-${MUI_ITERATION}"
	makeInstaller:
		CreateDirectory ${UNINSTALL_PATH}
		WriteUninstaller ${UNINSTALL_FILE}
		!insertmacro SetLastInstall ${UNINSTALL_FILE}
SectionEnd

Section "Uninstall"
	!ifdef _64_BIT
		SetRegView 64
		${DisableX64FSRedirection}
	!endif
	
	;Delete Files and directory
	Delete ${INSTALL_PATH}\QtCore4.dll
	Delete ${INSTALL_PATH}\QtDeclarative4.dll
	Delete ${INSTALL_PATH}\QtGui4.dll
	Delete ${INSTALL_PATH}\QtOpenGL4.dll
	Delete ${INSTALL_PATH}\QtNetwork4.dll
	Delete ${INSTALL_PATH}\QtSql4.dll
	Delete ${INSTALL_PATH}\QtHelp4.dll
	Delete ${INSTALL_PATH}\QtScript4.dll
	Delete ${INSTALL_PATH}\QtScriptTools4.dll
	Delete ${INSTALL_PATH}\QtSvg4.dll
	Delete ${INSTALL_PATH}\QtTest4.dll
	Delete ${INSTALL_PATH}\QtWebKit4.dll
	Delete ${INSTALL_PATH}\QtXml4.dll
	Delete ${INSTALL_PATH}\QtXmlPatterns4.dll
	Delete ${INSTALL_PATH}\QtCLucene4.dll
	Delete ${INSTALL_PATH}\QtAssistantClient4.dll
	Delete ${INSTALL_PATH}\QtDesigner4.dll
	Delete ${INSTALL_PATH}\QtDesignerComponents4.dll
	Delete ${INSTALL_PATH}\QtTest4.dll
	Delete ${INSTALL_PATH}\QtMultimedia4.dll
	Delete ${INSTALL_PATH}\phonon4.dll
	Delete ${INSTALL_PATH}\qscintilla2.dll
	Delete ${INSTALL_PATH}\half.dll
	Delete ${INSTALL_PATH}\IlmThread.dll
	Delete ${INSTALL_PATH}\Iex.dll
	Delete ${INSTALL_PATH}\IlmImf.dll
	Delete ${INSTALL_PATH}\Imath.dll
	Delete ${INSTALL_PATH}\zlibwapi.dll
	Delete ${INSTALL_PATH}\libpq.dll
	Delete ${INSTALL_PATH}\libeay32.dll
	Delete ${INSTALL_PATH}\ssleay32.dll
!ifdef _64_BIT
	Delete ${INSTALL_PATH}\libintl.dll
	Delete ${INSTALL_PATH}\libiconv.dll
!endif
!ifndef _64_BIT
	Delete ${INSTALL_PATH}\libintl-8.dll
	Delete ${INSTALL_PATH}\libiconv-2.dll
!endif
	Delete ${INSTALL_PATH}\phonon_backend\phonon_ds94.dll
	Delete ${INSTALL_PATH}\phonon_backend\phonon_ds9d4.dll
	Delete ${UNINSTALL_FILE}
	RMDir /r ${INSTALL_PATH}\sqldrivers
	RMDir /r ${INSTALL_PATH}\imageformats
	# delete phonon_backend if empty
	Push ${INSTALL_PATH}\phonon_backend
	Call un.isEmptyDir
	Pop $0
	StrCmp $0 1 0 +2
		RMDir /R ${INSTALL_PATH}\phonon_backend
	# delete install directory if empty
	Push ${INSTALL_PATH}
	Call un.isEmptyDir
	Pop $0
	StrCmp $0 1 0 +2
		RMDir /R ${INSTALL_PATH}

	;Delete Unistall Registry Entries
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${MUI_PRODUCT}"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_UNINSTALL_NAME}"  

SectionEnd

