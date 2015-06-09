
class IniConfig
{
%TypeHeaderCode
#include <iniconfig.h>
%End
public:
	IniConfig(QString configFile);
	IniConfig();
	~IniConfig();

	void clear(bool sectionOnly=false);

	void setFileName(const QString & fileName);

	void readFromFile( const QString & fileName = QString(), bool overwriteExisting = false );

	bool writeToFile( const QString & fileName = QString() );

	void setSection( const QString & section );
	void pushSection( const QString & section );
	void pushSubSection( const QString & subsection );
	void popSection();
	QString currentSection() const;

	QStringList sections() const;

	QStringList keys() const;

	bool readBool( const QString & key, bool def=false);
	int readInt( const QString & key, int def=0);
	QString readString( const QString & key, const QString & def=QString::null);
	QColor readColor( const QString & key, const QColor & def=Qt::black);
	QFont readFont( const QString & key, const QFont & def=QFont());
	QSize readSize( const QString & key, const QSize & def=QSize( 16,16 ) );
	QRect readRect( const QString & key, const QRect & def=QRect() ) const;
	QList<int> readIntList( const QString & key, const QList<int> & def = QList<int>() ) const;
	QByteArray readByteArray( const QString & key, const QByteArray & def = QByteArray() ) const;
	QDateTime readDateTime( const QString & key, const QDateTime & def = QDateTime() ) const;
	Interval readInterval( const QString & key, const Interval & def = Interval() ) const;

	void writeBool( const QString & key, bool val);
	void writeInt( const QString & key, int val);
	void writeString( const QString & key, const QString & val);
	void writeColor( const QString & key, const QColor & val);
	void writeFont( const QString & key, const QFont & val);
	void writeSize( const QString & key, const QSize & val );
	void writeRect( const QString & key, const QRect & val );
	void writeIntList( const QString & key, const QList<int> & val );
	void writeByteArray( const QString & key, const QByteArray & val );
	void writeDateTime( const QString & key, const QDateTime & val );
	void writeInterval( const QString & key, const Interval & val );
	
	void removeKey( const QString & key );
	void removeSection( const QString & group );
	void renameSection( const QString & before, const QString & after );
	void copySection( const QString & source, const QString & dest, bool clearExisting = true );
};

