
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

#include <qregexp.h>

#include "connection.h"
#include "database.h"
#include "joinedselect.h"
#include "table.h"

JoinedSelect::JoinedSelect(Table * primaryTable, QList<JoinCondition> joinConditions)
: mTable( primaryTable )
, mJoinConditions( joinConditions )
, mJoinOnly( false )
{}

JoinedSelect::JoinedSelect(Table * primaryTable, const QString & alias)
: mTable( primaryTable )
, mAlias( alias )
, mJoinOnly( false )
{
}

QString JoinedSelect::alias() const
{ return mAlias; }

Table * JoinedSelect::table() const
{ return mTable; }

QList<JoinCondition> JoinedSelect::joinConditions() const
{ return mJoinConditions; }

JoinedSelect & JoinedSelect::join( Table * joinTable, const QString & condition, JoinType type, bool ignoreResults, const QString & alias )
{
	if( mJoinConditions.isEmpty() )
		mJoinOnly = !(type == RightJoin || type == OuterJoin);
	JoinCondition jc;
	jc.table = joinTable;
	jc.condition = condition;
	jc.alias = alias;
	jc.type = type;
	jc.ignoreResults = ignoreResults;
	jc.joinOnly = !(type == LeftJoin || type == OuterJoin);
	mJoinConditions += jc;
	return *this;
}

static QString updateCondition(const QString & cond, Table * oldTable, Table * newTable)
{
	QString ret(cond);
	ret.replace( oldTable->tableName() + ".", newTable->tableName() + ".", Qt::CaseInsensitive );
	return ret;
}

JoinedSelect JoinedSelect::updateConditions( Table * oldTable, Table * newTable )
{
	JoinedSelect ret(*this);
	if( oldTable == newTable ) return ret;
	for( int i=0; i < mJoinConditions.size(); ++i )
		ret.mJoinConditions[i].condition = updateCondition(mJoinConditions[i].condition, oldTable, newTable);
	return ret;
}

ResultSet JoinedSelect::selectSingleTable(const QString & where, VarList args)
{
	if( !finalize() )
		return ResultSet(QList<RecordList>());
	QList<RecordList> results = mTable->database()->connection()->joinedSelect( *this, where, args );
	for( int i=0; i < results.size(); ++i ) {
		Table * t = (i==0) ? mTable : mJoinConditions[i-1].table;
		results[i] = t->processIncoming(results[i],false);
	}
	return ResultSet(results);
}

ResultSet JoinedSelect::selectInherited(int condInheritIndex, const QString & _where, VarList args)
{
	ResultSet ret;
	Table * orig = mJoinConditions[condInheritIndex].table;
	if( mJoinConditions[condInheritIndex].joinOnly && orig->children().size() ) {
		foreach( Table * table, orig->tableTree() ) {
			QString where(updateCondition(_where,orig,table));
			JoinedSelect js = updateConditions(orig,table);
			js.mJoinConditions[condInheritIndex].table = table;
			if( condInheritIndex + 1 == mJoinConditions.size() )
				ret.append( js.selectSingleTable(where, args) );
			else
				ret.append( js.selectInherited(condInheritIndex+1,where,args) );
		}
	} else {
		if( condInheritIndex + 1 == mJoinConditions.size() )
			ret.append( selectSingleTable(_where, args) );
		else
			ret.append( selectInherited(condInheritIndex+1,_where,args) );
	}
	return ret;
}

ResultSet JoinedSelect::select(const QString & _where, VarList args)
{
	ResultSet ret;
	if( mJoinOnly && mTable->children().size() ) {
		foreach( Table * table, mTable->tableTree() ) {
			JoinedSelect js = updateConditions(mTable,table);
			js.mTable = table;
			QString where(updateCondition(_where,mTable,table));
			ret.append(js.selectInherited(0,where,args));
		}
	} else
		ret.append( selectInherited(0,_where,args) );
	return ret;
}

static QString tableQuoted( const QString & table )
{
	if( table.contains( "-" ) || table == "user" )
		return "\"" + table + "\"";
	return table;
}

static QString generateJoinCondition( Field * fkey, Table * pkeyTable, const QString & pkeyalias, const QString & otherAlias )
{
	return tableQuoted(pkeyalias) + "." + pkeyTable->schema()->primaryKey().toLower() + "=" + tableQuoted(otherAlias) + "." + fkey->name().toLower();
}

static QString nextAlias( const QString & alias )
{
	QRegExp re( "(.*_)(\\d+)$" );
	if( re.exactMatch( alias ) ) {
		int num = re.cap(2).toInt();
		return re.cap(1) + QString::number(num+1);
	}
	return alias + "_1";
}

bool JoinedSelect::finalize()
{
	QStringList aliases;
	if( mAlias.isEmpty() )
		mAlias = mTable->schema()->tableName();
	aliases.append(mAlias);
	for( int i=0; i < mJoinConditions.size(); ++i ) {
		JoinCondition & jc = mJoinConditions[i];
		QString _alias = jc.alias.isEmpty() ? jc.table->schema()->tableName() : jc.alias;
		// Report an error if a user defined alias conflicts with another alias
		while( aliases.contains( _alias )  ) {
			if( !jc.alias.isEmpty() ) {
				LOG_5( "Alias used more than once: " + _alias );
				return false;
			}
			_alias = nextAlias( _alias );
		}
		jc.alias = _alias;
		aliases.append(_alias);
	}
	for( int i=0; i < mJoinConditions.size(); ++i ) {
		JoinCondition & jc = mJoinConditions[i];
		if( jc.condition.isEmpty() ) {
			Table * other = (i == 0) ? table() : mJoinConditions[i-1].table;
			QString otherAlias = i == 0 ? mAlias : mJoinConditions[i-1].alias;
			foreach( Field * f, other->schema()->columns() )
				if( f->foreignKeyTable() && f->foreignKeyTable()->isDescendant(jc.table->schema()) ) {
					jc.condition = generateJoinCondition( f, jc.table, jc.alias, otherAlias );
					break;
				}
			if( jc.condition.isEmpty() ) {
				foreach( Field * f, jc.table->schema()->columns() )
					if( f->foreignKeyTable() && f->foreignKeyTable()->isDescendant(other->schema()) ) {
						jc.condition = generateJoinCondition( f, other, otherAlias, jc.alias );
						break;
					}
			}
		}
	}
	return true;
}
