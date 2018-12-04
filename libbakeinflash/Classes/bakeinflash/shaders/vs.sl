//
// Vertex Shader
//

attribute vec2 position;
attribute vec2 texcoords;

uniform mat4 modelViewProjectionMatrix;
uniform vec4 modelColor;
uniform vec2 modelMode;

varying vec4 color;
varying vec2 texCoords;
varying vec2 mode;

void main()
{
	color = modelColor;
	mode = modelMode;
	texCoords = texcoords;
	gl_Position = modelViewProjectionMatrix * vec4(position.x, position.y, 0, 1);
}


