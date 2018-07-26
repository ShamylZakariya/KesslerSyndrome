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

uniform float PixelSize;

in vec2 TexCoord;

out vec4 oColor;

void main(void) {
    float pixelSize = round(mix(1.0f,PixelSize, Alpha));
    vec2 pixelateCoord = round((TexCoord * ColorTexSize) / pixelSize) * (pixelSize / ColorTexSize);
    vec4 tex = texture(ColorTex, pixelateCoord);
    oColor = tex;
}