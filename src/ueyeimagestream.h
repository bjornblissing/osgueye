/*
 * ueyeimagestream.h
 *
 *  Created on: Aug 16, 2013
 *      Author: Bjorn Blissing
 */

#ifndef _U_EYE_IMAGESTREAM_H_
#define _U_EYE_IMAGESTREAM_H_

#include <osg/ImageStream>
#include <uEye.h>

#define MAX_SEQ_BUFFERS 32767

class UEyeImageStream : public osg::ImageStream {
	public:
		UEyeImageStream(bool vSyncEnabled=true, size_t frameDelay=1);
		~UEyeImageStream();

		bool openCamera(const std::string& name);
		bool openCamera(unsigned long cameraId);
		double getFrameRate() const { return m_actualFrameRate; }
		int sensorSizeX() const { return m_sensorSizeX; }
		int sensorSizeY() const { return m_sensorSizeY; }
		float aspectRatio() const { return float(m_sensorSizeX)/float(m_sensorSizeY); }

		/** ImageSequence requires a call to update(NodeVisitor*) during the update traversal so return true.*/
		virtual bool requiresUpdateCall() const { return true; }
		virtual void update(osg::NodeVisitor* nv);

	private:
		bool m_init;
		bool m_memoryAllocated;
		bool m_cameraStarted;
		bool m_vSyncEnabled;
		int m_sensorSizeX;
		int m_sensorSizeY;
		int m_bitsPerPixel;
		HIDS m_cameraId;
		double m_actualFrameRate;
		int	   m_sequenceMemoryId[MAX_SEQ_BUFFERS];			// camera memory - buffer ID
		char*  m_sequenceMememyPointer[MAX_SEQ_BUFFERS];	// camera memory - pointer to buffer
		int    m_sequenceNumberId[MAX_SEQ_BUFFERS];			// variable to hold the number of the sequence buffer Id
		size_t m_numberOfFrames;
		size_t m_oldestFrame;
};

#endif
