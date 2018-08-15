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
uniform sampler2D Tonemap;
uniform sampler2D BackgroundFill;
uniform vec2 Aspect;
uniform float BackgroundFillRepeat;
uniform vec4 HighlightColor;

in vec2 TexCoord;

out vec4 oColor;

void main(void) {
    vec4 tex = texture(ColorTex, TexCoord);
    vec4 blobColor = texture(Tonemap, vec2(1 - (tex.a * Alpha),0));
    vec4 backgroundColor = texture(BackgroundFill, TexCoord * Aspect * BackgroundFillRepeat);
    blobColor.rgb *= backgroundColor.rgb;

    // add a highlight color
    blobColor.rgb += HighlightColor.rgb * HighlightColor.a * step(0.975, tex.a);

    oColor = blobColor;
}