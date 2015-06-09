
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#define UNICODE 1
#define _UNICODE 1

#include <qlibrary.h>
#include <qtimer.h>
#include <qpixmap.h>
#include <qimage.h>

#include "process.h"

#ifdef Q_WS_WIN

#include <qdir.h>

#include <wchar.h>


#include <windows.h>
#include <winperf.h>
#include <winerror.h>
#include <aclapi.h>
#include <psapi.h>
#include <Userenv.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <lm.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <qfileinfo.h>

Win32Process::Win32Process( QObject * parent )
: QObject( parent )
, mWorkingDir( QDir::currentPath() )
, mInheritableIOChannels( true )
, mExitCode( 0 )
, mError( QProcess::ProcessError(0) )
, mState( QProcess::NotRunning )
, mCheckupTimer( 0 )
{}

void Win32Process::setLogon( const QString & userName, const QString & password, const QString & domain )
{
	mUserName = userName;
	mPassword = password;
	mDomain = domain;
}

void Win32Process::setInheritableIOChannels( bool ioc )
{
	mInheritableIOChannels = ioc;
}

void Win32Process::setWorkingDirectory( const QString & wd )
{
	mWorkingDir = wd;
}

// An empty string list(default) will use the users
// default environment.  To create a blank environment
// pass a string list with 1 empty string.
void Win32Process::setEnvironment( QStringList env )
{
	mEnv = env;
}

/*
QIODevice * Win32Process::stdin()
{
	return 0;
}

QIODevice * Win32Process::stderr()
{
	return 0;
}

QIODevice * Win32Process::stdout()
{
	return 0;
}
*/
int Win32Process::exitCode()
{
	return mExitCode;
}

QProcess::ProcessError Win32Process::error() const
{
	return mError;
}

QString Win32Process::errorString() const
{
	return mErrorString;
}

bool Win32Process::isRunning()
{
	if( mState == QProcess::Running || mState == QProcess::Starting ) {
		DWORD exitCode;
		if( GetExitCodeProcess( mProcessInfo.hProcess, &exitCode ) && exitCode == STILL_ACTIVE ) {
			if( mState == QProcess::Starting ) {
				emit started();
				mState = QProcess::Running;
				emit stateChanged( mState );
			}
			//LOG_3( "Process Running" );
			return true;
		}
		mExitCode = exitCode;
		bool crashed = (exitCode == 0xf291 || (int)exitCode < 0);
		if( crashed )
			_error( QProcess::Crashed, "" );
		mState = QProcess::NotRunning;
		emit stateChanged( mState );
		emit finished( mExitCode, crashed ? QProcess::CrashExit : QProcess::NormalExit );
		LOG_3( "Process Returned " + QString::number( exitCode ) );
	}
	return false;
}

QProcess::ProcessState Win32Process::state() const
{
	return mState;
}

Q_PID Win32Process::pid()
{
	return &mProcessInfo;
}

void Win32Process::terminate()
{
	if(isRunning())
		TerminateProcess(mProcessInfo.hProcess, 0xf291);
}

void Win32Process::setLogonFlag( LogonFlag flag )
{
	mLogonFlag = flag;
}

/*
static void qt_create_pipe(Q_PIPE *pipe, bool in)
{
    // Open the pipes.  Make non-inheritable copies of input write and output
    // read handles to avoid non-closable handles (this is done by the
    // DuplicateHandle() call).

#if !defined(Q_OS_WINCE)
    SECURITY_ATTRIBUTES secAtt = { sizeof( SECURITY_ATTRIBUTES ), NULL, TRUE };

    HANDLE tmpHandle;
    if (in) {                   // stdin
        if (!CreatePipe(&pipe[0], &tmpHandle, &secAtt, 1024 * 1024))
            return;
        if (!DuplicateHandle(GetCurrentProcess(), tmpHandle, GetCurrentProcess(),
                             &pipe[1], 0, FALSE, DUPLICATE_SAME_ACCESS))
            return;
    } else {                    // stdout or stderr
        if (!CreatePipe(&tmpHandle, &pipe[1], &secAtt, 1024 * 1024))
            return;
        if (!DuplicateHandle(GetCurrentProcess(), tmpHandle, GetCurrentProcess(),
                             &pipe[0], 0, FALSE, DUPLICATE_SAME_ACCESS))
            return;
    }

    CloseHandle(tmpHandle);
#else
	Q_UNUSED(pipe);
	Q_UNUSED(in);
#endif
}
*/

static QString qt_create_commandline(const QString &program, const QStringList &arguments)
{
	QString programName = program;
	if (!programName.startsWith(QLatin1Char('\"')) && !programName.endsWith(QLatin1Char('\"')) && programName.contains(QLatin1String(" ")))
		programName = QLatin1String("\"") + programName + QLatin1String("\"");
	programName.replace(QLatin1String("/"), QLatin1String("\\"));

	QString args;
	// add the prgram as the first arrg ... it works better
	args = programName + QLatin1String(" ");
	for (int i=0; i<arguments.size(); ++i) {
		QString tmp = arguments.at(i);
		// in the case of \" already being in the string the \ must also be escaped
		tmp.replace( QLatin1String("\\\""), QLatin1String("\\\\\"") );
		// escape a single " because the arguments will be parsed
		tmp.replace( QLatin1String("\""), QLatin1String("\\\"") );
		if (tmp.isEmpty() || tmp.contains(QLatin1Char(' ')) || tmp.contains(QLatin1Char('\t'))) {
			// The argument must not end with a \ since this would be interpreted
			// as escaping the quote -- rather put the \ behind the quote: e.g.
			// rather use "foo"\ than "foo\"
			QString endQuote(QLatin1String("\""));
			int i = tmp.length();
			while (i>0 && tmp.at(i-1) == QLatin1Char('\\')) {
				--i;
				endQuote += QLatin1String("\\");
			}
			args += QLatin1String(" \"") + tmp.left(i) + endQuote;
		} else {
			args += QLatin1Char(' ') + tmp;
		}
	}
	return args;
}

// This opens CreateProcessWithLogonW dynamically, so that if it doesn't exist
// we can continue and try CreateProcessAsUserW, that could fail on
// windows 2000 if the account isn't local and doesn't have the
// right privilege
typedef int (WINAPI * ExtCreateProcessWithLogonW) (LPCWSTR,LPCWSTR,LPCWSTR,DWORD,
						LPCWSTR,LPWSTR,DWORD,LPVOID,
						LPCWSTR,LPSTARTUPINFOW,
						LPPROCESS_INFORMATION);

ExtCreateProcessWithLogonW getCreateProcessWithLogonW()
{
	static ExtCreateProcessWithLogonW cpwl = (ExtCreateProcessWithLogonW)QLibrary::resolve( "advapi32", "CreateProcessWithLogonW" );
	return cpwl;
}

// This opens CreateProcessWithLogonW dynamically, so that if it doesn't exist
// we can continue and try CreateProcessAsUserW, that could fail on
// windows 2000 if the account isn't local and doesn't have the
// right privilege
typedef BOOL (WINAPI * ExtCreateProcessWithTokenW) (HANDLE,DWORD,
						LPCWSTR,LPWSTR,DWORD,LPVOID,
						LPCWSTR,LPSTARTUPINFOW,
						LPPROCESS_INFORMATION);

ExtCreateProcessWithTokenW getCreateProcessWithTokenW()
{
	static ExtCreateProcessWithTokenW cpwl = (ExtCreateProcessWithTokenW)QLibrary::resolve( "advapi32", "CreateProcessWithTokenW" );
	return cpwl;
}

bool SetPrivilege( HANDLE hToken, LPCTSTR lpszPrivilege, bool enable, QString * error )
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	QString callInfo = "privilege: " + QString::fromUtf16((unsigned short *)lpszPrivilege) + " enable: " + QString(enable ? "true" : "false");
	LOG_5( callInfo );
	if ( !LookupPrivilegeValue( NULL /*local system*/, lpszPrivilege, &luid ) )
	{
		*error = "LookupPrivilegeValue failed for " + callInfo + " error: " + QString::number( GetLastError() );
		return false; 
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if( enable )
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.
	if ( !AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL) )
	{
		*error = "AdjustTokenPrivileges failed for " + callInfo + "error: " + QString::number( GetLastError() ); 
		return false; 
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		*error = "The token does not have the specified privilege. " + callInfo;
		return false;
	}

	return true;
}

BOOL GetLogonSID (HANDLE hToken, PSID *ppsid, QString * errMsg )
{
	BOOL bSuccess = FALSE;
	DWORD dwIndex;
	DWORD dwLength = 0;
	PTOKEN_GROUPS ptg = NULL;

	// Verify the parameter passed in is not NULL.
	if (NULL == ppsid)
		goto Cleanup;

	// Get required buffer size and allocate the TOKEN_GROUPS buffer.
	
	if (!GetTokenInformation(
			hToken,         // handle to the access token
			TokenGroups,    // get information about the token's groups 
			(LPVOID) ptg,   // pointer to TOKEN_GROUPS buffer
			0,              // size of buffer
			&dwLength       // receives required buffer size
		)) 
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
			goto Cleanup;
	
		ptg = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(),
			HEAP_ZERO_MEMORY, dwLength);
	
		if (ptg == NULL) {
			if( errMsg ) *errMsg = "Unable to allocate memory for token groups";
			goto Cleanup;
		}
	}

	// Get the token group information from the access token.
	
	if (!GetTokenInformation(
			hToken,         // handle to the access token
			TokenGroups,    // get information about the token's groups 
			(LPVOID) ptg,   // pointer to TOKEN_GROUPS buffer
			dwLength,       // size of buffer
			&dwLength       // receives required buffer size
			)) 
	{
		if( errMsg ) *errMsg = "Unable to get token groups";
		goto Cleanup;
	}

	// Loop through the groups to find the logon SID.
	
	for (dwIndex = 0; dwIndex < ptg->GroupCount; dwIndex++) 
		if ((ptg->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID)
				==  SE_GROUP_LOGON_ID) 
		{
		// Found the logon SID; make a copy of it.
	
			dwLength = GetLengthSid(ptg->Groups[dwIndex].Sid);
			*ppsid = (PSID) HeapAlloc(GetProcessHeap(),
						HEAP_ZERO_MEMORY, dwLength);
			if (*ppsid == NULL)
				goto Cleanup;
			if (!CopySid(dwLength, *ppsid, ptg->Groups[dwIndex].Sid)) 
			{
				HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
				goto Cleanup;
			}
			bSuccess = TRUE;
			break;
		}
	if( !bSuccess && errMsg ) *errMsg = "Unable to find logon SID";

Cleanup:

	// Free the buffer for the token groups.

	if (ptg != NULL)
		HeapFree(GetProcessHeap(), 0, (LPVOID)ptg);
	
	return bSuccess;
}


VOID FreeLogonSID (PSID *ppsid)
{
	HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
}


#define DESKTOP_ALL (DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | \
DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | \
DESKTOP_JOURNALPLAYBACK | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | \
DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ALL (WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | \
WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | \
WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS | \
WINSTA_EXITWINDOWS | WINSTA_ENUMERATE | WINSTA_READSCREEN | \
STANDARD_RIGHTS_REQUIRED)

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | \
GENERIC_EXECUTE | GENERIC_ALL)

BOOL AddAceToWindowStation(HWINSTA hwinsta, PSID psid, QString * errorMsg);

BOOL AddAceToDesktop(HDESK hdesk, PSID psid, QString * errorMsg);

bool SetupDesktopAccessForUserLogon( HANDLE userHandle, QString * errorMsg )
{
	HDESK hdesk = NULL;
	HWINSTA hwinsta = NULL, hwinstaSave = NULL;
	PSID pSid = NULL;
	bool success = false;

	// Save a handle to the caller's current window station.
	if ( (hwinstaSave = GetProcessWindowStation() ) == NULL) {
		if( errorMsg ) *errorMsg = "Current Process Has no window station";
		goto Cleanup;
	}

	// Get a handle to the interactive window station.
	hwinsta = OpenWindowStation( L"winsta0"/* the interactive window station */, FALSE /* handle is not inheritable */, READ_CONTROL | WRITE_DAC); // rights to read/write the DACL
	if (hwinsta == NULL) {
		if( errorMsg ) *errorMsg = "Unable to open the window station handle with sufficient rights to modify DACL";
		goto Cleanup;
	}

	// To get the correct default desktop, set the caller's 
	// window station to the interactive window station.
	if (!SetProcessWindowStation(hwinsta)) {
		if( errorMsg ) *errorMsg = "Unable to set the current process window station";
		goto Cleanup;
	}

	// Get a handle to the interactive desktop.
	hdesk = OpenDesktop(
		L"default",     // the interactive window station 
		0,             // no interaction with other desktop processes
		FALSE,         // handle is not inheritable
		READ_CONTROL | // request the rights to read and write the DACL
		WRITE_DAC | 
		DESKTOP_WRITEOBJECTS | 
		DESKTOP_READOBJECTS);
	
	// Restore the caller's window station.
	if( !SetProcessWindowStation(hwinstaSave) ) {
		if( errorMsg ) *errorMsg = "Unable to restore the current process window station";
		goto Cleanup;
	}

	if (hdesk == NULL) {
		if( errorMsg ) *errorMsg = "Unable to open the default desktop on winsta0";
		goto Cleanup;
	}

	// Get the SID for the client's logon session.
	if( !GetLogonSID(userHandle, &pSid, errorMsg) )
		goto Cleanup;

	// Allow logon SID full access to interactive window station.
	if( !AddAceToWindowStation(hwinsta, pSid, errorMsg) )
		goto Cleanup;

	// Allow logon SID full access to interactive desktop.
	if( !AddAceToDesktop(hdesk, pSid, errorMsg) ) {
		if( errorMsg ) *errorMsg = "AddAceToDesktop failed";
		goto Cleanup;
	}

	success = true;

Cleanup:
	if (hwinstaSave != NULL)
		SetProcessWindowStation( hwinstaSave );

	// Free the buffer for the logon SID.
	if (pSid)
		FreeLogonSID(&pSid);

	// Close the handles to the interactive window station and desktop.
	if (hwinsta)
		CloseWindowStation(hwinsta);

	if (hdesk)
		CloseDesktop(hdesk);

	return success;
}

BOOL AddAceToWindowStation(HWINSTA hwinsta, PSID psid, QString * errorMsg)
{
	ACCESS_ALLOWED_ACE   *pace = 0;
	ACL_SIZE_INFORMATION aclSizeInfo;
	BOOL                 bDaclExist;
	BOOL                 bDaclPresent;
	BOOL                 bSuccess = FALSE;
	DWORD                dwNewAclSize;
	DWORD                dwSidSize = 0;
	DWORD                dwSdSizeNeeded;
	PACL                 pacl = 0;
	PACL                 pNewAcl = 0;
	PSECURITY_DESCRIPTOR psd = NULL;
	PSECURITY_DESCRIPTOR psdNew = NULL;
	PVOID                pTempAce;
	SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
	unsigned int         i;
	
	// Obtain the DACL for the window station.
	if( !GetUserObjectSecurity( hwinsta, &si, psd, dwSidSize, &dwSdSizeNeeded) ) {

		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			psd = (PSECURITY_DESCRIPTOR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded );
	
			if (psd == NULL)
				goto Cleanup;
	
			psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded );
	
			if (psdNew == NULL)
				goto Cleanup;
	
			dwSidSize = dwSdSizeNeeded;
	
			if( !GetUserObjectSecurity( hwinsta, &si, psd, dwSidSize, &dwSdSizeNeeded) ) {
				if( errorMsg ) *errorMsg = "GetUserObjectSecurity failed";
				goto Cleanup;
			}
		}
		else {
			if( errorMsg ) *errorMsg = "GetUserObjectSecurity failed with unexpected error";
			goto Cleanup;
		}
	}

	// Create a new DACL.
	if( !InitializeSecurityDescriptor( psdNew, SECURITY_DESCRIPTOR_REVISION ) ) {
		if( errorMsg ) *errorMsg = "InitializeSecurityDescriptor failed";
		goto Cleanup;
	}

	// Get the DACL from the security descriptor.
	if( !GetSecurityDescriptorDacl( psd, &bDaclPresent, &pacl, &bDaclExist) ) {
		if( errorMsg ) *errorMsg = "GetSecurityDescriptorDacl failed";
		goto Cleanup;
	}

	// Initialize the ACL.
	ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
	aclSizeInfo.AclBytesInUse = sizeof(ACL);

	// Call only if the DACL is not NULL.
	if (pacl != NULL)
	{
		// get the file ACL size info
		if( !GetAclInformation( pacl, (LPVOID)&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation ) ) {
			if( errorMsg ) *errorMsg = "GetAclInformation failed";
			goto Cleanup;
		}
	}

	// Compute the size of the new ACL.
	dwNewAclSize = aclSizeInfo.AclBytesInUse +
			(2*sizeof(ACCESS_ALLOWED_ACE)) + (2*GetLengthSid(psid)) -
			(2*sizeof(DWORD));

	// Allocate memory for the new ACL.
	pNewAcl = (PACL)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwNewAclSize );

	if (pNewAcl == NULL)
		goto Cleanup;

	// Initialize the new DACL.
	if( !InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION) ) {
		if( errorMsg ) *errorMsg = "InitializeAcl failed";
		goto Cleanup;
	}

	// If DACL is present, copy it to a new DACL.
	if (bDaclPresent)
	{
		// Copy the ACEs to the new ACL.
		if (aclSizeInfo.AceCount)
		{
			for (i=0; i < aclSizeInfo.AceCount; i++)
			{
				// Get an ACE.
				if (!GetAce(pacl, i, &pTempAce))
					goto Cleanup;
	
				// Add the ACE to the new ACL.
				if( !AddAce( pNewAcl, ACL_REVISION, MAXDWORD, pTempAce, ((PACE_HEADER)pTempAce)->AceSize) ) {
					if( errorMsg ) *errorMsg = "AddAce failed";
					goto Cleanup;
				}
			}
		}
	}

	// Add the first ACE to the window station.

	pace = (ACCESS_ALLOWED_ACE *)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD) );

	if (pace == NULL)
		goto Cleanup;

	pace->Header.AceType  = ACCESS_ALLOWED_ACE_TYPE;
	pace->Header.AceFlags = CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
	pace->Header.AceSize  = DWORD(sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD));
	pace->Mask            = GENERIC_ACCESS;

	if( !CopySid(GetLengthSid(psid), &pace->SidStart, psid) ) {
		if( errorMsg ) *errorMsg = "CopySid failed";
		goto Cleanup;
	}

	if( !AddAce( pNewAcl, ACL_REVISION, MAXDWORD, (LPVOID)pace, pace->Header.AceSize) ) {
		if( errorMsg ) *errorMsg = "AddAce failed";
		goto Cleanup;
	}

	// Add the second ACE to the window station.
	pace->Header.AceFlags = NO_PROPAGATE_INHERIT_ACE;
	pace->Mask            = WINSTA_ALL;

	if( !AddAce( pNewAcl, ACL_REVISION, MAXDWORD, (LPVOID)pace, pace->Header.AceSize) )
		goto Cleanup;

	// Set a new DACL for the security descriptor.
	if( !SetSecurityDescriptorDacl( psdNew, TRUE, pNewAcl, FALSE) ) {
		if( errorMsg ) *errorMsg = "SetSecurityDescriptorDacl failed";
		goto Cleanup;
	}

	// Set the new security descriptor for the window station.
	if( !SetUserObjectSecurity(hwinsta, &si, psdNew) ) {
		if( errorMsg ) *errorMsg = "SetUserObjectSecurity failed";
		goto Cleanup;
	}

	// Indicate success.
	bSuccess = TRUE;

Cleanup:
	// Free the allocated buffers.
	if (pace != NULL)
		HeapFree(GetProcessHeap(), 0, (LPVOID)pace);

	if (pNewAcl != NULL)
		HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);

	if (psd != NULL)
		HeapFree(GetProcessHeap(), 0, (LPVOID)psd);

	if (psdNew != NULL)
		HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
	
	return bSuccess;
}

BOOL AddAceToDesktop(HDESK hdesk, PSID psid, QString * errorMsg)
{
	Q_UNUSED(errorMsg);

	ACL_SIZE_INFORMATION aclSizeInfo;
	BOOL                 bDaclExist;
	BOOL                 bDaclPresent;
	BOOL                 bSuccess = FALSE;
	DWORD                dwNewAclSize;
	DWORD                dwSidSize = 0;
	DWORD                dwSdSizeNeeded;
	PACL                 pacl = 0;
	PACL                 pNewAcl = 0;
	PSECURITY_DESCRIPTOR psd = NULL;
	PSECURITY_DESCRIPTOR psdNew = NULL;
	PVOID                pTempAce;
	SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
	unsigned int         i;
	
	// Obtain the security descriptor for the desktop object.

	if( !GetUserObjectSecurity( hdesk, &si, psd, dwSidSize, &dwSdSizeNeeded) )
	{
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			psd = (PSECURITY_DESCRIPTOR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded );

			if (psd == NULL)
				goto Cleanup;

			psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded );

			if (psdNew == NULL)
				goto Cleanup;

			dwSidSize = dwSdSizeNeeded;

			if( !GetUserObjectSecurity( hdesk, &si, psd, dwSidSize, &dwSdSizeNeeded) )
				goto Cleanup;
		}
			else
				goto Cleanup;
	}
	
	// Create a new security descriptor.
	if( !InitializeSecurityDescriptor( psdNew, SECURITY_DESCRIPTOR_REVISION) )
		goto Cleanup;

	// Obtain the DACL from the security descriptor.
	if( !GetSecurityDescriptorDacl( psd, &bDaclPresent, &pacl, &bDaclExist) )
		goto Cleanup;

	// Initialize.
	ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
	aclSizeInfo.AclBytesInUse = sizeof(ACL);

	// Call only if NULL DACL.
	if (pacl != NULL)
	{
		// Determine the size of the ACL information.
		if( !GetAclInformation( pacl, (LPVOID)&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation) )
			goto Cleanup;
	}
	
	// Compute the size of the new ACL.
	dwNewAclSize = aclSizeInfo.AclBytesInUse +
		sizeof(ACCESS_ALLOWED_ACE) +
		GetLengthSid(psid) - sizeof(DWORD);

	// Allocate buffer for the new ACL.
	pNewAcl = (PACL)HeapAlloc(
		GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		dwNewAclSize);

	if (pNewAcl == NULL)
		goto Cleanup;

	// Initialize the new ACL.
	if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
		goto Cleanup;

	// If DACL is present, copy it to a new DACL.
	if (bDaclPresent)
	{
		// Copy the ACEs to the new ACL.
		if (aclSizeInfo.AceCount)
		{
		for (i=0; i < aclSizeInfo.AceCount; i++)
		{
			// Get an ACE.
			if (!GetAce(pacl, i, &pTempAce))
				goto Cleanup;

			// Add the ACE to the new ACL.
			if (!AddAce(
				pNewAcl,
				ACL_REVISION,
				MAXDWORD,
				pTempAce,
				((PACE_HEADER)pTempAce)->AceSize)
			)
				goto Cleanup;
		}
		}
	}
	
	// Add ACE to the DACL.
	if( !AddAccessAllowedAce( pNewAcl, ACL_REVISION, DESKTOP_ALL, psid) )
		goto Cleanup;

	// Set new DACL to the new security descriptor.
	if( !SetSecurityDescriptorDacl( psdNew, TRUE, pNewAcl, FALSE) )
		goto Cleanup;

	// Set the new security descriptor for the desktop object.
	if( !SetUserObjectSecurity(hdesk, &si, psdNew) )
		goto Cleanup;

	// Indicate success.
	bSuccess = TRUE;

Cleanup:
	// Free buffers.
	if (pNewAcl != NULL)
		HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);

	if (psd != NULL)
		HeapFree(GetProcessHeap(), 0, (LPVOID)psd);

	if (psdNew != NULL)
		HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
	
	return bSuccess;
}

bool setupPrivileges( QString * error )
{
	HANDLE hToken;     /* process token */
	bool ret = false;
	
	if( !OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) )
	{
		*error = "OpenProcessToken() failed with code " + QString::number( GetLastError() );
		return false;
	}
	
	do {
		if( !SetPrivilege( hToken, SE_ASSIGNPRIMARYTOKEN_NAME, TRUE, error ) ) {
			*error = "SetPrivilege failed to set SE_ASSIGNPRIMARYTOKEN_NAME";
			break;
		}

		if( !SetPrivilege( hToken, SE_INCREASE_QUOTA_NAME, TRUE, error ) ) {
			*error = "SetPrivilege failed to set SE_INCREASE_QUOTA_NAME";
			break;
		}
	
		ret = true;
	} while( 0 );
	
	CloseHandle( hToken );
	return ret;
}

bool Win32Process::start( const QString & program, const QStringList & args )
{
	mState = QProcess::Starting;
	emit stateChanged( mState );

	BOOL success = false;
	// Setup the startup info struct
	STARTUPINFO startInfo;
	ZeroMemory(&startInfo, sizeof(STARTUPINFO));
	startInfo.cb = sizeof(STARTUPINFO); // DWORD cb
	// Later if we need to comm via stdin/stdout, we can
	// either use the windows function via these handles,
	// or hack qt to let us pass them to qsocket
	startInfo.dwFlags = STARTF_FORCEOFFFEEDBACK; // dwFlags  | STARTF_USESTDHANDLES

	QString command = qt_create_commandline(program, args);

	QString domain = mDomain;
	if( !mUserName.contains( "@" ) && domain.isEmpty() )
		domain = ".";

	ExtCreateProcessWithLogonW cpwl = getCreateProcessWithLogonW();
	if( cpwl ) {
		LOG_3( "Using CreateProcessWithLogonW" );

		wchar_t * command_wchar = 0;
		command_wchar = new wchar_t [ command.size() + 1 ];
		command_wchar[command.toWCharArray( command_wchar )] = 0;
		DWORD logonFlag = 0;
		if( mLogonFlag == LogonWithProfile )
			logonFlag = 0x1; //LOGON_WITH_PROFILE;
		else if( mLogonFlag == LogonWithNetCredentials )
			logonFlag = 0x2; //LOGON_NETCREDENTIALS_ONLY;

		success = cpwl(
			(WCHAR*)mUserName.utf16(),
			(WCHAR*)domain.utf16(),
			(WCHAR*)mPassword.utf16(),
			logonFlag,
			0,
			command_wchar,
			CREATE_NEW_PROCESS_GROUP | CREATE_UNICODE_ENVIRONMENT,
			0,
			(WCHAR*)mWorkingDir.utf16(),
			&startInfo,
			&mProcessInfo
		);
		delete [] command_wchar;

		if( !success ) {
			_error( QProcess::FailedToStart, "Unable to create process for user " + mDomain + "\\" + mUserName + " error was: " + QString::number( GetLastError() ) );
			return false;
		}
	}
		else
	{

		// Login as a different user
		HANDLE userHandle;
		success = LogonUserA(
			mUserName.toLatin1().data(),
			domain.toLatin1().data(),
			mPassword.toLatin1().data(),
			LOGON32_LOGON_INTERACTIVE,
			LOGON32_PROVIDER_DEFAULT,
			&userHandle
		);
	
		if( !success ) {
			_error( QProcess::FailedToStart, "Unable to login as " + mDomain + "\\" + mUserName + " error was: " + QString::number( GetLastError() ) );
			return false;
		}
	
		if( !SetupDesktopAccessForUserLogon( userHandle, &mErrorString ) ) {
			CloseHandle( userHandle );
			_error( QProcess::FailedToStart, "Unable to grant desktop access to user " + mDomain + "\\" + mUserName + " error was: " + mErrorString );
			return false;
		}
	
		// Get the user's environment
		LPVOID env;
		success = CreateEnvironmentBlock( &env, userHandle, FALSE );
	
		// Load system env only
		if( !success && GetLastError() == 203 )
			success = CreateEnvironmentBlock( &env, NULL, FALSE );
	
		if( !success ) {
			CloseHandle( userHandle );
			_error( QProcess::FailedToStart, "Unable to create environment for user " + mDomain + "\\" + mUserName + " error was: " + QString::number( GetLastError() ) );
			return false;
		}

		ExtCreateProcessWithTokenW cpwt = getCreateProcessWithTokenW();
		if( cpwt ) {
			LOG_3( "Using CreateProcessWithTokenW" );
	
			DWORD logonFlag = 0;
			if( mLogonFlag == LogonWithProfile )
				logonFlag = 0x1; //LOGON_WITH_PROFILE;
			else if( mLogonFlag == LogonWithNetCredentials )
				logonFlag = 0x2; //LOGON_NETCREDENTIALS_ONLY;
	
			success = cpwt(
				userHandle,
				logonFlag,
				0,
				(WCHAR*)command.utf16(),
				CREATE_NEW_PROCESS_GROUP | CREATE_UNICODE_ENVIRONMENT,
				env,
				(WCHAR*)mWorkingDir.utf16(),
				&startInfo,
				&mProcessInfo
			);
		} else { 
	
			QString errMsg;
			if( !setupPrivileges(&errMsg) ) {
				CloseHandle( userHandle );
				_error(QProcess::FailedToStart, "Error adjusting privileges: " + errMsg);
				return false;
			} 
			LOG_3( "Calling CreateProcessAsUserW" );
	
			// Create the new assburner process
			success = CreateProcessAsUserW(
				userHandle,
				0,//(WCHAR*)mCmd.utf16(),
				(WCHAR*)mCmd.utf16(),
				NULL,
				NULL,
				FALSE,
				DETACHED_PROCESS | CREATE_UNICODE_ENVIRONMENT,
				env,
				(WCHAR*)mWorkingDir.utf16(),
				&startInfo,
				&mProcessInfo
			);
		}

		DestroyEnvironmentBlock( env );
		if( !success ) {
			CloseHandle( userHandle );
			_error( QProcess::FailedToStart, "Unable to create process for user " + mDomain + "\\" + mUserName + " error was: " + QString::number( GetLastError() ) );
			return false;
		}

	}
/*	if( success  && mProcessInfo.hProcess != INVALID_HANDLE_VALUE )
	{
		WaitForSingleObject(mProcessInfo.hProcess, INFINITE);
		CloseHandle(mProcessInfo.hProcess);
	}

	if (mProcessInfo.hThread != INVALID_HANDLE_VALUE)
		CloseHandle(mProcessInfo.hThread); */

	mCheckupTimer = new QTimer( this );
	connect( mCheckupTimer, SIGNAL( timeout() ), SLOT( _checkup() ) );
	mCheckupTimer->start(15);
	return true;
}

void Win32Process::_checkup()
{
	isRunning();
	if( mCheckupTimer && mCheckupTimer->interval() == 15 )
		mCheckupTimer->start(50);
}

void Win32Process::_error(QProcess::ProcessError processError, const QString & errorStr)
{
	mErrorString = errorStr;
	LOG_5( errorStr );
	mError = processError;
	emit error(mError);
	if( mCheckupTimer ) {
		mCheckupTimer->stop();
		mCheckupTimer->deleteLater();
		mCheckupTimer = 0;
	}
	mState = QProcess::NotRunning;
	emit stateChanged( mState );
}

Win32Process * Win32Process::create( QObject * parent, const QString & cmd, const QString & userName, const QString & password, const QString & domain )
{
	Win32Process * p = new Win32Process(parent);
	p->setLogon( userName, password, domain );
	if( p->start(cmd) )
		return p;
	delete p;
	return 0;
}

/*
ProcessJob::ProcessJob( QObject * parent = 0, const QString & name )
: QProcess( parent )
{
	mWindowsJobHandle = CreateJobObjectW( NULL, (WCHAR*)name.utf16() );
	if( !mWindowsJobHandle ) {
		CloseHandle( userHandle );
//		*error = "Unable to create job object with name : " + mJob.name() + " Error was: " + QString::number( GetLastError() );
		//return false;
	}
}

void ProcessJob::addProcess( Process * process )
{
	success = AssignProcessToJobObject( mWindowsJobHandle, mProcessInfo.hProcess );
	if( !success ) {
		*error = "Unable to assign process to the job object";
		return false;
	}
}

QList<Process*> ProcessJob::processes()
{
	return QList<Process*>();
}

void ProcessJob::terminate();
*/

bool isRunning(int pid, const QString & name)
{
	HANDLE h;
	bool ret = false;
	if( pid<=0 ) return ret;
	h = OpenProcess(PROCESS_ALL_ACCESS, FALSE,pid);
	if ( h == NULL )
		return ret;
	if( !name.isEmpty() ){
		char processName[256];
		DWORD br;
		HMODULE module;
		if( EnumProcessModules( h, &module, sizeof(module), &br) ){
			GetModuleBaseNameA( h, module, processName, 256 );
			if( name == QString(processName) )
				ret = true;
		}
	}
	else ret = true;
	CloseHandle( h );
	return ret;
}

int processID()
{
	return GetCurrentProcessId();
}

bool killProcess(int pid)
{
	HANDLE hProcess = OpenProcess( PROCESS_TERMINATE, false, pid );
	if( !hProcess ) {
		LOG_5( "Unable to open process with pid: " + QString::number( pid ) );
		return false;
	}
	// Set a notable exit code so we can check for it in pidsByName
	bool ret = bool(TerminateProcess( hProcess, 666 ));
	LOG_5( (ret ? "Killed PID: " : "Failed to kill PID: ") + QString::number( pid ) );

	CloseHandle( hProcess );
	return ret;
}

QDateTime winFileTimeToQDT( FILETIME * pFileTime )
{
	QDateTime ret;
	SYSTEMTIME systemTime;
	if( FileTimeToSystemTime( pFileTime, &systemTime ) != 0 ) {
		ret = QDateTime( QDate( systemTime.wYear, systemTime.wMonth, systemTime.wDay ), QTime( systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds ) );
	} else {
		LOG_1( "FileTimeToSystemTime failed with error: " + QString::number( GetLastError() ) );
	}
	return ret;
}

QDateTime processStartTime(int pid)
{
	QDateTime ret;
	HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, pid );
	if( hProcess ) {
		FILETIME creationTime, exitTime, kernelTime, userTime;
		if( GetProcessTimes( hProcess, &creationTime, &exitTime, &kernelTime, &userTime ) != 0 ) {
			ret = winFileTimeToQDT(&creationTime);
		} else {
			LOG_1( "GetProcessTimes failed with error: " + QString::number( GetLastError() ) );
		}
		CloseHandle(hProcess);
	}
	return ret;
}

bool setProcessPriorityClass( int pid, DWORD priorityClass )
{
	HANDLE h;
	bool ret = false;
	if( pid<=0 ) return ret;
	h = OpenProcess(PROCESS_SET_INFORMATION, FALSE,pid);
	if ( h == NULL ) {
		LOG_3( QString("Unable to open process %1 to set priority class").arg( pid ) );
		return ret;
	}
	if( SetPriorityClass( h, priorityClass ) )
		ret = true;
	else
		LOG_3( QString("SetPriorityClass failed for process %1").arg( pid ) );
	CloseHandle( h );
	return ret;
	
}

int setProcessPriorityClassByName( const QString & name, DWORD priorityClass )
{
	int ret = 0;
	QList<int> pids;
	pidsByName( name, &pids );
	LOG_5( QString("Setting process priority class for %1 processes with name %2").arg( pids.size() ).arg( name ) );
	foreach( int pid, pids )
		if( setProcessPriorityClass( pid, priorityClass ) )
			ret++;
	return ret;
}

// Returns false if no process with name is running
// Returns true and sets *pid to the process id of the
// first process it finds with name
bool isRunningByName( const QString & name, int * pid )
{
	QList<int> pids;
	int cnt = pidsByName( name, pid ? &pids : 0 );
	if( pid && pids.size() ) *pid = pids[0];
	return cnt > 0;
}

/*
int getProcessPid( const QString & rawName, int instanceNumber )
{
	LOG_5( "Looking up pid for process: " + rawName );
	int ret = 0;
	PDH_HQUERY query;
	if( ERROR_SUCCESS == PdhOpenQuery( 0, 0, &query ) ) {
		PDH_HCOUNTER counter;
		QString processName = rawName;
		if( instanceNumber > 0 ) processName += "#" + QString::number( instanceNumber );
		QString path = QString( "\\Process(%1)\\ID Process" ).arg( processName );
		if( ERROR_SUCCESS == PdhAddCounter( query, 
#ifdef UNICODE
			(LPCTSTR)path.utf16(),
#else
			(LPCTSTR)path.toLatin1(),
#endif
			0, &counter ) ) {
			
			if( ERROR_SUCCESS == PdhCollectQueryData( query ) ) {
				PDH_FMT_COUNTERVALUE  ItemBuffer;
				// Format the performance data record.
				if( ERROR_SUCCESS == PdhGetFormattedCounterValue( counter, PDH_FMT_LONG, 0, &ItemBuffer ) ) {
					ret = ItemBuffer.longValue;
					LOG_5( "Found pid for process: " + rawName + " " + QString::number( ret ) );
				} else
					LOG_5( "PdhGetFormattedCounterValue failed" );
			} else
				LOG_5( "PdhCollectQueryData failed" );
		} else
			LOG_5( "PdhAddCounter failed" );
		PdhCloseQuery( &query );
	} else
		LOG_5( "PdhOpenQuery failed" );
	return ret;
}
*/

int pidsByName( const QString & name, QList<int> * pidList, bool caseSensitive )
{
/*
	PDH_STATUS  pdhStatus               = ERROR_SUCCESS;
	LPTSTR      szCounterListBuffer     = NULL;
	DWORD       dwCounterListSize       = 0;
	LPTSTR      szInstanceListBuffer    = NULL;
	DWORD       dwInstanceListSize      = 0;
	LPTSTR      szThisInstance          = NULL;
	int processCount = 0;

	QString compareName = name;
	if( !caseSensitive )
		compareName = compareName.toLower();
	if( compareName.endsWith( ".exe" ) )
		compareName = compareName.left( compareName.size() - 4 );

	// Refresh the list
	PdhEnumObjects( 0, 0, 0, 0, PERF_DETAIL_WIZARD, TRUE );

	// Determine the required buffer size for the data. 
	pdhStatus = PdhEnumObjectItems (
		NULL,                   // real time source
		NULL,                   // local machine
		TEXT("Process"),        // object to enumerate
		szCounterListBuffer,    // pass NULL and 0
		&dwCounterListSize,     // to get length required
		szInstanceListBuffer,   // buffer size 
		&dwInstanceListSize,    // 
		PERF_DETAIL_WIZARD,     // counter detail level
		0); 

	if (pdhStatus == PDH_MORE_DATA) 
	{
		// Allocate the buffers and try the call again.
		szCounterListBuffer = (LPTSTR)malloc (
			(dwCounterListSize * sizeof (TCHAR)));
		szInstanceListBuffer = (LPTSTR)malloc (
			(dwInstanceListSize * sizeof (TCHAR)));
	
		if ((szCounterListBuffer != NULL) &&
			(szInstanceListBuffer != NULL)) 
		{
			pdhStatus = PdhEnumObjectItems (
				NULL,                 // real time source
				NULL,                 // local machine
				TEXT("Process"),      // object to enumerate
				szCounterListBuffer,  // buffer to receive counter list
				&dwCounterListSize, 
				szInstanceListBuffer, // buffer to receive instance list 
				&dwInstanceListSize,    
				PERF_DETAIL_WIZARD,   // counter detail level
				0);
		
			if (pdhStatus == ERROR_SUCCESS) 
			{
				LOG_5("Enumerating Processes:");
	
				// Walk the instance list. The list can contain one
				// or more null-terminated strings. The last string 
				// is followed by a second null-terminator.
				int instanceNumber = 0;
				QString lastProcessName;
				for (szThisInstance = szInstanceListBuffer;
						*szThisInstance != 0;
						szThisInstance += lstrlen(szThisInstance) + 1) 
				{
#ifndef UNICODE
					QString processNameRaw = QString::fromLatin1(szThisInstance);
#else
					QString processNameRaw = QString::fromUtf16((const ushort *)szThisInstance);
#endif
					if( processNameRaw == lastProcessName )
						instanceNumber++;
					else
						instanceNumber = 0;
					lastProcessName = processNameRaw;
					QString processName = processNameRaw;
					LOG_5( "Found Process: " + processName + "#" + QString::number(instanceNumber) );
					if( !caseSensitive ) processName = processName.toLower();
					if( processName == compareName ) {
						processCount++;
						if( pidList ) {
							int pid = getProcessPid( processNameRaw, instanceNumber );
							if( pid ) (*pidList) << pid;
						}
					}
				}
			} else
				LOG_5( QString("PdhEnumObjectItems failed with %1.").arg(pdhStatus) );
		} else 
			LOG_5( "\nUnable to allocate buffers" );
	
		if (szCounterListBuffer != NULL) 
			free (szCounterListBuffer);
	
		if (szInstanceListBuffer != NULL) 
			free (szInstanceListBuffer);
	} else
		LOG_5(QString("\nPdhEnumObjectItems failed with %1").arg(pdhStatus));
	
	return processCount;
*/

	DWORD processIds[1024], bytesReturned;
//	LOG_5( "Collecting windows Process IDs" );
	EnumProcesses( processIds, sizeof(processIds), &bytesReturned );
	int i=bytesReturned/sizeof(DWORD) -1;
	int retcount=0;
//	LOG_5( "Opening processes and reading their executable names" );
	for(; i>=0; --i){
		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, processIds[i] );
		if( !hProcess )
			continue;

		wchar_t processNameW[512];
		DWORD len = GetProcessImageFileNameW( hProcess, processNameW, 512 );
		if( len == 0 ) {
			if( processIds[i] != 4 )
				LOG_5( "Failure getting process image name for process: " + QString::number( processIds[i] ) );
		} else {
			QString processName = QFileInfo(QString::fromWCharArray( processNameW, len )).fileName();
			//LOG_5( "Got Process Name: " + processName );
			if( (caseSensitive && name == processName) || (!caseSensitive && name.toLower() == processName.toLower()) ){
				DWORD exitCode = 0;
				// Consider a process to be non-existent if we killed it previously with terminateprocess
				if( GetExitCodeProcess( hProcess, &exitCode ) && exitCode != STILL_ACTIVE )
					continue;
				retcount++;
				if( pidList )
					(*pidList) += processIds[i];
			}
		}

/*
		HMODULE module;
		if( EnumProcessModules( hProcess, &module, sizeof(module), &bytesReturned) )
		{
			int size = GetModuleBaseNameW( hProcess, module, processNameW, 512 );
			if( size == 0 ) {
				LOG_1( "Failure opening process with ID: " + QString::number( processIds[i] ) );
			} else {
				QString processName = QString::fromWCharArray( processNameW, size );
	
				if( (caseSensitive && name == processName) || (!caseSensitive && name.toLower() == processName.toLower()) ){
					retcount++;
					if( pidList )
						(*pidList) += processIds[i];
				}
			} 
		} */
		
		CloseHandle( hProcess );
	}
//	LOG_5( QString("Finished, returning %1 processes found with name %2").arg(retcount).arg(name) );
	return retcount;

}


static QList<HWND> sGetHandlesRet;
static QString sGetHandlesRE;

BOOL CALLBACK GetHandles_EnumWindowsProc( HWND hwnd, LPARAM  )
{
	char temp[1024];
	QRegExp re( sGetHandlesRE );

	if( GetWindowTextA( hwnd, temp, 1024 ) )
	{
		QString title( temp );
		if( title.contains( re ) )
			sGetHandlesRet << hwnd;
	}
	return true;
}

int killAllWindows( const QString & windowTitleRE )
{
	int count = 0;
	QList<HWND> windows = getWindowsByName( windowTitleRE );
	foreach ( HWND hwnd, windows ) {
		qint32 processId = windowProcess(hwnd);
		if( processId ) {
			if( killProcess( processId ) )
				count++;
		}
	}
	return count;
}

QList<HWND> getWindowsByName( const QString & nameRE )
{
	sGetHandlesRE = nameRE;
	sGetHandlesRet.clear();
	EnumDesktopWindows( NULL, (WNDENUMPROC)GetHandles_EnumWindowsProc, (LPARAM)0 );
	return sGetHandlesRet;
}

struct FindMatchingWindowsStruct
{
	QStringList titles;
	QString windowFound;
	QList<qint32> processIds;
	bool caseSensitive;
};

BOOL CALLBACK findMatchingWindow_WindowsProc( HWND hwnd, LPARAM param )
{
	char temp[1024];
	
	FindMatchingWindowsStruct * fmws = (FindMatchingWindowsStruct*)param;

	if( fmws->processIds.contains( windowProcess(hwnd) ) ) {
		if( GetWindowTextA( hwnd, temp, 1024 ) )
		{
			QString title( temp );
			foreach( QString titleMatch, fmws->titles )
				if( title.contains( titleMatch, fmws->caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive ) ) {
					fmws->windowFound = title;
					LOG_5( "Found matching window title: " + title );
					return false;
				}
		}
	}
	return true;
}

bool findMatchingWindow( int pid, QStringList titles, bool matchProcessChildren, bool caseSensitive, QString * foundTitle )
{
	FindMatchingWindowsStruct fmws;
	fmws.titles = titles;
	fmws.caseSensitive = caseSensitive;
	fmws.processIds = QList<qint32>();
	fmws.processIds += pid;
	if( matchProcessChildren )
		fmws.processIds += processChildrenIds(pid,true);

	EnumDesktopWindows( NULL, (WNDENUMPROC)findMatchingWindow_WindowsProc, (LPARAM)(&fmws) );

	if( !fmws.windowFound.isEmpty() ) {
		if( foundTitle )
			*foundTitle = fmws.windowFound;
		return true;
	}
	return false;
}

bool systemShutdown( bool reboot, const QString & message )
{
#ifndef EWX_FORCEIFHUNG
#define EWX_FORCEIFHUNG 0x00000010
#endif
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	
	// Get a token for this process.
	if ( !OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) ) {
		LOG_1( "Couldn't Open the process token to get shutdown privileges" );
		return false;
	}

	// Get the LUID for the shutdown privilege.
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1; // one privilege to set
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	
	// Get the shutdown privilege for this process.
	AdjustTokenPrivileges(hToken, false, &tkp, 0,(PTOKEN_PRIVILEGES)NULL, 0);
	
	if (GetLastError() != ERROR_SUCCESS) {
		LOG_1( "Unable to get SE_SHUTDOWN_NAME privilege" );
		return false;
	}

/*
	// Shut down the system and force all applications to close.
	uint flags = EWX_FORCEIFHUNG | (reboot ? EWX_REBOOT : EWX_SHUTDOWN | EWX_POWEROFF);
	if ( !ExitWindowsEx(flags, 0 ) ) {
		LOG_1( "Windows Shutdown Failed with error: " + QString::number( GetLastError() ) );
		return false;
	}
	return true;
*/
	BOOL success = InitiateSystemShutdownEx( NULL, message.isEmpty() ? NULL : (WCHAR*)message.utf16(),
		/*timeout=*/0, /*force=*/BOOL(true), BOOL(reboot),
		SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE | SHTDN_REASON_FLAG_PLANNED );
	if ( !success ) {
		LOG_1( "Windows Shutdown Failed with error: " + QString::number( GetLastError() ) );
		return false;
	}
	return true;
}

static ProcessMemInfo processMemoryInfoWorker( qint32 pid, bool recursive, QList<qint32> * pidsChecked )
{
	ProcessMemInfo ret;
	PROCESS_MEMORY_COUNTERS pmc;

	if( pid == 0 ) return ret;
	if( pidsChecked->contains( pid ) ) return ret;

	pidsChecked->append( pid );

	HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid );
	if( hProcess && GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc) ) ) {
		ret.caps = ProcessMemInfo::CurrentSize | ProcessMemInfo::MaxSize;
		ret.currentSize = pmc.PagefileUsage / 1024;
		ret.maxSize = pmc.PeakPagefileUsage / 1024;
	}

	if( hProcess ) CloseHandle( hProcess );

	LOG_5( "Memory info found for pid " + QString::number( pid ) + QString( recursive ? " retrieving children pids" : "" ) );

	if( recursive ) {
		QList<qint32> children = processChildrenIds( pid );
		QStringList childPidStrings;
		foreach( qint32 childPid, children )
			childPidStrings += QString::number(childPid);
		LOG_5( "Getting memory info for children with pids " + childPidStrings.join(",") );
		foreach( qint32 childPid, children ) {
			ProcessMemInfo childMem = processMemoryInfoWorker( childPid, true, pidsChecked );
			if( childMem.caps == (ProcessMemInfo::CurrentSize | ProcessMemInfo::MaxSize) ) {
				ret.currentSize += childMem.currentSize;
				ret.maxSize += childMem.maxSize;
			}
		}
	}
	return ret;
}

ProcessMemInfo processMemoryInfo( qint32 pid, bool recursive )
{
	QList<qint32> pidsChecked;
	return processMemoryInfoWorker( pid, recursive, &pidsChecked );
}

// If returnParent is true, then the processId is matched, and the parentProcessId is returned
// else the parentProcessId is matched, and the processId's are returned.
static QList<qint32> enumProcessesForPidMatch( qint32 pid, bool returnParent )
{
	QList<qint32> ret;
	PROCESSENTRY32 procentry;
	procentry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0 );
	if( hSnapShot != INVALID_HANDLE_VALUE ) {
		BOOL bContinue = Process32First( hSnapShot, &procentry ) ;
		// While there are processes, keep looping.
		while( bContinue )
		{
			if( returnParent && (quint32)pid == procentry.th32ProcessID ) {
				ret << procentry.th32ParentProcessID;
				break;
			}
			if( !returnParent && (quint32)pid == procentry.th32ParentProcessID )
				ret << procentry.th32ProcessID;
	
			procentry.dwSize = sizeof(PROCESSENTRY32) ;
			bContinue = Process32Next( hSnapShot, &procentry );
		}//while ends
		CloseHandle( hSnapShot );
	}
	return ret;
}

struct ParentProcessItem
{
	ParentProcessItem( qint32 _processId, qint32 _parentProcessId ) : processId(_processId), parentProcessId(_parentProcessId) {}
	qint32 processId, parentProcessId;
};

// If returnParent is true, then the processId is matched, and the parentProcessId is returned
// else the parentProcessId is matched, and the processId's are returned.
static QList<ParentProcessItem> processParentTable()
{
	QList<ParentProcessItem> ret;
	PROCESSENTRY32 procentry;
	procentry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0 );
	if( hSnapShot != INVALID_HANDLE_VALUE ) {
		BOOL bContinue = Process32First( hSnapShot, &procentry ) ;
		while( bContinue )
		{
			ret.append( ParentProcessItem( procentry.th32ProcessID, procentry.th32ParentProcessID ) );
			procentry.dwSize = sizeof(PROCESSENTRY32) ;
			bContinue = Process32Next( hSnapShot, &procentry );
		}
		CloseHandle( hSnapShot );
	}
	return ret;
}

qint32 processParentId( qint32 pid )
{
	QList<qint32> list = enumProcessesForPidMatch( pid, true );
	return list.size() ? list[0] : 0;
}

QList<qint32> processChildrenIds( qint32 pid, bool recursive )
{
	Q_UNUSED(recursive);
	if( recursive ) {
		QList<ParentProcessItem> parentTable = processParentTable();
		QList<qint32> ret;
		ret.append(pid);
		bool foundMatch = false;
		do {
			foundMatch = false;
			foreach( ParentProcessItem item, parentTable ) {
				if( ret.contains(item.parentProcessId) && !ret.contains(item.processId) ) {
					ret.append( item.processId );
					foundMatch = true;
				}
			}
		} while( foundMatch );
		return ret;
	}
	return enumProcessesForPidMatch( pid, false );
}

qint32 windowProcess( HWND hWin )
{
	DWORD processId = 0;
	GetWindowThreadProcessId( hWin, &processId );
	return processId;
}

struct WindowTitlesInfoStruct
{
	QList<qint32> validPids;
	QList<WindowInfo> windowInfoResults;
};

static BOOL CALLBACK windowTitlesByProcess_EnumWindowsProc( HWND hwnd, LPARAM param )
{
	WindowTitlesInfoStruct * is = (WindowTitlesInfoStruct*)param;
	wchar_t temp[1024];

	qint32 processId = windowProcess(hwnd);
	if( is->validPids.contains(processId) ) {
		WindowInfo wi;
		if( GetWindowTextW( hwnd, temp, 1024 ) > 0 )
			wi.title = QString::fromWCharArray( temp );
		wi.processId = processId;
		wi.hWin = hwnd;
		is->windowInfoResults << wi;
	}
	return true;
}

QList<WindowInfo> windowInfoByProcess( qint32 pid, bool recursive )
{
	WindowTitlesInfoStruct is;
	is.validPids << pid;
	if( recursive )
		is.validPids += processChildrenIds( pid, true );
	DWORD threadId = GetCurrentThreadId();
	HDESK hdesk = GetThreadDesktop( threadId );
	EnumDesktopWindows( hdesk, (WNDENUMPROC)windowTitlesByProcess_EnumWindowsProc, (LPARAM)(&is) );
	return is.windowInfoResults;
}

bool processHasNamedWindow( int pid, const QString & nameRE, bool processRecursive )
{
	QList<WindowInfo> windowInfos = windowInfoByProcess( pid, processRecursive );
	QRegExp re( nameRE );
	foreach( WindowInfo wi, windowInfos ) {
		if( wi.title.contains( re ) )
			return true;
	}
	return false;
}

bool isWow64()
{
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	static LPFN_ISWOW64PROCESS fnIsWow64Process = 0;
	bool isWow64 = false;
	
	if( !fnIsWow64Process )
		fnIsWow64Process = (LPFN_ISWOW64PROCESS)QLibrary::resolve( "kernel32", "IsWow64Process" );
	
	if (fnIsWow64Process)
	{
		BOOL bIsWow64;
		fnIsWow64Process(GetCurrentProcess(),&bIsWow64);
		isWow64 = bool(bIsWow64);
	}
	return isWow64;
}

static bool setRegKeyHKLMDword( const char * path, const char * key, DWORD val )
{
	bool ret = false;
	HKEY hkey;
	if( RegCreateKeyExA( HKEY_LOCAL_MACHINE, path, 0, 0, 0, KEY_ALL_ACCESS | (isWow64() ? KEY_WOW64_64KEY : 0), 0, &hkey, 0 ) == ERROR_SUCCESS ) {
		DWORD value = 1;
		if( ERROR_SUCCESS == RegSetValueExA( hkey, key, 0, REG_DWORD, (const BYTE *)&val, sizeof(DWORD) ) )
			ret = true;
		RegCloseKey( hkey );
	} else
		LOG_3( "Unable to create/open registry key HKEY_LOCAL_MACHINE\\" + QString::fromLatin1(path) + "\\" + QString::fromLatin1(key) );
	return ret;
}

// Have to pass this flag when running under wow64 or else
// registry redirection will bite us in the ass
#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif
bool disableWindowsErrorReporting( const QString & executableName )
{
	bool ret = false;
	if( executableName.isEmpty() ) {
		ret = setRegKeyHKLMDword( "Software\\Microsoft\\PCHealth\\ErrorReporting", "DoReport", 0 );
		ret &= setRegKeyHKLMDword( "Software\\Microsoft\\PCHealth\\ErrorReporting", "ShowUI", 0 );
		ret &= setRegKeyHKLMDword( "Software\\Microsoft\\Windows\\Windows Error Reporting", "DontShowUI", 1 );
	} else {
		ret = setRegKeyHKLMDword( "Software\\Microsoft\\PCHealth\\ErrorReporting\\ExclusionList", executableName.toLatin1().constData(), 1 );
	}
	return ret;
}

QString localDomain()
{
	LPWKSTA_INFO_100 pBuf = NULL;
	NET_API_STATUS nStatus;
	QString ret;

	if( (nStatus = NetWkstaGetInfo(NULL, 100, (LPBYTE *)&pBuf)) == NERR_Success )
		ret = QString::fromWCharArray( pBuf->wki100_langroup );
	else
		LOG_1( "Call to NetWkstaGetInfo failed with error: " + QString::number( nStatus ) + ", couldnt find current domain" );

	if( pBuf )
		NetApiBufferFree(pBuf);

	return ret;
}


typedef ULONGLONG (WINAPI * ExtGetTickCount64) ();
ExtGetTickCount64 getTickCount64()
{
	static ExtGetTickCount64 gtc = (ExtGetTickCount64)QLibrary::resolve( "kernel32", "GetTickCount64" );
	return gtc;
}

Interval systemUpTime()
{
	ExtGetTickCount64 gtc64 = getTickCount64();
	if( gtc64 ) {
		ULONGLONG ms = gtc64();
		// Seconds is only an int, but that gives us 68 years...
		// never will a windows system be up that long, nor will this code matter if it was
		return Interval( ms / 1000 );
	}

	/* This will likely fail on a localized version of windows.  FUCK YOU Microsoft
	 * We could possible fix this with a bunch more ugly code, but not worth it as the above will
	 * be whats actually used in most cases going forward */
	PDH_HQUERY phQuery;
	DWORD errorCode;
	if( (errorCode = PdhOpenQuery( 0, 0, &phQuery )) != ERROR_SUCCESS ) {
		LOG_1( "Error opening pdh query, error code: " + QString::number(errorCode) );
		return Interval();
	}
	PDH_HCOUNTER phCounter;
	const WCHAR * counterPath = L"\\\\.\\System\\System Up Time";
	if( (errorCode = PdhAddCounter( phQuery, counterPath, 0, &phCounter )) != ERROR_SUCCESS ) {
		PdhCloseQuery( &phQuery );
		LOG_1( "Error opening counter path: " + QString::fromWCharArray( counterPath ) + " error code: " + QString::number(errorCode) );
		return Interval();
	}
	if( (errorCode = PdhCollectQueryData( phQuery )) != ERROR_SUCCESS ) {
		PdhCloseQuery( &phQuery );
		LOG_1( "PdhCollectQueryData returned error code: " + QString::number(errorCode) );
		return Interval();
	}
	PDH_FMT_COUNTERVALUE uptimeValue;
	if( (errorCode = PdhGetFormattedCounterValue( phCounter, PDH_FMT_LARGE, NULL, &uptimeValue )) != ERROR_SUCCESS ) {
		PdhCloseQuery( &phQuery );
		LOG_1( "PdhGetFormattedCounterValue returned error code: " + QString::number(errorCode) );
		return Interval();
	}

	PdhCloseQuery( &phQuery );
	DWORD seconds = (DWORD) (uptimeValue.largeValue);
	return Interval(seconds);
}

typedef HRESULT (WINAPI * ExtSetCurrentProcessExplicitAppUserModelID) ( PCWSTR AppID );
ExtSetCurrentProcessExplicitAppUserModelID setCurrentProcessExplicitAppUserModelID()
{
	static ExtSetCurrentProcessExplicitAppUserModelID proc = (ExtSetCurrentProcessExplicitAppUserModelID)QLibrary::resolve( "shell32", "SetCurrentProcessExplicitAppUserModelID" );
	return proc;
}

bool qSetCurrentProcessExplicitAppUserModelID( const QString & appId )
{
	ExtSetCurrentProcessExplicitAppUserModelID proc = setCurrentProcessExplicitAppUserModelID();
	if( proc ) {
		HRESULT res = proc( (const WCHAR*)appId.utf16() );
		return res == S_OK ? true : false;
	}
	return false;
}

QString currentExecutableFilePath()
{
	wchar_t executableNameW[512];
	DWORD len = GetModuleFileNameW( NULL, executableNameW, 512 );
	if( len == 0 ) {
		LOG_1( "GetModuleFileNameW failed, error was: " + QString::number( GetLastError() ) );
		return QString();
	}
	return QString::fromWCharArray( executableNameW, len );
}

bool saveScreenShot( const QString & path )
{
	// get the device context of the screen
	HDC hScreenDC = CreateDC(L"DISPLAY", NULL, NULL, NULL);     
	if( hScreenDC == NULL ) {
		LOG_1( "CreateDC(\"DISPLAY\", NULL, NULL, NULL) failed" );
		return false;
	}
	
	// and a device context to put it in
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
	if( hMemoryDC == NULL ) {
		LOG_1( "CreateCompatibleDC(hScreenDC) failed" );
		DeleteDC(hScreenDC);
		return false;
	}
	
	int width = GetDeviceCaps(hScreenDC, HORZRES);
	int height = GetDeviceCaps(hScreenDC, VERTRES);

	// maybe worth checking these are positive values
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
	if( hBitmap == NULL ) {
		LOG_1( "CreateCompatibleBitmap(hScreenDC, x, y) failed" );
		DeleteDC(hMemoryDC);
		DeleteDC(hScreenDC);
		return false;
	}
	
	// get a new bitmap
	HGDIOBJ hOldBitmap = SelectObject(hMemoryDC, hBitmap);

	if( BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY) == 0 ) {
		LOG_1( "BitBlt from screen to memory failed" );
		DeleteDC(hMemoryDC);
		DeleteDC(hScreenDC);
		return false;
	}
	
	SelectObject(hMemoryDC, hOldBitmap);

	QPixmap pixmap = QPixmap::fromWinHBITMAP( hBitmap );
	QImage image = pixmap.toImage();
	bool ret = image.save( path );
	if( !ret )
		LOG_1( "Failed to save screenshot to file " + path );

	// clean up
	DeleteDC(hMemoryDC);
	DeleteDC(hScreenDC);
	DeleteObject(hBitmap);

	return ret;
}


#endif // Q_OS_WIN

