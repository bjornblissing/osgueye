/*
 * camerarectification.cpp
 *
 *  Created on: Oct , 2014
 *      Author: Bjorn Blissing
 */
#include "camerarectification.h"
#include <osg/Program>
#include <osg/Shader>


const std::string CameraRectification::m_rectificationVertexSource(
	"#version 110\n"
	"// Camera rectification vertex shader\n"
	"\n"
	"varying vec2 textureCoord; // To fragment shader\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
	"    textureCoord = gl_MultiTexCoord0.xy;\n"
	"}\n"
);
const std::string CameraRectification::m_rectificationFragmentSource(
	"#version 110\n"
	"// Camera rectification fragment shader\n"
	"\n"
	"uniform sampler2D textureSampler; // Texture to be rectified\n"
	"uniform vec2 imageSize; // Used to re-project pixel space to uv-space\n"
	"\n"
	"// Camera calibration information\n"
	"uniform vec2 opticalCenter; // Optical center in pixel space\n"
	"uniform vec2 focalLength; // Focal length in pixel space\n"
	"uniform vec2 radialDistortion; // Coefficients\n"
	"uniform vec2 tangentialDistortion; // Coefficients\n"
	"\n"
	"varying vec2 textureCoord; // From vertex shader\n"
	"\n"
	"void main()\n"
	"{\n"
	"    vec2 opticalCenterUV = opticalCenter / imageSize;\n"
	"    vec2 focalLengthUV = focalLength / imageSize;\n"
	"    vec2 lensCoordinates = (textureCoord - opticalCenterUV) / focalLengthUV;\n"
	"\n"
	"    float radiusSquared = dot(lensCoordinates, lensCoordinates);\n"
	"    float radiusQuadrupled = radiusSquared * radiusSquared;\n"
	"\n"
	"    float radialCoeff = radialDistortion.x * radiusSquared + radialDistortion.y * radiusQuadrupled;\n"
	"\n"
	"    float dx = tangentialDistortion.x * 2.0 * lensCoordinates.x * lensCoordinates.y \n"
	"             + tangentialDistortion.y * (radiusSquared + 2.0 * lensCoordinates.x * lensCoordinates.x);\n"
	"    float dy = tangentialDistortion.x * (radiusSquared + 2.0 * lensCoordinates.x * lensCoordinates.x) \n"
	"             + tangentialDistortion.y * 2.0 * lensCoordinates.x * lensCoordinates.y;\n"
	"    \n"
	"    vec2 tangentialCoeff = vec2(dx, dy);\n"
	"    \n"
	"    vec2 distortedUV = ((lensCoordinates + lensCoordinates * radialCoeff + tangentialCoeff) * focalLengthUV) + opticalCenterUV;\n"
	"\n"
	"    gl_FragColor = texture2D(textureSampler, distortedUV);\n"
	"}\n"
);

void CameraRectification::applyRectification(osg::ref_ptr<osg::StateSet> stateSet) {
	osg::ref_ptr<osg::Program> cameraRectificationProgram = new osg::Program();
	osg::ref_ptr<osg::Shader> vertexShader = new osg::Shader(osg::Shader::VERTEX);
	vertexShader->setShaderSource(m_rectificationVertexSource);
	cameraRectificationProgram->addShader(vertexShader);
	osg::ref_ptr<osg::Shader> fragmentShader = new osg::Shader(osg::Shader::FRAGMENT);
	fragmentShader->setShaderSource(m_rectificationFragmentSource);
	cameraRectificationProgram->addShader(fragmentShader);
	osg::ref_ptr<osg::Uniform> opticalCenterUniform = 
		new osg::Uniform("opticalCenter", m_opticalCenter); // Optical center in pixel space
	osg::ref_ptr<osg::Uniform> focalLengthUniform = 
		new osg::Uniform("focalLength", m_focalLength); // Focal length in pixel space
	osg::ref_ptr<osg::Uniform> radialDistortionUniform = 
		new osg::Uniform("radialDistortion", m_radialDistortion); // Radial distortion coefficients
	osg::ref_ptr<osg::Uniform> tangentialDistortionUniform = 
		new osg::Uniform("tangentialDistortion", m_tangentialDistortion); // Tangential distortion coefficients
	osg::ref_ptr<osg::Uniform> imageSizeUniform = 
		new osg::Uniform("imageSize", m_imageSize); // Used to re-project pixel space to UV-space

	stateSet->addUniform(opticalCenterUniform);
	stateSet->addUniform(focalLengthUniform);
	stateSet->addUniform(radialDistortionUniform);
	stateSet->addUniform(tangentialDistortionUniform);
	stateSet->addUniform(imageSizeUniform);
	stateSet->setAttributeAndModes( cameraRectificationProgram, osg::StateAttribute::ON );
}
