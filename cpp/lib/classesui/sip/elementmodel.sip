
class ElementModel : RecordSuperModel
{
%TypeHeaderCode
#include <elementmodel.h>
%End
public:
	ElementModel( QObject * parent /TransferThis/ = 0  );
	~ElementModel();

	static Element getElement( const QModelIndex & index );

	void setElementList( ElementList list );
	ElementList elementList() const;

	QModelIndex findElement( const Element &, bool build = true );

	bool namesNeedContext();
	void setNamesNeedContext( bool );

	bool secondColumnIsLocation() const;
	void setSecondColumnIsLocation( bool scil );

	virtual QStringList mimeTypes() const;

public slots:
	virtual void added( RecordList );
	virtual void removed( RecordList );
	virtual void updated( Record, Record );
};
