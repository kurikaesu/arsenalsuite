
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

#include <assert.h>

#include <qdatetime.h>

#include "connection.h"
#include "expression.h"
#include "interval.h"
#include "record.h"
#include "table.h"
#include "tableschema.h"

namespace Stone {

struct Context
{
	Context() : exp(0), par(0) {}
	Context(const ExpressionPrivate * _exp, const Context & _parent) : exp(_exp), par(&_parent) {}
	const ExpressionPrivate * exp;
	const Context * par;
};

struct ToStrContext : public Context
{
	ToStrContext(Expression::StringType st) : Context(), stringType(st) {}
	ToStrContext(const ExpressionPrivate * _exp, const ToStrContext & _parent) : Context(_exp,_parent), stringType(_parent.stringType) {}
	Expression::StringType stringType;
};

struct PrepStruct
{
	PrepStruct() : hasSubQuery(false) {}
	bool hasSubQuery;
};

struct PrepContext : public Context
{
	PrepContext(PrepStruct * ps) : Context(), prepStruct(ps) {}
	PrepContext(const ExpressionPrivate * _exp, const PrepContext & _parent) : Context(_exp,_parent), prepStruct(_parent.prepStruct) {}
	PrepStruct * prepStruct;
};

#define TS_CTX ToStrContext c(this,pc)
#define PREP_CTX PrepContext c(this,pc)

class ExpressionPrivate {
public:
	void ref() {
		refCount.ref();
	}
	void deref() {
		bool neZero = refCount.deref();
		if( !neZero ) {
			delete this;
		}
	}
	
	ExpressionPrivate( Expression::Type _type ) : type(_type) {}
	ExpressionPrivate( const ExpressionPrivate & other ) : type(other.type) {}
	virtual ExpressionPrivate * copy() const { return new ExpressionPrivate(*this); }
	virtual ~ExpressionPrivate() {}
	virtual QList<ExpressionPrivate*> children() const { return QList<ExpressionPrivate*>(); }
	virtual QVariant::Type valueType() const { return QVariant::Invalid; }
	virtual QString toString(const ToStrContext &) const { return QString(); }
	virtual QVariant value(const Record & ) const { return QVariant(); }
	virtual void prepareForExec(const PrepContext & pc, Connection * conn) {
		PREP_CTX;
		foreach( ExpressionPrivate * child, children() )
			child->prepareForExec(c,conn);
	}
	bool matches( const Record & record ) const { return value(record).toBool(); }
	virtual void append( ExpressionPrivate * ) { assert(0); }
	virtual bool isOnlySupplemental() const
	{
		foreach( ExpressionPrivate * child, children() )
			if( !child->isOnlySupplemental() )
				return false;
		return false;
	}
	bool containsWhereClause()
	{
		if( type == Expression::Limit_Expression || type == Expression::OrderBy_Expression ||
			type == Expression::Field_Expression || type == Expression::Table_Expression ||
			type == Expression::Query_Expression || (type >= Expression::Union_Expression && type <= Expression::ExceptAll_Expression) || type == Expression::Value_Expression )
			return false;
		return true;
	}
	bool isQuery()
	{
		return type == Expression::Query_Expression || (type >= Expression::Union_Expression && type <= Expression::ExceptAll_Expression);
	}
	Expression::Type type;
private:
	QAtomicInt refCount;
};

class _CompoundExpression : public ExpressionPrivate {
public:
	_CompoundExpression( Expression::Type type ) : ExpressionPrivate( type ) {}
	_CompoundExpression( const _CompoundExpression & other ) : ExpressionPrivate(other)
	{
		foreach( ExpressionPrivate * p, other.expressions ) {
			append( p->copy() );
		}
	}
	
	~_CompoundExpression() {
		foreach( ExpressionPrivate * p, expressions ) {
			p->deref();
		}
	}
	
	ExpressionPrivate * copy() const { return new _CompoundExpression(*this); }

	void append( ExpressionPrivate * p ) {
		if( p ) {
			p->ref();
			expressions.append(p);
		}
	}
	
	virtual QString toString(const ToStrContext & pc) const {
		TS_CTX;
		QStringList parts;
		foreach( ExpressionPrivate * child, expressions )
			parts.append( child->toString(c) );
		return "(" + parts.join( separator() ) + ")";
	}
	
	virtual QString separator() const
	{ assert(0); return QString(); }
	
	virtual QList<ExpressionPrivate*> children() const
	{ return expressions; }
	
	QList<ExpressionPrivate*> expressions;
};

class _OneArgExpression : public ExpressionPrivate
{
public:
	_OneArgExpression( Expression::Type type, ExpressionPrivate * _child )
	: ExpressionPrivate(type)
	, child(_child)
	{
		if( child )
			child->ref();
	}
	_OneArgExpression( const _OneArgExpression & other )
	: ExpressionPrivate(other)
	, child(0)
	{
		if( other.child ) {
			child = other.child->copy();
			child->ref();
		}
	}
	~_OneArgExpression()
	{
		if( child )
			child->deref();
	}
	ExpressionPrivate * copy() const { return new _OneArgExpression(*this); }
	virtual QList<ExpressionPrivate*> children() const
	{
		QList<ExpressionPrivate*> ret;
		if( child ) ret << child;
		return ret;
	}
	virtual QVariant value( const Record & record ) const
	{ return child ? child->value(record) : QVariant(); }
	ExpressionPrivate * child;
};

class _TwoArgExpression : public ExpressionPrivate
{
public:
	_TwoArgExpression( Expression::Type type, ExpressionPrivate * _left, ExpressionPrivate * _right )
	: ExpressionPrivate(type)
	, left(_left)
	, right(_right)
	{
		if( left )
			left->ref();
		if( right )
			right->ref();
	}
	_TwoArgExpression( const _TwoArgExpression & other )
	: ExpressionPrivate(other)
	, left(0)
	, right(0)
	{
		if( other.left ) {
			left = other.left->copy();
			left->ref();
		}
		if( other.right ) {
			right = other.right->copy();
			right->ref();
		}
	}
	~_TwoArgExpression()
	{
		if( left )
			left->deref();
		if( right )
			right->deref();
	}
	ExpressionPrivate * copy() const { return new _TwoArgExpression(*this); }
	virtual QList<ExpressionPrivate*> children() const
	{ 
		QList<ExpressionPrivate*> ret;
		if( left ) ret << left;
		if( right ) ret << right;
		return ret;
	}
	ExpressionPrivate * left, * right;
};

class _AliasExpression : public _OneArgExpression
{
public:
	_AliasExpression( ExpressionPrivate * child, const QString & _alias = QString() )
	: _OneArgExpression( Expression::Alias_Expression, child )
	, alias(_alias)
	{}
	_AliasExpression( const _AliasExpression & other )
	: _OneArgExpression( other )
	, alias(other.alias)
	, tableAlias(other.tableAlias)
	{}
	ExpressionPrivate * copy() const { return new _AliasExpression(*this); }
	QString toString(const ToStrContext & pc) const
	{
		TS_CTX;
		QString ret = child->toString(c);
		if( !alias.isEmpty() )
			ret += " AS " + alias;
		return ret;
	}
	
	QString alias, tableAlias;
};

class _AndExpression : public _CompoundExpression
{
public:
	_AndExpression() : _CompoundExpression( Expression::And_Expression ) {}
	_AndExpression( const _AndExpression & other )
	: _CompoundExpression( other ) {}
	
	ExpressionPrivate * copy() const { return new _AndExpression(*this); }
	
	QVariant::Type valueType() const {
		return QVariant::Bool;
	}
	QVariant value( const Record & record ) const {
		foreach( ExpressionPrivate * c, expressions )
			if( !c->matches(record) )
				return false;
		return true;
	}
	virtual QString separator() const { return " AND "; }
};

class _OrExpression : public _CompoundExpression
{
public:
	_OrExpression() : _CompoundExpression( Expression::Or_Expression ) {}
	_OrExpression( const _OrExpression & other ) : _CompoundExpression(other) {}
	ExpressionPrivate * copy() const { return new _OrExpression(*this); }
	QVariant::Type valueType() const {
		return QVariant::Bool;
	}
	QVariant value( const Record & record ) const {
		foreach( ExpressionPrivate * c, expressions )
			if( c->matches(record) )
				return true;
		return false;
	}
	virtual QString separator() const { return " OR "; }
};

class _IsNullExpression: public _OneArgExpression
{
public:
	_IsNullExpression( ExpressionPrivate * _child, Expression::Type type )
	: _OneArgExpression( type, _child ) {}
	_IsNullExpression( const _IsNullExpression & other ) : _OneArgExpression(other) {}
	ExpressionPrivate * copy() const { return new _IsNullExpression(*this); }
	QVariant::Type valueType() const {
		return QVariant::Bool;
	}
	QVariant value( const Record & record ) const {
		QVariant v = child->value(record);
		bool isNull = true;
		if( v.userType() == qMetaTypeId<Record>() )
			isNull = !v.value<Record>().isValid();
		else
			isNull = v.isNull();
		return type == Expression::IsNotNull_Expression ? !isNull : isNull;
	}
	QString toString(const ToStrContext & pc) const 
	{
		TS_CTX;
		return child->toString(c) + (type == Expression::IsNotNull_Expression ? " IS NOT NULL" : " IS NULL");
	}
};

bool eq( const QVariant & v1, const QVariant & v2 )
{
	QVariant cvt1(v1), cvt2(v2);
	QVariant::Type t1 = (QVariant::Type)v1.userType(), t2 = (QVariant::Type)v2.userType();
	const QVariant::Type rt = (QVariant::Type)qMetaTypeId<Record>();
	if( t1 == rt && t2 == rt )
		return v1.value<Record>() == v2.value<Record>();
	if( t1 == rt )
		cvt1 = QVariant(v1.value<Record>().key());
	else if( t2 == rt )
		cvt2 = QVariant(v2.value<Record>().key());
	return cvt1 == cvt2;
}

class _InExpression : public _CompoundExpression
{
public:
	_InExpression( ExpressionPrivate * leftExp ) : _CompoundExpression( Expression::In_Expression )
	{
		append(leftExp);
	}
	_InExpression( const _InExpression & other ) : _CompoundExpression(other) {}
	ExpressionPrivate * copy() const { return new _InExpression(*this); }
	QVariant::Type valueType() const {
		return QVariant::Bool;
	}
	QVariant value( const Record & record ) const {
		if( expressions.size() < 2 ) return false;
		bool foundMatch = false;
		QVariant left = expressions[0]->value(record);
		for( int i = 1; i < expressions.size(); ++i ) {
			if( eq(expressions[i]->value(record), left) ) {
				foundMatch = true;
				break;
			}
		}
		return type == Expression::In_Expression ? foundMatch : !foundMatch;
	}
	QString toString(const ToStrContext & pc) const
	{
		if( expressions.size() < 2 ) return QString();
		TS_CTX;
		QStringList parts;
		for( int i = 1; i < expressions.size(); ++i )
			parts.append( expressions[i]->toString(c) );
		return expressions[0]->toString(c) + QString(type==Expression::NotIn_Expression ? " NOT" : "") + " IN (" + parts.join(",") + ")";
	}
};

class _LikeExpression : public _TwoArgExpression
{
public:
	_LikeExpression( ExpressionPrivate * _left, ExpressionPrivate * _right, bool caseSensitive )
	: _TwoArgExpression( caseSensitive ? Expression::Like_Expression : Expression::ILike_Expression, _left, _right )
	{}
	_LikeExpression( const _LikeExpression & other ) : _TwoArgExpression(other) {}
	ExpressionPrivate * copy() const { return new _LikeExpression(*this); }
	QVariant::Type valueType() const {
		return QVariant::Bool;
	}
	static QString likeToRegEx( const QString & like )
	{
		QString ret;
		ret.reserve(like.size() + 20);
		for( int i = 0; i < like.size(); ++i ) {
			QChar c = like.at(i);
			if( c == '%' ) {
				ret.append( ".*" );
				continue;
			}
			if( c == '_' ) {
				ret.append( "." );
				continue;
			}
			if( c == '(' || c == ')' || c == '.' || c == '*' || c == '{' || c == '}' || c == '[' || c == ']' || c == '\\' || c == '^' || c == '$' || c == '|' || c == '+' || c == '?' )
				ret.append( '\\' );
			ret.append( c );
		}
		ret.squeeze();
		return ret;
	}
	QVariant value( const Record & record ) const
	{
		return QRegExp(likeToRegEx(right->value(record).toString()), type == Expression::Like_Expression ? Qt::CaseSensitive : Qt::CaseInsensitive).exactMatch(left->value(record).toString());
	}
	QString toString(const ToStrContext & pc) const
	{
		TS_CTX;
		return left->toString(c) + (type == Expression::Like_Expression ? " LIKE " : " ILIKE ") + right->toString(c);
	}
};

static QString quoteAndEscapeString( const QString & str )
{
	QString out;
	out.reserve(str.size() * 1.1 + 2);
	out.append("\'");
	bool escapedString = false;
	for( int i = 0; i < str.size(); ++i ) {
		QChar c = str[i];
		if( c == '\\' ) {
			out.append("\\\\");
			escapedString = true;
		} else if( c == '\b' ) {
			out.append("\\b");
			escapedString = true;
		} else if( c == '\f' ) {
			out.append("\\f");
			escapedString = true;
		} else if( c == '\n' ) {
			out.append("\\n");
			escapedString = true;
		} else if( c == '\t' ) {
			out.append("\\t");
			escapedString = true;
		} else if( c == '\'' )
			out.append("''");
		else
			out.append(c);
	}
	out.append("\'");
	if( escapedString )
		out = "E" + out;
	out.squeeze();
	return out;
}

class _RegexExpression : public _OneArgExpression
{
public:
	_RegexExpression( ExpressionPrivate * p, const QString & _regex, Qt::CaseSensitivity _cs )
	: _OneArgExpression( Expression::RegEx_Expression, p )
	, regex(_regex)
	, cs(_cs)
	{}
	_RegexExpression( const _RegexExpression & other ) : _OneArgExpression(other), regex(other.regex), cs(other.cs) {}
	ExpressionPrivate * copy() const { return new _RegexExpression(*this); }

	QVariant::Type valueType() const {
		return QVariant::Bool;
	}

	QString toString(const ToStrContext & pc) const {
		TS_CTX;
		return child->toString(c) + " " + (cs == Qt::CaseSensitive ? "~ " : "~* ") + quoteAndEscapeString(regex);
	}
	
	QVariant value( const Record & record ) const
	{
		return QRegExp(regex,cs).indexIn(child->value(record).toString()) >= 0;
	}
	
	QString regex;
	Qt::CaseSensitivity cs;
};

bool gt( const QVariant & v1, const QVariant & v2 )
{
	QVariant::Type t1 = (QVariant::Type)v1.userType(), t2 = (QVariant::Type)v2.userType();
	if( t1 != t2 )
		return false;
	switch( t1 ) {
		case QVariant::Bool:
			return v1.toBool() && !v2.toBool();
		case QVariant::Char:
			return v1.toChar() > v2.toChar();
		case QVariant::Date:
			return v1.toDate() > v2.toDate();
		case QVariant::DateTime:
			return v1.toDateTime() > v2.toDateTime();
		case QVariant::Double:
			return v1.toDouble() > v2.toDouble();
		case QVariant::Int:
			return v1.toInt() > v2.toInt();
		case QVariant::LongLong:
			return v1.toLongLong() > v2.toLongLong();
		case QVariant::String:
			return v1.toString() > v2.toString();
		case QVariant::Time:
			return v1.toTime() > v2.toTime();
		case QVariant::UInt:
			return v1.toUInt() > v2.toUInt();
		case QVariant::ULongLong:
			return v1.toULongLong() > v2.toULongLong();
		default:
			if( t1 == qMetaTypeId< ::Interval>() )
				return v1.value<Interval>() > v2.value<Interval>();
	}
	return false;
}

bool gte( const QVariant & v1, const QVariant & v2 )
{
	if( v1 == v2 ) return true;
	return gt(v1,v2);
}

bool lt( const QVariant & v1, const QVariant & v2 )
{
	return !gte(v1,v2);
}

bool lte( const QVariant & v1, const QVariant & v2 )
{
	return !gt(v1,v2);
}

class _ComparisonExpression : public _CompoundExpression
{
public:
	_ComparisonExpression( Expression::Type type ) : _CompoundExpression( type ) {}
	_ComparisonExpression( const _ComparisonExpression & other ) : _CompoundExpression(other) {}
	ExpressionPrivate * copy() const { return new _ComparisonExpression(*this); }

	QVariant::Type valueType() const {
		return QVariant::Bool;
	}

	QVariant value( const Record & record ) const
	{
		if( expressions.size() != 2 ) return false;
		QVariant v = expressions[0]->value(record);
		QVariant v2 = expressions[1]->value(record);
		switch( type ) {
			case Expression::Equals_Expression:
				return eq(v,v2);
			case Expression::NEquals_Expression:
				return !eq(v,v2);
			case Expression::Larger_Expression:
				return gt(v,v2);
			case Expression::LargerOrEquals_Expression:
				return gte(v,v2);
			case Expression::Less_Expression:
				return lt(v,v2);
			case Expression::LessOrEquals_Expression:
				return lte(v,v2);
			case Expression::BitAnd_Expression:
				return v.toULongLong() & v2.toULongLong();
			case Expression::BitOr_Expression:
				return v.toULongLong() | v2.toULongLong();
			case Expression::BitXor_Expression:
				return v.toULongLong() ^ v2.toULongLong();
			case Expression::Plus_Expression:
			{
				QVariant::Type t1 = (QVariant::Type)v.userType(), t2 = (QVariant::Type)v2.userType(),
				intervalType = (QVariant::Type)qMetaTypeId< ::Interval>();
				if( t1 == QVariant::String && t2 == QVariant::String )
					return v.toString() + v2.toString();
				if( t1 == QVariant::Double || t2 == QVariant::Double )
					return v.toDouble() + v2.toDouble();
				if( t1 == intervalType ) {
					Interval i = v.value<Interval>();
					if( t2 == QVariant::Date )
						return i.adjust(QDateTime(v2.toDate()));
					if( t2 == QVariant::DateTime )
						return i.adjust(v2.toDateTime());
					if( t2 == intervalType )
						return QVariant::fromValue<Interval>(i + v2.value<Interval>());
				}
				if( t2 == intervalType ) {
					Interval i = v2.value<Interval>();
					if( t1 == QVariant::Date )
						return i.adjust(QDateTime(v.toDate()));
					if( t1 == QVariant::DateTime )
						return i.adjust(v.toDateTime());
					if( t1 == intervalType )
						return QVariant::fromValue<Interval>(i + v.value<Interval>());
				}
				return v.toLongLong() + v2.toLongLong();
			}
			default:
				break;
		}
		return QVariant();
	}
	virtual QString separator() const {
		switch( type ) {
			case Expression::Equals_Expression:
				return " = ";
			case Expression::NEquals_Expression:
				return " != ";
			case Expression::Larger_Expression:
				return " > ";
			case Expression::LargerOrEquals_Expression:
				return " >= ";
			case Expression::Less_Expression:
				return " < ";
			case Expression::LessOrEquals_Expression:
				return " <= ";
			case Expression::BitAnd_Expression:
				return " & ";
			case Expression::BitOr_Expression:
				return " | ";
			case Expression::BitXor_Expression:
				return " ^ ";
			case Expression::Plus_Expression:
			{
				if( expressions.size() == 2 ) {
					QVariant::Type vt1 = expressions[0]->valueType(), vt2 = expressions[1]->valueType();
					if( vt1 == QVariant::String && vt2 == QVariant::String )
						return " || ";
				}
				return " + ";
			}
			default:
				break;
		}
		return QString();
	}
};

class _NotExpression : public _OneArgExpression
{
public:
	_NotExpression( ExpressionPrivate * _child ) : _OneArgExpression( Expression::Not_Expression, _child )
	{}
	_NotExpression( const _NotExpression & other ) : _OneArgExpression(other) {}
	ExpressionPrivate * copy() const { return new _NotExpression(*this); }

	QVariant::Type valueType() const {
		return QVariant::Bool;
	}
	QVariant value( const Record & record ) const
	{
		return child ? !child->matches(record) : false;
	}
	QString toString(const ToStrContext & pc) const {
		TS_CTX;
		QString cs = child ? child->toString(c) : QString();
		if( cs.size() && !(cs.startsWith('(') && cs.endsWith(')')) )
			return "NOT (" + cs + ")";
		return "NOT " + cs;
	}
};

class _OrderByExpression : public _TwoArgExpression
{
public:
	_OrderByExpression( ExpressionPrivate * e, ExpressionPrivate * _orderField, Expression::OrderByDirection _dir )
	: _TwoArgExpression( Expression::OrderBy_Expression, e, _orderField )
	, dir(_dir)
	{}
	_OrderByExpression( const _OrderByExpression & other ) : _TwoArgExpression(other), dir(other.dir) {}
	ExpressionPrivate * copy() const { return new _OrderByExpression(*this); }

	virtual bool isOnlySupplemental() const
	{
		return left ? left->isOnlySupplemental() : true;
	}

	QString toString(const ToStrContext & pc) const {
		TS_CTX;
		QString ret = left ? left->toString(c) : QString();
		if( left && left->type == Expression::OrderBy_Expression )
			ret += ", ";
		else
			ret += " ORDER BY ";
		return ret + right->toString(c) + (dir == Expression::Ascending ? " ASC" : " DESC");
	}
	Expression::OrderByDirection dir;
};

class _LimitExpression : public _OneArgExpression
{
public:
	_LimitExpression( ExpressionPrivate * child, int _limit, int _offset )
	: _OneArgExpression( Expression::Limit_Expression, child )
	, limit( _limit )
	, offset( _offset )
	{}
	_LimitExpression( const _LimitExpression & other ) : _OneArgExpression(other), limit(other.limit), offset(other.offset) {}
	ExpressionPrivate * copy() const { return new _LimitExpression(*this); }

	virtual bool isOnlySupplemental() const
	{
		return child ? child->isOnlySupplemental() : true;
	}

	QString toString(const ToStrContext & pc) const {
		TS_CTX;
		return (child ? child->toString(c) : 0) + " LIMIT " + QString::number(limit) + (offset > 0 ? " OFFSET " + QString::number(offset) : QString());
	}
	int limit, offset;
};

// This is meant to be temporarily used in the expression tree
class _RecordValueExpression : public ExpressionPrivate
{
public:
	_RecordValueExpression( const Record & _record )
	: ExpressionPrivate( Expression::RecordValue_Expression )
	, record(_record)
	{}
	Record record;
};

class _ValueExpression : public ExpressionPrivate
{
public:
	_ValueExpression( const QVariant & _v ) : ExpressionPrivate( Expression::Value_Expression ), v(_v) {}
	_ValueExpression( const _ValueExpression & other ) : ExpressionPrivate(other), v(other.v) {}
	ExpressionPrivate * copy() const { return new _ValueExpression(*this); }
	QVariant::Type valueType() const {
		return (QVariant::Type)v.userType();
	}
	virtual QVariant value( const Record & ) const
	{
		if( v.userType() == qMetaTypeId<Record>() && !v.value<Record>().isRecord() )
			return QVariant(QVariant::Int);
		return v;
	}
	virtual QString toString(const ToStrContext & c) const {
		QString ret, type;
		bool needQuotes = true;
		switch( v.userType() ) {
			case QVariant::String:
				ret = quoteAndEscapeString(v.toString());
				needQuotes = false;
				break;
			case QVariant::StringList:
			{
				QStringList qae;
				foreach( QString s, v.toStringList() )
					qae.append( quoteAndEscapeString(s) );
				ret = qae.join(",");
				needQuotes = false;
				break;
			}
			case QVariant::Time:
			{
				QTime t = v.toTime();
				if( t.isNull() ) {
					ret = "NULL";
					needQuotes = false;
				} else
					ret = t.toString(Qt::ISODate);
				break;
			}
			case QVariant::Date:
			{
				QDate d = v.toDate();
				if( d.isNull() ) {
					ret = "NULL";
					needQuotes = false;
				} else {
					ret = d.toString(Qt::ISODate);
					type = "date";
				}
				break;
			}
			case QVariant::DateTime:
			{
				QDateTime dt = v.toDateTime();
				if( dt.isNull() ) {
					ret = "NULL";
					needQuotes = false;
				} else {
					ret = dt.toString(Qt::ISODate);
					type = "timestamp";
				}
				break;
			}
			default:
				if( v.userType() == qMetaTypeId<Interval>() ) {
					ret = v.value<Interval>().toString();
					type = "interval";
				} else if( v.userType() == qMetaTypeId<Record>() ) {
					if( c.stringType == Expression::InfoString )
						ret = v.value<Record>().debug();
					else {
						needQuotes = false;
						Record r = v.value<Record>();
						if( r.isValid() )
							ret = QString::number(r.key(true));
						else
							ret = "NULL";
					}
				} else {
					needQuotes = false;
					if( v.isNull() )
						ret = "NULL";
					else
						ret = v.toString();
				}
		}
		if( needQuotes )
			ret = "'" + ret + "'";
		if( type.size() )
			ret += "::" + type;
		return ret;
	}
	QVariant v;
};

class _NullExpression : public ExpressionPrivate
{
public:
	_NullExpression( Field::Type _castType = Field::Invalid ) : ExpressionPrivate( Expression::Null_Expression ), castType(_castType) {}
	_NullExpression( const _NullExpression & other ) : ExpressionPrivate(other), castType(other.castType) {}
	ExpressionPrivate * copy() const { return new _NullExpression(*this); }
	QVariant::Type valueType() const {
		return QVariant::Type(Field::qvariantType(castType));
	}
	virtual QVariant value( const Record & ) const
	{
		return castType == Field::Invalid ? QVariant() : QVariant(castType);
	}
	virtual QString toString(const ToStrContext &) const {
		return castType == Field::Invalid ? "NULL" : "NULL::" + Field::dbTypeString(castType);
	}
	Field::Type castType; // Invalid for untyped nulls
};

class _ValuesExpression : public ExpressionPrivate
{
public:
	_ValuesExpression( RecordList records, const QString & _alias, FieldList _fields )
	: ExpressionPrivate( Expression::Values_Expression )
	, fields(_fields)
	, alias(_alias)
	{
		foreach( Record r, records ) {
			foreach( Field * f, fields ) {
				values << r.getValue(f);
			}
		}
	}
	_ValuesExpression( const _ValuesExpression & other ) : ExpressionPrivate(other), fields(other.fields), values(other.values), alias(other.alias) {}
	ExpressionPrivate * copy() const { return new _ValuesExpression(*this); }

	virtual QString toString(const ToStrContext & pc) const {
		QStringList colNames;
		QStringList rowStrings;
		const int cols = fields.size();
		const int rows = values.size() / cols;
		for( int r = 0; r < rows; r++ ) {
			QStringList valueStrings;
			for( int i = 0; i < cols; ++i ) {
				QString vs = _ValueExpression(values[r*cols+i]).toString(pc);
				if( r == 0 ) {
					colNames << fields[i]->name().toLower();
					vs += "::" + fields[i]->dbTypeString();
				}
				valueStrings += vs;
			}
			rowStrings += "(" + valueStrings.join(",") + ")";
		}
		return "(VALUES " + rowStrings.join(",") + ") AS " + alias + "(\"" + colNames.join("\",\"") + "\")";
	}

	FieldList fields;
	QList<QVariant> values;
	QString alias;
};

class _FieldExpression : public ExpressionPrivate
{
public:
	_FieldExpression( Field * f ) : ExpressionPrivate( Expression::Field_Expression ), field(f) {}
	_FieldExpression( const _FieldExpression & other ) : ExpressionPrivate(other), field(other.field) {}
	ExpressionPrivate * copy() const { return new _FieldExpression(*this); }

	QVariant::Type valueType() const {
		return (QVariant::Type)field->qvariantType();
	}
	virtual QVariant value( const Record & record ) const {
		QVariant ret = record.getValue(field);
		if( ret.userType() == qMetaTypeId<Record>() ) {
			if( !ret.value<Record>().isValid() )
				ret = QVariant(QVariant::Int);
		}
		return ret;
	}
	virtual QString toString(const ToStrContext & pc) const {
		QString tn;
		if( pc.exp && pc.exp->type == Expression::Alias_Expression ) {
			_AliasExpression * ae = (_AliasExpression*)pc.exp;
			tn = ae->tableAlias;
		}
//		if( tn.isEmpty() )
//			tn = field->table()->tableName();
		if( pc.stringType == Expression::InfoString )
			return (tn.isEmpty() ? QString() : tn + ".") + field->name();
		return (tn.isEmpty() ? QString() : tn + ".") + "\"" + field->name().toLower() + "\"";
		
	}
	Field * field;
};

class _TableOidExpression : public ExpressionPrivate
{
public:
	_TableOidExpression() : ExpressionPrivate( Expression::TableOid_Expression ) {}
	_TableOidExpression( const _TableOidExpression & other ) : ExpressionPrivate(other) {}
	ExpressionPrivate * copy() const { return new _TableOidExpression(*this); }
	QVariant::Type valueType() const {
		return QVariant::Int;
	}
	virtual QString toString(const ToStrContext &) const {
		return "tableoid";
	}
};

class _TableExpression : public ExpressionPrivate
{
public:
	_TableExpression( Table * t, bool _only = false ) : ExpressionPrivate( Expression::Table_Expression ), table(t), only(_only) {}
	_TableExpression( const _TableExpression & other ) : ExpressionPrivate(other), table(other.table), only(other.only) {}
	ExpressionPrivate * copy() const { return new _TableExpression(*this); }

	virtual QString toString(const ToStrContext &) const {
		QString ret = "\"" + table->tableName().toLower() + "\"";
		if( only )
			ret = "ONLY " + ret;
		return ret;
	}
	Table * table;
	bool only;
};

class _QueryExpression : public _OneArgExpression
{
public:
	_QueryExpression( ExpressionList _returns, ExpressionList _fromList, ExpressionPrivate * _child )
	: _OneArgExpression( Expression::Query_Expression, _child )
	, returns(_returns)
	, fromList(_fromList)
	{
	}
	_QueryExpression( const _QueryExpression & other ) : _OneArgExpression(other), returns(other.returns), fromList(other.fromList) {}
	ExpressionPrivate * copy() const { return new _QueryExpression(*this); }
	
	virtual void prepareForExec(const PrepContext & pc, Connection * conn) {
		PREP_CTX;
		// Do the children first, to avoid modifying Table expressions that are
		// part of an already modified expression
		_OneArgExpression::prepareForExec(pc, conn);
		
		foreach( Expression e, returns )
			if(e.p)
				e.p->prepareForExec(c,conn);
		foreach( Expression e, fromList )
			if(e.p)
				e.p->prepareForExec(c,conn);
		
		// If there is a valid changeset enabled, then we must
		// transform the query to reflect updated records for
		// each from table
		if( ChangeSet::current().isValid() ) {
			if( !pc.prepStruct->hasSubQuery ) {
				const Context * p = &pc;
				while( p ) {
					if( p->exp && (p->exp->type == Expression::Query_Expression || p->exp->type == Expression::In_Expression || p->exp->type == Expression::NotIn_Expression) ) {
						c.prepStruct->hasSubQuery = true;
						break;
					}
					p = p->par;
				}
			}
			
			if( !pc.prepStruct->hasSubQuery )
				return;
			
			for( int i = fromList.size() - 1; i >= 0; --i ) {
				Expression from = fromList[i];
				
				if( from.type() == Expression::Table_Expression ) {
					RecordList added, updated, removed;
					Table * table = ((_TableExpression*)from.p)->table;
					ChangeSet::current().visibleRecordsChanged( &added, &updated, &removed, QList<Table*>() << table );
					if( added.isEmpty() && updated.isEmpty() && removed.isEmpty() )
						continue;
					Field * pkey = table->schema()->field(table->schema()->primaryKeyIndex());
					bool haveExclusions = updated.size() + removed.size() > 0;
					Expression mod = Query( table->schema()->columns(), from, haveExclusions ? !Expression::createField(pkey).in(updated+removed) : Expression() );
					if( added.size() + updated.size() > 0 ) {
						// Ensure each record has primary keys
						foreach( Record r, added ) r.key(true);
						foreach( Record r, updated ) r.key(true);
						mod = Expression::createCombination( Expression::UnionAll_Expression, ExpressionList() << mod << Query( ExpressionList(), Expression::createValues( added + updated, "v" ), Expression() ) );
					}
					fromList.replace(i, mod);
				}
			}
		}
	}
	
	// Translates each FROM expression
	void transformToInheritedUnion(TableList tables, QList<RecordReturn> & rrl)
	{
		QMap<Table*,FieldList> fieldsByTable;
		QVector<int> typesByPosition(1);
		QList<QVariant> allArgs;
		QMap<Table*,QStringList> colsByTable;
		QStringList selects;

		//bool colsNeedTableName = innerW.toLower().contains( "join" );

		// First position is the table position
		typesByPosition[0] = Field::Int;
		RecordReturn rr;
		rr.tableOidPos = 0;
		
		
		foreach( Table * table, tables ) {
			TableSchema * schema = table->schema();
			FieldList fields = schema->columns();
			fieldsByTable[table] = fields;
			QVector<int> positions(fields.size());
			int queryCol = 1, recordCol = 0;
			foreach( Field * f, fields ) {
				if( f->flag( Field::NoDefaultSelect ) ) {
					positions[recordCol++] = -1;
					continue;
				}
				while( queryCol < typesByPosition.size() && typesByPosition[queryCol] != f->type() )
					queryCol++;
				if( queryCol >= typesByPosition.size() ) {
					typesByPosition.resize(queryCol+1);
					typesByPosition[queryCol] = f->type();
				}
				positions[recordCol++] = queryCol++;
			}
			
			rr.columnPositions[table] = positions;
		}

		int tablePos = 0;
		ExpressionList unionQueries;

		foreach( Table * table, tables ) {
			TableSchema * schema = table->schema();
			FieldList fields = schema->columns();
			fieldsByTable[table] = fields;
			int queryCol = 1;
			ExpressionList returns;
			returns.append( Expression::tableOid() );
			foreach( Field * f, fields ) {
				if( f->flag( Field::NoDefaultSelect ) )
					continue;
				while( queryCol < typesByPosition.size() && typesByPosition[queryCol] != f->type() ) {
					if( tablePos == 0 )
						returns.append( Expression::null(f->type()).alias(QString("c%1").arg(queryCol)) );
					else
						returns.append( Expression::null() );
					queryCol++;
				}
				Expression fe = Expression::createField(f);
				//if( tablePos == 0 )
				//	fe = fe.alias( QString("c%1").arg(queryCol) );
				returns.append(fe);
				queryCol++;
			}
			while( queryCol < typesByPosition.size() ) {
				if( tablePos == 0 )
					returns.append( Expression::null(Field::Type(typesByPosition[queryCol])).alias(QString("c%1").arg(queryCol)) );
				else
					returns.append( Expression::null() );
				queryCol++;
			}
			unionQueries.append( Query( returns, Expression(table,/*only=*/true), Expression() ) );
			tablePos++;
		}

		Expression union_ = Expression::createCombination( Expression::UnionAll_Expression, unionQueries );
		fromList = ExpressionList(union_);
		rrl.append(rr);
	}
	
	QVariant::Type valueType() const {
		if( returns.size() == 1 )
			return returns[0].p->valueType();
		return _OneArgExpression::valueType();
	}
	virtual QString toString(const ToStrContext & pc) const
	{
		TS_CTX;
		QString query;
		query = "SELECT ";
		QStringList retStrings;
		if( returns.size() ) {
			foreach( Expression ret, returns )
				retStrings += ret.p->toString(c);
			query += retStrings.join(", ");
		} else
			query += "*";
		
		if( fromList.size() ) {
			QStringList fromClauses;
			foreach( Expression from, fromList )
				if( from.p ) {
					QString fromStr = from.p->toString(c);
					if( from.p->type == Expression::Query_Expression || (from.p->type >= Expression::Union_Expression && from.p->type <= Expression::ExceptAll_Expression) )
						fromStr = "(" + fromStr + ") AS iq";
					fromClauses += fromStr;
				}
			query += " FROM " + fromClauses.join(",");
		}
		if( child ) {
			if( !child->isOnlySupplemental() )
				query += " WHERE ";
			query += child->toString(c);
		}
		// If we aren't a top-level query then we surround ourselves with ()
		if( pc.exp && (pc.exp->type < Expression::Union_Expression || pc.exp->type > Expression::ExceptAll_Expression) ) {
			query = "(" + query + ")";
		}
		return query;
	}
	ExpressionList returns, fromList;
};

class _CombinationExpression : public _CompoundExpression
{
public:
	_CombinationExpression(Expression::Type type)
	: _CompoundExpression( type )
	{}
	_CombinationExpression( const _CombinationExpression & other ) : _CompoundExpression(other) {}
	ExpressionPrivate * copy() const { return new _CombinationExpression(*this); }

	virtual QString toString(const ToStrContext & pc) const
	{
		TS_CTX;
		const char * combStrings [] = { " UNION ", " INTERSECT ", " EXCEPT " };
		
		int idx = qBound(0, (int)type - (int)Expression::Union_Expression, 5);
		QString comb(combStrings[idx/2]);
		if( idx % 2 )
			comb += "ALL ";
		QStringList subs;
		foreach( ExpressionPrivate * e, expressions )
			subs.append( e->toString(c) );
		return subs.join(comb);
	}
};

class _SqlExpression : public ExpressionPrivate
{
public:
	_SqlExpression(const QString & _sql)
	: ExpressionPrivate( Expression::Sql_Expression )
	, sql(_sql)
	{}
	_SqlExpression( const _SqlExpression & other ) : ExpressionPrivate(other), sql(other.sql) {}
	ExpressionPrivate * copy() const { return new _SqlExpression(*this); }
	
	virtual QString toString(const ToStrContext & pc) const
	{
		return sql;
	}

	QString sql;
};

TableAlias::TableAlias( Table * table, const QString & alias )
: mTable( table )
, mAlias( alias )
{}
	
Expression TableAlias::operator()(const Expression & e)
{
	if( !e.isValid() ) return e;
	if( e.type() == Expression::Alias_Expression ) {
		((_AliasExpression*)e.p)->tableAlias = mAlias;
		return e;
	}
	_AliasExpression * ae = new _AliasExpression(e.p);
	ae->tableAlias = mAlias;
	return Expression(ae);
}

TableAlias::operator Expression()
{
	return Expression(mTable).alias(mAlias);
}


Expression::Expression()
: p( 0 )
{
}

Expression::Expression( const QVariant & v )
: p( new _ValueExpression( v ) )
{
	p->ref();
}

Expression::Expression( const Record & r )
: p( new _RecordValueExpression( r ) )
{
	p->ref();
}

Expression::Expression( Table * t, bool only )
: p( new _TableExpression( t, only ) )
{
	p->ref();
}

Expression::Expression( ExpressionPrivate * _p )
: p( _p )
{
	if( p )
		p->ref();
}

Expression::Expression( const Expression & other )
: p( other.p )
{
	if( p )
		p->ref();
}

Expression::~Expression()
{
	if( p )
		p->deref();
}

Expression & Expression::operator=( const Expression & other )
{
	if( other.p != p ) {
		if( p )
			p->deref();
		p = other.p;
		if( p )
			p->ref();
	}
	return *this;
}

Expression::Type Expression::type() const
{
	return p ? p->type : Invalid_Expression;
}

/*
Expression Expression::in( std::initializer_list<Expression> list )
{
	_InExpression * ret = new _InExpression(p);
	for( const Expression * e = list.begin(); e != list.end(); ++e )
		ret->append(e->p);
	return Expression(ret);
}*/

Expression Expression::isNull()
{
	if( !p ) return Expression();
	return Expression(new _IsNullExpression(p,Expression::IsNull_Expression));
}

Expression Expression::isNotNull()
{
	if( !p ) return Expression();
	return Expression(new _IsNullExpression(p,Expression::IsNotNull_Expression));
}

static ExpressionPrivate * getRecordLink( const Expression & base, const Record & r )
{
	// We only support linking from a field expression to a record
	Field * f = ((_FieldExpression*)base.p)->field;
	TableSchema * fs = f->table();
	TableSchema * rs = r.table()->schema();
	if( f->flag(Field::ForeignKey) ) {
		TableSchema * fkt = f->foreignKeyTable();
		if( fkt->isDescendant(rs) )
			return new _ValueExpression(qVariantFromValue<Record>(r));
		foreach( Field * field, rs->columns() ) {
			if( field->flag(Field::ForeignKey) && fkt->isDescendant(field->foreignKeyTable()) ) {
				return new _ValueExpression(r.getValue(field));
			}
		}
	}
	else if( f->flag(Field::PrimaryKey) ) {
		if( fs->isDescendant(r.table()->schema()) )
			return new _ValueExpression(qVariantFromValue<Record>(r));
		foreach( Field * field, rs->columns() ) {
			if( field->flag(Field::ForeignKey) && fs->isDescendant(field->foreignKeyTable()) ) {
				return new _ValueExpression(r.getValue(field));
			}
		}
	}
	return 0;
}

Expression Expression::in( const ExpressionList & expressions )
{
	_InExpression * ret = new _InExpression(p);
	foreach( const Expression & e, expressions ) {
		// Find the link between the tables, and convert to a value expression
		if( e.type() == RecordValue_Expression ) {
			if( !p->type == Field_Expression ) continue;
			ret->append(getRecordLink(*this, ((_RecordValueExpression*)e.p)->record));
		} else
			ret->append(e.p);
	}
	return Expression(ret);
}

Expression Expression::in( const RecordList & records )
{
	_InExpression * ret = new _InExpression(p);
	foreach( const Record & r, records ) {
		ret->append(getRecordLink(*this, r));
	}
	return Expression(ret);
}

Expression Expression::like( const Expression & e )
{
	return p && e.p ? Expression( new _LikeExpression( p, e.p, true ) ) : Expression();
}

Expression Expression::ilike( const Expression & e )
{
	return p && e.p ? Expression( new _LikeExpression( p, e.p, false ) ) : Expression();
}

Expression Expression::regexSearch( const QString & regex, Qt::CaseSensitivity cs )
{
	return p ? Expression( new _RegexExpression( p, regex, cs ) ) : Expression();
}

Expression Expression::orderBy( const Expression & e, Expression::OrderByDirection dir ) const
{
	return Expression( e.p ? new _OrderByExpression( p, e.p, dir ) : 0 );
}

Expression Expression::limit( int limit, int offset ) const
{
	return Expression( new _LimitExpression( p, limit, offset ) );
}

Expression Expression::contextualAnd( const Expression & other )
{
	if( !p )
		return other;
	if( !other.p )
		return *this;
	QVariant::Type t1 = p->valueType(), t2 = other.p->valueType();
	return create( (t1 == QVariant::Bool || t2 == QVariant::Bool) ? And_Expression : BitAnd_Expression, *this, other );
}

Expression Expression::contextualOr( const Expression & other )
{
	if( !p )
		return other;
	if( !other.p )
		return *this;
	QVariant::Type t1 = p->valueType(), t2 = other.p->valueType();
	return create( (t1 == QVariant::Bool || t2 == QVariant::Bool) ? Or_Expression : BitOr_Expression, *this, other );
}

Expression Expression::alias( const QString & alias ) const
{
	if( !isValid() ) return *this;
	if( type() == Alias_Expression ) {
		((_AliasExpression*)p)->alias = alias;
		return *this;
	}
	return Expression( new _AliasExpression(p, alias) );
}

QString Expression::toString(Expression::StringType stringType) const
{
	ToStrContext ctx(stringType);
	return p ? p->toString(ctx) : QString();
}

bool Expression::matches( const Record & record ) const
{
	bool ret = p ? p->value(record).toBool() : false;
	//qDebug() << record.debug() << " matches " << toString() << ": " << ret;
	return ret;
}

QList<Table*> findTables( ExpressionPrivate * p )
{
	QList<Table*> ret;
	if( p->type == Expression::Field_Expression ) {
		Table * t = ((_FieldExpression*)p)->field->table()->table();
		if( !ret.contains(t) )
			ret.append(t);
	}
	foreach( ExpressionPrivate * child, p->children() ) {
		// Don't decent into sub-queries
		if( child->type == Expression::Query_Expression )
			continue;
		ret += findTables(child);
	}
	return ret;
}

RecordList Expression::select() const
{
	if( !p ) return RecordList();
	QList<Table*> tables = findTables(p);
	Table * leafTable = 0;
	foreach( Table * t, tables ) {
		if( !leafTable || leafTable->tableTree().contains(t) ) {
			leafTable = t;
			continue;
		}
		// If we have two tables that don't share a common anscestor
		// then we abort since we don't yet support joins
		if( !t->tableTree().contains(leafTable) ) {
			leafTable = 0;
			break;
		}
	}
	if( !leafTable ) {
		QStringList tableNames;
		foreach( Table * t, tables )
			tableNames.append(t->tableName());
		LOG_1( "Unable to determine which table to select from, possible choices are: " + tableNames.join(",") );
	}
	return leafTable ? leafTable->select( *this ) : RecordList();
}

Expression Expression::createField( Field * field )
{
	return Expression( new _FieldExpression(field) );
}

Expression Expression::createValue( const QVariant & value )
{
	return Expression( new _ValueExpression(value) );
}

Expression Expression::createQuery( const ExpressionList & returns, const ExpressionList & _fromList, const Expression & exp )
{
/*	foreach( Expression e, returns )
		if( e.containsWhereClause() ) {
			qDebug() << "Cannot pass an expression with a where clause as a return field: " << e.toString();
			return Expression();
		} */
	ExpressionList fromList;
	foreach( Expression from, _fromList ) {
		if( from.type() == Alias_Expression )
			from = Expression(((_AliasExpression*)from.p)->child);
		if( from.isValid() && !(from.type() == Query_Expression || from.type() == Table_Expression || from.type() == Values_Expression) ) {
			qDebug() << "The from clause of a query must be another query or a table";
			return Expression();
		}
		if( from.isValid() )
			fromList.append(from);
	}
	if( fromList.isEmpty() ) {
		QList<Table*> tables;
		foreach( Expression e, returns ) {
			foreach( Table * t, findTables(e.p) )
				if( !tables.contains(t) )
					tables.append(t);
		}
		foreach( Table * t, tables )
			fromList.append( Expression(t) );
	}
	return Expression( new _QueryExpression(returns, fromList, exp.p) );
}

Expression Expression::createCombination( Expression::Type type, const ExpressionList & queries )
{
	if( type < Expression::Union_Expression || type > Expression::ExceptAll_Expression ) {
		qDebug() << "Invalid combination type, valid types are Union, UnionAll, Intersect, IntersectAll, Except, and ExceptAll";
		return Expression();
	}
	foreach( Expression e, queries )
		if( !e.isQuery() ) {
			qDebug() << "createCombination only accepts queries";
			return Expression();
		}
	_CombinationExpression * ce = new _CombinationExpression(type);
	foreach( Expression e, queries )
		ce->append(e.p);
	return Expression(ce);
}

Expression Expression::createValues( RecordList records, const QString & alias, FieldList * _fields )
{
	if( records.isEmpty() || alias.isEmpty() ) return Expression();
	FieldList fields;
	if( _fields )
		fields = *_fields;
	else
		fields = records[0].table()->schema()->columns();
	return Expression( new _ValuesExpression( records, alias, fields ) );
}

Expression Expression::create( Expression::Type type, const Expression & child )
{
	switch( type ) {
		case Not_Expression:
		{
			Expression::Type invertTypes [][2] = {
				{ IsNull_Expression, IsNotNull_Expression },
				{ In_Expression, NotIn_Expression },
				{ Equals_Expression, NEquals_Expression },
				{ Invalid_Expression, Invalid_Expression }
			};
			for( int i = 0; invertTypes[i][0] != Invalid_Expression; ++i ) {
				Expression::Type a = invertTypes[i][0], b = invertTypes[i][1];
				if( child.type() == a ) {
					child.p->type = b;
					return child;
				}
				if( child.type() == b ) {
					child.p->type = a;
					return child;
				}
			}
			return Expression(new _NotExpression( child.p ));
        }
		default:
			break;
	};
	return Expression();
}

Expression Expression::create( Expression::Type type, const Expression & child1, const Expression & child2 )
{
	switch( type ) {
		case Expression::And_Expression:
		{
			if( child1.p && child1.p->type == Expression::And_Expression ) {
				((_AndExpression*)(child1.p))->append(child2.p);
				return child1;
			}
			_AndExpression * ret = new _AndExpression();
			ret->append( child1.p );
			ret->append( child2.p );
			return Expression(ret);
		}
		case Expression::Or_Expression:
		{
			if( child1.p && child1.p->type == Expression::Or_Expression ) {
				((_OrExpression*)(child1.p))->append(child2.p);
				return child1;
			}
			_OrExpression * ret = new _OrExpression();
			ret->append( child1.p );
			ret->append( child2.p );
			return Expression(ret);
		}
		case Expression::Equals_Expression:
		case Expression::NEquals_Expression:
		case Expression::Larger_Expression:
		case Expression::LargerOrEquals_Expression:
		case Expression::Less_Expression:
		case Expression::LessOrEquals_Expression:
		case Expression::BitAnd_Expression:
		case Expression::BitOr_Expression:
		case Expression::BitXor_Expression:
		case Expression::Plus_Expression:
		{
			_ComparisonExpression * ret = new _ComparisonExpression( type );
			if( type == Equals_Expression ) {
				if( child1.type() == RecordValue_Expression && child2.type() == Field_Expression ) {
					ret->append(getRecordLink(child2,((_RecordValueExpression*)child1.p)->record));
					ret->append(child2.p);
					return Expression(ret);
				}
				else if( child2.type() == RecordValue_Expression && child1.type() == Field_Expression ) {
					ret->append(child1.p);
					ret->append(getRecordLink(child1,((_RecordValueExpression*)child2.p)->record));
					return Expression(ret);
				}
			}
			ret->append( child1.p );
			ret->append( child2.p );
			return Expression(ret);
		}
		default:
			break;
	};
	return Expression();
}

bool Expression::isOnlySupplemental() const
{
	return p ? p->isOnlySupplemental() : true;
}

bool Expression::containsWhereClause() const
{
	return p ? p->containsWhereClause() : false;
}

bool Expression::isQuery() const
{
	return p ? p->isQuery() : false;
}

bool Expression::isValidVariantType( QVariant::Type type )
{
	switch( type ) {
		case QVariant::String:
		case QVariant::StringList:
		case QVariant::Time:
		case QVariant::Date:
		case QVariant::DateTime:
		case QVariant::Int:
		case QVariant::UInt:
		case QVariant::Double:
		case QVariant::LongLong:
		case QVariant::ULongLong:
		case QVariant::Bool:
		case QVariant::Char:
			return true;
		default:
			if( type == QVariant::Type(qMetaTypeId<Interval>()) )
				return true;
			return false;
	}
	return false;
}

Expression Expression::copy() const
{
	if( p )
		return Expression(p->copy());
	return Expression();
}

Expression Expression::prepareForExec( Connection * conn, bool * reflectsChangeset, bool makeCopy ) const
{
	if( makeCopy ) {
		return copy().prepareForExec(conn,reflectsChangeset,false);
	}
	
	if( p ) {
		PrepStruct ps;
		PrepContext ctx(&ps);
		p->prepareForExec(ctx,conn);
		if( reflectsChangeset )
			*reflectsChangeset = ps.hasSubQuery;
	}
	return *this;
}

// SELECT * FROM A WHERE col=val;
// SELECT * FROM (SELECT tableoid, * FROM ONLY A UNION SELECT tableoid, * FROM ONLY B) as iq WHERE iq.col=val;
Expression Expression::transformToInheritedUnion(TableList tables, QList<RecordReturn> & rr)
{
	Expression query;
	if( p && p->type == Query_Expression )
		query = *this;
	else
		query = Query( ExpressionList(), Expression(), *this );
	((_QueryExpression*)query.p)->transformToInheritedUnion(tables,rr);
	return query;
}

Expression Expression::tableOid()
{
	return Expression(new _TableOidExpression());
}

Expression Expression::null( Field::Type castType )
{
	return Expression(new _NullExpression(castType));
}

Expression Expression::sql( const QString & sql )
{
	return Expression(new _SqlExpression(sql));
}


ExpressionList::ExpressionList(const QList<uint> & l)
{ foreach(uint i, l) *this << Expression(i); }

ExpressionList::ExpressionList(const QList<ExpressionContainer> & ecl)
: QList<ExpressionContainer>(ecl) {}

ExpressionList::ExpressionList(const QList<Expression> & ecl)
: QList<ExpressionContainer>( *(QList<ExpressionContainer>*)(&ecl) ) {}

ExpressionList::ExpressionList(const Expression & e)
{ if( e.isValid() ) append(e); }

ExpressionList::ExpressionList(const FieldList & fl)
{ foreach( Field * f, fl ) append(Expression::createField(f)); }

ExpressionList::ExpressionList(const QStringList & sl)
{ foreach( QString s, sl ) append(Expression::createValue(s)); }

} // namespace Stone