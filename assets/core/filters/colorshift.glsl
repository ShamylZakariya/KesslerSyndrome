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
#version 150
uniform float Alpha;
uniform sampler2D ColorTex;
uniform vec2 ColorTexSize;
uniform vec2 ColorTexSizeInverse;

uniform vec4 Offset;
uniform vec4 Multiplier;

in vec2 TexCoord;

out vec4 oColor;

void main(void) {
    vec4 tex = texture(ColorTex, TexCoord);
    oColor = mix(tex, Offset + (tex * Multiplier), Alpha);
}