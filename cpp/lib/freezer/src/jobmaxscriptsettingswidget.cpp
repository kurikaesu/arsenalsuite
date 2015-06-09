
/* $Author$
 * $LastChangedDate: 2010-02-16 17:20:26 +1100 (Tue, 16 Feb 2010) $
 * $Rev: 9358 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/src/jobmaxscriptsettingswidget.cpp $
 */


#include "jobmaxscript.h"
#include "jobmaxscriptsettingswidget.h"

JobMaxScriptSettingsWidget::JobMaxScriptSettingsWidget( QWidget * parent, JobSettingsWidget::Mode mode )
: CustomJobSettingsWidget( parent, mode )
, JobMaxUtils(this)
, mIgnoreChanges( false )
{
	setupUi(this);
	connect( mSilentCheck, SIGNAL( toggled( bool ) ), SLOT( settingsChange() ) );
	connect( m64BitCheck, SIGNAL( toggled( bool ) ), SLOT( settingsChange() ) );

	if( mode == JobSettingsWidget::ModifyJobs )
		mVerticalLayout->addLayout( mApplyResetLayout );
}

JobMaxScriptSettingsWidget::~JobMaxScriptSettingsWidget()
{
}

QStringList JobMaxScriptSettingsWidget::supportedJobTypes()
{
	return jobTypes();
}

QStringList JobMaxScriptSettingsWidget::jobTypes()
{
	return QStringList( "MaxScript" );
}

template<class T> static QList<T> unique( QList<T> list )
{
	return list.toSet().toList();
}

template<class T> static Qt::CheckState listToCheckState( QList<T> list )
{
	list = unique(list);
	return list.size() == 1 ? (list[0] ? Qt::Checked : Qt::Unchecked) : Qt::PartiallyChecked;
}

template<class T> static void checkBoxApplyMultiple( QCheckBox * cb, QList<T> values )
{
	Qt::CheckState state = listToCheckState(values);
	cb->setTristate( state == Qt::PartiallyChecked );
	cb->setCheckState( state );
}

void JobMaxScriptSettingsWidget::resetSettings()
{
	clearCache();
	JobMaxScriptList jms( mSelectedJobs );
	mIgnoreChanges = true;
	checkBoxApplyMultiple( mSilentCheck, jms.silents() );
	reset64BitCheckBox( jms, m64BitCheck );
	mIgnoreChanges = false;

	CustomJobSettingsWidget::resetSettings();
}

void JobMaxScriptSettingsWidget::applySettings()
{
	JobMaxScriptList jms( mSelectedJobs );
	if( mSilentCheck->checkState() != Qt::PartiallyChecked )
		jms.setSilents( mSilentCheck->checkState() == Qt::Checked );
	apply64BitCheckBox( jms, m64BitCheck );

	if( mMode != JobSettingsWidget::ModifyJobs )
		jms.commit();

	CustomJobSettingsWidget::applySettings();
}
