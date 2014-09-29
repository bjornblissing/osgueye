/*
 * main.cpp
 *
 *  Created on: Aug 16, 2013
 *      Author: Bjorn Blissing
 */

#include <iostream>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osg/PolygonMode>
#include <osg/Texture2D>
#include <osg/Shader>
#include <osgGA/StateSetManipulator>

#include "ueyeimagestream.h"


osg::Geode* createHUDQuad( float width, float height)
{
	osg::ref_ptr<osg::Geometry> geom = osg::createTexturedQuadGeometry(osg::Vec3(0.0f,0.0f,0.0f),
									   osg::Vec3(width,0.0f,0.0f),
									   osg::Vec3(0.0f,0.0f,height),
									   0.0f,
									   1.0f,
									   1.0f,
									   0.0f);
	osg::ref_ptr<osg::Geode> quad = new osg::Geode;
	quad->addDrawable( geom );
	int values = osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED;
	quad->getOrCreateStateSet()->setAttribute(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL), values );
	quad->getOrCreateStateSet()->setMode( GL_LIGHTING, values );
	return quad.release();
}

osg::Geode* createCameraPlane(unsigned long cameraId) {
	osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
	texture->setTextureSize( 1280, 1024);
	texture->setFilter(osg::Texture::MIN_FILTER , osg::Texture::LINEAR);
	texture->setFilter(osg::Texture::MAG_FILTER , osg::Texture::LINEAR);
	texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
	texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
	texture->setResizeNonPowerOfTwoHint(false);
	// Create uEyeImageStream
	osg::ref_ptr<UEyeImageStream> uEyeImageStream = new UEyeImageStream(false);
	uEyeImageStream->openCamera(cameraId);
	texture->setImage(uEyeImageStream);
	osg::ref_ptr<osg::Geode> quadGeode = createHUDQuad(uEyeImageStream->aspectRatio(), 1.0f);

	// Apply texture to quad
	osg::ref_ptr<osg::StateSet> stateSet = quadGeode->getOrCreateStateSet();
	stateSet->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);

	// Use camera rectification
	osg::ref_ptr<osg::Program> cameraRectificationProgram = new osg::Program();
	osg::ref_ptr<osg::Shader> vertexShader = new osg::Shader(osg::Shader::VERTEX);
	vertexShader->loadShaderSourceFromFile("camerarectification.vert");
	cameraRectificationProgram->addShader(vertexShader);
	osg::ref_ptr<osg::Shader> fragmentShader = new osg::Shader(osg::Shader::FRAGMENT);
	fragmentShader->loadShaderSourceFromFile("camerarectification.frag");
	cameraRectificationProgram->addShader(fragmentShader);
	osg::ref_ptr<osg::Uniform> opticalCenterUniform = 
		new osg::Uniform("opticalCenter", osg::Vec2(619.876, 500.871)); // Optical center in pixel space
	osg::ref_ptr<osg::Uniform> focalLengthUniform = 
		new osg::Uniform("focalLength", osg::Vec2(1130.81, 1130.7)); // Focal length in pixel space
	osg::ref_ptr<osg::Uniform> radialDistortionUniform = 
		new osg::Uniform("radialDistortion", osg::Vec2(-0.189419, 0.228431)); // Radial distortion coefficients
	osg::ref_ptr<osg::Uniform> tangentialDistortionUniform = 
		new osg::Uniform("tangentialDistortion", osg::Vec2(0.000941997, 3.85398e-05)); // Tangential distortion coefficients
	osg::ref_ptr<osg::Uniform> imageSizeUniform = 
		new osg::Uniform("imageSize", osg::Vec2(1280.0, 1024.0)); // Used to re-project pixel space to UV-space

	stateSet->addUniform(opticalCenterUniform);
	stateSet->addUniform(focalLengthUniform);
	stateSet->addUniform(radialDistortionUniform);
	stateSet->addUniform(tangentialDistortionUniform);
	stateSet->addUniform(imageSizeUniform);
	stateSet->setAttributeAndModes( cameraRectificationProgram, osg::StateAttribute::ON );

	return quadGeode.release();	
}

int main( int argc, char** argv )
{
	// use an ArgumentParser object to manage the program arguments.
	osg::ArgumentParser arguments(&argc,argv);
	// Create viewer
	osgViewer::Viewer viewer(arguments);
	osg::ref_ptr<osg::Group> root = new osg::Group;
	root->addChild(createCameraPlane(1));
	
	viewer.addEventHandler(new osgViewer::StatsHandler());
	viewer.setSceneData(root);
	// Start Viewer
	viewer.run();
	return 0;
}
