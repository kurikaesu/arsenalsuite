
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

#ifndef RECENT_VALUES_H
#define RECENT_VALUES_H

#include <qlist.h>
#include <qstring.h>
#include <qvariant.h>

#include "blurqt.h"

/// Save and stores recent values in an IniConfig object
/// Contains useful functions for filling menus, for recent files and
/// other uses.
class STONE_EXPORT RecentValues
{
public:
	/// configKey will be appended with ValueXX, where XX is it's position in the list of recent values
	/// if it has a name set, it also have an entry appended with NameXX.
	RecentValues( const QString & configGroup, int defaultLimit = 10, const QString & configKey = "Recent" );
	RecentValues( const RecentValues & other );

	struct RecentNameValue {
		RecentNameValue( const QString & n = QString(), const QVariant & v = QVariant() ) : name(n), value(v) {}
		bool operator==(const RecentNameValue&other) { return other.name == name && other.value == value; }
		QString name;
		QVariant value;
	};

	QList<QVariant> recentValues() const;
	QList<RecentNameValue> recentNameValues() const;

	/// Name is value.toString() by default.
	void addRecentValue( const QVariant & value, const QString & name = QString() );

	void writeValue( int index, RecentNameValue );
	RecentNameValue readValue( int index );

protected:
	QRegExp configValueRegEx() const;
	QRegExp configNameRegEx() const;

	QString mConfigGroup, mConfigKey;
	int mRecentLimit;
};

#endif // RECENT_VALUES_H

