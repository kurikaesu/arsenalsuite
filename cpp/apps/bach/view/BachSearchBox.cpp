/*
 * Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
 * SVN_META_HEADURL = "$HeadURL: $"
 * SVN_META_ID = "$Id: BachSearchBox.cpp 9408 2010-03-03 22:35:49Z brobison $"
 */

//---------------------------------------------------------------------------------------------
#include "BachSearchBox.h"
#include "utils.h"
#include <QDebug>

namespace
{
	//---------------------------------------------------------------------------------------------
	QString
	RemoveQuotes( const QString & a_Str )
	{
		QString s = a_Str;
		if ( s.startsWith( '\"' ) )
			s = s.mid( 1 );
		if ( s.endsWith( '\"' ) )
			s = s.mid( 0, s.length() - 1 );
		return s;
	}


	//---------------------------------------------------------------------------------------------
	bool
	StartsWith( const QString & a_Str, const QString & a_SubStr )
	{
		if ( a_Str.isEmpty() )
			return a_Str.startsWith( a_SubStr );

		QString str = RemoveQuotes( a_Str );
		QString substr = RemoveQuotes( a_SubStr );
		return str.startsWith( substr );
	}


	//---------------------------------------------------------------------------------------------
	void
	ReplacePart( const QString & a_SourceStart, const QString & a_Dst, QStringList & o_Parts )
	{
		// DBG( "replacepart a["+a_SourceStart+"]["+a_Dst+"]" );
		for ( int idx = 0 ; idx < o_Parts.size() ; ++idx )
		{
			if ( StartsWith( o_Parts[ idx ], a_SourceStart ) )
			{
				o_Parts[ idx ] = a_Dst;
				return;
			}
		}

		// not found? add it!
		o_Parts << a_Dst;
	}

	//---------------------------------------------------------------------------------------------
	void
	EmptyParts( QStringList & o_Parts, QStringList & o_SSParts )
	{
		for ( int idx = 0 ; idx < o_Parts.size() ; ++idx )
		{
			ReplacePart( o_Parts[ idx ], "", o_SSParts );
		}
		o_Parts.clear();
	}

	//---------------------------------------------------------------------------------------------
	template< class WidgetT, class NumT >
	QString
	GetPart( const QString & a_Prefix, const WidgetT * a_Widget, const QComboBox * a_ComboBox, QStringList & o_Values )
	{
		NumT val = a_Widget->value();
		QString str = "\"" + a_Prefix + a_ComboBox->currentText() + QString::number( val ) + "\"";
		o_Values << str;
		return str;
	}

	//---------------------------------------------------------------------------------------------
	void
	GetPart( const QString & a_Prefix, const QGroupBox * a_CB, const QCheckBox * a_DCB,
			 const QDateTimeEdit * a_Value, QStringList & o_Parts, QStringList & o_SSParts )
	{
		if ( a_CB->isChecked() && a_DCB->isChecked() )
		{
			o_Parts.clear();
			o_Parts << QString( a_Prefix+a_Value->text() );
			ReplacePart( a_Prefix, o_Parts.last(), o_SSParts );
		}
		else
		{
			EmptyParts( o_Parts, o_SSParts );
		}
	}

}

//---------------------------------------------------------------------------------------------
BachSearchBox::BachSearchBox( QWidget * a_Parent )
:	QFrame( a_Parent )
,	m_SSCB( NULL )
{
	setupUi( this );

	connect( FilePathBox,    		SIGNAL( clicked(bool) ), SLOT( somethingChanged() ) );
	connect( ProjectBox,    		SIGNAL( clicked(bool) ), SLOT( somethingChanged() ) );
	connect( KeywordBox,    		SIGNAL( clicked(bool) ), SLOT( somethingChanged() ) );
	connect( KW_Has_Value,    		SIGNAL( clicked(bool) ), SLOT( somethingChanged() ) );
	connect( ExifBox,    			SIGNAL( clicked(bool) ), SLOT( somethingChanged() ) );
	connect( DateRangeBox,    		SIGNAL( clicked(bool) ), SLOT( somethingChanged() ) );
	connect( CameraInfoBox,    		SIGNAL( clicked(bool) ), SLOT( somethingChanged() ) );
	connect( ImageDetailsBox,    	SIGNAL( clicked(bool) ), SLOT( somethingChanged() ) );

	connect( DR_Taken_From_CB,  	SIGNAL( clicked(bool) ), SLOT( somethingChanged() ) );
	connect( DR_Taken_To_CB,    	SIGNAL( clicked(bool) ), SLOT( somethingChanged() ) );
	connect( DR_Imported_From_CB,   SIGNAL( clicked(bool) ), SLOT( somethingChanged() ) );
	connect( DR_Imported_To_CB,    	SIGNAL( clicked(bool) ), SLOT( somethingChanged() ) );

	connect( CI_Aperture_OP,    	SIGNAL( currentIndexChanged(int) ), SLOT( somethingChanged() ) );
	connect( CI_ISO_OP,    			SIGNAL( currentIndexChanged(int) ), SLOT( somethingChanged() ) );
	connect( CI_Lens_OP,   	 		SIGNAL( currentIndexChanged(int) ), SLOT( somethingChanged() ) );
	connect( CI_Shutter_OP,    		SIGNAL( currentIndexChanged(int) ), SLOT( somethingChanged() ) );
	connect( ID_Height_OP, 	  	 	SIGNAL( currentIndexChanged(int) ), SLOT( somethingChanged() ) );
	connect( ID_Width_OP,    		SIGNAL( currentIndexChanged(int) ), SLOT( somethingChanged() ) );

	connect( DR_Taken_From_Value,   SIGNAL( dateTimeChanged(const QDateTime&) ), SLOT( somethingChanged() ) );
	connect( DR_Taken_To_Value,    	SIGNAL( dateTimeChanged(const QDateTime&) ), SLOT( somethingChanged() ) );
	connect( DR_Imported_From_Value,SIGNAL( dateTimeChanged(const QDateTime&) ), SLOT( somethingChanged() ) );
	connect( DR_Imported_To_Value,  SIGNAL( dateTimeChanged(const QDateTime&) ), SLOT( somethingChanged() ) );

	connect( CI_Aperture_Value,    	SIGNAL( valueChanged(double) ), SLOT( somethingChanged() ) );
	connect( CI_ISO_Value,    		SIGNAL( valueChanged(double) ), SLOT( somethingChanged() ) );
	connect( CI_Lens_Value,    		SIGNAL( valueChanged(double) ), SLOT( somethingChanged() ) );
	connect( CI_Shutter_Value,    	SIGNAL( valueChanged(double) ), SLOT( somethingChanged() ) );

	connect( Proj_Value,    		SIGNAL( currentIndexChanged(int) ), SLOT( somethingChanged() ) );

	connect( FP_Value,    			SIGNAL( textChanged(const QString &) ), SLOT( somethingChanged() ) );
	connect( KW_Value,    			SIGNAL( textChanged(const QString &) ), SLOT( somethingChanged() ) );
	connect( EI_Value,    			SIGNAL( textChanged(const QString &) ), SLOT( somethingChanged() ) );

	connect( ID_Height_Value,    	SIGNAL( valueChanged(int) ), SLOT( somethingChanged() ) );
	connect( ID_Width_Value,    	SIGNAL( valueChanged(int) ), SLOT( somethingChanged() ) );

	DR_Taken_To_Value->setDateTime( QDateTime::currentDateTime() );
	DR_Imported_To_Value->setDateTime( QDateTime::currentDateTime() );
}

//---------------------------------------------------------------------------------------------
QString
BachSearchBox::getSearchOptions( const QString & a_CurrentOptions )
{
	return a_CurrentOptions;
}


//---------------------------------------------------------------------------------------------
void
BachSearchBox::somethingChanged()
{
	if ( !m_SSCB )
	{
		DEBG( "Our callback is null, how?" );
		return;
	}

	QString ss = m_SSCB->getSearchString();
	QStringList ssparts;
	SmartTokenise( ss, ssparts );
	QStringList parts;

//	DBG( ">>"+ss );
//	for( int idx = 0 ; idx < ssparts.size() ; ++idx )
//		DBG( ssparts[ idx ] );
//	DBG( "<<<<<<<<<" );

	//------------------------------------------
	if ( ProjectBox->isChecked() )
	{
		m_Proj_Parts.clear();
		QString s ( "proj:"+Proj_Value->currentText() );
		if ( s.contains( " " ) )
			s = "\""+s+"\"";
		m_Proj_Parts << s;
		ReplacePart( "proj:", m_Proj_Parts[ 0 ], ssparts );
	}
	else
	{
		EmptyParts( m_Proj_Parts, ssparts );
	}

	//------------------------------------------
	if ( FilePathBox->isChecked() )
	{
		m_FP_Parts.clear();
		QString s ( "file:"+FP_Value->text() );
		if ( s.contains( " " ) )
			s = "\""+s+"\"";
		m_FP_Parts << s;
		ReplacePart( "file:", m_FP_Parts[ 0 ], ssparts );
	}
	else
	{
		EmptyParts( m_FP_Parts, ssparts );
	}

	//------------------------------------------
	if ( KeywordBox->isChecked() )
	{
		m_KW_Parts.clear();
		QString s ( "keyword:"+KW_Value->text() );
		if ( s.contains( " " ) )
			s = "\""+s+"\"";
		m_KW_Parts << s;
		ReplacePart( "keyword:", m_KW_Parts[ 0 ], ssparts );
	}
	else
	{
		EmptyParts( m_KW_Parts, ssparts );
	}

	//------------------------------------------
	if ( !KW_Has_Value->isChecked() )
	{
		m_HKW_Parts.clear();
		m_HKW_Parts << "haskeyword:no";
		ReplacePart( "haskeyword:no", m_HKW_Parts[ 0 ], ssparts );
	}
	else
	{
		EmptyParts( m_HKW_Parts, ssparts );
	}

	//------------------------------------------
	if ( ExifBox->isChecked() )
	{
		m_EI_Parts.clear();
		QString s ( "exif:"+EI_Value->text() );
		if ( s.contains( " " ) )
			s = "\""+s+"\"";
		m_EI_Parts << s;
		ReplacePart( "exif:", m_EI_Parts[ 0 ], ssparts );
	}
	else
	{
		EmptyParts( m_EI_Parts, ssparts );
	}

	//------------------------------------------
	GetPart( "from:", 			DateRangeBox, DR_Taken_From_CB, 	DR_Taken_From_Value, 	m_DR_Taken_From_Parts, 	ssparts );
	GetPart( "to:", 			DateRangeBox, DR_Taken_To_CB, 		DR_Taken_To_Value, 		m_DR_Taken_To_Parts, 	ssparts );
	GetPart( "from_import:", 	DateRangeBox, DR_Imported_From_CB, 	DR_Imported_From_Value, m_DR_Imported_From_Parts, ssparts );
	GetPart( "to_import:", 		DateRangeBox, DR_Imported_To_CB, 	DR_Imported_To_Value, 	m_DR_Imported_To_Parts, 	ssparts );

	//------------------------------------------
	if ( CameraInfoBox->isChecked() )
	{
		ReplacePart( "iso", 		GetPart< QDoubleSpinBox, double >( "iso", 		CI_ISO_Value, 		CI_ISO_OP, m_CI_Parts ), ssparts );
		ReplacePart( "aperture", 	GetPart< QDoubleSpinBox, double >( "aperture", 	CI_Aperture_Value, 	CI_Aperture_OP, m_CI_Parts ), ssparts );
		ReplacePart( "lens", 		GetPart< QDoubleSpinBox, double >( "lens", 		CI_Lens_Value, 		CI_Lens_OP, m_CI_Parts ), ssparts );
		ReplacePart( "shutter", 	GetPart< QDoubleSpinBox, double >( "shutter", 	CI_Shutter_Value, 	CI_Shutter_OP, m_CI_Parts ), ssparts );
	}
	else
	{
		EmptyParts( m_CI_Parts, ssparts );
	}

	//------------------------------------------
	if ( ImageDetailsBox->isChecked() )
	{
		ReplacePart( "width", 		GetPart< QSpinBox, int >( "width",  ID_Width_Value,  ID_Width_OP, m_ID_Parts  ), ssparts );
		ReplacePart( "height", 		GetPart< QSpinBox, int >( "height", ID_Height_Value, ID_Height_OP, m_ID_Parts ), ssparts );
	}
	else
	{
		EmptyParts( m_ID_Parts, ssparts );
	}

	ss = ssparts.join( " " );

//	DBG( "<<"+ss );

	m_SSCB->setSearchString( ss );
}

//---------------------------------------------------------------------------------------------
void
BachSearchBox::SetSearchStringCallback( SearchStringCallback * a_SSCB )
{
	m_SSCB = a_SSCB;
}
