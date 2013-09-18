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


class UEyeImageStream : public osg::ImageStream {
public:
	UEyeImageStream(bool vSyncEnabled=true);
	~UEyeImageStream();
	
	bool openCamera(unsigned long id=0);
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
	int m_memoryId;
	unsigned long m_cameraId;
	double m_actualFrameRate;
	char* m_imageMemory;
};

#endif 
