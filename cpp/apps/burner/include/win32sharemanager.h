
#ifndef WIN32_SHARE_MANAGER_H
#define WIN32_SHARE_MANAGER_H

#include <qatomic.h>
#include <qstring.h>
#include <qhash.h>

#include "mapping.h"

/**
 *  This class manages windows drive mappings.  Allowing multiple
 *  clients to reserve mappings that don't conflict.
 */

class Win32ShareManager
{
public:
	Win32ShareManager();

	class MappingRef;

	bool hasConflict( MappingList mappings );
	
	MappingRef map( MappingList mappings, bool forceUnmount, QString * err = 0 );

	static Win32ShareManager * instance();

protected:
	void unmap( QString mappings );

	struct MapInfo {
		MapInfo() : refs(0) {}
		Mapping mapping;
		int refs;
	};
	QHash<QString,MapInfo> mCurrentMappings;
	friend class MappingRef;
};

class Win32ShareManager::MappingRef
{
public:
	MappingRef();
	MappingRef( const MappingRef & );
	~MappingRef();

	MappingRef & operator=(const MappingRef &);

	bool isValid() { return mRef; }
	void release();
protected:
	MappingRef( Win32ShareManager * manager, QAtomicInt * ref, QString mappings );

	QAtomicInt * mRef;
	Win32ShareManager * mManager;
	QString mMappings;
	friend class Win32ShareManager;
};

#endif // WIN32_SHARE_MANAGER_H
