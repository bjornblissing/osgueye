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

bool UEyeImageStream::openCamera(const std::string& name) 
{
	static const int eepromSize = 64;
	// Get number of cameras
	INT nNumCam;
	if( is_GetNumberOfCameras( &nNumCam ) == IS_SUCCESS) {
		// Must have at least one camera connected
		if( nNumCam > 0 ) {
			// Create new list with suitable size
			UEYE_CAMERA_LIST* cameraList;
			cameraList = (UEYE_CAMERA_LIST*) new BYTE [sizeof (DWORD) + nNumCam * sizeof (UEYE_CAMERA_INFO)];
			cameraList->dwCount = nNumCam;
	
			if (is_GetCameraList( cameraList ) == IS_SUCCESS) {
				// Loop over available cameras
				for (DWORD listId = 0; listId < cameraList->dwCount; ++listId) {
					UEYE_CAMERA_INFO cameraInfo = cameraList->uci[listId];
					osg::notify(osg::DEBUG_INFO) << "CameraId: " << cameraInfo.dwCameraID << std::endl;
					osg::notify(osg::DEBUG_INFO) << "DeviceId: " << cameraInfo.dwDeviceID << std::endl;
					osg::notify(osg::DEBUG_INFO) << "Model   : " << cameraInfo.Model << std::endl;
					osg::notify(osg::DEBUG_INFO) << "Serial  : " << cameraInfo.SerNo << std::endl;
					// Check if camera is not being used
					if (cameraInfo.dwInUse == 0) {
						HIDS cameraHandle = cameraInfo.dwDeviceID | IS_USE_DEVICE_ID;
						int statusCode = is_InitCamera (&cameraHandle, NULL);
						if (statusCode == IS_SUCCESS) {
							// Read EEPROM string
							char pcString[eepromSize];
							is_ReadEEPROM(cameraHandle, 0, pcString, eepromSize);
							std::string eepromName(pcString);
							// Disconnect camera
							is_ExitCamera (cameraHandle);
							// Check if name is correct
							if (name.compare(eepromName) == 0) {
								delete[] cameraList;
								return openCamera(cameraHandle);
							}
						}
					}
				}
			}
			osg::notify(osg::WARN) << "Warning: No camera found with name: " << name << std::endl;
			delete[] cameraList;
			return false;
		}
		osg::notify(osg::WARN) << "Warning: No cameras connected"<< std::endl;
		return false;
	}
	osg::notify(osg::WARN) << "Warning: Unable to enumerate cameras" << std::endl;
	return false;
}

bool UEyeImageStream::openCamera(unsigned long cameraId)
{
	m_cameraId = cameraId;
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
			osg::notify(osg::DEBUG_INFO) << "Pixel clock: " << maximumPixelClock << std::endl;
			//unsigned int incrementPixelClock = nRange[2];
			nRet = is_PixelClock(m_cameraId, IS_PIXELCLOCK_CMD_SET, (void*)&maximumPixelClock, sizeof(maximumPixelClock));
			if (nRet != IS_SUCCESS) {
				osg::notify(osg::WARN) << "WARNING! Failed to set pixel clock to: " << maximumPixelClock <<  std::endl;
			}
			// Set desired frame rate
			int desiredFrameRate = 60;
			nRet = is_SetFrameRate(m_cameraId, desiredFrameRate, &m_actualFrameRate);
			if (nRet != IS_SUCCESS) {
				osg::notify(osg::WARN) << "WARNING! Failed to set frame rate to: " << desiredFrameRate <<  std::endl;
				osg::notify(osg::WARN) << "\tActual frame rate: \t" << m_actualFrameRate << std::endl;
			}
			
			if (osg::round(m_actualFrameRate) != desiredFrameRate) {
				osg::notify(osg::WARN) << "WARNING! Could not set correct frame rate!"<< std::endl;
				osg::notify(osg::WARN) << "\tDesired frame rate:\t" << desiredFrameRate << std::endl;
				osg::notify(osg::WARN) << "\tActual frame rate: \t" << m_actualFrameRate << std::endl;
			}
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
		double disableValue = 0.0;
		UEYE_AUTO_INFO autoInfo;

		if (is_GetAutoInfo(m_cameraId, &autoInfo) == IS_SUCCESS) {
			// Sensor white balance is supported
			if (autoInfo.AutoAbility & AC_SENSOR_WB) {
				osg::notify(osg::DEBUG_INFO) << "Sensor white balance supported" << std::endl;
				if (is_SetAutoParameter(m_cameraId, IS_SET_ENABLE_AUTO_SENSOR_WHITEBALANCE, &disableValue, NULL) == IS_SUCCESS) {
					osg::notify(osg::DEBUG_INFO) << "Sensor auto white balance disabled" << std::endl;
				} else {
					osg::notify(osg::DEBUG_INFO) << "Sensor auto white balance could not be disabled" << std::endl;
				}
			}
			// Sensor white balance is not supported
			else {
				if (autoInfo.AutoAbility & AC_WHITEBAL) {
					osg::notify(osg::DEBUG_INFO) << "Sensor white balance supported in software" << std::endl;
					// Try to disable software white balance
					if (is_SetAutoParameter(m_cameraId, IS_SET_ENABLE_AUTO_WHITEBALANCE, &disableValue, NULL) == IS_SUCCESS) {
						osg::notify(osg::DEBUG_INFO) << "Software auto white balance disabled" << std::endl;
					} else {
						osg::notify(osg::DEBUG_INFO) << "Software auto white balance could not be disabled" << std::endl;
					}
				}
			}
		}

		// Check if camera can be set to free run, aka run at full speed
		if (is_SetExternalTrigger (m_cameraId,IS_SET_TRIGGER_SOFTWARE) != IS_SUCCESS) {
			osg::notify(osg::WARN) << "Error: Failed to set camera in free run mode!" << std::endl;

			// free buffers
			for (size_t i = 0; i < m_numberOfFrames; ++i) {
				is_FreeImageMem( m_cameraId, m_sequenceMememyPointer[i], m_sequenceMemoryId[i] );
			}

			is_ExitCamera (m_cameraId);
			osg::notify(osg::WARN) << "Warning: Unable to set camera in free run mode: " << m_cameraId << std::endl;
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
		osg::notify(osg::WARN) << "Warning: Unable to init camera: " << m_cameraId << std::endl;
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