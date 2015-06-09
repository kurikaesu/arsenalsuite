

#ifndef JOB_FILTER_EDIT_H
#define JOB_FILTER_EDIT_H

#include <qwidget.h>
#include <qstring.h>

class QLineEdit;
class QPushButton;

class JobFilterEdit : public QWidget
{
Q_OBJECT
public:
	JobFilterEdit(QWidget * parent = 0);

	QLineEdit * lineEdit() const { return mLineEdit; }
	QPushButton * clearButton() const { return mClearButton; }
	
	// Returns the contents of the filter transformed into a fragment of a sql where statement
	QString sqlFilter() const;
signals:
	void filterChanged( const QString & sqlFilter );

protected slots:
	void slotTextEdited();
	void slotClearText();

protected:
	QLineEdit * mLineEdit;
	QPushButton * mClearButton;
};

#endif // JOB_FILTER_EDIT_H
