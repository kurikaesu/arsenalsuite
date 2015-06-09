
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

#ifndef PHONON_IMAGE_SEQUENCE_PROVIDER
#define PHONON_IMAGE_SEQUENCE_PROVIDER

void registerPhononImageSequenceProviderPlugin();

#ifdef USE_PHONON

#include "imagesequenceprovider.h"
#include <phonon/mediaobject.h>
#include <phonon/videowidget.h>

class PhononImageSequenceProvider : public ImageSequenceProvider
{
public:
	PhononImageSequenceProvider( const QString & path, QObject * parent = 0 );
	virtual ~PhononImageSequenceProvider();

	bool isOpen();
	virtual QString path();
	virtual int frameStart();
	virtual int frameEnd();

	virtual QImage image( int frameNumber );
	virtual ImageStatus status( int frameNumber );

	Phonon::MediaObject * mMedia;
	Phonon::VideoWidget * mVideo;

protected:
	class Private;
	Private * d;

	QString mFilePath;
};

class PhononImageSequenceProviderPlugin : public ImageSequenceProviderPlugin
{
public:
	PhononImageSequenceProviderPlugin();
	virtual ~PhononImageSequenceProviderPlugin();

	virtual QStringList fileExtensions();

	virtual bool supportsFormat( const QString & fileName = QString() );

	virtual ImageSequenceProvider * createProvider( const QString & fileName );
protected:
	PhononImageSequenceProvider * mProvider;
};

#endif // USE_PHONON

#endif // PHONON_IMAGE_SEQUENCE_PROVIDER

