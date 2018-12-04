//
// Fragment Shader
//

precision lowp float;

varying vec4 color;
varying vec2 texCoords;
varying vec2 mode;

uniform sampler2D modelTexture;
 
void main()
{
	gl_FragColor = color;
	if (mode.x > 0.0)
	{
		gl_FragColor *= texture2D(modelTexture, texCoords);
	}
}
