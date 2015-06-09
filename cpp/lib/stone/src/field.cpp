
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

#include <qbuffer.h>
#include <qcolor.h>
#include <qdatetime.h>
#include <qimage.h>
#include <qmetatype.h>
#include <qstringlist.h>

#include "blurqt.h"
#include "field.h"
#include "interval.h"
#include "tableschema.h"
#include "schema.h"

namespace Stone {

struct Field::ForeignKeyPrivate {
	ForeignKeyPrivate( const QString & table )
	: mTable( table )
	, mIndexDeleteMode( 0 )
	, mReferenced( 0 )
	{}
	QString mTable;
	int mIndexDeleteMode;
	TableSchema * mReferenced;
};


static bool isFlagsValid( Field::Flags & f )
{
	// Certain errors can be dealt with without problems
	if( (f & Field::PrimaryKey) && !(f & Field::Unique) ) f = Field::Flags( f | Field::Unique );
	if( !(f & Field::ForeignKey) && (f & Field::ReverseAccess) ) f = Field::Flags( f ^ Field::ReverseAccess );
	// Others cannot
	if( (f & Field::PrimaryKey) && (f & Field::ForeignKey) ) return false;
	if( (f & Field::PrimaryKey) && (f & Field::LocalVariable) ) return false;
	return true;
}

Field::Field( TableSchema * table, const QString & name, Field::Type type, Field::Flags flags, const QString & method )
: mTable( table )
, mName( name )
, mMethodName( method.isEmpty() ? name : method )
, mType( type )
, mFlags( flags )
, mForeignKey( 0 )
, mIndex( 0 )
, mPos( -1 )
{
	isFlagsValid( mFlags );
	if( mTable && !mTable->addField( this ) )
		mTable = 0;
	if( mTable && flag( PrimaryKey ) ) {
		mIndex = new IndexSchema( "key", mTable, false );
		mIndex->setField( this );
		mIndex->addField( this );
	}
}

Field::Field( TableSchema * table, const QString & name, const QString & fkeyTable, Field::Flags flags, bool index, int indexDeleteMode )
: mTable( table )
, mName( name )
, mMethodName( name.mid(4) )
, mType( Field::UInt )
, mFlags( flags )
, mForeignKey( new ForeignKeyPrivate( fkeyTable )  )
, mIndex( 0 )
, mPos( -1 )
{
	isFlagsValid( mFlags );
	if( mTable && !mTable->addField( this ) )
		mTable = 0;
	if( index && mTable )
		setHasIndex( true, indexDeleteMode );
}

Field::~Field()
{
	setHasIndex( false );
	if( mForeignKey ) {
		delete mForeignKey;
		mForeignKey = 0;
	}
	if( mTable )
		mTable->removeField( this );
}

TableSchema * Field::table() const
{
	return mTable;
}

QString Field::name() const
{
	return mName;
}

void Field::setName( const QString & name )
{
	if( mMethodName.isEmpty() || mMethodName == mName || (flag( ForeignKey ) && mName.mid(4) == mMethodName) ) {
		if( flag( ForeignKey ) && mName.left(4)=="fkey" )
			mMethodName = name.mid(4);
		else
			mMethodName = name;
	}
	mName = name;
}

QString Field::placeholder(int i) const
{
	return ":" + mName.toLower() + (i == 0 ? QString() : QString::number(i));
}

QString pluralizeName( const QString & name )
{
	if( name.right(1) == "s" )
		return name + "es";
	if( name.right(1) == "y" )
		return name.left(name.length()-1) + "ies";
	if( name.right(2) == "ed" )
		return name;
	return name + "s";
}

QString Field::generatedPluralMethodName() const
{
	return pluralizeName( methodName() );
}

QString Field::pluralMethodName() const
{
	if( mPluralMethodName.isEmpty() )
		return generatedPluralMethodName();
	return mPluralMethodName;
}

void Field::setPluralMethodName( const QString & pmn )
{
	if( pmn == generatedPluralMethodName() )
		mPluralMethodName = QString();
	else
		mPluralMethodName = pmn;
}

Field::Type Field::type() const
{
	return mType;
}

void Field::setType( Type t )
{
	mType = t;
}

Field::Flags Field::flags() const
{
	return mFlags;
}

bool Field::setFlags( Flags f )
{
	if( !isFlagsValid( f ) ) return false;
	mFlags = f;
	return true;
}

bool Field::flag( int f ) const
{
	return (mFlags & f);
}

void Field::setFlag( int f, bool v )
{
	if( v )
		mFlags = Flags(mFlags|f);
	else
		mFlags = Flags(mFlags&(f^0xFFFF));
}

struct FlagMap { Field::Flags f; const char * s; };

static FlagMap sFlagMap [] = 
{
	{ Field::PrimaryKey, "PrimaryKey" },
	{ Field::ForeignKey, "ForeignKey" },
	{ Field::NotNull, "NotNull" },
	{ Field::Unique, "Unique" },
	{ Field::LocalVariable, "LocalVariable" },
	{ Field::ReverseAccess, "ReverseAccess" },
	{ Field::TableDisplayName, "TableDisplayName" },
	{ Field::NoDefaultSelect, "NoDefaultSelect" },
	{ Field::None, 0 }
};

static const char * stringFromFlag( int f )
{
	for( FlagMap * first = sFlagMap; first->f; ++first ) {
		if( first->f == f )
			return first->s;
	}
	return 0;
}

QString Field::flagString() const
{
	QStringList ret;
	for( FlagMap * first = sFlagMap; first->f; ++first ) {
		if( first->f & mFlags )
			ret += "Field::" + QString(first->s);
	}
	return "Field::Flags( " + QString( ret.isEmpty() ? "Field::None" : ret.join( " | " ) ) + ")";
}

QString Field::methodName() const
{
    return mMethodName;
}

void Field::setMethodName( const QString & mn )
{
	if( !mn.isEmpty() )
		mMethodName = mn;
}

QString fieldToDisplayName( const QString & name )
{
	QString ret = name;
	if( !ret.isEmpty() ) {
		ret.replace( QRegExp( "([A-Z])" ), " \\1" );
		ret.replace( "_", " " );
		ret[0] = ret[0].toUpper();
	}
	return ret.simplified();
}

QString Field::generatedDisplayName() const
{
	return fieldToDisplayName( methodName() );
}

QString Field::displayName() const
{
	if( mDisplayName.isEmpty() )
		mDisplayName = generatedDisplayName();
	return mDisplayName;
}

void Field::setDisplayName( const QString & displayName )
{
	mDisplayName = displayName;
}

QString Field::docs() const
{
	return mDocs;
}

void Field::setDocs( const QString & docs )
{
	mDocs = docs;
}

int Field::pos() const
{
	return mPos;
}

void Field::setPos( int pos )
{
	mPos = pos;
}

QVariant Field::defaultValue() const
{
	return mDefault;
}

void Field::setDefaultValue( const QVariant & var )
{
	mDefault = var;
}

// Returns a string that can be inserted into source code ( eg. "abc", 0 )
QString Field::defaultValueString() const
{
	if( mType == String ) {
		if( !mDefault.toString().isEmpty() )
			return "\"" + mDefault.toString() + "\"";
		return "QString()";
	} else if( mType == Date )
		return QString("QDate::fromString(\"%1\")").arg(mDefault.toString());
	else if( mType == DateTime )
		return QString("QDateTime::fromString(\"%1\")").arg(mDefault.toString());
	else if( mType == Time )
		return QString("QTime::fromString(\"%1\")").arg(mDefault.toString());
	else if( mDefault.isNull() && (mType == UInt || mType == Int || mType == ULongLong) )
		return "0";
	else if( mDefault.isNull() && (mType == Float || mType == Double ) )
		return "0.0";
	else if( mType == Bool )
		return mDefault.toBool() ? "true" : "false";
	else if( mType == ByteArray )
		return "QByteArray()";
	return mDefault.toString();
}

//
// Foreign Key Stuff
//
TableSchema * Field::foreignKeyTable() const
{
	if( !mForeignKey ) return 0;
	if( !mForeignKey->mReferenced )
		mForeignKey->mReferenced = mTable->schema()->tableByClass( mForeignKey->mTable );
	return mForeignKey->mReferenced;
}

QString Field::foreignKey() const
{
	return mForeignKey ? mForeignKey->mTable : "";
}

void Field::setForeignKey( const QString & tableName )
{
	if( tableName.isEmpty() && mForeignKey )
	{
		delete mForeignKey;
		mForeignKey = 0;
	} else if( !tableName.isEmpty() && !mForeignKey )
		mForeignKey = new ForeignKeyPrivate( tableName );
	else if( mForeignKey ) {
		mForeignKey->mTable = tableName;
		mForeignKey->mReferenced = 0;
	}
}

void Field::setHasIndex( bool hi, int indexDelMode )
{
	if( hi && mForeignKey && mForeignKey->mIndexDeleteMode != indexDelMode ) {
		mForeignKey->mIndexDeleteMode = indexDelMode;
	}
		
	if( ( mIndex != 0 ) == hi ) return;
		
	if( hi ) {
		mIndex = new IndexSchema( mName.left(1).toUpper() + mName.mid( 1 ), mTable, !flag(Unique) );
		mIndex->addField( this );
		mIndex->setField( this );
		if( mForeignKey ) {
			mForeignKey->mIndexDeleteMode = indexDelMode;
		}
    } else {
		mTable->removeIndex( mIndex );
		mIndex = 0;
	}
}

bool Field::hasIndex() const
{
    return mIndex != 0;
}

IndexSchema * Field::index()
{
    return mIndex;
}

void Field::removeIndex( IndexSchema * index )
{
	if( mIndex == index )
		mIndex = 0;
}

int Field::indexDeleteMode() const
{
    return mForeignKey ? mForeignKey->mIndexDeleteMode : DoNothingOnDelete;
}

static const char * indexDeleteModes [] =
{
	"DoNothingOnDelete",
	"UpdateOnDelete",
	"CascadeOnDelete",
	0
};

QString Field::indexDeleteModeString() const
{
	return indexDeleteModes[indexDeleteMode()];
}

int Field::indexDeleteModeFromString( const QString & s )
{
	QString sl = s.toLower();
	for( int i=0; indexDeleteModes[i]; i++ )
		if( QString(indexDeleteModes[i]).toLower() == sl )
			return i;
	
	// Default value
	return DoNothingOnDelete;
}

const char * Field::variantTypeStrings[] =
{
	"Invalid",
	"String",
	"UInt",
	"ULongLong",
	"Int",
	"Date",
	"DateTime",
	"Interval",
	"Double",
	"Double", // Since we only really support postgres, where float and double are always double
	"Bool",
	"ByteArray",
	"Color",
	"Image",
	"Time",
    0
};

const char * Field::listTypeStrings[] =
{
	"Invalid",
	"QStringList",
	"QList<uint>",
	"QList<qulonglong>",
	"QList<int>",
	"QList<QDate>",
	"QList<QDateTime>",
	"QList<Interval>",
	"QList<double>",
	"QList<double>", // Since we only really support postgres, where float and double are always double
	"QList<bool>",
	"QList<QByteArray>",
	"QList<QColor>",
	"QList<QImage>",
	"QList<QTime>",
	0
};

const char * Field::typeStrings[] =
{
	"Invalid",
	"QString",
	"uint",
	"qulonglong",
	"int",
	"QDate",
	"QDateTime",
	"Interval",
	"double",
	"double",  // Since we only really support postgres, where float and double are always double
	"bool",
	"QByteArray",
	"QColor",
	"QImage",
	"QTime",
	0
};

const char * Field::dbTypeStrings[] =
{
	"Invalid",
	"text",
	"int",
	"int8",
	"int",
	"date",
	"timestamp",
	"interval",
	"float8",
	"float",
	"boolean",
	"bytea",
	"color",
	"bytea",
	"time",
	0
};

// Returns the enum for the type 
// or -1 if the type is not found
Field::Type Field::stringToType( const QString & ts )
{
	for( int i=(int)Invalid; variantTypeStrings[i]; ++i )
		if( variantTypeStrings[i] == ts )
			return (Field::Type)i;
	for( int i=(int)Invalid; typeStrings[i]; ++i )
		if( typeStrings[i] == ts )
			return (Field::Type)i;
	if( ts.toLower() == "float" )
		return Field::Float;
	return Invalid;
}

QVariant Field::variantFromString( const QString & str, Type t )
{
	QVariant ret;
	switch( t ) {
		case Invalid:
			return ret;
		case String:
			return QVariant( str );
		case UInt:
			return QVariant( str.toUInt() );
		case ULongLong:
			return QVariant( str.toULongLong() );
		case Int:
			return QVariant( str.toInt() );
		case Date:
			return QVariant( QDate::fromString( str, Qt::ISODate ) );
		case DateTime:
			return QVariant( QDateTime::fromString( str, Qt::ISODate ) );
		case Time:
			return QVariant( QTime::fromString( str ) );
		case Interval:
			if( str.isNull() )
				return QVariant( qMetaTypeId< ::Interval>() );
			return qVariantFromValue( Interval::fromString( str ) );
		case Double:
			return QVariant( str.toDouble() );
		case Float:
			return QVariant( str.toFloat() );
		case Bool:
			return QVariant( str.toLower() == "true" || str == "1" || str.toLower() == "t" );
		case ByteArray:
			return ret;
		case Color:
		{
			if( str.size() ) {
				QString s = str;
				if( s[0] == '(' && s[s.size()-1] == ')' )
					s = s.mid(1,s.size()-2);
				QStringList parts = s.split(',');
				if( parts.size() >= 3 && parts.size() <= 4 )
					return QVariant( QColor(parts[0].toInt(), parts[1].toInt(), parts[2].toInt(), parts.size() == 4 ? parts[3].toInt() : 0 ) );
			}
			return QVariant(QVariant::Color);
		}
		default:
			return ret;
	};
}

QVariant Field::coerce( const QVariant & v )
{
	if( flag( Field::ForeignKey ) && (v.type() == QVariant::Int || v.type() == QVariant::UInt) && v.toInt() == 0 )
		return QVariant(QVariant::Int); // Null int
	// Convert string to interval or color object
	if( v.type() == QVariant::String && (type() == Field::Interval || type() == Field::Color) )
		return Field::variantFromString( v.toString(), type() );
	if( v.type() == QVariant::ByteArray && type() == Field::Image ) {
		QByteArray ba = v.toByteArray();
		if( !ba.isEmpty() ) {
			QImage img;
			QBuffer buffer(&ba);
			buffer.open(QIODevice::ReadOnly);
			img.load(&buffer,"PNG");
			return QVariant(img);
		}
	}
	return v;
}

QVariant Field::dbPrepare( const QVariant & v )
{
	if( v.userType() == qMetaTypeId<Record>() ) {
		uint key = qvariant_cast<Record>(v).key(false);
		return (key == 0 && flag(Field::ForeignKey)) ? QVariant(QVariant::Int) : QVariant(key);
	}
	if( flag( Field::ForeignKey ) && v.toInt() == 0 )
		return QVariant(QVariant::Int); // Construct a null variant
	if( type() == Field::Bool && (v.type() == QVariant::Int || v.type() == QVariant::UInt) )
		return QVariant( v.toInt() != 0 );
	if( (type() == Field::Int || type() == Field::UInt ) && v.type() == QVariant::Bool )
		return QVariant(v.toBool() ? 1 : 0);
	if( type() == Field::Color && v.type() == QVariant::Color && !v.isNull() ) {
		QColor c = v.value<QColor>();
		return QVariant( QString("(%1,%2,%3,%4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha()) );
	}
	if( type() == Field::Interval && v.userType() == qMetaTypeId< ::Interval>() ) {
		return qvariant_cast< ::Interval>(v).toString();
	}
	if( type() == Field::Image && v.type() == QVariant::Image ) {
		QByteArray ba;
		QBuffer buffer(&ba);
		buffer.open(QIODevice::WriteOnly);
		v.value<QImage>().save(&buffer, "PNG");
		return QVariant(ba);
	}
	return v;
}

QString Field::typeFromVariantCode() const
{
	int type = mType;
	if( type == Interval ) return "qvariant_cast<Interval>(%1)";
	if( type == Float ) type = Double;
	if( type == Image || type == Color ) return "%1.value<" + typeString() + ">()";
	return QString("%1.to") + variantTypeStrings[type] + "()";
}

QString Field::variantFromTypeCode() const
{
	if( mType == Interval )
		return "qVariantFromValue(%1)";
	return "QVariant(%1)";
}

// Returns a string of the c++ type
QString Field::typeString() const
{
    return typeStrings[mType];
}

// Returns a string of the variant type
QString Field::variantTypeString() const
{
    return variantTypeStrings[mType];
}
// Returns a string of the list type( eg. QStringList, QValueList<float> ).
QString Field::listTypeString() const
{
    return listTypeStrings[mType];
}

// Returns a string of the list type( eg. QStringList, QValueList<float> ).
QString Field::dbTypeString() const
{
    return dbTypeStrings[mType];
}

QString Field::dbTypeString(Field::Type type)
{
    return dbTypeStrings[type];
}

int Field::qvariantType() const
{
	return qvariantType(mType);
}

int Field::qvariantType(Field::Type type)
{
	static int types [] = {
 		(int)QVariant::Invalid, // Invalid=0,
		(int)QVariant::String, // String,
		(int)QVariant::UInt, // UInt,
		(int)QVariant::ULongLong, // ULongLong,
		(int)QVariant::Int, //Int,
		(int)QVariant::Date, //Date,
		(int)QVariant::DateTime, //DateTime,
		-1, //Interval,
		(int)QVariant::Double, //Double,
		(int)QVariant::Double, // Float,
		(int)QVariant::Bool, //Bool,
		(int)QVariant::ByteArray, //ByteArray,
		(int)QVariant::Color, //Color,
		(int)QVariant::Image, //Image
		(int)QVariant::Time, //Time
	};

	int ret = types[type];
	if( ret == -1 ) {
		if( type == Field::Interval )
			ret = qMetaTypeId< ::Interval>();
	}
	return ret;
}

QString Field::diff( Field * after )
{
	QStringList ret;
	if( type() != after->type() )
		ret += "Type Changed from " + typeString() + " to " + after->typeString();
	for( int i=0; i<Field::LastFlag; i++ ) {
		Flags f((Field::Flags)i);
		if( flag(f) != after->flag(f) )
			ret += QString(flag(f) ? "-" : "+") + " " + stringFromFlag(i);
	}
	if( pluralMethodName() != after->pluralMethodName() )
		ret += "Plural Method Name changed from " + pluralMethodName() + " to " + after->pluralMethodName();
	if( methodName() != after->methodName() )
		ret += "Method Name changed from " + methodName() + " to " + after->methodName();
	if( displayName() != after->displayName() )
		ret += "Display Name changed from " + displayName() + " to " + after->displayName();
	if( defaultValue() != after->defaultValue() )
		ret += "Default Value changed from " + defaultValueString() + " to " + after->defaultValueString();
	if( (bool(foreignKeyTable()) != bool(after->foreignKeyTable())) || (foreignKeyTable() && foreignKeyTable()->tableName().toLower() != after->foreignKeyTable()->tableName().toLower()) )
		ret += "Foreign Key Table changed from " + (foreignKeyTable() ? foreignKeyTable()->tableName() : "") + " to " + (after->foreignKeyTable() ? after->foreignKeyTable()->tableName() : "");
	return ret.isEmpty() ? QString() : ("\t" + table()->tableName() + "." + name() + " Changes:\n\t\t" + ret.join("\n\t\t"));
}

// Optmized to not copy(because of implicit sharing) if one already contains all of two
FieldList operator|(const FieldList & one, const FieldList & two)
{
	bool firstComplete = two.size() <= one.size();
	FieldList ret;
	foreach( Field * f, two )
		if( !one.contains(f) ) {
			if( firstComplete ) {
				firstComplete = false;
				ret = one;
			}
			ret += f;
		}
	return firstComplete ? one : ret;
}

FieldList operator&(const FieldList & one, const FieldList & two)
{
	if( one.size() == two.size() ) {
		bool same = true;
		foreach( Field * f, one )
			if( !two.contains(f) ) {
				same = false;
				break;
			}
		if( same ) return one;
	}
	FieldList ret;
	foreach( Field * f, one )
		if( two.contains(f) )
			ret += f;
	return ret;
}

} //namespace
