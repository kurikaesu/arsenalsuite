
/*
 *
 * Copyright 2012 Blur Studio Inc.
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

#ifndef JOINED_SELECT_H
#define JOINED_SELECT_H

#include <qlist.h>
#include <qstring.h>
#include <qvariant.h>

#include "blurqt.h"

typedef QList<QVariant> VarList;

namespace Stone {

class Table;
class ResultSet;

enum JoinType {
	InnerJoin,
	LeftJoin,
	RightJoin,
	OuterJoin
};

class STONE_EXPORT JoinCondition
{
public:
	QString condition, alias;
	JoinType type;
	Table * table;
	bool ignoreResults, joinOnly;
};

class STONE_EXPORT JoinedSelect
{
public:
	JoinedSelect(Table * primaryTable, QList<JoinCondition> joinConditions = QList<JoinCondition>());
	JoinedSelect(Table * primaryTable, const QString & alias);
	
	QString alias() const;
	Table * table() const;
	
	// If this is set then any tables with inheritance will result in JOIN ONLY table, and multiple selects will be performed for each child table
	// If false then the table will be selected without the only clause and return the table_oid to know the proper table for each row.  Unselected
	// columns will have their NotSelected bits set
	bool joinOnly() const { return mJoinOnly; }
	
	QList<JoinCondition> joinConditions() const;

	JoinedSelect & join( Table * joinTable, const QString & condition = QString(), JoinType type = InnerJoin, bool ignoreResults = false, const QString & alias = QString() );
	template<typename T> JoinedSelect & join( QString condition = QString(), JoinType joinType = InnerJoin, bool ignoreResults = false, const QString & alias = QString() )
	{ return join( T::table(), condition, joinType, ignoreResults, alias ); }

	ResultSet select(const QString & where = QString(), VarList args = VarList());
	
protected:
	ResultSet selectSingleTable(const QString & where, VarList args);
	ResultSet selectInherited(int condInheritIndex, const QString & where, VarList args);
	JoinedSelect updateConditions( Table * oldTable, Table * newTable );
	bool finalize();
	
	Table * mTable;
	QString mAlias;
	QList<JoinCondition> mJoinConditions;
	bool mJoinOnly;
};

}; // namespace Stone

#include "resultset.h"

#endif // JOINED_SELECT_H
