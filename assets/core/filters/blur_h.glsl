vertex:
#version 150
uniform mat4 ciModelViewProjection;
uniform mat4 ciModelMatrix;

in vec4 ciPosition;
in vec2 ciTexCoord0;

out vec2 TexCoord;

void main(void) {
    gl_Position = ciModelViewProjection * ciPosition;
    TexCoord = ciTexCoord0;
}

fragment:
uniform float Alpha;
uniform sampler2D ColorTex;
uniform vec2 ColorTexSize;
uniform vec2 ColorTexSizeInverse;

uniform vec2 Kernel[__SIZE__];

in vec2 TexCoord;

out vec4 oColor;

void main( void )
{
	int i;
	vec4 sample = vec4(0);
	
	for ( i = 0; i < __SIZE__; i++ )
	{
		sample += texture( ColorTex, TexCoord + (Alpha * ColorTexSizeInverse * vec2( Kernel[i].s, 0 ))) * Kernel[i].t;
	}

	oColor = sample;
}