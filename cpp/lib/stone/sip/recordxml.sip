
class RecordXmlSaver
{
%TypeHeaderCode
#include <recordxml.h>
%End
public:
	RecordXmlSaver(RecordList rl = RecordList());

	QList<QDomElement> addRecords( RecordList rl );
	QDomElement addRecord( const Record & r );

	QDomDocument document() const;
	bool saveToFile(const QString & fileName, QString * errorMessage /Out/);

	static bool toFile(RecordList rl, const QString & fileName, QString * errorMessage /Out/);
	static QDomDocument toDocument(RecordList rl, QString * errorMessage /Out/);
};


class RecordXmlLoader
{
%TypeHeaderCode
#include <recordxml.h>
%End
public:
	RecordXmlLoader(const QString & fileName);
	RecordXmlLoader(QDomDocument document);
	RecordXmlLoader();

	RecordList records() const;
	QMap<Record,QDomElement> recordElementMap() const;

	bool loadFile( const QString & fileName, QString * errorMessage /Out/);
	bool loadDocument( const QDomDocument & document, QString * errorMessage /Out/);

	static RecordList fromFile(const QString & fileName, QString * errorMessage /Out/);
	static RecordList fromDocument(const QDomDocument & document, QString * errorMessage /Out/);
};
