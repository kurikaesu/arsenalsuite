
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
 * $Id: glutil.h 7547 2009-01-23 20:43:01Z newellm $
 */

#ifndef GL_UTIL_H
#define GL_UTIL_H

#include <qgl.h>

#include "stonegui.h"

class QImage;
class QGLContext;

STONEGUI_EXPORT bool glIsExtensionSupported( const QGLContext * context, const QString & extension );

class STONEGUI_EXPORT GLTexture
{
public:
	GLTexture( QGLContext * context, const QImage & image );
	~GLTexture();

	QGLContext * context;
	GLuint handle;
	// Either GL_TEXTURE_RECTANGLE_NV or GL_TEXTURE_2D
	GLuint textureMode;
	float umax, vmax, aspect;
	int width, height;
private:
	// Disable copy
	GLTexture( const GLTexture &  ){}
	void loadTexture( const QImage & qimage );
};

#endif // GL_UTIL_H

