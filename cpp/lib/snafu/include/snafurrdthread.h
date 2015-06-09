/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Snafu.
 *
 * Snafu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Snafu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Snafu; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: snafurrdthread.h 5408 2007-12-18 00:13:49Z brobison $
 */

#ifndef SNAFURRDTHREAD_H
#define SNAFURRDTHREAD_H

#include <QThread>
#include <QProcess>
#include <QImage>
#include <QString>

#include "graph.h"
#include "graphds.h"
#include "graphrelationship.h"

class SnafuRrdThread : public QThread
{
	Q_OBJECT

public:
	SnafuRrdThread(Graph graph, QObject * parent);
	void run();
	QImage graphImage();

	void setRrdBin( const QString & );

public slots:
	void processFinished(int,int);

signals:
	void graphReady();

private:
	QProcess * mProcess;
	QImage mGraphImage;
	Graph mGraph;
	GraphDsList mDsList;
	QStringList mRrdArgs;
	QString mRrdBin;
	QStringList mColorScheme;

	void genArgs();
	void rrdHeader();

};

#endif

