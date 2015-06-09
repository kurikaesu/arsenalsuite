
/* $Author: newellm $
 * $LastChangedDate: 2011-12-13 12:58:31 -0800 (Tue, 13 Dec 2011) $
 * $Rev: 12504 $
 * $HeadURL: svn://newellm@svn.blur.com/blur/trunk/cpp/lib/assfreezer/src/jobsettingswidget.cpp $
 */

#include "database.h"

#include "group.h"
#include "jobhistory.h"
#include "jobhistorytype.h"
#include "jobservice.h"
#include "jobtask.h"
#include "user.h"
#include "usergroup.h"

#include "hostselector.h"

#include "jobsettingswidget.h"
#include "jobsettingswidgetplugin.h"
#include "jobenvironment.h"
#include "jobenvironmentwindow.h"
#include "usernotifydialog.h"

JobSettingsWidget::JobSettingsWidget( QWidget * parent, Mode mode )
: QWidget( parent )
, mMode( mode )
, mChanges( false )
, mIgnoreChanges( false )
, mSelectedJobsProxy( 0 )
{
	setupUi(this);

	mPersonalPriorityLabel->hide();
	mPersonalPrioritySpin->hide();
	mLoggingEnabledCheck->hide();
	
	/* Instant Settings connections */
	connect( mResetInstantSettings, SIGNAL( clicked() ), SLOT( resetSettings() ) );
	connect( mApplyInstantSettings, SIGNAL( clicked() ), SLOT( applySettings() ) );
	connect( mPrioritySpin, SIGNAL( valueChanged(int,bool) ), SLOT( settingsChange() ) );
	connect( mPersonalPrioritySpin, SIGNAL( valueChanged(int,bool) ), SLOT( settingsChange() ) );
	connect( mDeleteOnCompleteCheck, SIGNAL( stateChanged(int) ), SLOT( settingsChange() ) );

	connect( mPacketCombo, SIGNAL( currentIndexChanged(int) ), SLOT( settingsChange() ) );

	connect( mPacketSizeSpin, SIGNAL( valueChanged(int,bool) ), SLOT( settingsChange() ) );
	connect( mSlotsSpin, SIGNAL( valueChanged(int,bool) ), SLOT( settingsChange() ) );

	//connect( mEmailErrorsCheck, SIGNAL( stateChanged(int) ), SLOT( settingsChange() ) );
	//connect( mEmailCompleteCheck, SIGNAL( stateChanged(int) ), SLOT( settingsChange() ) );
	connect( mEmailErrorListButton, SIGNAL( clicked() ), SLOT( showEmailErrorListWindow() ) );
	connect( mJabberErrorListButton, SIGNAL( clicked() ), SLOT( showJabberErrorListWindow() ) );
	//connect( mJabberErrorsCheck, SIGNAL( stateChanged(int) ), SLOT( settingsChange() ) );
	//connect( mJabberCompleteCheck, SIGNAL( stateChanged(int) ), SLOT( settingsChange() ) );
	connect( mEmailCompleteListButton, SIGNAL( clicked() ), SLOT( showEmailCompleteListWindow() ) );
	connect( mJabberCompleteListButton, SIGNAL( clicked() ), SLOT( showJabberCompleteListWindow() ) );
	connect( mUseAutoCheck, SIGNAL( stateChanged(int) ), SLOT( setAutoPacketSize(int) ) );
	connect( mAllHostsCheck, SIGNAL( stateChanged(int) ), SLOT( settingsChange() ) );
	connect( mHostListButton, SIGNAL( clicked() ), SLOT( showHostSelector() ) );
	connect( mMaxTaskTimeSpin, SIGNAL( valueChanged( int,bool ) ), SLOT( settingsChange() ) );
	connect( mMaxLoadTimeSpin, SIGNAL( valueChanged( int,bool ) ), SLOT( settingsChange() ) );
	connect( mMaxQuietTimeSpin, SIGNAL( valueChanged( int,bool ) ), SLOT( settingsChange() ) );
	connect( mMinMemorySpin, SIGNAL( valueChanged( int,bool ) ), SLOT( settingsChange() ) );
	connect( mMaxMemorySpin, SIGNAL( valueChanged( int,bool ) ), SLOT( settingsChange() ) );
	connect( mPrioritizeOuterTasksCheck, SIGNAL( stateChanged(int) ), SLOT( settingsChange() ) );
	connect( mEnvironmentButton, SIGNAL( clicked() ), SLOT( showEnvironmentWindow() ) );
	connect( mServiceTree->model(), SIGNAL( dataChanged(const QModelIndex &, const QModelIndex &) ), SLOT( settingsChange() ) );
	connect( mLoggingEnabledCheck, SIGNAL( stateChanged(int) ), SLOT( settingsChange() ) );

	mSelectedJobsProxy = new RecordProxy( this );
	mPrioritySpin->setProxy( mSelectedJobsProxy );
	mPersonalPrioritySpin->setProxy( mSelectedJobsProxy );
	mDeleteOnCompleteCheck->setProxy( mSelectedJobsProxy );
	mPrioritizeOuterTasksCheck->setProxy( mSelectedJobsProxy );
	mLoggingEnabledCheck->setProxy( mSelectedJobsProxy );

	mPacketSizeSpin->setMinimum( 1 );
	mSlotsSpin->setMinimum( 1 );

	registerBuiltinCustomJobSettingsWidgets();
	updateCustomJobSettingsWidget();

	setEnabled( false );

	if( mMode == SubmitJobs ) {
		mApplyInstantSettings->hide();
		mResetInstantSettings->hide();
	}

	// Store the user list once
	mMainUserList = Employee::select();
}

JobSettingsWidget::~JobSettingsWidget()
{
}

void JobSettingsWidget::setSelectedJobs( JobList selected )
{
	if( selected != mSelectedJobs ) {
		mSelectedJobs = selected;
		updateCustomJobSettingsWidget();
		resetSettings();
		setEnabled( !mSelectedJobs.isEmpty() );
	}
}

CustomJobSettingsWidget * JobSettingsWidget::currentCustomWidget()
{
	if( mCustomJobSettingsStack->isHidden() ) return 0;
	return qobject_cast<CustomJobSettingsWidget*>(mCustomJobSettingsStack->currentWidget());
}

void JobSettingsWidget::updateCustomJobSettingsWidget()
{
	JobTypeList jtl = mSelectedJobs.jobTypes().unique();
	CustomJobSettingsWidget * w = 0;
	if( jtl.size() == 1 ) {
		JobType jobType = jtl[0];
		QMap<QString,CustomJobSettingsWidget*>::iterator it = mCustomJobSettingsWidgetMap.find( jobType.name() );
		if( it != mCustomJobSettingsWidgetMap.end() )
			w = *it;
		else {
			if( CustomJobSettingsWidgetsFactory::supportsJobType( jobType.name() ) ) {
				w = CustomJobSettingsWidgetsFactory::createCustomJobSettingsWidget( jobType.name(), mCustomJobSettingsStack, mMode );
				if( w ) {
					foreach( QString jt, w->supportedJobTypes() )
						mCustomJobSettingsWidgetMap[jt] = w;
					mCustomJobSettingsStack->addWidget( w );
					emit customJobSettingsWidgetCreated( w );
				}
			}
		}
	}
	if( w ) {
		w->setSelectedJobs( mSelectedJobs );
		if( w != mCustomJobSettingsStack->currentWidget() )
			mCustomJobSettingsStack->setCurrentWidget( w );
	} else
		mCustomJobSettingsStack->setCurrentWidget( mEmptyPage );
}

void JobSettingsWidget::showHostSelector()
{
	HostSelector hs(this);
	if( mSelectedJobs.jobTypes().unique().size() == 1 )
		hs.setServiceFilter( mSelectedJobs.jobServices().services().unique() );
	hs.setHostList( mUpdatedHostListString );
	if( hs.exec() == QDialog::Accepted ){
		mUpdatedHostListString = hs.hostStringList();
		mUpdatedHostList = hs.hostList();
		settingsChange();
	}
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

template<class C,class T> static QList<T> castList( QList<C> list )
{
	QList<T> ret;
	foreach( C v, list )
		ret.append( (T)v );
	return ret;
}

template<class T> static void checkBoxApplyMultiple( QCheckBox * cb, QList<T> values )
{
	Qt::CheckState state = listToCheckState(values);
	cb->setTristate( state == Qt::PartiallyChecked );
	cb->setCheckState( state );
}

void getOwnerNotifyMethods( QString original, User u, bool & jabber, bool & email )
{
	jabber = email = false;
	QStringList parts = original.split(',');
	QRegExp userGrep( "^" + u.name() + ":.+" );
	int i = parts.indexOf( userGrep );
	if( i >= 0 ) {
		LOG_5( "getOwnerNotifyMethods: Found owner entry" );
		QString notifyString = parts[i];
		QStringList notifyParts = notifyString.split(":");
		if( notifyParts.size() == 2 ) {
			LOG_5( "getOwnerNotifyMethods: Parsed methods: " + notifyParts[1] );
			if( notifyParts[1].contains("e") )
				email = true;
			if( notifyParts[1].contains("j") )
				jabber = true;
		}
	}
}

void JobSettingsWidget::resetSettings()
{
	mIgnoreChanges = true;
	LOG_5( mSelectedJobs.keyString() );

	mSelectedJobsProxy->setRecordList( mSelectedJobs, false, false );

	QList<int> maxTaskMinutes;
	foreach( int seconds, mSelectedJobs.maxTaskTimes() ) {
		maxTaskMinutes.append( seconds / 60 );
	}
	mMaxTaskTimeSpin->setValues( maxTaskMinutes );

	QList<int> maxLoadMinutes;
	foreach( int seconds, mSelectedJobs.maxLoadTimes() ) {
		maxLoadMinutes.append( seconds / 60 );
	}
	mMaxLoadTimeSpin->setValues( maxLoadMinutes );

	QList<int> maxQuietMinutes;
	foreach( int seconds, mSelectedJobs.maxQuietTimes() ) {
		maxQuietMinutes.append( seconds / 60 );
	}
	mMaxQuietTimeSpin->setValues( maxQuietMinutes );

	QList<int> minMemories;
	foreach( int mbs, mSelectedJobs.minMemories() ) {
		minMemories.append( mbs / 1024 );
	}
	mMinMemorySpin->setValues( minMemories );

	QList<int> maxMemories;
	foreach( int mbs, mSelectedJobs.maxMemories() ) {
		maxMemories.append( mbs / 1024 );
	}
	mMaxMemorySpin->setValues( maxMemories );

	QList<int> maxErrors;
	foreach( int errors, mSelectedJobs.maxErrors() ) {
		maxErrors.append( errors );
	}
	mMaxErrorsSpin->setValues( maxErrors );

	QList<int> packetSize = unique( castList<unsigned int,int>(mSelectedJobs.packetSizes()) );
	bool containsAuto = packetSize.contains(0);
	packetSize.removeAll(0);
	bool containsValue = !packetSize.isEmpty();
	mUseAutoCheck->setTristate( containsAuto && containsValue );
	mUseAutoCheck->setCheckState( (containsAuto && containsValue) ? Qt::PartiallyChecked : (containsAuto ? Qt::Checked : Qt::Unchecked) );
	mPacketSizeSpin->setValues( QList<int>() << (packetSize.isEmpty() ? 10 : packetSize[0]) );
	mPacketSizeSpin->setEnabled(!containsAuto);

	mSlotsSpin->setValues( castList<unsigned int, int>(mSelectedJobs.assignmentSlots()) );

	QStringList packetTypes( unique( mSelectedJobs.packetTypes() ) );
	bool containsFixedPacketType = packetTypes.contains( "preassigned" ) || packetTypes.contains( "continuous" );
	if( mMode == ModifyJobs ) {
		mPacketCombo->setEnabled( !containsFixedPacketType && packetTypes.size() == 1 );
		mPacketCombo->clear();
		if ( !containsFixedPacketType )
			mPacketCombo->addItems( QStringList() << "Random" << "Sequential" << "Iterative" << "Continuous" );
		mPacketGroup->setEnabled( !packetTypes.contains("preassigned") );
	} else {
		mPacketCombo->clear();
		mPacketCombo->addItems( QStringList() << "Random" << "Sequential" << "Continuous" << "Iterative" << "Preassigned" );
	}
	if( packetTypes.size() == 1 ) {
		QString pt = packetTypes[0];
		pt[0] = pt[0].toUpper();
		if( containsFixedPacketType )
			mPacketCombo->addItem( pt );
		mPacketCombo->setCurrentIndex( mPacketCombo->findText( pt ) );
	} else
		mPacketCombo->setCurrentIndex( -1 );

	extractNotifyUsers();
	mNotifyChanged = false;

	QStringList hostLists = mSelectedJobs.hostLists();
	bool hasPreassigned = false;
	QList<bool> emptyLists;
	foreach( QString s, hostLists ) {
		emptyLists.append( s.isEmpty() );
	}

	foreach( Job j, mSelectedJobs )
		hasPreassigned |= j.packetType() == "preassigned";

	Qt::CheckState allHosts = listToCheckState( emptyLists );
	mAllHostsCheck->setEnabled( !hasPreassigned );
	mAllHostsCheck->setTristate( allHosts == Qt::PartiallyChecked );
	mAllHostsCheck->setCheckState( allHosts );
	mHostListButton->setEnabled( allHosts == Qt::Unchecked );
	
	mUpdatedHostList = HostList();
	mUpdatedHostListString = QString();
	mUpdatedEnvironment = mSelectedJobs[0].environment().environment();
	buildServiceTree();
	mChanges = false;

	mApplyInstantSettings->setEnabled(false);
	mResetInstantSettings->setEnabled(false);

	if( !User::currentUser().userGroups().groups().contains( Group::recordByName("RenderOps") ) ) {
		mSlotsSpin->setEnabled(false);
		mMinMemorySpin->setEnabled(false);
		mMaxMemorySpin->setEnabled(false);
		mMaxErrorsSpin->setEnabled(false);
	}

	mIgnoreChanges = false;
}

void JobSettingsWidget::extractNotifyUsers()
{
	emailErrorList.clear();
	jabberErrorList.clear();

	emailCompleteList.clear();
	jabberCompleteList.clear();

	foreach( Job j, mSelectedJobs )
	{
		QStringList parts = j.notifyOnError().split(',');
		for( QStringList::Iterator it = parts.begin(); it != parts.end(); ++it)
		{
			QStringList userNotifyParts = it->split(':');
			if( userNotifyParts.size() == 2 )
			{
				QString methods = userNotifyParts[1];
				User user = User::recordByUserName(userNotifyParts[0]);
				if( user.isRecord() )
				{
					if( methods.contains("e") )
						if( !emailErrorList.contains(user) )
							emailErrorList.append(user);

					if( methods.contains("j") )
						if( !jabberErrorList.contains(user) )
							jabberErrorList.append(user);
				}
			}
		}

		parts = j.notifyOnComplete().split(',');
		for( QStringList::Iterator it = parts.begin(); it != parts.end(); ++it)
		{
			QStringList userNotifyParts = it->split(':');
			if( userNotifyParts.size() == 2 )
			{
				QString methods = userNotifyParts[1];
				User user = User::recordByUserName(userNotifyParts[0]);
				if( user.isRecord() )
				{
					if( methods.contains("e") )
						if( !emailCompleteList.contains(user) )
							emailCompleteList.append(user);

					if( methods.contains("j") )
						if( !jabberCompleteList.contains(user) )
							jabberCompleteList.append(user);
				}
			}
		}
	}
}

QString JobSettingsWidget::buildNotifyString( UserList emailList, UserList jabberList )
{
	QString notifyString;
	QStringList parts;

	for( UserIter user = emailList.begin(); user != emailList.end(); ++user )
	{
		parts += (*user).name() + ":e";
	}

	for( UserIter user = jabberList.begin(); user != jabberList.end(); ++user)
	{
		if( parts.contains((*user).name() + ":e"))
		{
			int index = parts.indexOf((*user).name() + ":e");
			parts[index] += "j";
		}
		else
			parts += (*user).name() + ":j";
	}

	notifyString = parts.join(",");

	return notifyString;
}

QString updateNotifyMethod( const QString & original, Qt::CheckState jabber, Qt::CheckState email )
{
	QStringList parts = original.split(":");
	QString methods;
	if( email == Qt::Checked || (parts[1].contains("e") && email == Qt::PartiallyChecked) )
		methods += "e";
	if( jabber == Qt::Checked || (parts[1].contains("j") && jabber == Qt::PartiallyChecked) )
		methods += "j";
	if( methods.isEmpty() )
		return "";
	return parts[0] + ":" + methods;
}

QString updateOwnerNotifyString( const QString & original, User u, Qt::CheckState jabber, Qt::CheckState email )
{
	QStringList parts = original.split(',').filter(".+");
	QRegExp userGrep( "^" + u.name() + ":.*" );
	int i = parts.indexOf( userGrep );
	if( i >= 0 ) {
		QString newMethod = updateNotifyMethod(parts[i],jabber,email);
		if( newMethod.isEmpty() )
			parts.removeAt(i);
		else
			parts[i] = newMethod;
	} else {
		QString newMethod = updateNotifyMethod(u.name()+":",jabber,email);
		if( !newMethod.isEmpty() )
			parts += newMethod;
	}
	QString ret = parts.join(",");
	LOG_5( "Updated notify string: " + ret );
	return ret.isEmpty() ? QString::null : ret;
}

void JobSettingsWidget::setAutoPacketSize(int checkState)
{
	Qt::CheckState cs = (Qt::CheckState)checkState;
	mPacketSizeSpin->setEnabled( cs == Qt::Unchecked );
	settingsChange();
}

void JobSettingsWidget::applySettings()
{
	mIgnoreChanges = true;

	/* want to make it so only Admins or the owner can change a job...
	 * in progress
	foreach( Job j, mSelectedJobs ) {
		if(!mAdminEnabled || j.user() != User::currentUser()) {
			mInstantChanges = false;
			mApplyInstantSettings->setEnabled(false);
			mResetInstantSettings->setEnabled(false);
			resetInstantSettings();
			return;
		}
	}
	*/

	mSelectedJobsProxy->applyChanges();
	mSelectedJobs = mSelectedJobsProxy->records();

	if( mMaxTaskTimeSpin->changed() )
		mSelectedJobs.setMaxTaskTimes( mMaxTaskTimeSpin->value() * 60 );
	if( mMaxLoadTimeSpin->changed() )
		mSelectedJobs.setMaxLoadTimes( mMaxLoadTimeSpin->value() * 60 );
	if( mMaxQuietTimeSpin->changed() )
		mSelectedJobs.setMaxQuietTimes( mMaxQuietTimeSpin->value() * 60 );
	if( mMinMemorySpin->changed() )
		mSelectedJobs.setMinMemories( mMinMemorySpin->value() * 1024 );
	if( mMaxMemorySpin->changed() )
		mSelectedJobs.setMaxMemories( mMaxMemorySpin->value() * 1024 );
	if( mMaxErrorsSpin->changed() )
		mSelectedJobs.setMaxErrors( mMaxErrorsSpin->value() );

	if( mUseAutoCheck->checkState() != Qt::PartiallyChecked )
		if( mPacketSizeSpin->changed() )
			mSelectedJobs.setPacketSizes( mUseAutoCheck->isChecked() ? 0 : mPacketSizeSpin->value() );
	if( mSlotsSpin->changed() )
		mSelectedJobs.setAssignmentSlots( mSlotsSpin->value() );

	// Dont change preassigned jobs packet type
	if( mPacketGroup->isEnabled() ) {
		QString pt = mPacketCombo->currentText();
		pt[0] = pt[0].toLower();
		mSelectedJobs.setPacketTypes( pt );
	}

	if( mAllHostsCheck->checkState() != Qt::PartiallyChecked ) {
		if( mSelectedJobs[0].packetType() == "preassigned" ) {
			// This will be empty if the list hasn't been updated
			if( mUpdatedHostList.size() ) {
				foreach( Job j, mSelectedJobs )
					j.changePreassignedTaskListWithStatusPrompt( mUpdatedHostList, window() );
			}
		} else
			mSelectedJobs.setHostLists( mAllHostsCheck->checkState() == Qt::Checked ? "" : mUpdatedHostListString );
	}
	if( mNotifyChanged ) {
		// Build the user strings
		QString notifyOnErrorString = buildNotifyString(emailErrorList, jabberErrorList);
		QString notifyOnCompleteString = buildNotifyString(emailCompleteList, jabberCompleteList);

		foreach( Job j, mSelectedJobs ) {
			//j.setNotifyOnError( updateOwnerNotifyString( j.notifyOnError(), j.user(), mJabberErrorsCheck->checkState(), mEmailErrorsCheck->checkState() ) );
			j.setNotifyOnError( notifyOnErrorString );
				//j.setNotifyOnComplete( updateOwnerNotifyString( j.notifyOnComplete(), j.user(), mJabberCompleteCheck->checkState(), mEmailCompleteCheck->checkState() ) );
			j.setNotifyOnComplete( notifyOnCompleteString );
			mSelectedJobs.update(j);
		}
	}

	if( mPrioritySpin->changed() )
		mSelectedJobs.setPriorities( mPrioritySpin->value() );

	if( mPersonalPrioritySpin->changed() )
		mSelectedJobs.setPersonalPriorities( mPersonalPrioritySpin->value() );

	if( !mUpdatedEnvironment.isEmpty() ) {
		JobEnvironmentList jel = mSelectedJobs.environments();
		jel.setEnvironments( mUpdatedEnvironment );
		jel.commit();
	}

    saveServiceTree();

	if( mMode == ModifyJobs ) {
		Database::current()->beginTransaction();
		mSelectedJobs.commit();
		Database::current()->commitTransaction();
	}

	mChanges = false;
	mApplyInstantSettings->setEnabled(false);
	mResetInstantSettings->setEnabled(false);

	mIgnoreChanges = false;

	resetSettings();
}

void JobSettingsWidget::settingsChange()
{
	if( !mIgnoreChanges ) {
		if( mMode == SubmitJobs ) {
			applySettings();
			return;
		}
		mChanges = true;
		mApplyInstantSettings->setEnabled(true);
		mResetInstantSettings->setEnabled(true);
		mPacketSizeSpin->setEnabled( !mUseAutoCheck->isChecked() );
		mHostListButton->setEnabled( !mAllHostsCheck->isChecked() );
	}
}

void JobSettingsWidget::showEnvironmentWindow()
{
	JobEnvironmentWindow jew(this);
	jew.setEnvironment( mUpdatedEnvironment );
	if( jew.exec() == QDialog::Accepted ){
		mUpdatedEnvironment = jew.environment();
		settingsChange();
	}
}

void JobSettingsWidget::showEmailErrorListWindow()
{
	UserNotifyDialog und(this);
	und.setMainUserList(mMainUserList);
	und.setUsers(emailErrorList);
	if( und.exec() == QDialog::Accepted ) {
		mNotifyChanged = true;
		emailErrorList = und.userList();
		settingsChange();
	}
}

void JobSettingsWidget::showJabberErrorListWindow()
{
	UserNotifyDialog und(this);
	und.setMainUserList(mMainUserList);
	und.setUsers(jabberErrorList);
	if( und.exec() == QDialog::Accepted ) {
		mNotifyChanged = true;
		jabberErrorList = und.userList();
		settingsChange();
	}
}

void JobSettingsWidget::showEmailCompleteListWindow()
{
	UserNotifyDialog und(this);
	und.setMainUserList(mMainUserList);
	und.setUsers(emailCompleteList);
	if( und.exec() == QDialog::Accepted ) {
		mNotifyChanged = true;
		emailCompleteList = und.userList();
		settingsChange();
	}
}

void JobSettingsWidget::showJabberCompleteListWindow()
{
	UserNotifyDialog und(this);
	und.setMainUserList(mMainUserList);
	und.setUsers(jabberCompleteList);
	if( und.exec() == QDialog::Accepted ) {
		mNotifyChanged = true;
		jabberCompleteList = und.userList();
		settingsChange();
	}
}

void JobSettingsWidget::buildServiceTree()
{
	mServiceTree->setRootElement();

	QMap<uint, uint> depMap;
	foreach( Job job, mSelectedJobs )
	{
		ServiceList installed = job.jobServices().services();
		foreach( Service s, installed )
		{
			uint key = s.key();
			if( !depMap.contains( key ) )
				depMap[key] = 1;
				else
				depMap[key]++;
		}
	}

	ServiceList checked, tri;
	for( QMap<uint, uint>::Iterator it = depMap.begin(); it != depMap.end(); ++it ){
		if( it.value() == mSelectedJobs.size() )
			checked += Service( it.key() );
		else
			tri += Service( it.key() );
	}

	mServiceTree->setChecked( checked );
	mServiceTree->setNoChange( tri );

	mServiceTree->resizeColumnToContents(0);
	mServiceTree->resizeColumnToContents(1);
	mServiceTree->resizeColumnToContents(2);
}

void JobSettingsWidget::saveServiceTree()
{
	//qWarning("saving service tree\n");
	ServiceList on = mServiceTree->checkedElements(); 
	ServiceList nc = mServiceTree->noChangeElements();
	foreach( Job job, mSelectedJobs )
	{
		//qWarning("checking "+job.name().toAscii());
		JobServiceList installed = job.jobServices();
		JobServiceList toRemove;
		foreach( JobService js, installed )
			if( !on.contains( js.service() ) && !nc.contains( js.service() ) )
				toRemove += js;
		toRemove.remove();

		foreach( Service s, on ) {
			if( !installed.services().contains( s ) ) {
				JobService js = JobService();
				js.setJob(job);
				js.setService(s);
				js.commit();
			}
		}
	}
}

CustomJobSettingsWidget::CustomJobSettingsWidget( QWidget * parent, JobSettingsWidget::Mode mode )
: QGroupBox( parent )
, mJobServiceBridge( 0 )
, mApplyResetLayout( 0 )
, mApplySettingsButton( 0 )
, mResetSettingsButton( 0 )
, mMode( mode )
, mChanges( false )
{
	if( mode == JobSettingsWidget::ModifyJobs ) {
		mApplyResetLayout = new QHBoxLayout();
		mApplySettingsButton = new QPushButton( "Apply", this );
		mResetSettingsButton = new QPushButton( "Reset", this );
	
		connect( mApplySettingsButton, SIGNAL( clicked() ), SLOT( applySettings() ) );
		connect( mResetSettingsButton, SIGNAL( clicked() ), SLOT( resetSettings() ) );
	
		mApplyResetLayout->addStretch();
		mApplyResetLayout->addWidget( mApplySettingsButton );
		mApplyResetLayout->addWidget( mResetSettingsButton );
	}
}

CustomJobSettingsWidget::~CustomJobSettingsWidget()
{
	delete mJobServiceBridge;
}

JobSettingsWidget * CustomJobSettingsWidget::jobSettingsWidget() const
{
	QWidget * w = parentWidget();
	while( w && !w->inherits("JobSettingsWidget") )
		w = w->parentWidget();
	if( w )
		return (JobSettingsWidget*)w;
	return 0;
}

void CustomJobSettingsWidget::setSelectedJobs( JobList selected )
{
	mSelectedJobs = selected;
	resetSettings();
}

void CustomJobSettingsWidget::resetSettings()
{
	mChanges = false;
	if( mMode == JobSettingsWidget::ModifyJobs ) {
		mApplySettingsButton->setEnabled( false );
		mResetSettingsButton->setEnabled( false );
	}
}

void CustomJobSettingsWidget::applySettings()
{
	mChanges = false;
	if( mMode == JobSettingsWidget::ModifyJobs ) {
		mApplySettingsButton->setEnabled( false );
		mResetSettingsButton->setEnabled( false );
	}
}

void CustomJobSettingsWidget::settingsChange()
{
	if( mMode == JobSettingsWidget::SubmitJobs )
		applySettings();
	else {
		mChanges = true;
		mApplySettingsButton->setEnabled( true );
		mResetSettingsButton->setEnabled( true );
	}
}

void CustomJobSettingsWidget::setJobServiceBridge( JobServiceBridge * jobServiceBridge )
{
	mJobServiceBridge = jobServiceBridge;
}

JobServiceList CustomJobSettingsWidget::getJobServices( const Job & job )
{
	if( mJobServiceBridge )
		return mJobServiceBridge->getJobServices(job);
	return job.jobServices();
}

void CustomJobSettingsWidget::removeJobServices( const Job & job, JobServiceList jobServices )
{
	if( mJobServiceBridge )
		mJobServiceBridge->removeJobServices( job, jobServices );
	else
		jobServices.remove();
}

void CustomJobSettingsWidget::applyJobServices( const Job & job, JobServiceList jobServices )
{
	if( mJobServiceBridge )
		mJobServiceBridge->applyJobServices( job, jobServices );
	else
		jobServices.commit();
}

