/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "backends/graphics/opengl/opengl-graphics.h"
#include "common/mutex.h"

OpenGLGraphicsManager::OpenGLGraphicsManager()
	:
	_gameTexture(0), _overlayTexture(0), _mouseTexture(0),
	_screenChangeCount(0),
	_transactionMode(0)
	
	{

	memset(&_oldVideoMode, 0, sizeof(_oldVideoMode));
	memset(&_videoMode, 0, sizeof(_videoMode));
	memset(&_transactionDetails, 0, sizeof(_transactionDetails));

	_videoMode.mode = GFX_NORMAL;
	_videoMode.scaleFactor = 1;
	_videoMode.fullscreen = false;
}

OpenGLGraphicsManager::~OpenGLGraphicsManager() {

}

void OpenGLGraphicsManager::init() {

}

//
// Feature
//

bool OpenGLGraphicsManager::hasFeature(OSystem::Feature f) {
	return false;
}

void OpenGLGraphicsManager::setFeatureState(OSystem::Feature f, bool enable) {

}

bool OpenGLGraphicsManager::getFeatureState(OSystem::Feature f) {
	return false;
}

//
// Screen format and modes
//

static const OSystem::GraphicsMode s_supportedGraphicsModes[] = {
	{"1x", "Normal", GFX_NORMAL},
	{0, 0, 0}
};

const OSystem::GraphicsMode *OpenGLGraphicsManager::getSupportedGraphicsModes() const {
	return s_supportedGraphicsModes;
}

int OpenGLGraphicsManager::getDefaultGraphicsMode() const {
	return GFX_NORMAL;
}

bool OpenGLGraphicsManager::setGraphicsMode(int mode) {
	return false;
}

int OpenGLGraphicsManager::getGraphicsMode() const {
	return 0;
}

#ifdef USE_RGB_COLOR

Graphics::PixelFormat OpenGLGraphicsManager::getScreenFormat() const {
	return Graphics::PixelFormat();
}

Common::List<Graphics::PixelFormat> OpenGLGraphicsManager::getSupportedFormats() {
	return Common::List<Graphics::PixelFormat>();
}

#endif

void OpenGLGraphicsManager::initSize(uint width, uint height, const Graphics::PixelFormat *format) {
	assert(_transactionMode == kTransactionActive);

#ifdef USE_RGB_COLOR
	//avoid redundant format changes
	Graphics::PixelFormat newFormat;
	if (!format)
		newFormat = Graphics::PixelFormat::createFormatCLUT8();
	else
		newFormat = *format;

	assert(newFormat.bytesPerPixel > 0);

	if (newFormat != _videoMode.format) {
		_videoMode.format = newFormat;
		_transactionDetails.formatChanged = true;
		_screenFormat = newFormat;
	}
#endif

	// Avoid redundant res changes
	if ((int)width == _videoMode.screenWidth && (int)height == _videoMode.screenHeight)
		return;

	_videoMode.screenWidth = width;
	_videoMode.screenHeight = height;

	_transactionDetails.sizeChanged = true;
}

int OpenGLGraphicsManager::getScreenChangeID() const {
	return 0;
}

//
// GFX
//

void OpenGLGraphicsManager::beginGFXTransaction() {
	assert(_transactionMode == kTransactionNone);

	_transactionMode = kTransactionActive;
	_transactionDetails.sizeChanged = false;
	_transactionDetails.needHotswap = false;
	_transactionDetails.needUpdatescreen = false;
#ifdef USE_RGB_COLOR
	_transactionDetails.formatChanged = false;
#endif

	_oldVideoMode = _videoMode;
}

OSystem::TransactionError OpenGLGraphicsManager::endGFXTransaction() {
	int errors = OSystem::kTransactionSuccess;

	assert(_transactionMode != kTransactionNone);

	if (_transactionMode == kTransactionRollback) {
		if (_videoMode.fullscreen != _oldVideoMode.fullscreen) {
			errors |= OSystem::kTransactionFullscreenFailed;

			_videoMode.fullscreen = _oldVideoMode.fullscreen;
		/*} else if (_videoMode.aspectRatioCorrection != _oldVideoMode.aspectRatioCorrection) {
			errors |= OSystem::kTransactionAspectRatioFailed;

			_videoMode.aspectRatioCorrection = _oldVideoMode.aspectRatioCorrection;*/
		} else if (_videoMode.mode != _oldVideoMode.mode) {
			errors |= OSystem::kTransactionModeSwitchFailed;

			_videoMode.mode = _oldVideoMode.mode;
			_videoMode.scaleFactor = _oldVideoMode.scaleFactor;
#ifdef USE_RGB_COLOR
		} else if (_videoMode.format != _oldVideoMode.format) {
			errors |= OSystem::kTransactionFormatNotSupported;

			_videoMode.format = _oldVideoMode.format;
			_screenFormat = _videoMode.format;
#endif
		} else if (_videoMode.screenWidth != _oldVideoMode.screenWidth || _videoMode.screenHeight != _oldVideoMode.screenHeight) {
			errors |= OSystem::kTransactionSizeChangeFailed;

			_videoMode.screenWidth = _oldVideoMode.screenWidth;
			_videoMode.screenHeight = _oldVideoMode.screenHeight;
			_videoMode.overlayWidth = _oldVideoMode.overlayWidth;
			_videoMode.overlayHeight = _oldVideoMode.overlayHeight;
		}

		if (_videoMode.fullscreen == _oldVideoMode.fullscreen &&
			//_videoMode.aspectRatioCorrection == _oldVideoMode.aspectRatioCorrection &&
			_videoMode.mode == _oldVideoMode.mode &&
			_videoMode.screenWidth == _oldVideoMode.screenWidth &&
		   	_videoMode.screenHeight == _oldVideoMode.screenHeight) {

			// Our new video mode would now be exactly the same as the
			// old one. Since we still can not assume SDL_SetVideoMode
			// to be working fine, we need to invalidate the old video
			// mode, so loadGFXMode would error out properly.
			_oldVideoMode.setup = false;
		}
	}

#ifdef USE_RGB_COLOR
	if (_transactionDetails.sizeChanged || _transactionDetails.formatChanged) {
#else
	if (_transactionDetails.sizeChanged) {
#endif
		unloadGFXMode();
		if (!loadGFXMode()) {
			if (_oldVideoMode.setup) {
				_transactionMode = kTransactionRollback;
				errors |= endGFXTransaction();
			}
		} else {
			//setGraphicsModeIntern();
			//clearOverlay();

			_videoMode.setup = true;
			_screenChangeCount++;
		}
	} else if (_transactionDetails.needHotswap) {
		//setGraphicsModeIntern();
		if (!hotswapGFXMode()) {
			if (_oldVideoMode.setup) {
				_transactionMode = kTransactionRollback;
				errors |= endGFXTransaction();
			}
		} else {
			_videoMode.setup = true;
			_screenChangeCount++;

			if (_transactionDetails.needUpdatescreen)
				internUpdateScreen();
		}
	} else if (_transactionDetails.needUpdatescreen) {
		//setGraphicsModeIntern();
		internUpdateScreen();
	}

	_transactionMode = kTransactionNone;
	return (OSystem::TransactionError)errors;
}

//
// Screen
//

int16 OpenGLGraphicsManager::getHeight() {
	return _videoMode.screenHeight;
}

int16 OpenGLGraphicsManager::getWidth() {
	return _videoMode.screenWidth;
}

void OpenGLGraphicsManager::setPalette(const byte *colors, uint start, uint num) {

}

void OpenGLGraphicsManager::grabPalette(byte *colors, uint start, uint num) {

}

void OpenGLGraphicsManager::copyRectToScreen(const byte *buf, int pitch, int x, int y, int w, int h) {

}

Graphics::Surface *OpenGLGraphicsManager::lockScreen() {
	_lockedScreen = Graphics::Surface();
	return &_lockedScreen;
}

void OpenGLGraphicsManager::unlockScreen() {

}

void OpenGLGraphicsManager::fillScreen(uint32 col) {

}

void OpenGLGraphicsManager::updateScreen() {

}

void OpenGLGraphicsManager::setShakePos(int shakeOffset) {

}

void OpenGLGraphicsManager::setFocusRectangle(const Common::Rect& rect) {

}

void OpenGLGraphicsManager::clearFocusRectangle() {

}

//
// Overlay
//

void OpenGLGraphicsManager::showOverlay() {

}

void OpenGLGraphicsManager::hideOverlay() {

}

Graphics::PixelFormat OpenGLGraphicsManager::getOverlayFormat() const {
	return Graphics::PixelFormat();
}

void OpenGLGraphicsManager::clearOverlay() {

}

void OpenGLGraphicsManager::grabOverlay(OverlayColor *buf, int pitch) {

}

void OpenGLGraphicsManager::copyRectToOverlay(const OverlayColor *buf, int pitch, int x, int y, int w, int h) {

}

int16 OpenGLGraphicsManager::getOverlayHeight() {
	return _videoMode.overlayHeight;
}

int16 OpenGLGraphicsManager::getOverlayWidth() {
	return _videoMode.overlayWidth;
}

//
// Cursor
//

bool OpenGLGraphicsManager::showMouse(bool visible) {
	return false;
}

void OpenGLGraphicsManager::warpMouse(int x, int y) {

}

void OpenGLGraphicsManager::setMouseCursor(const byte *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keycolor, int cursorTargetScale, const Graphics::PixelFormat *format) {

}

void OpenGLGraphicsManager::setCursorPalette(const byte *colors, uint start, uint num) {

}

void OpenGLGraphicsManager::disableCursorPalette(bool disable) {

}

//
// Misc
//

void OpenGLGraphicsManager::displayMessageOnOSD(const char *msg) {

}

//
// Intern
//

void OpenGLGraphicsManager::internUpdateScreen() {

}

bool OpenGLGraphicsManager::loadGFXMode() {
	return false;
}

void OpenGLGraphicsManager::unloadGFXMode() {

}

bool OpenGLGraphicsManager::hotswapGFXMode() {
	return false;
}
