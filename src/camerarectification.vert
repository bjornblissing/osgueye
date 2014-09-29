#version 110
// Camera rectification vertex shader


varying vec2 textureCoord; // To fragment shader

void main()
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    textureCoord = gl_MultiTexCoord0.xy;
}
