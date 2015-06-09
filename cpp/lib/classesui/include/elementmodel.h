
#ifndef ELEMENT_MODEL_H
#define ELEMENT_MODEL_H

#include "classesui.h"

#include "recordsupermodel.h"
#include "element.h"
#include "elementstatus.h"
#include "project.h"
#include "projectstatus.h"
#include "user.h"

class CLASSESUI_EXPORT ElementModel : public RecordSuperModel
{
Q_OBJECT
public:
	ElementModel( QObject * parent = 0 );
	~ElementModel();

	static Element getElement( const QModelIndex & index );

	void setElementList( ElementList list );
	ElementList elementList() const;

	// If build is false, it will not build the tree internally
	// to find the element, in other words, if the element isn't
	// displayed, then it won't be found.
	QModelIndex findElement( const Element &, bool build = true );

	bool namesNeedContext() const { return mNamesNeedContext; }
	void setNamesNeedContext( bool );

	bool secondColumnIsLocation() const { return mSecondColumnIsLocation; }
	void setSecondColumnIsLocation( bool scil );

	virtual QStringList mimeTypes() const;

protected:
	bool dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );

public slots:
	virtual void added( RecordList );
	virtual void removed( RecordList );
	virtual void updated( Record, Record );
protected:
	bool mNamesNeedContext;
	bool mSecondColumnIsLocation;
};


#endif // ELEMENT_MODEL_H
