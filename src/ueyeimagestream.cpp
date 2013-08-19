/*
 * ueyeimagestream.cpp
 *
 *  Created on: Aug 16, 2013
 *      Author: Bjorn Blissing
 */
#include "ueyeimagestream.h"
#include <iostream>

UEyeImageStream::UEyeImageStream() : m_init(false), m_cameraStarted(false), m_memoryAllocated(false)
{
}

UEyeImageStream::~UEyeImageStream() {
	if (m_memoryAllocated) {
		is_FreeImageMem (m_cameraId, m_imageMemory, m_memoryId); 
	}
	if (m_cameraStarted) {
		is_ExitCamera (m_cameraId);
	}
}

bool UEyeImageStream::openCamera(unsigned long id) {
	OPENGL_DISPLAY display;

	int numerOfAvailableCameras;

	if( is_GetNumberOfCameras( &numerOfAvailableCameras ) == IS_SUCCESS) {
		osg::notify(osg::DEBUG_INFO) << "Number of cameras found: " << numerOfAvailableCameras << std::endl;
		// If higher camera requested then available
		if (id > numerOfAvailableCameras) return false;
	}

	// If cameraid is zero use first available camera else use camera by id.
	m_cameraId = (id > 0) ? (unsigned long) id | IS_USE_DEVICE_ID : id;

	// Try to init camera
	if(is_InitCamera (&m_cameraId, NULL) == IS_SUCCESS)
	{
		SENSORINFO sInfo;
		is_GetSensorInfo(m_cameraId, &sInfo);

		m_sensorSizeX = sInfo.nMaxWidth;     
		m_sensorSizeY = sInfo.nMaxHeight; 
		
		// Set pixel clock to maximum to enable high frame rates
		unsigned int range[3];
		memset(range, 0, sizeof(range));
		// Get pixel clock range
		INT nRet = is_PixelClock(m_cameraId, IS_PIXELCLOCK_CMD_GET_RANGE, (void*)range, sizeof(range));

		if (nRet == IS_SUCCESS)	{
			//unsigned int minimumPixelClock = nRange[0];
			unsigned int maximumPixelClock = range[1];
			//unsigned int incrementPixelClock = nRange[2];
			
			is_PixelClock(m_cameraId, IS_PIXELCLOCK_CMD_SET, (void*)&maximumPixelClock, sizeof(maximumPixelClock));

			// Set desired frame rate
			int desiredFrameRate = 60;
			is_SetFrameRate(m_cameraId, desiredFrameRate, &m_actualFrameRate);
			osg::notify(osg::DEBUG_INFO) << "Actual frame rate reported: " << m_actualFrameRate << std::endl;
		}
		
		// Allocate memory for image
		m_bitsPerPixel = 24;
		is_AllocImageMem (m_cameraId, m_sensorSizeX, m_sensorSizeY, m_bitsPerPixel,&m_imageMemory, &m_memoryId); 

		// Connect memory to sensor image
		is_SetImageMem (m_cameraId, m_imageMemory, m_memoryId); 
		m_memoryAllocated = true;

		// Set color mode
		is_SetColorMode(m_cameraId, IS_CM_RGB8_PACKED);

		// Check if camera can be set to free run, aka run at full speed
		if(is_SetExternalTrigger (m_cameraId,IS_SET_TRIGGER_SOFTWARE) != IS_SUCCESS)
		{
			is_FreeImageMem (m_cameraId, m_imageMemory, m_memoryId); 
			is_ExitCamera (m_cameraId); 
			exit(EXIT_FAILURE);
			return false;
		}

		// Start image capture
		is_CaptureVideo(m_cameraId, IS_DONT_WAIT);
		m_cameraStarted = true;

		// Set image
		this->setImage(m_sensorSizeX, m_sensorSizeY, 1, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, (BYTE*)(m_imageMemory), osg::Image::NO_DELETE,1); 

	} else {
		return false;
	}
	m_init = true;
	return true;
}

void UEyeImageStream::update(osg::NodeVisitor* /*nv*/){
	if (m_init) {
		this->setImage(m_sensorSizeX, m_sensorSizeY, 1, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, (BYTE*)(m_imageMemory), osg::Image::NO_DELETE,1);  
	}
}