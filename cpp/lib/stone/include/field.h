
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

#ifndef FIELD_H
#define FIELD_H

#include <qstring.h>
#include <qvariant.h>

#include "blurqt.h"


namespace Stone {

class TableSchema;
class IndexSchema;

STONE_EXPORT QString fieldToDisplayName( const QString & name );
STONE_EXPORT QString pluralizeName( const QString & name );

/**
 * \ingroup Stone
 */
class STONE_EXPORT Field
{
public:
	/**
	 *  This enum describes the type of data this field holds
	 */
	enum Type {
		Invalid=0,
		String,
		UInt,
		ULongLong,
		Int,
		Date,
		DateTime,
		Interval,
		Double,
		Float,
		Bool,
		ByteArray,
		Color,
		Image,
		Time
	};

	/**
	 *  Or'able flags describing properties of this field.
	 */
	enum Flags {
		None = 0,
		PrimaryKey = 1,
		ForeignKey = 2,
		NotNull = 4,
		Unique = 8,
		LocalVariable = 16,
		ReverseAccess = 32,
		TableDisplayName = 64, // Used to indicate that a field is the primary name for a record, useful for generic debug information, etc.
		NoDefaultSelect = 128, // Indicates a column is not selected by default, it will be selected when accessed or when manually specified
		Compress = 256, // use qCompress to internally compress contents of this field - BUG FIXME
		LastFlag = 512
	};	

	/** 
	 *  Constructs a new field that is not a foreign key.
	 */
	Field( TableSchema * table, const QString & name, Field::Type type = Field::Int, Field::Flags flags = Field::None, const QString & method = QString() );

	/**
	 *  Constructs a new field that has a foreign key relationship.
	 */
	Field( TableSchema * table, const QString & name, const QString & fkeyTable, Field::Flags flags = Field::ForeignKey, bool index = false, int indexDeleteMode=DoNothingOnDelete );

	~Field();

	/** Returns the table this field belongs to */
	TableSchema * table() const;

	/** Returns the name of this field, as used by the database */
	QString name() const;

	/** Sets the name of this field, this must match the underlying database */
	void setName( const QString & );

	/** Returns the placeholder of this field, as used by the database */
	QString placeholder( int i = 0 ) const;

	QString generatedPluralMethodName() const;
	QString pluralMethodName() const;
	void setPluralMethodName( const QString & );

	/** Returns the type of this field. */
	Type type() const;

	/** Changes the type of this field.
	 *  NOTE: This should not be called if this field
	 *  describes an existing table in the database
	 */
	void setType( Type type );

	/** Returns the alternative name for this field, used
	 *  for the method name for generate classes.
	 */
	QString methodName() const;

	/** Sets the method name for this field */
	void setMethodName( const QString & );
	
	/** Returns a display name automatically created from the
	 *  method name by capitalizing the first letter and adding
	 *  a space before each subsequent capital letter in the name
	 **/
	QString generatedDisplayName() const;
	
	/** Display name is used to show the name in gui's
	 *  If not set it will return generatedDisplayName()
	 **/
	QString displayName() const;
	void setDisplayName( const QString & );
	
	/** Returns the flags for this field */
	Flags flags() const;
	
	/** Sets the flags for this field */
	bool setFlags( Flags f );

	/** Returns true if flag \param f is set for this field. */
	bool flag( int f ) const;

	/** Sets flag \param f to \param enabled */
	void setFlag( int f, bool enabled );

	/** Returns a string of flags usable for c++ code generation */
	QString flagString() const;

	/** Documentation string in doxygen format, inserted into generated
	 *  code */
	QString docs() const;
	void setDocs( const QString & docs );

	/** Returns the internal position of this field in the tables
	 *  list of fields. */
	int pos() const;

	/** Sets the position of this field, should only be called by the
	 *  table that owns this field.
	 */
	void setPos( int pos );

	/**  Returns the default value for this field, if set */
	QVariant defaultValue() const;

	/**  Sets the default value for this field */
	void setDefaultValue( const QVariant & );

	/** Returns a string that can be inserted into source code ( eg. "abc", 0 ) */
	QString defaultValueString() const;

	//
	// Foreign Key Stuff
	//
	/** Returns the table that this foreign key points to */
	TableSchema * foreignKeyTable() const;
	/** Sets the table that this foreign key points to */
	void setForeignKey( const QString & tableName );
	/** Returns the name of the table this foreign key points to */
	QString foreignKey() const;
	
	/** Enum describing actions to be performed on the foreign key table
	 *  when a record from this table is deleted.
	 */
	enum {
		DoNothingOnDelete = 0,
		UpdateOnDelete    = 1,
		CascadeOnDelete	  = 1 << 1
	};
	
	/** Returns true if there is an internal index associated with this field */
	bool hasIndex() const;

	/** Returns a pointer to the index for this field. */
	IndexSchema * index();

	/** Creates or destroys the index associated with this field, according to \param hasIndex */
	void setHasIndex( bool hasIndex, int indexDeleteMode = DoNothingOnDelete );

	/** Returns the action performed on the foreign key table when a record from this table is deleted. */
	int indexDeleteMode() const;

	/** Returns a string representing the indexDeleteMode used for c++ code generation. */
	QString indexDeleteModeString() const;

	/** Returns an IndexDeleteMode enum according to \param modeString */
	static int indexDeleteModeFromString( const QString & modeString );
	
	void removeIndex( IndexSchema * index );
	
	/// Returns the enum for the type 
	/// or -1 if the type is not found
	static Type stringToType( const QString & );
	static QVariant variantFromString( const QString &, Type );

	/// Attempts to convert the input variant to the proper storage format for this field type
	/// For example converts interval and color string representations into their proper binary
	/// value wrapped inside a qvariant
	QVariant coerce( const QVariant & v );

	/// Attempts to convert the variant from the internal binary representation to the format
	/// expected for database inserts and updates
	QVariant dbPrepare( const QVariant & v );

	/// Returns a string of the c++ type
	QString typeString() const;
	/// Returns a string of the variant type
	QString variantTypeString() const;
	QString typeFromVariantCode() const;
	QString variantFromTypeCode() const;

	/// Returns a string of the list type( eg. QStringList, QValueList<float> ).
	QString listTypeString() const;
	
	QString dbTypeString() const;
	
	static QString dbTypeString(Field::Type);

	int qvariantType() const;
	static int qvariantType(Field::Type);

	QString diff( Field * after );
	
private:
	static const char * typeStrings[];
	static const char * listTypeStrings[];
	static const char * variantTypeStrings[];
	static const char * dbTypeStrings[];

protected:
	struct ForeignKeyPrivate;
	TableSchema * mTable;
	QString mName, mMethodName;
	mutable QString mDisplayName, mPluralMethodName;
	Type mType;
	Flags mFlags;
	QVariant mDefault;
	ForeignKeyPrivate * mForeignKey;
	IndexSchema * mIndex;
	int mPos;
	QString mDocs;
};

typedef QList<Field *> FieldList;
typedef QList<Field *>::Iterator FieldIter;

FieldList operator|(const FieldList & one, const FieldList & two);
FieldList operator&(const FieldList & one, const FieldList & two);

} //namespace

using Stone::Field;
using Stone::FieldList;

#endif // FIELD_H

