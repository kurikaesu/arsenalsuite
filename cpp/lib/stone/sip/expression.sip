

// Converts a python object into an expression
// Valid values are any Record, Expression instance or
// any value that can be converted to a QVariant whos
// type returns true when passed to Expression::isValidVariantType
%MappedType ExpressionArg
{
%TypeHeaderCode
#include "expression.h"
#include "pyembed.h"
#define ExpressionArg Expression
%End

%ConvertToTypeCode
	
	if (sipCanConvertToType(sipPy, sipType_Record, 0)) {
		// Type Check
		if( sipIsErr == NULL ) return 1;
		int state, err = 0;
		Record * r = (Record*)sipConvertToType( sipPy, sipType_Record, 0, 0, &state, &err );
		if( r && !err ) {
			*sipCppPtr = new Expression(*r);
			sipReleaseType( r, sipType_Record, state );
		}
	}
	else if(sipCanConvertToType( sipPy, sipType_Expression, 0 )) {
		if( sipIsErr == NULL ) return 1;
		int state, err = 0;
		Expression * e = (Expression*)sipConvertToType( sipPy, sipType_Expression, 0, 0, &state, &err );
		if( e && !err ) {
			*sipCppPtr = new Expression(*e);
			sipReleaseType( e, sipType_Expression, state );
		}
	} else if(sipCanConvertToType( sipPy, sipType_StaticFieldExpression, 0 )) {
		if( sipIsErr == NULL ) return 1;
		int state, err = 0;
		StaticFieldExpression * e = (StaticFieldExpression*)sipConvertToType( sipPy, sipType_StaticFieldExpression, 0, 0, &state, &err );
		if( e && !err ) {
			*sipCppPtr = new Expression(*e);
			sipReleaseType( e, sipType_StaticFieldExpression, state );
		}
	} else if(sipCanConvertToType( sipPy, sipType_Table, 0)) {
		if( sipIsErr == NULL ) return 1;
		int state, err = 0;
		Table * t = (Table*)sipConvertToType( sipPy, sipType_Table, 0, 0, &state, &err );
		if( t && !err ) {
			*sipCppPtr = new Expression(t);
			sipReleaseType( t, sipType_Table, state );
		}
	} else if(sipCanConvertToType( sipPy, sipType_QVariant, 0)) {
		// Can we convert to a QVariant?  Sip will let any type be wrapped so we have to limit it here
		int state, err = 0;
		QVariant * v = (QVariant*)sipConvertToType( sipPy, sipType_QVariant, 0, 0, &state, &err );
		if( v && !err ) {
			bool isValidType = Expression::isValidVariantType(QVariant::Type(v->userType()));
			if( sipIsErr )
				*sipCppPtr = new Expression(*v);
			sipReleaseType( v, sipType_QVariant, state );
			if( sipIsErr == NULL )
				return isValidType ? 1 : 0;
		} else if ( sipIsErr == NULL )
			return 0;
	}
	return sipGetState(sipTransferObj);
%End
%ConvertFromTypeCode
	return 0;
%End
};

class Expression
{
%TypeHeaderCode
#include <expression.h>
%End

public:
	Expression();
	Expression( const ExpressionArg & );
	Expression( Table * table, bool only = false );

	~Expression();
	
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

	bool isValid() const;
	Type type() const;

	enum StringType {
		QueryString,
		InfoString
	};
	QString toString(StringType=InfoString) const;

	bool matches( const Record & record ) const;

	Expression operator==( const ExpressionArg & );
	Expression operator!=( const ExpressionArg & );
	Expression operator>( const ExpressionArg & );
	Expression operator<( const ExpressionArg & );
	Expression operator>=( const ExpressionArg & );
	Expression operator<=( const ExpressionArg & );
	Expression operator&( const ExpressionArg & );
	Expression & operator&=( const ExpressionArg & );
	Expression operator|( const ExpressionArg & );
	Expression & operator|=( const ExpressionArg & );
	Expression operator^( const ExpressionArg & );
	Expression operator+( const ExpressionArg & );

	Expression isNull();
	Expression isNotNull();

	Expression in( const QList<ExpressionArg> & expressions ) /PyName=in_/;
	Expression in( const ExpressionArg & expression ) /PyName=in_/;
	Expression in( const RecordList & records ) /PyName=in_/;

	Expression like( const ExpressionArg & e );
	Expression ilike( const ExpressionArg & e );
	Expression regexSearch( const QString & regex, Qt::CaseSensitivity cs = Qt::CaseSensitive );

	Expression contextualAnd( const ExpressionArg & other );
	Expression contextualOr( const ExpressionArg & other );

	enum OrderByDirection {
		Ascending,
		Descending
	};
	
	Expression orderBy( const Expression &, OrderByDirection direction = Descending ) const;
	Expression limit( int limit, int offset=0 ) const;

	MappedRecordList select() const throw(SqlException,LostConnectionException);
	
	bool containsWhereClause() const;
	bool isQuery() const;

	static Expression createQuery( const QList<ExpressionArg> & returns, const QList<ExpressionArg> & from, const ExpressionArg & exp );
	static Expression createCombination( Expression::Type type, const QList<ExpressionArg> & queries );
	static Expression createField( Field * field );
	static Expression createValue( const QVariant & value );
	static Expression create( Expression::Type type, const ExpressionArg & child );
	static Expression create( Expression::Type type, const ExpressionArg & child1, const ExpressionArg & child2 );
	static Expression createValues( RecordList records, const QString & alias, QList<Field*> * fields /In/ = 0 );
	static Expression tableOid();
	static Expression null( Field::Type castType = Field::Invalid );

	static Expression sql( const QString & );

	Expression copy();
	
	Expression prepareForExec( Connection * conn, bool * reflectsChangeset /Out/ = 0, bool makeCopy = true ) const;
};
/*
class ExpressionList : QList<Expression>
{
%TypeHeaderCode
#include <expression.h>
%End
public:
	ExpressionList() {}
	ExpressionList( const Expression & e ) { append(e); }
	ExpressionList & operator<<( const Expression & e ) { append(e); return *this; }
};
*/

Expression Query( QList<ExpressionArg> returns, const QList<ExpressionArg> & from, const ExpressionArg & exp );

Expression Query( QList<ExpressionArg> returns, const ExpressionArg & from, const ExpressionArg & exp );

Expression Query( QList<ExpressionArg> returns, const ExpressionArg & exp );

Expression Query( ExpressionArg returns, const ExpressionArg & exp );

Expression and_() /NoArgParser/;
%MethodCode
	Expression ret;
	SIP_SSIZE_T end = PySequence_Size(sipArgs);
	for( SIP_SSIZE_T i = 0; i < end; i++ ) {
		PyObject * ob = PySequence_GetItem(sipArgs,i);
		bool cc = ob && sipCanConvertToType( ob, sipType_ExpressionArg, 0 );
		Py_XDECREF(ob);
		if( !cc ) {
			PyErr_SetString(PyExc_TypeError, "and_ requires one or more Expression objects as arguments" );
			return 0;
		}
	}
	for( SIP_SSIZE_T i = 0; i < end; i++ ) {
		PyObject * ob = PySequence_GetItem(sipArgs,i);
		int state, err = 0;
		Expression * e = (Expression*)sipConvertToType( ob, sipType_ExpressionArg, 0, 0, &state, &err );
		if( e && !err ) {
			ret &= *e;
			sipReleaseType( e, sipType_Expression, state );
		}
		Py_XDECREF(ob);
	}
	return sipConvertFromNewType(new Expression(ret),sipType_Expression,NULL);
%End

Expression or_() /NoArgParser/;
%MethodCode
	Expression ret;
	SIP_SSIZE_T end = PySequence_Size(sipArgs);
	for( SIP_SSIZE_T i = 0; i < end; i++ ) {
		PyObject * ob = PySequence_GetItem(sipArgs,i);
		bool cc = ob && sipCanConvertToType( ob, sipType_ExpressionArg, 0 );
		Py_XDECREF(ob);
		if( !cc ) {
			PyErr_SetString(PyExc_TypeError, "or_ requires one or more Expression objects as arguments" );
			return 0;
		}
	}
	for( SIP_SSIZE_T i = 0; i < end; i++ ) {
		PyObject * ob = PySequence_GetItem(sipArgs,i);
		int state, err = 0;
		Expression * e = (Expression*)sipConvertToType( ob, sipType_ExpressionArg, 0, 0, &state, &err );
		if( e && !err ) {
			ret |= *e;
			sipReleaseType( e, sipType_Expression, state );
		}
		Py_XDECREF(ob);
	}
	return sipConvertFromNewType(new Expression(ret),sipType_Expression,NULL);
%End

Expression not_( const ExpressionArg & e );
%MethodCode
	sipRes = new Expression(!(*a0));
%End

Expression ValueExpression( const QVariant & v );
Expression ev_( const QVariant & v );
Expression FieldExpression( Field * f );

class StaticFieldExpression
{
%TypeHeaderCode
#include <expression.h>
%End
public:
	StaticFieldExpression( Field * field );

	operator Expression();
	operator Field();
	
	Expression operator==( const ExpressionArg & e );
	Expression operator!=( const ExpressionArg & e );
	Expression operator>( const ExpressionArg & e );
	Expression operator<( const ExpressionArg & e );
	Expression operator>=( const ExpressionArg & e );
	Expression operator<=( const ExpressionArg & e );
	Expression operator&( const ExpressionArg & );
	Expression operator|( const ExpressionArg & );
	Expression operator^( const ExpressionArg & );
	Expression operator+( const ExpressionArg & e );

	Expression isNull();
	Expression isNotNull();
	Expression in( const QList<ExpressionArg> & expressions ) /PyName=in_/;
	Expression in( const ExpressionArg & expression ) /PyName=in_/;
	Expression in( const RecordList & records ) /PyName=in_/;
	Expression like( const ExpressionArg & e );
	Expression ilike( const ExpressionArg & e );
	Expression regexSearch( const QString & regex, Qt::CaseSensitivity cs = Qt::CaseSensitive );
};


