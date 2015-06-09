
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

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <qlist.h>

#include "blurqt.h"
#include "field.h"

// Fuck Visual Studio!
//#include <initializer_list>

namespace Stone {
// Proposed usage:
//.select( Delivery.Project == 1 && Delivery.DateStart >= dateStart && Delivery.DateStart <= dateEnd )

class Connection;
class Record;
class RecordList;
class Table;
struct RecordReturn;
typedef QList<Table*> TableList;

class ExpressionList;
class ExpressionPrivate;
class TableExpression;

class STONE_EXPORT Expression
{
public:
	Expression();
	Expression( const Expression & );
	Expression( const QVariant & );
	Expression( const Record & );
	Expression( Table * table, bool only = false );
	
	~Expression();
	Expression & operator=( const Expression & );
	
	enum Type {
		Invalid_Expression,
		Field_Expression,
		Value_Expression,
		RecordValue_Expression,
		And_Expression,
		Or_Expression,
		Equals_Expression,
		NEquals_Expression,
		Not_Expression,
		Larger_Expression,
		LargerOrEquals_Expression,
		Less_Expression,
		LessOrEquals_Expression,
		BitAnd_Expression,
		BitOr_Expression,
		BitXor_Expression,
		Plus_Expression,
		IsNull_Expression,
		IsNotNull_Expression,
		In_Expression,
		NotIn_Expression,
		Like_Expression,
		ILike_Expression,
		OrderBy_Expression,
		RegEx_Expression,
		Limit_Expression,
		Query_Expression,
		Table_Expression,
		// _CombinationExpression depends on these being in this order(starting index doesn't matter)
		Union_Expression,
		UnionAll_Expression,
		Intersect_Expression,
		IntersectAll_Expression,
		Except_Expression,
		ExceptAll_Expression,
		
		Alias_Expression,
		Values_Expression,
		Null_Expression,
		TableOid_Expression,
		Sql_Expression
	};

	bool isValid() const { return bool(p); }
	
	Type type() const;
	
	enum StringType {
		QueryString,
		InfoString
	};
	
	QString toString(StringType=InfoString) const;
	bool matches( const Record & record ) const;

	// Logical, sql AND
	Expression operator&&( const Expression & );
	Expression operator&&( const QVariant & v );
	// Logical, sql OR
	Expression operator||( const Expression & );
	Expression operator||( const QVariant & v );
	// Equals, sql =
	Expression operator==( const Expression & ) const;
	Expression operator==( const QVariant & v );
	
	// Not Equal, sql !=, <>
	Expression operator!=( const Expression & );
	Expression operator!=( const QVariant & v );
    Expression operator !() const;

	// Larger, sql >
	Expression operator>( const Expression & );
	Expression operator>( const QVariant & v );
	// Lesser, sql <
	Expression operator<( const Expression & );
	Expression operator<( const QVariant & v );
	// Larger or Equal, sql >=
	Expression operator>=( const Expression & );
	Expression operator>=( const QVariant & v );
	// Lesser or Equal, sql <=
	Expression operator<=( const Expression & );
	Expression operator<=( const QVariant & v );
	// Binary and, sql &
	// Dual purpose in python, becomes logical AND
	// if either of the arguments are boolean
	Expression operator&( const Expression & );
	Expression operator&( const QVariant & );
	Expression & operator&=( const Expression & );
	
	// Binary or, sql |
	// Dual purpose in python, becomes logical OR
	// if either of the arguments are boolean
	Expression operator|( const Expression & );
	Expression operator|( const QVariant & );
	Expression & operator|=( const Expression & );

	// Current XOR, should this be changed to pow
	// to match sql?
	Expression operator^( const Expression & );
	Expression operator^( const QVariant & );
	// 
	Expression operator+( const Expression & );
	Expression operator+( const QVariant & );
	

	Expression isNull();
	Expression isNotNull();
	
// Fuck Visual Studio!
//	Expression in( std::initializer_list<Expression> list );
	Expression in( const ExpressionList & expressions );
	Expression in( const RecordList & records );
	
	// Config.Config.like( 'Assburner%' )
	Expression like( const Expression & e );
	Expression ilike( const Expression & e );
	Expression regexSearch( const QString & regex, Qt::CaseSensitivity cs = Qt::CaseSensitive );
	//Expression regex();
	
	// These are mainly used to support contextual operator | and &
	// in python because operator && and || cannot be overloaded
	// They determine whether to do a binary op or logical comparison
	// depending on the output types of the involved expressions
	Expression contextualAnd( const Expression & other );
	Expression contextualOr( const Expression & other );
	
	enum OrderByDirection {
		Ascending,
		Descending
	};
	
	Expression orderBy( const Expression &, OrderByDirection direction = Descending ) const;
	Expression limit( int limit, int offset=0 ) const;
	
	RecordList select() const;
	
	Expression alias( const QString & ) const;

	// Returns true if the expression only contains orderby and/or limit
	bool isOnlySupplemental() const;
	bool containsWhereClause() const;
	bool isQuery() const;
	
	// Returns returnField from table(which could be a child of returnFields table)
	// If table is 0 returnField's table() is used.
	// If returns is empty a star(*) is used
	static Expression createQuery( const ExpressionList & returns, const ExpressionList & from, const Expression & exp );
	// Each expression in queries must be of type Query_Expression, Union_Expression, Intersects_Expression
	static Expression createCombination( Expression::Type type, const ExpressionList & queries );
	static Expression createField( Field * field );
	static Expression createValue( const QVariant & value );
	static Expression create( Expression::Type type, const Expression & child );
	static Expression create( Expression::Type type, const Expression & child1, const Expression & child2 );
	static Expression createValues( RecordList records, const QString & alias, FieldList * fields = 0 );
	static Expression tableOid();
	static Expression null( Field::Type castType = Field::Invalid );
	
	// This is unable to reflect changeset changes, use with caution
	static Expression sql( const QString & );
		
	static bool isValidVariantType( QVariant::Type type );
	
	// Creates a deep copy of this expression tree
	Expression copy() const;
	
	Expression prepareForExec( Connection * conn, bool * reflectsChangeset = 0, bool makeCopy = true ) const;
	Expression transformToInheritedUnion(TableList tables, QList<RecordReturn> & rr);

protected:
	explicit Expression( ExpressionPrivate * );
public:
	ExpressionPrivate * p;
	friend class ExpressionPrivate;
	friend class TableAlias;
	friend class ExpressionContainer;
};

class STONE_EXPORT TableAlias
{
public:
	TableAlias( Table * table, const QString & alias );
	
	Expression operator()(const Expression & expression);
	operator Expression();
protected:
	Table * mTable;
	QString mAlias;
};

class STONE_EXPORT ExpressionContainer : public Expression
{
public:
	ExpressionContainer(const Expression & e = Expression()) : Expression(e) {}
	bool operator==(const Expression & e) const { return e.p == p; }
	// Cause MS visual studio sucks ass once again.  Soon I will get mingw-w64 up and ditch this microshit garbage for good
	uint hash() const { return qHash(p); }
};

inline uint qHash( const ExpressionContainer & ec ) { return ec.hash(); }

class STONE_EXPORT ExpressionList : public QList<ExpressionContainer>
{
public:
	ExpressionList() {}
	ExpressionList(const QList<uint> & l);
	ExpressionList(const QList<ExpressionContainer> & ecl);
	ExpressionList(const QList<Expression> & ecl);
	ExpressionList(const Expression & e);
	ExpressionList(const FieldList & fl);
	ExpressionList(const QStringList & sl);
	Expression operator[](int i) const { return Expression(QList<ExpressionContainer>::operator[](i)); }
	ExpressionList & operator<<( const Expression & e ) { append(e); return *this; }
};

inline Expression Query( ExpressionList returns, const ExpressionList & from, const Expression & exp )
{ return Expression::createQuery(returns,from,exp); }

inline Expression Query( ExpressionList returns, const Expression & from, const Expression & exp )
{ return Expression::createQuery(returns,from,exp); }

inline Expression Query( ExpressionList returns, const Expression & exp )
{ return exp.type() == Expression::Table_Expression ? Expression::createQuery(returns,exp,Expression()) : Expression::createQuery(returns,Expression(),exp); }

inline Expression AndExpression( const Expression & e1, const Expression & e2 )
{ return e1.isValid() ? Expression::create( Expression::And_Expression, e1, e2 ) : e2; }

inline Expression OrExpression( const Expression & e1, const Expression & e2 )
{ return e1.isValid() ? Expression::create( Expression::Or_Expression, e1, e2 ) : e2; }

inline Expression EqualsExpression( const Expression & e1, const Expression & e2 )
{ return Expression::create( Expression::Equals_Expression, e1, e2 ); }

inline Expression NEqualsExpression( const Expression & e1, const Expression & e2 )
{ return Expression::create( Expression::NEquals_Expression, e1, e2 ); }

inline Expression NotExpression( const Expression & e1)
{ return Expression::create( Expression::Not_Expression, e1 ); }

inline Expression LargerExpression( const Expression & e1, const Expression & e2 )
{ return Expression::create( Expression::Larger_Expression, e1, e2 ); }

inline Expression LargerOrEqualsExpression( const Expression & e1, const Expression & e2 )
{ return Expression::create( Expression::LargerOrEquals_Expression, e1, e2 ); }

inline Expression LessExpression( const Expression & e1, const Expression & e2 )
{ return Expression::create( Expression::Less_Expression, e1, e2 ); }

inline Expression LessOrEqualsExpression( const Expression & e1, const Expression & e2 )
{ return Expression::create( Expression::LessOrEquals_Expression, e1, e2 ); }

inline Expression BitAndExpression( const Expression & e1, const Expression & e2 )
{ return Expression::create( Expression::BitAnd_Expression, e1, e2 ); }

inline Expression BitOrExpression( const Expression & e1, const Expression & e2 )
{ return Expression::create( Expression::BitOr_Expression, e1, e2 ); }

inline Expression BitXorExpression( const Expression & e1, const Expression & e2 )
{ return Expression::create( Expression::BitXor_Expression, e1, e2 ); }

inline Expression PlusExpression( const Expression & e1, const Expression & e2 )
{ return Expression::create( Expression::Plus_Expression, e1, e2 ); }

inline Expression OrderByExpression( const Expression & e1, const Expression & e2, Expression::OrderByDirection direction = Expression::Descending )
{ return e1.orderBy( e2, direction ); }

inline Expression LimitExpression( const Expression & e1, int limit )
{ return e1.limit( limit ); }

inline Expression ValueExpression( const QVariant & v )
{ return Expression::createValue( v ); }

inline Expression ev_( const QVariant & v )
{ return Expression::createValue( v ); }

template<typename T> Expression _evt( const T & t )
{ return Expression::createValue( QVariant::fromValue<T>(t) ); }

inline Expression FieldExpression( Field * f )
{ return Expression::createField( f ); }

inline Expression Expression::operator&&( const Expression & e ) { return AndExpression( *this, e ); }
inline Expression Expression::operator&&( const QVariant & v ) { return AndExpression( *this, ev_(v) ); }
inline Expression Expression::operator||( const Expression & e ) { return OrExpression( *this, e ); }
inline Expression Expression::operator||( const QVariant & v ) { return OrExpression( *this, ev_(v) ); }
inline Expression Expression::operator==( const Expression & e ) const { return EqualsExpression( *this, e ); }
inline Expression Expression::operator==( const QVariant & v ) { return EqualsExpression( *this, ev_(v) ); }
inline Expression Expression::operator!=( const Expression & e ) { return NEqualsExpression( *this, e ); }
inline Expression Expression::operator!=( const QVariant & v ) { return NEqualsExpression( *this, ev_(v) ); }
inline Expression Expression::operator!() const { return NotExpression( *this ); }
inline Expression Expression::operator>( const Expression & e ) { return LargerExpression( *this, e ); }
inline Expression Expression::operator>( const QVariant & v ) { return LargerExpression( *this, ev_(v) ); }
inline Expression Expression::operator<( const Expression & e ) { return LessExpression( *this, e ); }
inline Expression Expression::operator<( const QVariant & v ) { return LessExpression( *this, ev_(v) ); }
inline Expression Expression::operator>=( const Expression & e ) { return LargerOrEqualsExpression( *this, e ); }
inline Expression Expression::operator>=( const QVariant & v ) { return LargerOrEqualsExpression( *this, ev_(v) ); }
inline Expression Expression::operator<=( const Expression & e ) { return LessOrEqualsExpression( *this, e ); }
inline Expression Expression::operator<=( const QVariant & v ) { return LessOrEqualsExpression( *this, ev_(v) ); }
inline Expression Expression::operator&( const Expression & e ) { return contextualAnd( e ); }
inline Expression Expression::operator&( const QVariant & v ) { return contextualAnd( ev_(v) ); }
inline Expression & Expression::operator&=( const Expression & e ) { *this = contextualAnd(e); return *this; }
inline Expression Expression::operator|( const Expression & e ) { return contextualOr( e ); }
inline Expression Expression::operator|( const QVariant & v ) { return contextualOr( ev_(v) ); }
inline Expression & Expression::operator|=( const Expression & e ) { *this = contextualOr(e); return *this; }
inline Expression Expression::operator^( const Expression & e ) { return BitXorExpression( *this, e ); }
inline Expression Expression::operator^( const QVariant & v ) { return BitXorExpression( *this, ev_(v) ); }
inline Expression Expression::operator+( const Expression & e ) { return PlusExpression( *this, e ); }
inline Expression Expression::operator+( const QVariant & v ) { return PlusExpression( *this, ev_(v) ); }

class STONE_EXPORT StaticFieldExpression
{
public:
	StaticFieldExpression( Field * field ) : mField( field ) {}

	inline operator ExpressionList() const { return ExpressionList(Expression(*this)); }

	inline Field * operator->() { return mField; }

	inline operator Expression() const { return FieldExpression(mField); }
	inline operator Field*() { return mField; }
	inline operator Field() { return *mField; }
	inline Expression operator==( const Expression & e ) { return Expression(*this) == e; }
	inline Expression operator==( const QVariant & v ) { return Expression(*this) == v; }
	inline Expression operator!=( const Expression & e ) { return Expression(*this) != e; }
	inline Expression operator!=( const QVariant & v ) { return Expression(*this) != v; }
	inline Expression operator!() const { return !Expression(*this); }
	inline Expression operator&&( const Expression & e ) { return Expression(*this) && e; }
	inline Expression operator&&( const QVariant & v ) { return Expression(*this) && v; }
	inline Expression operator||( const Expression & e ) { return Expression(*this) || e; }
	inline Expression operator||( const QVariant & v ) { return Expression(*this) || v; }
	inline Expression operator>( const Expression & e ) { return Expression(*this) > e; }
	inline Expression operator>( const QVariant & v ) { return Expression(*this) > v; }
	inline Expression operator<( const Expression & e ) { return Expression(*this) < e; }
	inline Expression operator<( const QVariant & v ) { return Expression(*this) < v; }
	inline Expression operator>=( const Expression & e ) { return Expression(*this) >= e; }
	inline Expression operator>=( const QVariant & v ) { return Expression(*this) >= v; }
	inline Expression operator<=( const Expression & e ) { return Expression(*this) <= e; }
	inline Expression operator<=( const QVariant & v ) { return Expression(*this) <= v; }
	inline Expression operator&( const Expression & e ) { return Expression(*this) & e; }
	inline Expression operator&( const QVariant & v ) { return Expression(*this) & v; }
	inline Expression operator|( const Expression & e ) { return Expression(*this) | e; }
	inline Expression operator|( const QVariant & v ) { return Expression(*this) | v; }
	inline Expression operator^( const Expression & e ) { return Expression(*this) ^ e; }
	inline Expression operator^( const QVariant & v ) { return Expression(*this) ^ v; }
	inline Expression operator+( const Expression & e ) { return Expression(*this) + e; }
	inline Expression operator+( const QVariant & v ) { return Expression(*this) + v; }
	
	//inline Expression in( std::initializer_list<Expression> il ) { return Expression(*this).in(il); }
	inline Expression isNull() { return Expression(*this).isNull(); }
	inline Expression isNotNull() { return Expression(*this).isNotNull(); }
	inline Expression in( const ExpressionList & expressions ) { return Expression(*this).in(expressions); }
	inline Expression in( const Expression & expression ) { return Expression(*this).in(expression); }
	inline Expression in( const RecordList & records ) { return Expression(*this).in(records); }
	inline Expression like( const Expression & e ) { return Expression(*this).like(e); }
	inline Expression ilike( const Expression & e ) { return Expression(*this).ilike(e); }
	inline Expression regexSearch( const QString & regex, Qt::CaseSensitivity cs = Qt::CaseSensitive ) { return Expression(*this).regexSearch(regex,cs); }

private:
	Field * mField;
};

}

using Stone::Expression;

#endif // EXPRESSION_H
