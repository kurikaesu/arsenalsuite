
#include "path.h"

#include "host.h"

#include "win32sharemanager.h"

Win32ShareManager::Win32ShareManager()
{}

static Win32ShareManager * sInstance = 0;

Win32ShareManager * Win32ShareManager::instance()
{
	if( !sInstance )
		sInstance = new Win32ShareManager();
	return sInstance;
}

bool Win32ShareManager::hasConflict( MappingList mappings )
{
	bool success = true;
	foreach( Mapping mapping, mappings )
	{
		MapInfo & mi = mCurrentMappings[mapping.mount()];
		if( mi.refs > 0 && mi.mapping != mapping ) {
			success = false;
			break;
		}
	}
	return success;
}

// currently assuming no conflicts
Win32ShareManager::MappingRef Win32ShareManager::map( MappingList mappings, bool forceUnmount, QString * _err )
{
	MappingRef ref;

	if( mappings.isEmpty() )
		return ref;

	bool mappingSuccess = true;
	ref = MappingRef( this, new QAtomicInt(), QString() );
	QString _mappings;
	foreach( Mapping mapping, mappings ) {
		MapInfo & mi = mCurrentMappings[mapping.mount()];
		if( mi.refs == 0 ) {
			mi.mapping = mapping;
			// If we are in our own logon session then we will force the unmount
			// This needs to be accounted for when we start to use multiple burners
			// simultaneously
			QString err;
			if( !mapping.map( forceUnmount, &err ) ) {
				LOG_1( "Unable to map drive: " + err );
				QString msg = Host::currentHost().name() + " " + err;
				sendEmail( QStringList("newellm@blur.com"), msg, msg, "thePipe@blur.com" );
				mappingSuccess = false;
				if( _err ) *_err = err;
				break;
			}
		} else if( mi.mapping != mapping ) {
			mappingSuccess = false;
			LOG_5( "Mapping conflict.  Drive " + mapping.mount() + " is already mapped to " + driveMapping( mapping.mount()[0].toLatin1() ) );
			break;
		}
		mi.refs++;
		ref.mMappings += mapping.mount();
	}

	if( !mappingSuccess )
		ref.release();

	return ref;
}

void Win32ShareManager::unmap( QString mappings )
{
	for( int i=0; i < mappings.size(); i++ ) {
		QString driveLetter = QString(mappings[i]);
		MapInfo & mi = mCurrentMappings[driveLetter];
		mi.refs--;
		if( mi.refs == 0 ) {
			LOG_5( "Mapping to " + driveLetter + " released" );
		}
	}
}

Win32ShareManager::MappingRef::MappingRef( Win32ShareManager * manager, QAtomicInt * ref, QString mappings )
: mRef( ref )
, mManager(manager)
, mMappings( mappings )
{
	if( ref )
		ref->ref();
}

Win32ShareManager::MappingRef::MappingRef( const MappingRef & other )
: mRef( other.mRef )
, mManager( other.mManager )
, mMappings( other.mMappings )
{
	if( mRef )
		mRef->ref();
}

Win32ShareManager::MappingRef & Win32ShareManager::MappingRef::operator=(const MappingRef & other)
{
	if( other.mRef != mRef ) {
		release();
		mRef = other.mRef;
		mMappings = other.mMappings;
		mManager = other.mManager;
		if( mRef ) mRef->ref();
	}
	return *this;
}

Win32ShareManager::MappingRef::MappingRef()
: mRef( 0 )
, mManager( 0 )
{}

Win32ShareManager::MappingRef::~MappingRef()
{
	release();
}

void Win32ShareManager::MappingRef::release()
{
	if( mRef && !mRef->deref() && mManager ) {
		mManager->unmap( mMappings );
		delete mRef;
	}
	mRef = 0;
}
