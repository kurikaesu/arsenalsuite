
#include <qdialogbuttonbox.h>
#include <qlayout.h>
#include <qtextedit.h>

#include "pathtemplatedialog.h"
#include "resinerror.h"
#include "filetracker.h"

PathTemplateDialog::PathTemplateDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	connect( mUseScriptCheck, SIGNAL( toggled( bool ) ), SLOT( slotUseScriptToggled( bool ) ) );
	connect( mEditScriptButton, SIGNAL( clicked() ), SLOT( slotEditScript() ) );
}

void PathTemplateDialog::accept()
{
	if( mName->text().isEmpty() ) {
		ResinError::nameEmpty( this, "Name" );
		return;
	}

	PathTemplate exist = PathTemplate::recordByName( mName->text() );
	LOG_5( QString::number(exist.key()) + " -- " + QString::number( mTemplate.key() ) );
	if( exist.isRecord() && exist != mTemplate ) {
		ResinError::nameTaken( this, mName->text() );
		return;
	}
	
	if( mTemplate.isRecord() )
		FileTracker::invalidatePathCache();

	QDialog::accept();
}

void PathTemplateDialog::setPathTemplate( const PathTemplate & pt )
{
	mTemplate = pt;
	mName->setText( mTemplate.name() );
	mPathTemplate->setText( mTemplate.pathTemplate() );
	mFileNameTemplate->setText( mTemplate.fileNameTemplate() );
	bool useScript = !mTemplate.pythonCode().isEmpty();
	mUseScriptCheck->setChecked( useScript );
	slotUseScriptToggled( useScript );
}

PathTemplate PathTemplateDialog::pathTemplate()
{
	mTemplate.setName( mName->text() );
	mTemplate.setPathTemplate( mPathTemplate->text() );
	mTemplate.setFileNameTemplate( mFileNameTemplate->text() );
	return mTemplate;
}

void PathTemplateDialog::slotUseScriptToggled( bool useScript )
{
	mPathTemplate->setEnabled( !useScript );
	mFileNameTemplate->setEnabled( !useScript );
	mEditScriptButton->setEnabled( useScript );
}

void PathTemplateDialog::slotEditScript()
{
	QDialog * d = new QDialog( this );
	QTextEdit * te = new QTextEdit( d );
	QDialogButtonBox * bb = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, d );
	QVBoxLayout * vb = new QVBoxLayout( d );
	vb->addWidget(te);
	vb->addWidget(bb);
	te->setPlainText( mTemplate.pythonCode() );
	connect( bb, SIGNAL( accepted() ), d, SLOT( accept() ) );
	connect( bb, SIGNAL( rejected() ), d, SLOT( reject() ) );
	
	if( d->exec() == QDialog::Accepted )
		mTemplate.setPythonCode( te->toPlainText() );
	delete d;
}
