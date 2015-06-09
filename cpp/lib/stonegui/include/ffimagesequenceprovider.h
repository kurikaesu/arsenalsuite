
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
 * $Id: ffimagesequenceprovider.h 5411 2007-12-18 01:03:08Z brobison $
 */

#ifndef FFMPEG_IMAGE_SEQUENCE_PROVIDER
#define FFMPEG_IMAGE_SEQUENCE_PROVIDER

void registerFFImageSequenceProviderPlugin();

#ifdef USE_FFMPEG

#include "imagesequenceprovider.h"

class FFImageSequenceProvider : public ImageSequenceProvider
{
public:
	FFImageSequenceProvider( const QString & path, QObject * parent = 0 );
	virtual ~FFImageSequenceProvider();

	bool isOpen();
	virtual QString path();
	virtual int frameStart();
	virtual int frameEnd();

	virtual QImage image( int frameNumber );
	virtual ImageStatus status( int frameNumber );

protected:
	class Private;
	Private * d;
	QString mFilePath;
};


class FFImageSequenceProviderPlugin : public ImageSequenceProviderPlugin
{
public:
	FFImageSequenceProviderPlugin();
	virtual ~FFImageSequenceProviderPlugin();

	virtual QStringList fileExtensions();

	virtual bool supportsFormat( const QString & fileName = QString() );

	virtual ImageSequenceProvider * createProvider( const QString & fileName );
};

#endif // USE_FFMPEG

#endif // FFMPGEG_IMAGE_SEQUENCE_PROVIDER

