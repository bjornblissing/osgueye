/*
 * camerarectification.h
 *
 *  Created on: Oct , 2014
 *      Author: Bjorn Blissing
 */
#include <osg/Referenced>
#include <osg/StateSet>

class CameraRectification : public osg::Referenced {
public:
	CameraRectification() : m_opticalCenter(osg::Vec2(640.0, 480.0))
		, m_focalLength(osg::Vec2(1000.0, 1000.0))
		, m_radialDistortion(osg::Vec2(0.0, 0.0))
		, m_tangentialDistortion(osg::Vec2(0.0, 0.0))
		, m_imageSize(osg::Vec2(1280, 1024)) {};
	void setOpticalCenter(osg::Vec2 opticalCenter) { m_opticalCenter = opticalCenter; }
	void setFocalLength(osg::Vec2 focalLength) { m_focalLength = focalLength; }
	void setRadialDistortionCoefficients(osg::Vec2 radialDistortion) { m_radialDistortion = radialDistortion; }
	void setTangentialDistortionCoefficients(osg::Vec2 tangentialDistortion) { m_tangentialDistortion = tangentialDistortion; }
	void setImageSize(osg::Vec2 imageSize) { m_imageSize = imageSize; }
	void applyRectification(osg::ref_ptr<osg::StateSet> stateSet);
protected:
	~CameraRectification() {}; // Since we inherit from osg::Referenced we must make destructor protected
	osg::Vec2 m_opticalCenter;
	osg::Vec2 m_focalLength;
	osg::Vec2 m_radialDistortion;
	osg::Vec2 m_tangentialDistortion;
	osg::Vec2 m_imageSize;

	static const std::string m_rectificationVertexSource;
	static const std::string m_rectificationFragmentSource;
};