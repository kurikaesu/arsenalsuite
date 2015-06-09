
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
 * $Id: glutil.cpp 7547 2009-01-23 20:43:01Z newellm $
 */

#include <qstring.h>
#include <qstringlist.h>
#include <qimage.h>
#include <QGLWidget>
#include <QGLContext>
#include <QGLFormat>

#include "glutil.h"

#ifndef _MSC_VER
#include "GL/glext.h"
#endif

bool glIsExtensionSupported( QGLContext * context, const QString & extension )
{
	context->makeCurrent();
	return QString::fromLatin1( (char*)glGetString( GL_EXTENSIONS ) ).split( " " ).contains( extension );
}

// 
GLuint textureMode( QGLContext * context )
{
	static GLuint sTextureMode;
	static bool sTextureModeInit = false;
	if( !sTextureModeInit ) {
		#ifdef GL_NV_texture_rectangle
			bool useGL_NV_texture_rectangle = glIsExtensionSupported( context, "GL_NV_texture_rectangle" );
			if( useGL_NV_texture_rectangle ){
				sTextureMode = GL_TEXTURE_RECTANGLE_NV;
			} else
		#endif // GL_NV_texture_rectangle
			{
				qWarning("NOT USING GL_TEXTURE_RECTANGLE_NV");
				sTextureMode = GL_TEXTURE_2D;
			
				// Set up the texturing params
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP);
			
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
		sTextureModeInit = true;
	}
	return sTextureMode;
}

GLTexture::GLTexture( QGLContext * ctx, const QImage & image )
: context( ctx )
{
	loadTexture( image );
}

GLTexture::~GLTexture()
{
	context->makeCurrent();
	glDeleteTextures( 1, &handle );
}

void GLTexture::loadTexture( const QImage & qimage )
{
	context->makeCurrent();

	QImage image = QGLWidget::convertToGLFormat( qimage );
	int w = image.width(), h = image.height();
	uchar * bits = image.bits();
	GLenum format = GL_RGBA;

	aspect = w/(float)h;
	width = w;
	height = h;
	textureMode = ::textureMode(context);

	// Enable texturing
	glEnable( textureMode );

	// Generate a texture object
	glGenTextures( 1, &handle );

	// Make it the current texture
	glBindTexture( textureMode, handle );

	/* The GL_NV_texture_rectangle OpenGL extension allows
	* us to load 2D textures with any dimensions.  When using
	* these textures, we must use GL_TEXTURE_RECTANGLE_NV instead
	* of GL_TEXTURE_2D, and also the u and v coordinates go from
	* 0 to width, and 0 to height, instead of 0-1.0 */
#ifdef GL_NV_texture_rectangle
	if( textureMode == GL_TEXTURE_RECTANGLE_NV ){
		umax = w;
		vmax = h;

		glTexImage2D( GL_TEXTURE_RECTANGLE_NV, 0, 4, w, h, 0, format, GL_UNSIGNED_BYTE, bits );

	}else
#endif // GL_NV_texture_rectangle

	{
		/* Since we have to use a power of two texture, we need to find
		* the smallest power of two texture that the image will fit into
		* then we need to know what part(u and v, from 0-1.0) of the texture that the image covers
		* and use that for our u and v texture coords */
		int tw = 2, th = 2;
		while( tw < w ) tw*=2;
		while( th < h ) th*=2;
	
		// Get the portion of the texture that the image will fill
		// the -1.0/(2*tw) keeps the OpenGL blending from blending with
		// part of the texture that doesn't contain the image
		umax = w/(float)tw - 1.0/(2*tw);
		vmax = h/(float)th - 1.0/(2*th);
	
		// Create the texture, but pass 0 as the image data
		glTexImage2D( GL_TEXTURE_2D, 0, 4, tw, th, 0, format, GL_UNSIGNED_BYTE, 0 );
		// Now fill in the part of the texture that we are going to use
		glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, w, h, format, GL_UNSIGNED_BYTE, bits );
	
	}
}

