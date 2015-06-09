

#ifndef JOB_FILTER_EDIT_H
#define JOB_FILTER_EDIT_H

#include <qwidget.h>
#include <qstring.h>

#include "expression.h"

#include "stonegui.h"

class QLineEdit;
class QPushButton;
class QLabel;

class Field;

class STONEGUI_EXPORT FilterEdit : public QWidget
{
Q_OBJECT
public:
	FilterEdit( QWidget * parent = 0, Field * matchField = 0, const QString & label = QString() );
	
	enum FilterType {
		WildCardContainsFilter,
		WildCardFullMatchFilter,
		RegExFilter
		//, AdvancedFilter TODO allow filtering arbitrary fields
	};
	
	enum UpdateMode {
		UpdateOnEnter,
		UpdateOnEdit
	};
	
	QLineEdit * lineEdit() const { return mLineEdit; }
	QPushButton * clearButton() const { return mClearButton; }
	QLabel * label() const { return mLabel; }
	
	// Returns the contents of the filter transformed into a fragment of a sql where statement
	QString sqlFilter() const;
	
	Expression expression() const;
	
	Field * matchField() const { return mMatchField; }
	void setMatchField( Field * field );
	
	UpdateMode updateMode() const { return mUpdateMode; }
	void setUpdateMode( UpdateMode );
	
signals:
	void filterChanged( const Expression & );

protected slots:
	void slotEmitFilterChanged();
	void slotClearText();

protected:
	bool eventFilter( QObject * o, QEvent * e );

	Field * mMatchField;
	QLabel * mLabel;
	QLineEdit * mLineEdit;
	QPushButton * mClearButton;
	FilterType mFilterType;
	bool mCaseSensitive;
	UpdateMode mUpdateMode;
};

#endif // JOB_FILTER_EDIT_H
