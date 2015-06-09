
/*
 *
 * Copyright 2012 Blur Studio Inc.
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

#ifndef GRAPHITE_H
#define GRAPHITE_H

#include <qdatetime.h>
#include <qstring.h>

#include "blurqt.h"

/**
 *  Path refers to a graphite path consisting of Element[.Element]+
 *  Path can contain %(user) which will be replace by the current username
 *  and %(host) which will be replaced by the current hostname.
 *  Values will be written from a worker thread so this function 
 *  will return immediately.
 **/
STONE_EXPORT void graphiteRecord( const QString & path, double value, const QDateTime & timestamp = QDateTime::currentDateTime() );

#endif // GRAPHITE_H
