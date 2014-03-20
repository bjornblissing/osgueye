/*
 * ueyeimagestream.cpp
 *
 *  Created on: Aug 16, 2013
 *      Author: Bjorn Blissing
 */
#include "ueyeimagestream.h"
#include <iostream>

UEyeImageStream::UEyeImageStream(bool vSyncEnabled, size_t frameDelay) : m_init(false),
	m_memoryAllocated(false),
	m_cameraStarted(false),
	m_sensorSizeX(0),
	m_sensorSizeY(0),
	m_bitsPerPixel(0),
	m_cameraId(0),
	m_actualFrameRate(0.0),
	m_sequenceMemoryId(),
	m_sequenceMememyPointer(),
	m_sequenceNumberId(),
	m_vSyncEnabled(vSyncEnabled),
	m_numberOfFrames(frameDelay),
	m_oldestFrame(0)
{
}

UEyeImageStream::~UEyeImageStream()
{
	if (m_memoryAllocated) {
		// free buffers
		for (size_t i = 0; i < m_numberOfFrames; ++i) {
			is_FreeImageMem( m_cameraId, m_sequenceMememyPointer[i], m_sequenceMemoryId[i] );
		}
	}

	if (m_cameraStarted) {
		is_ExitCamera (m_cameraId);
	}
}

bool UEyeImageStream::openCamera(unsigned long id)
{
	int numerOfAvailableCameras;

	if ( is_GetNumberOfCameras( &numerOfAvailableCameras ) == IS_SUCCESS) {
		osg::notify(osg::DEBUG_INFO) << "Number of cameras found: " << numerOfAvailableCameras << std::endl;

		// If higher camera requested then available
		if (id > unsigned long(numerOfAvailableCameras)) { return false; }
	}

	// If cameraid is zero use first available camera else use camera by id.
	m_cameraId = (id > unsigned long(0)) ? (unsigned long) id | IS_USE_DEVICE_ID : id;

	// Try to init camera
	if (is_InitCamera (&m_cameraId, NULL) == IS_SUCCESS) {
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

		for (size_t i = 0; i < m_numberOfFrames; ++i) {
			is_AllocImageMem (m_cameraId, m_sensorSizeX, m_sensorSizeY, m_bitsPerPixel,&m_sequenceMememyPointer[i], &m_sequenceMemoryId[i]);
			is_AddToSequence(m_cameraId, m_sequenceMememyPointer[i], m_sequenceMemoryId[i] );
		}

		// Connect memory to sensor image
		m_memoryAllocated = true;
		// Set color mode
		is_SetColorMode(m_cameraId, IS_CM_RGB8_PACKED);
		double dblVal = 1.0;
		UEYE_AUTO_INFO autoInfo;

		if (is_GetAutoInfo(m_cameraId, &autoInfo) == IS_SUCCESS) {
			// Sensor Whitebalance is supported
			if (autoInfo.AutoAbility & AC_SENSOR_WB) {
				osg::notify(osg::DEBUG_INFO) << "Sensor whitebalance supported" << std::endl;
				is_SetAutoParameter(m_cameraId, IS_SET_ENABLE_AUTO_SENSOR_WHITEBALANCE, &dblVal, NULL);
			}
			// Sensor Whitebalance is not supported
			else {
				if (autoInfo.AutoAbility & AC_WHITEBAL) {
					osg::notify(osg::DEBUG_INFO) << "Sensor whitebalance supported in software" << std::endl;
					// Try to activate software whitebalance
					is_SetAutoParameter(m_cameraId, IS_SET_ENABLE_AUTO_WHITEBALANCE, &dblVal, NULL);
				}
			}
		}

		// Check if camera can be set to free run, aka run at full speed
		if (is_SetExternalTrigger (m_cameraId,IS_SET_TRIGGER_SOFTWARE) != IS_SUCCESS) {
			osg::notify(osg::WARN) << "Error: Failed to set camera in freerun mode!" << std::endl;

			// free buffers
			for (size_t i = 0; i < m_numberOfFrames; ++i) {
				is_FreeImageMem( m_cameraId, m_sequenceMememyPointer[i], m_sequenceMemoryId[i] );
			}

			is_ExitCamera (m_cameraId);
			exit(EXIT_FAILURE);
			return false;
		}

		// Start image capture
		if (m_vSyncEnabled) {
			// Acquire image from camera
			is_FreezeVideo(m_cameraId, IS_DONT_WAIT);
		} else {
			// Live capture, i.e. connect memory to live video stream, but no vertical sync
			is_CaptureVideo(m_cameraId, IS_DONT_WAIT);
		}

		m_cameraStarted = true;
		// Set image
		this->setImage(m_sensorSizeX, m_sensorSizeY, 1, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, (BYTE*)(m_sequenceMememyPointer[m_oldestFrame]), osg::Image::NO_DELETE,1);
	} else {
		return false;
	}

	m_init = true;
	return true;
}

void UEyeImageStream::update(osg::NodeVisitor* /*nv*/)
{
	if (m_init) {
		if (m_vSyncEnabled) {
			// Acquire image from camera
			is_FreezeVideo(m_cameraId, IS_DONT_WAIT);
		}

		int currentImageNumber;
		char* imageMemory, *imageMemoryLast;
		is_GetActSeqBuf(m_cameraId, &currentImageNumber, &imageMemory, &imageMemoryLast);
		size_t i;

		for (i=0 ; i<m_numberOfFrames ; i++) {
			if (imageMemory == m_sequenceMememyPointer[i] ) {
				break;
			}
		}

		m_oldestFrame = (i + 1) % m_numberOfFrames;
		this->setImage(m_sensorSizeX, m_sensorSizeY, 1, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, (BYTE*)(m_sequenceMememyPointer[m_oldestFrame]), osg::Image::NO_DELETE,1);
	}
}