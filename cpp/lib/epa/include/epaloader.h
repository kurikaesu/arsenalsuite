
/*
 *
 * Copyright 2008 Blur Studio Inc.
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
 * $Id: path.h 7214 2008-10-21 18:03:54Z newellm $
 */

#ifndef EPALOADER_H
#define EPALOADER_H

#include <qglobal.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qlist.h>
#include <QXmlStreamReader>
#include <QTreeWidgetItem>
#include <QProcess>
#include <qmap.h>

#ifdef __GNUC__
#define __get_loc__ (QString(__FILE__) + ":" + QString::number(__LINE__) + " " + __PRETTY_FUNCTION__)
#else
#define __get_loc__ (QString(__FILE__) + ":" + QString::number(__LINE__) + " " + __func__)
#endif
#define LOG( m ) Log( m,  __get_loc__ )

void Log( const QString & message, const QString & file = QString() );

class EpaLoader
{
public:
	EpaLoader(const QString &, bool useExistingEnvironment=true);
	~EpaLoader();

	void mergeEnvironment( const QStringList & );
	QProcess * launch(const QStringList &);
	QProcess * shell(const QStringList &);

	void setRoot( const QString & );
	void setArch( const QString & );
	void setSep( const QString & );
	void setTreeWidgetItem( QTreeWidgetItem * );
	void setOverride( const QMap<QString, QString> & );
	void setArgs( const QStringList & );

	QMap<QString, QString> environment() const;
	QStringList envList() const;

private:
// options from client
	QString mRoot;
	QString mArch;
	QString mSep;
	QMap<QString, QString> mOverride;
	QStringList mCmdArgs;

//gui stuff
	QTreeWidgetItem * mTreeWidgetItem;
	QString mPresetLabel;
	void addTreeItem( const QString &, const QString &, QTreeWidgetItem * );

// xml parsing stuff
	void loadXml(const QString &, QTreeWidgetItem * parent=0);
	QString mKeyString;
	QString mValueString;
	QString mCurrentTag;;
	QStringList mLoaded;
	QString expandText(const QString &);

// stuff grokked via XML
	QMap<QString, QString> mEnv;
	QMap<QPair<QString, QTreeWidgetItem *>, QTreeWidgetItem *> mTreeItemMap;
	QString mWorkingDir;
	QString mCmdLine;

};

#endif // EPALOADER_H

