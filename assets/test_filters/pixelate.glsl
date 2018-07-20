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
uniform ivec2 ciWindowSize;

uniform sampler2D ColorTex;
uniform float PixelSize;

in vec2 TexCoord;

out vec4 oColor;

void main(void) {
    vec2 coord = round((TexCoord * ciWindowSize) / PixelSize) * (PixelSize / ciWindowSize);
    vec4 tex = texture(ColorTex, coord);
    oColor = tex;
}