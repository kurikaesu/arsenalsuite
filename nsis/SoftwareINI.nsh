##
#   :namespace  SoftwareINI
#
#   :remarks    Contains a macro that will write the correct info into a ini file for updating software.
#				and a function that converts a month code with 2 digit number padding to the short name for a month.
#   
#	:usage		!insertmacro UpdateSettingsINI ${BLUR_SOFTWARE_INI} "${BUILD_TYPE} ${MAX_VERSION}" "v${PRODUCT_VERSION}.${MUI_SVNREV}"
#	
#   :author     mikeh@blur.com
#   :author     Blur Studio
#   :date       09/06/12
#

!include FileFunc.nsh

!define	BLUR_SOFTWARE_INI	'C:\blur\software.ini'

# Converts a month code with 2 digit number padding to the short name for a month.
#	${GetTime} "" "LS" $0 $1 $2 $3 $4 $5 $6
#	Push $1
#	Call GetShortMonth
#	Pop $1
Function GetShortMonth
	Exch $0
		
	StrCmp $0 01 0 +3
	StrCpy $0 "Jan"
	goto end
	StrCmp $0 02 0 +3
	StrCpy $0 "Feb"
	goto end
	StrCmp $0 03 0 +3
	StrCpy $0 "Mar"
	goto end
	StrCmp $0 04 0 +3
	StrCpy $0 "Apr"
	goto end
	StrCmp $0 05 0 +3
	StrCpy $0 "May"
	goto end
	StrCmp $0 06 0 +3
	StrCpy $0 "Jun"
	goto end
	StrCmp $0 07 0 +3
	StrCpy $0 "Jul"
	goto end
	StrCmp $0 08 0 +3
	StrCpy $0 "Aug"
	goto end
	StrCmp $0 09 0 +3
	StrCpy $0 "Sep"
	goto end
	StrCmp $0 10 0 +3
	StrCpy $0 "Oct"
	goto end
	StrCmp $0 11 0 +3
	StrCpy $0 "Nov"
	goto end
	StrCmp $0 12 0 +3
	StrCpy $0 "Dec"
	goto end
	
	end:
		Exch $0
FunctionEnd

!macro UpdateSettingsINI _FILENAME _VERSION_NAME _INSTALLED_VERSION _INSTALLER_FILENAME
	# Get the current date time                  "Tue May 29 15:00:11 2012"
	${GetTime} "" "L" $0 $1 $2 $3 $4 $5 $6
	StrCpy $3 $3 3	# convert to 3 day week name
	# Convert the month int to a short month
	Push $1
	Call GetShortMonth
	Pop $1
	# write the install date time					"Tue May 29 15:00:11 2012"
	WriteINIStr "${_FILENAME}" "${_VERSION_NAME}" "installed" "$3 $1 $0 $4:$5:$6 $2"
	# write the version number
	WriteINIStr "${_FILENAME}" "${_VERSION_NAME}" "version" "${_INSTALLED_VERSION}"
	# write the filename as it was built
	WriteINIStr "${_FILENAME}" "${_VERSION_NAME}" "filename" "${_INSTALLER_FILENAME}"
!macroend
