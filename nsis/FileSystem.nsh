# remove the blur directory if it is empty
#Push $INSTDIR\stdplugs\backups				# specific the directory
#Call un.isEmptyDir							# Check if empty
#Pop $0										# Get Result
#StrCmp $0 1 0 +2							# check result
#	RMDir /R $INSTDIR\stdplugs\backups		# it the directory is empty remove it
#Function isEmptyDir
#  # Stack ->                    # Stack: <directory>
#  Exch $0                       # Stack: $0
#  Push $1                       # Stack: $1, $0
#  FindFirst $0 $1 "$0\*.*"
#  strcmp $1 "." 0 _notempty
#    FindNext $0 $1
#    strcmp $1 ".." 0 _notempty
#      ClearErrors
#      FindNext $0 $1
#      IfErrors 0 _notempty
#        FindClose $0
#        Pop $1                  # Stack: $0
#        StrCpy $0 1
#        Exch $0                 # Stack: 1 (true)
#        goto _end
#     _notempty:
#       FindClose $0
#       ClearErrors
#       Pop $1                   # Stack: $0
#       StrCpy $0 0
#       Exch $0                  # Stack: 0 (false)
#  _end:
#FunctionEnd

!macro removeEmptyDirectory _directory
	Push ${_directory}
	Exch $0                       # Stack: $0
	Push $1                       # Stack: $1, $0
	FindFirst $0 $1 "$0\*.*"
	strcmp $1 "." 0 +11#_notempty
	FindNext $0 $1
	strcmp $1 ".." 0 +9#_notempty
	  ClearErrors
	  FindNext $0 $1
	  IfErrors 0 +6#_notempty
		FindClose $0
		Pop $1                  # Stack: $0
		StrCpy $0 1
		Exch $0                 # Stack: 1 (true)
		goto +5#_end
	 #_notempty:
	   FindClose $0
	   ClearErrors
	   Pop $1                   # Stack: $0
	   StrCpy $0 0
	   Exch $0                  # Stack: 0 (false)
	#_end:
	Pop $0
	StrCmp $0 1 0 +2
		RMDir /R ${_directory}
!macroend


# remove all files but a specific one
#Push $INSTDIR\plugins\blur				# specify the directory
#Push blurPython${PYTHON_VERSION}.dlx	# specify the file we want to save
#Call un.RmFilesButOne					# remove all other files from the directory
Function un.RmFilesButOne
 Exch $R0 ; exclude file
 Exch
 Exch $R1 ; route dir
 Push $R2
 Push $R3
 
  FindFirst $R3 $R2 "$R1\*.*"
  IfErrors Exit
 
  Top:
   StrCmp $R2 "." Next
   StrCmp $R2 ".." Next
   StrCmp $R2 $R0 Next
   IfFileExists "$R1\$R2\*.*" Next
    Delete "$R1\$R2"
 
   #Goto Exit ;uncomment this to stop it being recursive (delete only one file)
 
   Next:
    ClearErrors
    FindNext $R3 $R2
    IfErrors Exit
   Goto Top
 
  Exit:
  FindClose $R3
 
 Pop $R3
 Pop $R2
 Pop $R1
 Pop $R0
FunctionEnd