
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#include "connection.h"
#include "database.h"
#include "interval.h"
#include "record.h"
#include "recordimp.h"
#include "table.h"
#include "tableschema.h"

static int sRecordCount = 0;

static QVariant sNullVariant;

namespace Stone {

RecordImp * Record::current( bool read ) const
{
	if( !mImp ) return 0;
	
	RecordImp * ret = mImp;
	ChangeSet cs = ChangeSet::current();
	
	// If there's only one version, we can just decide to return it or an empty imp
	if( !mImp->hasVersions() ) {
		// If the changeset attached is not currently visible, then we return the empty imp
		if( mImp->mChangeSet().isValid() && !cs.isVisible(mImp->mChangeSet) )
			ret = mImp->mTable->emptyImp();
	} else if( cs.isValid() || mImp->mChangeSet().isValid() ) {
		bool readPristine = false;
		if( read ) {
			ChangeSet::ReadMode readMode = cs.readMode();
			if( readMode == ChangeSet::Read_Direct || (readMode == ChangeSet::Read_Pristine && !(mImp->mState & RecordImp::MODIFIED)) )
				return mImp;
			readPristine = !cs.isValid() || cs.readMode() == ChangeSet::Read_Pristine;
		}
		ret = mImp->version(readPristine ? ChangeSet() : cs);
		if( ret->mChangeSet().isValid() && (!cs.isValid() || !cs.isVisible(ret->mChangeSet)) )
			ret = mImp->mTable->emptyImp();
		
	} else {
		// If we are holding a reference to a non-changeset modified copy that is already committed, then we need
		// to grab the pristine copy instead
		if( ret->mState & RecordImp::HOLDS_OLD_VALUES && ret->mParent )
			ret = ret->mParent;
	}
	if( !read && ret != mImp ) {
		ret->ref();
		mImp->deref();
		mImp = ret;
	}
	return ret;
}

// static
int Record::totalRecordCount()
{ return sRecordCount; }

int Record::totalRecordImpCount()
{ return RecordImp::totalCount(); }

Record::Record( RecordImp * imp, bool )
: mImp( imp )
{
	sRecordCount++;
	if( mImp )
		mImp->ref();
}

Record::Record( Table * table )
: mImp( 0 )
{
	sRecordCount++;
	if( table ) {
		mImp = table->emptyImp();
		mImp->ref();
	}
}

Record::Record( const Record & r )
: mImp( r.mImp )
{
	sRecordCount++;
	if( mImp )
		mImp->ref();
}

Record::~Record()
{
	sRecordCount--;
	if( mImp )
		mImp->deref();
}

Record Record::getVersion( const ChangeSet & cs ) const
{
	if( mImp ) {
		RecordImp * v = mImp->version(cs);
		if( v->mChangeSet != cs )
			v = 0;
		return Record(v);
	}
	return *this;
}

Record Record::pristine() const
{
	return mImp ? Record(mImp->version(ChangeSet())) : *this;
}

Record Record::parent() const
{
	return mImp->mParent ? Record(mImp->mParent) : pristine();
}

Record & Record::operator=( const Record & r )
{
	if( mImp != r.mImp ) {
		if( mImp ) mImp->deref();
		mImp = r.mImp;
		if( mImp ) mImp->ref();
	}
	return *this;
}

bool isChanged( const QString & a, const QString & b )
{
	bool ai = a.isNull() || a.isEmpty();
	bool bi = b.isNull() || b.isEmpty();
	if( ai && bi )
		return false;
	return (a!=b);
}

bool Record::operator==( const Record & other ) const
{
	return mImp==other.mImp ||
		(
			(mImp && other.mImp)
			&& (mImp->mTable && mImp->key() && mImp->mTable == other.mImp->mTable && mImp->key() == other.mImp->key())
		);
}

bool Record::operator!=( const Record & other ) const
{
	return !operator==(other);
}

bool Record::operator <( const Record & other ) const
{
	if( operator==(other) ) return false;
	if( mImp && other.mImp ) {
		if( mImp->mTable == other.mImp->mTable )
			return mImp->key() < other.mImp->key();
		return mImp->mTable < other.mImp->mTable;
	}
	return mImp < other.mImp;
}

bool Record::isRecord() const
{
	RecordImp * ri = current();
	return ri && (ri->mState & (RecordImp::COMMITTED | RecordImp::INSERT_PENDING)) && !(ri->mState & RecordImp::DELETE_PENDING);
}

ChangeSet Record::changeSet() const
{
	return mImp ? mImp->mChangeSet() : ChangeSet();
}

uint Record::generateKey() const
{
	if( !mImp ) return 0;
	uint k = current()->key();
	if( !k ) {
		current(true);
		Table * t = table();
		if( t ) {
			k = t->connection()->newPrimaryKey( t->schema() );
			mImp = mImp->setColumn( t->schema()->primaryKeyIndex(), k );
			t->keyIndex()->recordsIncoming(RecordList(*this));
		}
	}
	return k;
}

const QVariant & Record::getValue( const QString & column ) const
{
	RecordImp * ri = current();
	const QVariant & ret( ri ? ri->getValue( column ) : sNullVariant );
	if( ret.userType() == qMetaTypeId<Record>() ) {
		// We have to call getValue so we can return a const QVariant & of the key
		Record r = qvariant_cast<Record>(ret);
		Table * t = r.table();
		return t ? r.getValue(t->schema()->primaryKeyIndex()) : sNullVariant;
	}
	return ret;
}

Record & Record::setValue( const QString & column, const QVariant & value )
{
	if( !mImp ) return *this;
	if( value.userType() == qMetaTypeId<Record>() ) {
		setForeignKey( column, qvariant_cast<Record>(value) );
		return *this;
	}
	current(false);
	mImp = mImp->setValue( column, value );
	return *this;
}

const QVariant & Record::getValue( int column ) const
{
	RecordImp * ri = current();
	return ri ? ri->getColumn( column ) : sNullVariant;
}

const QVariant & Record::getValue( Field * f ) const
{
	RecordImp * ri = current();
	return ri ? ri->getColumn( f ) : sNullVariant;
}

Record & Record::setValue( int column, const QVariant & value )
{
	current(false);
	if( mImp )
		mImp = mImp->setColumn( column, value );
	return *this;
}

Record & Record::setValue( Field * f, const QVariant & value )
{
	current(false);
	if( mImp )
		mImp = mImp->setColumn( f, value );
	return *this;
}

Record Record::foreignKey( const QString & column ) const
{
	do {
		Table * t = table();
		if( !t ) break;
		Field * f = t->schema()->field( column );
		if( !f ) break;
		return foreignKey( f );
	} while( 0 );
	return Record();
}

Record Record::foreignKey( int column ) const
{
	do {
		Table * t = table();
		if( !t ) break;
		Field * f = t->schema()->field( column );
		if( !f ) break;
		return foreignKey( f );
	} while( 0 );
	return Record();
}

Record Record::foreignKey( Field * f, int lookupMode ) const
{
	if( !f->flag( Field::ForeignKey ) || !mImp ) return Record();
	RecordImp * ri = current();
	QVariant val = ri->getColumn( f );
	Table * t = table();
	Table * fkt = t->database()->tableFromSchema( f->foreignKeyTable() );
	if( !fkt ) return Record();
	if( val.canConvert( QVariant::LongLong ) ) {
		qlonglong ll = val.toLongLong();
		return fkt->record( ll, lookupMode );
	}
	if( val.userType() == qMetaTypeId<Record>() ) {
		Record r = qvariant_cast<Record>(val);
		if( !(lookupMode & Index::UseCache) )
			r.reload();
		return r;
	}
	return Record();
}

Record & Record::setForeignKey( int column, const Record & other )
{
	Table * t = table();
	if( !t ) return *this;
	Field * f = t->schema()->field( column );
	if( !f ) return *this;
	if( f->type() == Field::UInt || f->type() == Field::ULongLong || f->type() == Field::Int ) {
		setValue( f, other.isValid() ? (other.key() ? other.key() : qVariantFromValue<Record>(other)) : QVariant() );
	}
	return *this;
}

Record & Record::setForeignKey( const QString & column, const Record & other )
{
	Table * t = table();
	if( !t ) return *this;
	Field * f = t->schema()->field( column );
	if( !f ) return *this;
	if( f->type() == Field::UInt || f->type() == Field::ULongLong || f->type() == Field::Int ) {
		setValue( f, other.isValid() ? (other.key() ? other.key() : qVariantFromValue<Record>(other)) : QVariant() );
	}
	return *this;
}

Record & Record::setForeignKey( Field * f, const Record & other )
{
	if( f->type() == Field::UInt || f->type() == Field::ULongLong || f->type() == Field::Int ) {
		setValue( f, other.isValid() ? (other.key() ? other.key() : qVariantFromValue<Record>(other)) : QVariant() );
	}
	return *this;
}

Record & Record::setColumnLiteral( const QString & column, const QString & literal )
{
	if( !mImp ) return *this;
	Table * t = mImp->table();
	if( t ) {
		int fp = t->schema()->fieldPos( column );
		current(false);
		mImp = mImp->setColumnLiteral( fp, true );
		mImp->setColumn( fp, QVariant( literal ) );
	}
	return *this;
}

QString Record::columnLiteral( const QString & column ) const
{
	if( mImp ) {
		RecordImp * ri = current();
		Table * t = ri->table();
		if( t ) {
			int fp = t->schema()->fieldPos( column );
			if( ri->isColumnLiteral( fp ) )
				return ri->getColumn( fp ).toString();
		}
	}
	return QString::null;
}

QString Record::debug() const
{
	return QString("%1(@%2 *%3 %4 %5 %6)")
		.arg(	table()->schema()->className())
		.arg(	QString::number((qulonglong)mImp,16))
		.arg(	key())
		.arg(	displayName())
		.arg(	stateString())
		.arg(	mImp->mChangeSet.pointerString());
}

QString Record::stateString() const
{
	QStringList ret;
	RecordImp * ri = mImp;//current();
	if( ri ) {
		if( ri->mState & RecordImp::COMMITTED )
			ret += "Committed";
		if( ri->mState & RecordImp::MODIFIED )
			ret += "Modified";
		if( ri->mState & RecordImp::DELETED )
			ret += "Deleted";
		if( ri->mState & RecordImp::EMPTY_SHARED )
			ret += "Empty/Shared";
		if( ri->mState & RecordImp::COMMIT_ALL_FIELDS )
			ret += "Commit All Fields";
		if( ri->mState & RecordImp::DISCARDED )
			ret += "Discarded";
		if( ri->mState & RecordImp::INSERT_PENDING )
			ret += "Insert Pending";
		if( ri->mState & RecordImp::UPDATE_PENDING )
			ret += "Update Pending";
		if( ri->mState & RecordImp::DELETE_PENDING )
			ret += "Delete Pending";
		if( ri->mState & RecordImp::MODIFIED_SINCE_QUEUED )
			ret += "Modified Since Queued";
		
	}
	return ret.join(", ");
}

	
QString Record::displayName() const
{
	Table * t = table();
	if( t ) {
		TableSchema * ts = t->schema();
		Field * field = 0;
		foreach( Field * f, ts->fields() )
			if( f->flag( Field::TableDisplayName ) ) {
				field = f;
				break;
			}
		if( !field ) field = ts->field("name",true);
		if( !field ) field = ts->field("displayname",true);
		if( !field ) field = ts->field(ts->className(),true);
		if( !field ) field = ts->field(ts->tableName(),true);
		
		if( field ) return getValue( field->pos() ).toString();
	}
	return QString();
}

static QString tabAlign( const QStringList & keylist, const QStringList & valueList, int tabSize = 8 )
{
	int max_w = 0;
	foreach( QString s, keylist )
		max_w = qMax( s.size(), max_w );
	max_w = qMin( tabSize * 6, max_w + tabSize - 1 );
	max_w /= tabSize;
	QStringList ret;
	QString tab("\t\t\t\t\t\t");
	for( int i = 0; i < qMin(keylist.size(),valueList.size()); i++ ) {
		QString k = keylist[i];
		k += tab.left( qMax(0,max_w - k.size() / tabSize) );
		ret << (k + ": " + valueList[i]);
	}
	return ret.join("\n");
}

QString Record::dump() const
{
	RecordImp * ri = current();
	Table * t = table();
	QStringList keys, values;
	keys << (t ? t->tableName() : QString("Invalid Record")) + " @ 0x" + QString::number((quint64)mImp,16);
	values << stateString();
	if( t ) {
		QStringList fieldNames = t->schema()->fieldNames();
		fieldNames.sort();
		foreach( QString fieldName, fieldNames ) {
			Field * f = t->schema()->field(fieldName);
			QVariant v = ri->isColumnSelected(f->pos()) ? ri->getColumn(f) : "NOT SELECTED";
			keys << f->name();
			if( f->type() == Field::Interval )
				values << v.value<Interval>().toString();
			else if( v.userType() == qMetaTypeId<Record>() && f->flag( Field::ForeignKey ) ) {
				Record r = v.value<Record>();
				if( r.isRecord() )
					values << QString::number(r.key());
				else
					values << "<Uncommitted " + r.table()->schema()->className() + " @ 0x" + QString::number((quint64)r.mImp,16) + ">";
			} else
				values << v.toString();
		}
	}
	return tabAlign(keys,values);
}

QString Record::changeString() const
{
	QString changeString;
	Record orig;
	if( mImp ) {
		RecordImp * ri = current();
		Table * t = table();
		QStringList fieldNames = t->schema()->fieldNames();
		fieldNames.sort();
		foreach( QString fieldName, fieldNames ) {
			Field * f = t->schema()->field(fieldName);
			if( ri->isColumnModified(f->pos()) ) {
				if( !orig.isRecord() ) {
					orig = t->record( key() );
					if( !orig.isRecord() ) break;
				}
				if( !changeString.isEmpty() )
					changeString += "\n";
				QString oldString = f->dbPrepare(orig.getValue(f->pos())).toString();
				QString newString = f->dbPrepare(ri->getColumn(f->pos())).toString();
				if( oldString != newString ) {
					if( newString.size() > 1024 ) {
						newString.truncate(1024);
						newString += "...";
					}
					changeString += fieldName + " changed: " + oldString + "  to  " + newString;
				}
			}
		}
	}
	return changeString;
}

bool Record::isUpdated( Field * f ) const
{
	RecordImp * ri = current();
	if( !f && ri )
		return ri->mChangeSet().isValid() ? (ri->mState & RecordImp::MODIFIED_SINCE_QUEUED) : (ri->mState & RecordImp::MODIFIED);
	return ri->isColumnModified(f->pos());
}

void Record::selectFields( FieldList fields, bool refreshExisting )
{
	RecordImp * ri = current();
	if( isRecord() ) {
		if( !refreshExisting ) {
			if( fields.size() )
				fields = fields & ri->notSelectedColumns();
			else
				fields = ri->notSelectedColumns();
		}
		if( fields.size() )
			table()->selectFields( RecordList(*this), fields );
	}
}

Record & Record::reload( bool lockForUpdate )
{
	if( isRecord() ) {
		Record r;
		if( lockForUpdate ) {
			// isRecord() check above guarantees table() and schema() are valid pointers
			RecordList rl = table()->select( table()->schema()->primaryKey() + "=? FOR UPDATE", VarList() << key(), false, true );
			if( rl.size() == 1 )
				r = rl[0];
		} else
			r = imp()->table()->record( mImp->key(), true, false, true );
		if( r.imp() != imp() ) {
			mImp->deref();
			mImp = r.imp();
			if( mImp )
				mImp->ref();
		}
	}
	return *this;
}

int Record::remove()
{
	if( isRecord() ) {
		ChangeSet cs = ChangeSet::current();
		if( cs.isValid() ) {
			RecordImp * ret = mImp->copy();
			mImp->deref();
			mImp = ret;
			mImp->ref();
			cs.queue( ChangeSet::Change_Remove, *this );
			return 1;
		} else
			return table()->remove( *this );
	}
	return 0;
}

Record & Record::commit()
{
	if( isValid() )
		mImp = current(false)->commit();
	return *this;
}

Record Record::copy( Table * destTable ) const
{
	RecordImp * ri = current();
	if( ri && ri->table() ) {
		if( !destTable ) destTable = ri->table();
		if( destTable == ri->table() || destTable->schema() == ri->table()->schema() ) {
			RecordImp * imp = ri->copy(false);
			// Make this a record with no primary key or COMMITTED flag
			imp->mState = RecordImp::MODIFIED;
			imp->setColumn( imp->table()->schema()->primaryKeyIndex(), QVariant( 0 ) );
			for( int i=ri->table()->schema()->fieldCount()-1; i >= 0; i-- )
				imp->setColumnModified( i, true );
			return Record( imp, false );
		} else {
			TableSchema * src = ri->table()->schema(), * dest = destTable->schema();
			Record ret(destTable);
			foreach( Field * f, src->columns() ) {
				Field * destField = dest->field( f->name(), true );
				if( !destField ) destField = dest->field( f->methodName(), true );
				if( !destField ) continue;
				ret.setValue( destField, getValue( f ) );
			}
			return ret;
		}
	} else return Record(destTable);
}

void Record::checkImpType(TableSchema * ts)
{
	// Check against table, not schema, to make sure they belong to the same database
	if( mImp && (!mImp->table() || !mImp->table()->schema() || !ts->isDescendant( mImp->table()->schema() ) ) ) {
		mImp->deref();
		mImp = 0;
	}
	
	if( !mImp ) {
		mImp = ts->table()->emptyImp();
		mImp->ref();
	}
}

} // namespace
