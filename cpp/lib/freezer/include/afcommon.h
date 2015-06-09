
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Blur; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* $Header$
 */

#ifndef AFCOMMON_H
#define AFCOMMON_H

#include <qobject.h>

#ifdef FREEZER_MAKE_DLL
#define FREEZER_EXPORT Q_DECL_EXPORT
#else
#define FREEZER_EXPORT Q_DECL_IMPORT
#endif

#include "iniconfig.h"
#include "element.h"
#include "project.h"
#include "jobtype.h"

struct ViewColors;

extern FREEZER_EXPORT const char 	* LOGO_PATH,
					* CONFIG_PATH,
					* FRAGMENT_SHADER_PATH,
					* ICON_PATH;

#ifdef Q_OS_WIN
FREEZER_EXPORT extern const char * VNC_LINK;
#endif

struct FREEZER_EXPORT Options
{
	Options() : mJobColors(0), mFrameColors(0), mErrorColors(0), mHostColors(0), mControlModifierDepDragCheck(false) {}
	ViewColors * mJobColors, * mFrameColors, * mErrorColors, * mHostColors;
	QFont appFont;
	QFont jobFont;
	QFont frameFont;
	QFont summaryFont;
	QString frameCyclerPath;
	QString frameCyclerArgs;
	int mLimit;
	int mDaysLimit;
	int mRefreshInterval; // Minutes
	int mCounterRefreshInterval; // seconds
	bool mAutoRefreshOnWindowActivation, mRefreshOnViewChange;
	bool mControlModifierDepDragCheck;
};

FREEZER_EXPORT extern Options options;

// This structure holds all of the job settings
// that we can edit

struct FREEZER_EXPORT CounterState {
	int hostsTotal, hostsActive, hostsReady;
	int jobsTotal, jobsActive, jobsDone;
};

struct FREEZER_EXPORT JobFilter {
	// status filters
	QStringList statusToShow;

	// List of primary keys of users to show, empty list shows all.
	QList<uint> userList;

	// List of projects to hide, comma separated keys
	QList<uint> visibleProjects;
	bool showNonProjectJobs, allProjectsShown;

	// Only used for loading old ini format, stored here until
	// active projects select is finished
	QList<uint> hiddenProjects;

	// List of job types to show
	QList<uint> typesToShow;

	QList<uint> servicesToShow;

	// Elements ( shots ) to show
	ElementList elementList;
	
	uint mLimit;
	uint mDaysLimit;

	Expression mExtraFilters;
};

FREEZER_EXPORT void exploreFile( QString path );

#endif // AFCOMMON_H

