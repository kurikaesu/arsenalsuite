/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef COMMIT_CODE

#include <qhostinfo.h>
#include <qlibrary.h>
#include <qsettings.h>
#include <qsqldatabase.h>
#include <qlibrary.h>
#include <qprocess.h>

#include "blurqt.h"
#include "process.h"

#include "host.h"
#include "job.h"
#include "jobassignment.h"
#include "jobtask.h"
#include "syslog.h"
#include "syslogseverity.h"
#include "service.h"
#include "hostservice.h"
#include "hoststatus.h"
#include "hostgroup.h"
#include "hostgroupitem.h"

Host Host::currentHost()
{
	return recordByName( Host::currentHostName() );
}

QString Host::currentHostName()
{
	QString hostName;
	QStringList fakeNames = QProcess::systemEnvironment().filter("FAKEHOSTNAME=", Qt::CaseSensitive);
	if( fakeNames.isEmpty() ) {
		hostName = QHostInfo::localHostName().section(".",0,0);
	} else {
		hostName = fakeNames.first().section("=",1,1).section(".",0,0);
	}
	return hostName.toLower();
}

JobAssignmentList Host::activeAssignments() const
{
	return JobAssignment::select( "WHERE fkeyhost=? AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus FROM jobassignmentstatus WHERE status IN ('ready','copy','busy'))", VarList() << key() );
}

JobAssignmentList Host::activeAssignments(HostList hosts)
{
	return JobAssignment::select( QString("WHERE fkeyhost IN (%1) AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus FROM jobassignmentstatus WHERE status IN ('ready','copy','busy'))").arg(hosts.keyString()) );
}

int Host::syslogStatus()
{
	SysLogList activeEvents = SysLog::recordsByHostAndAck(key(), 0);
	if( activeEvents.size() ) {
		activeEvents = activeEvents.sorted("sysLogSeverity",false);
		if( activeEvents[0].isRecord() )
			return activeEvents[0].sysLogSeverity().key();
		else return 0;
	}
	return 0;
}

Host Host::autoRegister()
{
	Host newHost = Host();
	newHost.setName(Host::currentHostName());
	newHost.setOnline(1);
	newHost.commit();

	newHost.updateHardwareInfo();

	Service ab = Service::recordByName("Assburner");
	if( ab.isRecord() ) {
		HostService hs = HostService();
		hs.setHost(newHost);
		hs.setService(ab);
		hs.commit();
	}

	HostGroup hg = HostGroup::recordByName("All");
	if( hg.isRecord() ) {
		HostGroupItem hgi = HostGroupItem();
		hgi.setHost(newHost);
		hgi.setHostGroup(hg);
		hgi.commit();
	}

	return newHost;
}

#ifdef Q_OS_WIN
#include <windows.h>
#include <stdio.h>

SYSTEM_INFO w32_getSystemInfo( bool * success )
{
	typedef LPSYSTEM_INFO (WINAPI * LPFN_GETNATIVESYSTEMINFO) (LPSYSTEM_INFO);
	if( success ) *success = false;
	SYSTEM_INFO sysInfo;
	if( isWow64() ) {
		static LPFN_GETNATIVESYSTEMINFO fnGetNativeSystemInfo = 0;
		if( !fnGetNativeSystemInfo )
			fnGetNativeSystemInfo = (LPFN_GETNATIVESYSTEMINFO)QLibrary::resolve( "kernel32", "GetNativeSystemInfo" );
		if( fnGetNativeSystemInfo )
			fnGetNativeSystemInfo( &sysInfo );
	} else
		GetSystemInfo( &sysInfo );
	if( success ) *success = true;
	return sysInfo;
}

QString w32_getOsVersion( QString * servicePackVersion = 0, int * buildNumber = 0 )
{
	QString ret;
	OSVERSIONINFOEXW osvi;
	SYSTEM_INFO systemInfo;
	BOOL bOsVersionInfoEx;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
	
	// Try calling GetVersionEx using the OSVERSIONINFOEX structure.
	// If that fails, try using the OSVERSIONINFO structure.
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	
	if( !(bOsVersionInfoEx = GetVersionExW ((OSVERSIONINFOW *) &osvi)) )
	{
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFOW);
		if (! GetVersionEx ( (OSVERSIONINFOW *) &osvi) ) 
			return QString();
	}
	bool success = true;
	systemInfo = w32_getSystemInfo( &success );
	if( !success )
		return QString();
	
	if( servicePackVersion )
		*servicePackVersion = QString("%1.%2").arg(osvi.wServicePackMajor).arg(osvi.wServicePackMinor);
	
	if( buildNumber )
		*buildNumber = osvi.dwBuildNumber;
	
	switch( osvi.dwMajorVersion ) {
		case 4:
		{
			switch( osvi.dwMinorVersion ) {
				case 0:
					if( osvi.dwPlatformId == VER_PLATFORM_WIN32_NT )
						ret = "Windows NT 4.0";
					else
						ret = "Windows 95";
					break;
				case 10:
					ret = "Windows 98";
					break;
				case 90:
					ret = "Windows ME";
					break;
			};
		}
		case 5:
		{
			switch( osvi.dwMinorVersion ) {
				case 0:
					ret = "Windows 2000";
					break;
				case 1:
					ret = "Windows XP";
					break;
				case 2:
					if( (osvi.wProductType == VER_NT_WORKSTATION) && (systemInfo.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64) )
						ret = "Windows XP Professional x64";
					else if(osvi.wSuiteMask & 0x00008000 /*VER_SUITE_WH_SERVER*/)
						ret = "Windows Home Server";
					else {
						if( GetSystemMetrics(SM_SERVERR2) == 0 )
							ret = "Windows Server 2003";
						else
							ret = "Windows Server 2003 R2";
					}
					break;
			};
		}
		case 6:
		{
			switch( osvi.dwMinorVersion ) {
				case 0:
					if( osvi.wProductType != VER_NT_WORKSTATION )
						ret = "Windows Server 2008";
					else
						ret = "Windows Vista";
					break;
				case 1:
					if( osvi.wProductType != VER_NT_WORKSTATION )
						ret = "Windows Server 2008 R2";
					else
						ret = "Windows 7";
					break;
			};
		}
	};
	return ret;
}

#endif // Q_OS_WIN

void Host::updateHardwareInfo()
{
	SystemMemInfo mi = systemMemoryInfo();
	if( mi.caps & SystemMemInfo::TotalMemory )
		setMemory( mi.totalMemory / 1024 );
#ifdef Q_OS_LINUX
	QString cpu = backtick("cat /proc/cpuinfo");
	QRegExp cpuRx("physical id\\s+: (\\d+)");
	QRegExp bogoRx("bogomips\\s+: (\\d+)");
	QRegExp cpuCoresRx("cpu cores\\s+: (\\d+)");

	LOG_3( "trying to get CPU info\n"+cpu );
	if( bogoRx.indexIn(cpu) != -1 )
		setMhz( bogoRx.cap(1).toInt() );
	int cores = 1;
	if( cpuCoresRx.indexIn(cpu) != -1 )
		cores = cpuCoresRx.cap(1).toInt();

	int cpuId = 0;
	int pos = 0;
	while ((pos = cpuRx.indexIn(cpu, pos)) != -1) {
		int foundCpuId = cpuRx.cap(1).toInt();
		LOG_3("found cpu with physical id: " + QString::number(foundCpuId));
		if( foundCpuId > cpuId )
			cpuId = foundCpuId;
		pos += cpuRx.matchedLength();
		setCpus( (cpuId+1)*cores );
	}
	setCpuName(backtick("uname -p").replace("\n",""));
	setOs(backtick("uname").replace("\n",""));
	setOsVersion(backtick("uname -r").replace("\n",""));
	setArchitecture(backtick("uname -m").replace("\n",""));
	commit();
#endif
#ifdef Q_OS_MAC
	QString sys_profile = backtick("system_profiler -detailLevel -2");
	QRegExp mhzRx("(CPU|Processor) Speed: ([\\d.]+) GHz");
	if( mhzRx.indexIn(sys_profile) != -1 )
		setMhz( (mhzRx.cap(2).toFloat() * 1000) );

	QRegExp cpuNameRx("(CPU|Processor) Name: (.*)\n");
	if( cpuNameRx.indexIn(sys_profile) != -1 )
		setCpuName( cpuNameRx.cap(2).replace("\n","") );

	QRegExp memRx("Memory: (\\d+) GB");
	if( memRx.indexIn(sys_profile) != -1 )
		setMemory( (memRx.cap(1).toInt() * 1024) );

	QRegExp cpuRx("Number Of (CPUs|Cores): (\\d+)");
	if( cpuRx.indexIn(sys_profile) != -1 )
		setCpus( cpuRx.cap(2).toInt() );

	QRegExp osRx("System Version: (Mac OS X) ([\\d.]+)");
	if( osRx.indexIn(sys_profile) != -1 ) {
		setOs( osRx.cap(1) );
		setOsVersion( osRx.cap(2) );
	}

	setArchitecture(backtick("uname -m").replace("\n",""));
#endif
#ifdef Q_OS_WIN
	bool sysInfoSuccess;
	SYSTEM_INFO sysInfo = w32_getSystemInfo( &sysInfoSuccess );
	if( sysInfoSuccess ) {
		QString arch;
		switch( sysInfo.wProcessorArchitecture ) {
			case PROCESSOR_ARCHITECTURE_AMD64:
				arch = "x86_64";
				break;
			case PROCESSOR_ARCHITECTURE_IA64:
				arch = "Itanium";
				break;
			case PROCESSOR_ARCHITECTURE_INTEL:
				arch = "x86";
				break;
		}
#ifndef _WIN64
		if( !isWow64() ) setOs( "win32" );
		else
#endif
		setOs( "win64" );
		setArchitecture( arch );
		QString servicePackVersion;
		int buildNumber;
		setOsVersion( w32_getOsVersion(&servicePackVersion,&buildNumber) );
		setServicePackVersion(servicePackVersion);
		setBuildNumber(buildNumber);
		setCpus( sysInfo.dwNumberOfProcessors );
		QSettings mhzReg( "HKEY_LOCAL_MACHINE\\Hardware\\Description\\System\\CentralProcessor\\0", QSettings::NativeFormat );
		setMhz( mhzReg.value( "~MHz" ).toInt() );
		setWindowsDomain( localDomain() );
	}
#endif
	commit();
	
	
	/*
	 * Anything that will change every time this function is run should probably be in HostStatus, not in Host.
	 * All the above will be recalculated but rarely ever change and cause an actual update.
	 */
	
	HostStatus hs = hostStatus();
	Interval uptime = systemUpTime();
	hs.setSystemStartupTimestamp( uptime == Interval() ? QDateTime() : (uptime * -1.0).adjust(QDateTime::currentDateTime()) );
	hs.commit();
}

#endif // CLASS_FUNCTIONS

