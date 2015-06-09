# LastInstall
# Convenience Functions That use the registry to pass information back to the parent installer.
# Unless otherwise specified it will store registry keys in the 32bit location
#
# Usage:
# SetLastInstall $PATH
#	Stores $PATH in HKLM "Software\Blur" "LastInstall"
#
# ClearLastInstall
# Removes the key at HKLM "Software\Blur" "LastInstall"
#
# GetFromKey $KEY $SUB_KEY
#	Reads the specified key into $LAST_INSTALL, and removes it.
#	Does not limit itself to 32bit locations
#
# SetKeyFromLast $KEY $SUB_KEY
#	Reads the registry key at HKLM "Software\Blur" "LastInstall"
#	Removes the registry key "Software\Blur"
#	Stores the value it read from LastInstall in HKLM $KEY $SUB_KEY

Var LAST_INSTALL

!macro SetLastInstall PATH
	SetRegView 32
	WriteRegStr HKLM "Software\Blur" "LastInstall" ${PATH}
	SetRegView lastused
!macroend

Function ClearLastInstall
	SetRegView 32
	DeleteRegKey HKLM "Software\Blur"
	SetRegView lastused
FunctionEnd

!macro SetKeyFromLast KEY SUB_KEY
	SetRegView 32
	ReadRegStr $0 HKLM "Software\Blur" "LastInstall"
	DeleteRegKey HKLM "Software\Blur"
	SetRegView lastused
	WriteRegStr HKLM ${KEY} ${SUB_KEY} $0
!macroend

!macro GetFromKey KEY SUB_KEY
	ReadRegStr $LAST_INSTALL HKLM ${KEY} ${SUB_KEY}
	DeleteRegValue HKLM ${KEY} ${SUB_KEY}
!macroend