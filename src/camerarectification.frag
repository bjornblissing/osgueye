#version 110
// Camera rectification fragment shader

uniform sampler2D textureSampler; // Texture to be rectified
uniform vec2 imageSize; // Used to re-project pixel space to uv-space

// Camera calibration information
uniform vec2 opticalCenter; // Optical center in pixel space
uniform vec2 focalLength; // Focal length in pixel space
uniform vec2 radialDistortion; // Coefficients 
uniform vec2 tangentialDistortion; // Coefficients 

varying vec2 textureCoord; // From vertex shader

void main()
{
    vec2 opticalCenterUV = opticalCenter / imageSize;
    vec2 focalLengthUV = focalLength / imageSize;
    vec2 lensCoordinates = (textureCoord - opticalCenterUV) / focalLengthUV;

    float radiusSquared = dot(lensCoordinates, lensCoordinates);
    float radiusQuadrupled = radiusSquared * radiusSquared;

    float radialCoeff = radialDistortion.x * radiusSquared + radialDistortion.y * radiusQuadrupled;

    float dx = tangentialDistortion.x * 2.0 * lensCoordinates.x * lensCoordinates.y 
             + tangentialDistortion.y * (radiusSquared + 2.0 * lensCoordinates.x * lensCoordinates.x);
    float dy = tangentialDistortion.x * (radiusSquared + 2.0 * lensCoordinates.x * lensCoordinates.x) 
             + tangentialDistortion.y * 2.0 * lensCoordinates.x * lensCoordinates.y;
    
    vec2 tangentialCoeff = vec2(dx, dy);
    
    vec2 distortedUV = ((lensCoordinates + lensCoordinates * radialCoeff + tangentialCoeff) * focalLengthUV) + opticalCenterUV;

    gl_FragColor = texture2D(textureSampler, distortedUV);
}
