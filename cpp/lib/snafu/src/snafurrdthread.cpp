
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
 * $Id: snafurrdthread.cpp 5408 2007-12-18 00:13:49Z brobison $
 */

#include "snafurrdthread.h"
#include "graph.h"
#include "graphds.h"
#include "host.h"

SnafuRrdThread::SnafuRrdThread(Graph graph, QObject * parent) : QThread(parent), mGraph(graph)
{
}

void SnafuRrdThread::run()
{
	mColorScheme += "#00B0E0";//, fillcolour=>"#003050" },
	mColorScheme += "#E00000";//, fillcolour=>"#500000" },
	mColorScheme += "#E0E000";//, fillcolour=>"#877700" },
	mColorScheme += "#00E000";//, fillcolour=>"#005000" },
	mColorScheme += "#0000D0";
	mColorScheme += "#996600";
	mColorScheme += "#66CCFF";
	mColorScheme += "#666666";
	mColorScheme += "#339933";
	mColorScheme += "#3366cc";
	mColorScheme += "#ffcc99";

  mRrdBin = "/usr/bin/rrdtool";
	mRrdArgs = QStringList();
	mRrdArgs += "graph";
	mGraphImage = QImage();
	genArgs();
	rrdHeader();
	qWarning("going to run" + mRrdArgs.join(" ").toAscii());

	connect( mProcess, SIGNAL(finished(int,int)), this, SLOT(processFinished(int,int)) );
	mProcess->start( mRrdBin, mRrdArgs );

	exec();
}

void SnafuRrdThread::processFinished(int exitCode, int exitStatus)
{
	mGraphImage.loadFromData( mProcess->readAllStandardOutput() );
  emit graphReady();
	exit();
}

QImage SnafuRrdThread::graphImage()
{
	return mGraphImage;
}

void SnafuRrdThread::setRrdBin( const QString & binPath )
{
	mRrdBin = binPath;
}

void SnafuRrdThread::genArgs()
{
	// if there are duplicate varnames, prepend hostname
	bool dupes = false;
	QMap<QString, bool> seen;
	mDsList = mGraph.graphRelationships().graphDses();

	foreach( GraphDs ds, mDsList )
	{
		if( seen[ds.varname()] )
			dupes = true;

		seen[ds.varname()] = true;;
	}

	if( dupes )
		foreach( GraphDs ds, mDsList )
			ds.setVarname( ds.host().name() + "_" + ds.varname() );
	
	int field_count = 1;
	foreach ( GraphDs ds, mDsList.sorted("varname") )
	{
		mRrdArgs += "DEF:g"+ds.fieldName()+"="+ds.fileName()+":42:AVERAGE";
		mRrdArgs += "DEF:i"+ds.fieldName()+"="+ds.fileName()+":42:MIN";
		mRrdArgs += "DEF:a"+ds.fieldName()+"="+ds.fileName()+":42:MAX";

		if( ds.cdef().isEmpty() )
		{
			mRrdArgs += "CDEF:c"+ds.fieldName()+"=g"+ds.fieldName();
		}
		else
		{
			mRrdArgs += "CDEF:g"+ds.fieldName()+"=g"+ds.cdef();
			mRrdArgs += "CDEF:i"+ds.fieldName()+"=i"+ds.cdef();
			mRrdArgs += "CDEF:a"+ds.fieldName()+"=a"+ds.cdef();
			mRrdArgs += "CDEF:c"+ds.fieldName()+"=g"+ds.fieldName();
		}

		if( ds.negative() )
		{
			mRrdArgs += "CDEF:ineg"+ds.fieldName()+"=i"+ds.fieldName()+",-1.*";
			mRrdArgs += "CDEF:aneg"+ds.fieldName()+"=a"+ds.fieldName()+",-1.*";
			mRrdArgs += "CDEF:gneg"+ds.fieldName()+"=g"+ds.fieldName()+",-1.*";
			mRrdArgs += "CDEF:cneg"+ds.fieldName()+"=g"+ds.fieldName();
		}

		QString color = mColorScheme[ field_count % mColorScheme.size() ];
		QString lineType = "LINE1:g" + ds.fieldName();
		if( mGraph.stack() && mGraph.graphMax() && field_count == 1 )
			lineType = "AREA:a" + ds.fieldName();
		else if (mGraph.stack() && field_count == 1)
		  lineType = "AREA:g" + ds.fieldName();
		else if (mGraph.stack() && mGraph.graphMax())
			lineType = "STACK:a" + ds.fieldName();
		else if (mGraph.stack())
		  lineType = "STACK:g" + ds.fieldName();
		else if (mGraph.graphMax())
		  lineType = "AREA:a" + ds.fieldName();// + color["fill"];

		mRrdArgs += lineType + color + ":" + ds.varname();

		mRrdArgs += "COMMENT: Cur:";
		mRrdArgs += "GPRINT:"+ds.fieldName()+"LAST:%6.2lf%s";
		mRrdArgs += "COMMENT: Min:";
		mRrdArgs += "GPRINT:"+ds.fieldName()+"MIN:%6.2lf%s";
		mRrdArgs += "COMMENT: Avg:";
		mRrdArgs += "GPRINT:"+ds.fieldName()+"AVERAGE:%6.2lf%s";
		mRrdArgs += "COMMENT: Max:";
		mRrdArgs += "GPRINT:"+ds.fieldName()+"MAX:%6.2lf%s";

		field_count++;
	}
}

/*
int max_label_length() {
	int ret = 0;
	foreach( GraphDs ds, mGraph.ds() )
		if( ds.varname().length() > ret )
			ret = ds.varname().length();
	return ret;
}
*/

void SnafuRrdThread::rrdHeader() {
	mRrdArgs += "-";

	//mRrdArgs += "--title";
	//mRrdArgs += mGraph.title();

	mRrdArgs += "--start";
	mRrdArgs += mGraph.period();

/*
	if( !mGraph.periodEnd().isEmpty() )
	{
		mRrdArgs += "--end";
		mRrdArgs += mGraph.periodEnd();
	}
	*/

	mRrdArgs += "--vertical-label";
	mRrdArgs += mGraph.vlabel();
	mRrdArgs += "--height";
	mRrdArgs += QString::number(mGraph.height());
	mRrdArgs += "--width";
	mRrdArgs += QString::number(mGraph.width());

	mRrdArgs += "--upper-limit";
	mRrdArgs += QString::number(mGraph.upperLimit());
	mRrdArgs += "--lower-limit";
	mRrdArgs += QString::number(mGraph.lowerLimit());

	if( mGraph.upperLimit() > 0 || mGraph.lowerLimit() > 0 )
		mRrdArgs += "--rigid";

	mRrdArgs += "--imgformat";
	mRrdArgs += "--PNG";

	mRrdArgs += "--color";
	mRrdArgs += "BACK#000000";
	mRrdArgs += "--color";
	mRrdArgs += "CANVAS#170047";
	mRrdArgs += "--color";
	mRrdArgs += "SHADEA#505050";
	mRrdArgs += "--color";
	mRrdArgs += "SHADEB#D0D0D0";
	mRrdArgs += "--color";
	mRrdArgs += "GRID#505050";
	mRrdArgs += "--color";
	mRrdArgs += "MGRID#909090";
	mRrdArgs += "--color";
	mRrdArgs += "FONT#D0D0D0";
}


