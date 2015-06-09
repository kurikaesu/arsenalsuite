
#include "epaloader.h"
#include "qprocess.h"
#include "qfile.h"

void Log( const QString & message, const QString & file )
{
	qWarning( file.toLatin1() + " : " + message.toLatin1() );
}

EpaLoader::EpaLoader(const QString & presetLabel, bool useExistingEnvironment)
: mTreeWidgetItem(0)
, mRoot("")
, mCmdLine("")
, mWorkingDir("")
{
	mPresetLabel = presetLabel;
	mOverride.clear();
	mEnv.clear();
	if( useExistingEnvironment ) {
		QStringList envList = QProcess::systemEnvironment();
		QRegExp rx("([^=]+)=(.*)");
		foreach( QString var, envList ) {
			if( rx.exactMatch(var) ) {
				QString key =  rx.cap(1);
				QString value =  rx.cap(2);
				mEnv[key] = value;
				//LOG( "Setting key " + key + " to " + value );
			}
		}
	}
}

EpaLoader::~EpaLoader()
{
	//delete mTreeWidgetItem;
}

void EpaLoader::setRoot( const QString & path )
{
	mRoot = path;
	if( !mRoot.endsWith("/") )
		mRoot += "/";
	//LOG("root set to:"+mRoot);
}

void EpaLoader::setArch( const QString & arch )
{
	mArch = arch;
}

void EpaLoader::setSep( const QString & sep )
{
	mSep = sep;
}

void EpaLoader::mergeEnvironment(const QStringList & name)
{
	// first populate system vars, they get overridden by everything
	if( mTreeWidgetItem ) {
		QTreeWidgetItem * systemItem = new QTreeWidgetItem(mTreeWidgetItem, QStringList() << "System");
		systemItem->setIcon(0, QIcon(mRoot+"icons/dep.png"));

		foreach( QString key, mEnv.keys() ) {
			QTreeWidgetItem * depItem = new QTreeWidgetItem(systemItem, QStringList() << key << mEnv[key]);
			depItem->setData(0, Qt::UserRole, mPresetLabel);
			depItem->setIcon(0, QIcon(mRoot+"icons/dep.png"));
			depItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
		}
	}

	foreach( QString env, name )
		loadXml(env, mTreeWidgetItem);

	foreach( QString key, mOverride.keys() )
		mEnv[key] = mOverride[key];
}

void EpaLoader::addTreeItem(const QString & key, const QString & value, QTreeWidgetItem * parent)
{
	//LOG( "add tree item:"+key );
	QStringList texts;
	texts << key;
	texts << value;
	QTreeWidgetItem * depItem = new QTreeWidgetItem(parent, texts);
	depItem->setData(0, Qt::UserRole, mPresetLabel);
	depItem->setIcon(0, QIcon(mRoot+"icons/dep.png"));
	depItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
	if( key == "EPA_ICON" ) {
		QIcon icon(value);
		parent->setIcon(0, icon );
	}
	mTreeItemMap.insert( qMakePair(key, parent), depItem);
}

void EpaLoader::loadXml(const QString & path, QTreeWidgetItem * parent)
{
	QFile xmlFile(mRoot + path+".xml");
	//LOG("trying to load: "+ xmlFile.fileName());
	if (!xmlFile.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	QXmlStreamReader xml;
	xml.addData( xmlFile.readAll() );

	while( !xml.atEnd() ) {
		xml.readNext();

		if( xml.isStartElement() ) {
			mCurrentTag = xml.name().toString();
			//LOG( "start: "+ mCurrentTag );
			if( mCurrentTag == "set" ) {
				QString arch = xml.attributes().value("arch").toString();
				if( arch.isEmpty() || arch == mArch ) {
					mKeyString = xml.attributes().value("key").toString();
					//LOG("set key: "+mKeyString);
				}
			}
			if( mCurrentTag == "add" ) {
				QString arch = xml.attributes().value("arch").toString();
				if( arch.isEmpty() || arch == mArch ) {
					mKeyString = xml.attributes().value("key").toString();
					//LOG("append key: "+mKeyString);
				}
			}
		} else if (xml.isEndElement()) {
			//LOG( "end: "+xml.name().toString() );
			if( mCurrentTag == "set" && !mKeyString.isEmpty() ) {
				QString expandedValue = expandText(mValueString);
				//LOG("got expanded set: "+expandedValue);
				mEnv[mKeyString] = expandedValue;

				// do GUI stuff
				if( parent ) {
					addTreeItem( mKeyString, mEnv[mKeyString], parent );
				}
			} else if( mCurrentTag == "add" && !mKeyString.isEmpty() ) {
				QString expandedValue = expandText(mValueString);
				//LOG("got expanded add: "+expandedValue);
				if( mEnv.contains(mKeyString) )
					mEnv[mKeyString] = expandedValue + mSep + mEnv[mKeyString];
				else
					mEnv[mKeyString] = expandedValue;
				
				// do GUI stuff
				if( parent ) {
					if( mTreeItemMap.contains( qMakePair(mKeyString, parent) ) ) {
						//LOG( "update tree item"+mKeyString );
						QTreeWidgetItem * treeItem = mTreeItemMap[qMakePair(mKeyString, parent)];
						treeItem->setText(1, mEnv[mKeyString]);
					} else {
						addTreeItem( mKeyString, mEnv[mKeyString], parent );
					}
				}
			} else if( mCurrentTag == "dependOn" ) {
				QString dependOn = mValueString;
				if( !mLoaded.contains(dependOn) ) {
					if( parent ) {
						QTreeWidgetItem * depItem = new QTreeWidgetItem(parent, QStringList()<<dependOn);
						loadXml(dependOn, depItem);
					} else
						loadXml(dependOn);
					mLoaded += dependOn;
				}
			} else if( parent && mCurrentTag == "icon" ) {
				QIcon icon(mValueString);
				parent->setIcon(0, icon );
			}
			mKeyString.clear();
			mValueString.clear();
			mCurrentTag.clear();
		} else if (xml.isCharacters() && !xml.isWhitespace()) {
			mValueString = xml.text().toString();
			//LOG("data: "+mValueString);
		}
	}
}

QString EpaLoader::expandText(const QString & text)
{
	QRegExp rx("\\$([A-Z_]+)");
	int pos = 0;
	QString ret(text);

	while ((pos = rx.indexIn(ret, pos)) != -1) {
		QString key = rx.cap(1);
		//LOG( "found "+key+" in string "+ret );
		pos += rx.matchedLength();
		if( mEnv.contains(key) ) {
			//LOG("Environment key has value: "+mEnv.value(key));
			ret.replace("$"+key, mEnv.value(key));
			pos = 0;
		} else {
			//LOG("Environment key has no value");
			ret.replace("$"+key, "");
			pos = 0;
		}
	}
	return ret;
}

void EpaLoader::setTreeWidgetItem( QTreeWidgetItem * parent )
{
	mTreeWidgetItem = parent;
}

void EpaLoader::setOverride( const QMap<QString, QString> & over )
{
	mOverride = over;
}

void EpaLoader::setArgs( const QStringList & args )
{
	mCmdArgs = args;
}

QMap<QString, QString> EpaLoader::environment() const
{
	return mEnv;
}

QStringList EpaLoader::envList() const
{
	QStringList ret;
	foreach( QString key, mEnv.keys() )
		ret << key + "=" +mEnv[key];
	return ret;
}

QProcess * EpaLoader::launch(const QStringList & env)
{
	QProcess * proc = new QProcess();
	proc->setEnvironment(env);
	proc->setProcessChannelMode(QProcess::ForwardedChannels);
	if( mEnv.contains("EPA_WORKINGDIR") )
		proc->setWorkingDirectory(mEnv["EPA_WORKINGDIR"]);
	//QString cmdLine = "sh -c \""+ mEnv["EPA_CMDLINE"] +" &\"";
	QString cmdLine = mEnv["EPA_CMDLINE"];
	if(mCmdArgs.size() > 0)
		cmdLine += " "+ mCmdArgs.join(" ");
	LOG("CMD: "+cmdLine);
	proc->start(cmdLine);
	return proc;
}

QProcess * EpaLoader::shell(const QStringList & env)
{
	QProcess * proc = new QProcess();
	proc->setEnvironment(env);
	if( !mWorkingDir.isEmpty() )
		proc->setWorkingDirectory(mEnv["EPA_WORKINGDIR"]);
	QString cmdLine = "terminal";
	LOG("CMD: "+cmdLine);
	proc->start(cmdLine);
	return proc;
}

