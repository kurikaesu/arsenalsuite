/*
 * Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
 * SVN_META_HEADURL = "$HeadURL: $"
 * SVN_META_ID = "$Id: BachSearchBox.h 9408 2010-03-03 22:35:49Z brobison $"
 */

#ifndef _DRD_BACHSEARCHBOX_H_
#define _DRD_BACHSEARCHBOX_H_

#include <qframe.h>
#include "ui_bachSearchBox.h"

//---------------------------------------------------------------------------------------------
//
class BachSearchBox : public QFrame, public Ui_BachSearchBox
{
Q_OBJECT
public:
	//---------------------------------------------------------------------------------------------
	class SearchStringCallback
	{
	public:
		virtual ~SearchStringCallback(){}
		virtual QString getSearchString() const = 0;
		virtual void setSearchString( const QString & a_SS ) = 0;
	};

	//---------------------------------------------------------------------------------------------
	BachSearchBox( QWidget * parent );
	virtual ~BachSearchBox() { }

	//---------------------------------------------------------------------------------------------
	void SetSearchStringCallback( SearchStringCallback * a_SSCB );
	QString getSearchOptions( const QString & a_CurrentOptions );

protected slots:
	void somethingChanged();

private:
	SearchStringCallback * m_SSCB;
	QVBoxLayout * m_Layout;

	QStringList m_FP_Parts;
	QStringList m_Proj_Parts;
	QStringList m_KW_Parts;
	QStringList m_HKW_Parts;
	QStringList m_EI_Parts;
	QStringList m_DR_Taken_From_Parts;
	QStringList m_DR_Taken_To_Parts;
	QStringList m_DR_Imported_From_Parts;
	QStringList m_DR_Imported_To_Parts;
	QStringList m_CI_Parts;
	QStringList m_ID_Parts;
};

#endif /* _DRD_BACHSEARCHBOX_H_ */
