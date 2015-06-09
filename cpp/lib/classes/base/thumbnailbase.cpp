/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef COMMIT_CODE

#include <qdatetime.h>
#include <qfile.h>

#include "path.h"
#include "element.h"
#include "thumbnail.h"
//#include "thumbnailloader.h"
#include "thumbnail.h"
#include "element.h"

QString Thumbnail::filePath() const
{
	QString ep = element().path();
	if( ep.isEmpty() )
		return "";
	if( !Path( ep + "thumbnail.png" ).exists() && Path( ep + "_Attachments/thumbnail.png" ).exists() )
		return ep + "_Attachments/thumbnail.png";
	return ep + "thumbnail.png";
}

#endif

