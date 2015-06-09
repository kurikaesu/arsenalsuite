
QString fieldToDisplayName( const QString & name );
QString pluralizeName( const QString & name );

class Field
{
%TypeHeaderCode
#include <field.h>
%End

public:
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
		Image
	};

	enum Flags {
		None = 0,
		PrimaryKey = 1,
		ForeignKey = 2,
		NotNull = 4,
		Unique = 8,
		LocalVariable = 16,
		ReverseAccess = 32,
		TableDisplayName = 64,
		NoDefaultSelect = 128,
		LastFlag = 256
	};

	enum {
		DoNothingOnDelete,
		UpdateOnDelete,
		CascadeOnDelete
	};

	Field( TableSchema * table /TransferThis/, const QString & name, Field::Type type = Field::Int, Field::Flags flags = Field::None, const QString & method = QString() );
	Field( TableSchema * table /TransferThis/, const QString & name, const QString & fkeyTable, Field::Flags flags = Field::ForeignKey, bool index = false, int indexDeleteMode=DoNothingOnDelete );


	TableSchema * table() const;
	QString name() const;
	void setName( const QString & );

	Type type() const;
	void setType( Type type );

	QString methodName() const;
	void setMethodName( const QString & );

	QString generatedPluralMethodName() const;
	QString pluralMethodName() const;
	void setPluralMethodName( const QString & );

	QString generatedDisplayName() const;
	QString displayName() const;
	void setDisplayName( const QString & );

	Flags flags() const;
	bool setFlags( Flags f );
	bool flag( int f ) const;
	void setFlag( int f, bool );
	QString flagString() const;

	QString docs() const;
	void setDocs( const QString & docs );

	int pos() const;
	void setPos( int pos );

	QVariant defaultValue() const;
	void setDefaultValue( const QVariant & );
	QString defaultValueString() const;

	TableSchema * foreignKeyTable() const;
	void setForeignKey( const QString & tableName );
	QString foreignKey() const;
	
	bool hasIndex() const;
	IndexSchema * index();
	void setHasIndex( bool, int indexDeleteMode = DoNothingOnDelete );
	int indexDeleteMode() const;
	QString indexDeleteModeString() const;
	static int indexDeleteModeFromString( const QString & );
	
	static Type stringToType( const QString & );
	static QVariant variantFromString( const QString &, Type );

	QVariant coerce( const QVariant & v );

	QVariant dbPrepare( const QVariant & v );

	QString typeString() const;
	QString variantTypeString() const;
	QString listTypeString() const;
	
	QString dbTypeString() const;

	int qvariantType() const;
	static int qvariantType(Field::Type);
};

