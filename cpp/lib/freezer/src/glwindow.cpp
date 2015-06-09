/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Blur; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* $Author: newellm $
 * $LastChangedDate: 2012-07-26 12:02:28 -0700 (Thu, 26 Jul 2012) $
 * $Rev: 13450 $
 * $HeadURL: svn://newellm@svn.blur.com/blur/trunk/cpp/lib/assfreezer/src/glwindow.cpp $
 */

#include <qimage.h>
#include <qgl.h>
#include <qtimer.h>

#include <GL/glu.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif // GL_CLAMP_TO_EDGE

#include "glwindow.h"
#include "afcommon.h"
#include "GL/glu.h"

const char * FRAGMENT_SHADER =
"struct Output"
"{"
"	float3 color : COLOR;"
"};"
"Output main("
"	float4 pos : TEXCOORD0,"
"	uniform float4 colorClamp,"
"	uniform samplerRECT image )"
"{"
"	Output OUT;"
"	float4 col = texRECT( image, pos );"
"	col = col * colorClamp;"
"	OUT.color = col.www + col.xyz;"
"	return OUT;"
"}";

void cgErrorCallback(void){
#ifdef COMPILE_CG
	CGerror LastError = cgGetError();
	if(LastError)
		printf("%s\n\n", cgGetErrorString(LastError));
#endif
}

// Must have a context
bool isExtensionSupported( const QString & extension )
{
	return QString((char*)glGetString( GL_EXTENSIONS )).split(" ").contains( extension );
}

// Must have a context
void glVersion( int & major, int & minor )
{
	static int _major = -1, _minor = -1;
	if( major == -1 ) {
		QStringList glVersionStrings = QString( (const char *)glGetString(GL_VERSION) ).split('.');
		bool okMajor = false, okMinor = false;
		if( glVersionStrings.size() >= 2 ) {
			_major = glVersionStrings[0].toInt(&okMajor);
			_minor = glVersionStrings[1].left(1).toInt(&okMinor);
		}
		if( !okMajor || !okMinor )
			LOG_1( "Error Getting OpenGL Version, GL_VERSION string was: " + QString( (const char *)glGetString(GL_VERSION) ) );
		else
			LOG_3( "Got OpenGL Version: " + QString::number(_major) + "." + QString::number(_minor) );
	}
	major = _major;
	minor = _minor;
}

// Must have a context
static bool checkGLError()
{
	GLenum errorCode = glGetError();
	if( errorCode != GL_NO_ERROR ) {
		const GLubyte * errorString = gluErrorString(errorCode);
		LOG_1( "Got OpenGL error: " + QString::fromLatin1( (const char *)errorString ) );
		return true;
	}
	return false;
}

GLWindow::GLWindow( QWidget * parent )
: QGLWidget(parent)
, mTextureMode( 0 )
, mInitialized( false )
, mUseCG( false )
, mTextureValid(false)
, mColorMode(AllChannel)
, mScaleMode(ScaleAlways)
, mScaleFactor(1.0)
, mUseGL_NV_texture_rectangle( false )
{
}

GLWindow::~GLWindow(){
	if( mUseCG ){
#ifdef COMPILE_CG
		cgDestroyProgram( cgProgram );
		cgDestroyContext( cgContext );
#endif
	}
}

void GLWindow::ensureInitialized()
{
	if( !mInitialized )
		glInit();
	else if( QGLContext::currentContext() != context() )
		makeCurrent();
}

void GLWindow::initializeGL()
{
	mInitialized = true;
	
	if( checkGLError() ) LOG_TRACE
	// Set up the rendering context, define display lists etc.:
	glClearColor( 8/256.0, 5/256.0, 76/256.0, 0.0 );
	glDisable(GL_DEPTH_TEST);

	if( checkGLError() ) LOG_TRACE

#ifdef COMPILE_CG
	if( mUseCG && cgGLIsProfileSupported( CG_PROFILE_FP20 ) ){
		//LOG_3("Using Cg");
		// This is the most basic fragment profile, should run on quite a few cards
		cgFragmentProfile = CG_PROFILE_FP20;
		cgSetErrorCallback(cgErrorCallback);
		cgContext = cgCreateContext();
		// TODO: Include the shader source in the executable
		cgProgram = cgCreateProgram( cgContext, CG_SOURCE, FRAGMENT_SHADER, cgFragmentProfile, 0, 0);
		cgGLLoadProgram( cgProgram );
		cgImageParam = cgGetNamedParameter( cgProgram, "image" );
		cgColorClampParam = cgGetNamedParameter( cgProgram, "colorClamp" );
		cgGLBindProgram( cgProgram );
	}else
#endif
	{
		//LOG_3("Using stock OpenGL");
		mUseCG = false;
	}

// GL_TEXTURE_ENV_MODE defaults to GL_MODULATE
	// GL_TEXTURE_ENV_COLOR defaults to (0,0,0,0)

#ifdef GL_NV_texture_rectangle
	mUseGL_NV_texture_rectangle = isExtensionSupported( "GL_NV_texture_rectangle" );
	if( checkGLError() ) LOG_TRACE
#endif // GL_NV_texture_rectangle

#ifdef GL_NV_texture_rectangle
	if( mUseGL_NV_texture_rectangle ){
		//LOG_5( "Using GL_TEXTURE_RECTANGLE_NV" );
		mTextureMode = GL_TEXTURE_RECTANGLE_NV;
		// Only try to use cg, if texture_rectangle is supported
		// 4/23/04 WHY?
		mUseCG = true;
	} else
#endif // GL_NV_texture_rectangle
	{
		//qWarning("NOT USING GL_TEXTURE_RECTANGLE_NV");
		mTextureMode = GL_TEXTURE_2D;
	
		// Set up the texturing params
		GLint wrapMode = GL_CLAMP;
		int glMajor, glMinor;
		glVersion(glMajor,glMinor);
		if( glMajor >= 1 && glMinor >=2 )
			wrapMode = GL_CLAMP_TO_EDGE;
		
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, wrapMode);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, wrapMode);
	
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
}

TexInfo GLWindow::loadImage( QImage * imgPtr )
{
	makeCurrent();
	QImage image = convertToGLFormat( *imgPtr );
	return loadImage( image.width(), image.height(), image.bits(), GL_RGBA );
}

TexInfo GLWindow::loadImage( int w, int h, uchar * bits, GLenum format )
{
	ensureInitialized();

	// We get an opengl error here, must be from qt as i'm checking after
	// any opengl calls that have been made before this first happens
	checkGLError();
	
	// Return value
	TexInfo texInfo;
	texInfo.aspect = w/(float)h;
	texInfo.width = w;
	texInfo.height = h;

	// Enable texturing
	glEnable( mTextureMode );

	// Generate a texture object
	glGenTextures( 1, &texInfo.handle );

	// Make it the current texture
	glBindTexture( mTextureMode, texInfo.handle );

	/* The GL_NV_texture_rectangle OpenGL extension allows
	* us to load 2D textures with any dimensions.  When using
	* these textures, we must use GL_TEXTURE_RECTANGLE_NV instead
	* of GL_TEXTURE_2D, and also the u and v coordinates go from
	* 0 to width, and 0 to height, instead of 0-1.0 */
	/* TODO: Detect if this extension is available at RUNTIME and compile time */
#ifdef GL_NV_texture_rectangle
	if( mUseGL_NV_texture_rectangle ){
		texInfo.umax = w;
		texInfo.vmax = h;

		glTexImage2D( GL_TEXTURE_RECTANGLE_NV, 0, 4, w, h, 0, format, GL_UNSIGNED_BYTE, bits );
		
		if( checkGLError() ) {
			LOG_TRACE
			glDeleteTextures( 1, &texInfo.handle );
			texInfo.handle = 0;
			return texInfo;
		}
#ifdef COMPILE_CG
		// Set the texture to be used by the shader program
		if( mUseCG )
			cgGLSetTextureParameter(cgImageParam, texInfo.handle);
#endif // COMPILE_CG

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
		texInfo.umax = w/(float)tw - 1.0/(2*tw);
		texInfo.vmax = h/(float)th - 1.0/(2*th);
	
		// Create the texture, but pass 0 as the image data
		glTexImage2D( GL_TEXTURE_2D, 0, 4, tw, th, 0, format, GL_UNSIGNED_BYTE, 0 );
		
		if( checkGLError() ) {
			LOG_TRACE
			glDeleteTextures( 1, &texInfo.handle );
			texInfo.handle = 0;
			return texInfo;
		}
		
		// Now fill in the part of the texture that we are going to use
		glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, w, h, format, GL_UNSIGNED_BYTE, bits );

		if( checkGLError() ) {
			LOG_TRACE
			glDeleteTextures( 1, &texInfo.handle );
			texInfo.handle = 0;
			return texInfo;
		}
	}
	return texInfo;
}

void GLWindow::deleteImage( const TexInfo & texInfo )
{
	makeCurrent();
	glDeleteTextures( 1, &texInfo.handle );
}

void GLWindow::showImage( TexInfo texInfo )
{
	if( texInfo.handle == 0 )
		return;
	
	mCurrentTexture = texInfo;
	mTextureValid = true;

	// Make it the current texture
	glEnable( mTextureMode );
	glBindTexture(mTextureMode, texInfo.handle);

#ifdef COMPILE_CG
	// Set the texture to be used by the shader program
	if( mUseCG )
		cgGLSetTextureParameter(cgImageParam, texInfo.handle);
#endif

	updateTextureCoords();
	updateGL();
}

void GLWindow::resizeGL( int w, int h )
{
	glViewport( 0, 0, w, h );

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( 0, w, 0, h, 0.0, 16.0 );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	updateTextureCoords();
}

void GLWindow::updateTextureCoords()
{
	int w = width(), h = height();
	float aspect = w/(float)h;
	
	bool scale = false;
	if( (mScaleMode & ScaleDown) && (mCurrentTexture.width > w || mCurrentTexture.height > h) )
		scale = true;

	if( (mScaleMode & ScaleUp) && mCurrentTexture.width < w && mCurrentTexture.height < h )
		scale = true;

	if( scale ) {
		if( aspect > mCurrentTexture.aspect ){
			tex_y = 0;
			tex_h = h;
			tex_w = mCurrentTexture.aspect * tex_h;
			tex_x = (w-tex_w)/2;
		}else{
			tex_x = 0;
			tex_w = w;
			tex_h = tex_w / mCurrentTexture.aspect;
			tex_y = (h-tex_h)/2;
		}
		mScaleFactor = tex_w / float(mCurrentTexture.width);
		emit scaleFactorChange( mScaleFactor );
	} else {
		int spare_x = (int)((w - mCurrentTexture.width * mScaleFactor) / 2);
		int spare_y = (int)((h - mCurrentTexture.height * mScaleFactor) / 2);
		tex_y = spare_y;
		tex_h = mCurrentTexture.height * mScaleFactor;
		tex_x = spare_x;
		tex_w = mCurrentTexture.width * mScaleFactor;
	}
}

void GLWindow::paintGL()
{
	glClearColor( 8/256.0, 5/256.0, 76/256.0, 0.0 );
	glClear( GL_COLOR_BUFFER_BIT );

	if( !mTextureValid )
		return;

	if( !mUseCG && (mColorMode==AlphaChannel) ){
		glDisable( mTextureMode );
		glDisable( GL_BLEND );
		glColor3f( 1.0, 1.0, 1.0 );
		glBegin( GL_QUADS );
		glVertex2f( tex_x, tex_y );
		glVertex2f( tex_x + tex_w, tex_y );
		glVertex2f( tex_x + tex_w, tex_y + tex_h );
		glVertex2f( tex_x, tex_y + tex_h );
		glEnd();

		glEnable( mTextureMode );
		glEnable( GL_BLEND );
		glBlendFunc( GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA );
		glColor3f( 0.0, 0.0, 0.0 );
	} else {
		glDisable( mTextureMode );
		glDisable( GL_BLEND );
		glColor3f( 1.0, 1.0, 1.0 );

		glEnable( mTextureMode );
		glEnable( GL_BLEND );
		glBlendFunc( GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA );
		glColor3f( 0.0, 0.0, 0.0 );

		float r = (mColorMode==RedChannel) ? 1.0 : 0.0;
		float g = (mColorMode==GreenChannel) ? 1.0 : 0.0;
		float b = (mColorMode==BlueChannel) ? 1.0 : 0.0;
		float a = (mColorMode==AlphaChannel) ? 1.0 : 0.0;

		if( mColorMode==AllChannel )
			r = g = b = 1.0;

#ifdef COMPILE_CG
		if( mUseCG ){
			cgGLSetParameter4f( cgColorClampParam, r, g, b, a );
			cgGLEnableProfile( cgFragmentProfile );
			//glColor4f( 1.0, 1.0, 1.0, 1.0 );
		}else
#endif
			glColor4f( r, g, b, a );
		glDisable(GL_BLEND);
	}

	glBindTexture(mTextureMode, mCurrentTexture.handle);

	glTexParameteri(mTextureMode,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // Linear Filtering
	glTexParameteri(mTextureMode,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // Linear Filtering

	glBegin( GL_QUADS );
	glTexCoord2f( 0.0, 0.0 );
	glVertex2f( tex_x, tex_y );
	glTexCoord2f( mCurrentTexture.umax, 0.0 );
	glVertex2f( tex_x + tex_w, tex_y );
	glTexCoord2f( mCurrentTexture.umax, mCurrentTexture.vmax );
	glVertex2f( tex_x + tex_w, tex_y + tex_h );
	glTexCoord2f( 0.0, mCurrentTexture.vmax );
	glVertex2f( tex_x, tex_y + tex_h );
	glEnd();
}

void GLWindow::setColorMode( int colorMode )
{
	mColorMode = colorMode;
	updateGL();
}

void GLWindow::setScaleMode( int scaleMode )
{
	mScaleMode = scaleMode;
	updateTextureCoords();
	updateGL();
}

int GLWindow::scaleMode() const
{
	return mScaleMode;
}

void GLWindow::setScaleFactor( float scaleFactor )
{
	mScaleFactor = scaleFactor;
	updateTextureCoords();
	updateGL();
	emit scaleFactorChange( scaleFactor );
}

float GLWindow::scaleFactor() const
{
	return mScaleFactor;
}

