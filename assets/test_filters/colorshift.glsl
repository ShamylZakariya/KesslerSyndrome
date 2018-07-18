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
uniform sampler2D ColorTex;

in vec2 TexCoord;

out vec4 oColor;

void main(void) {
    vec4 tex = texture(ColorTex, TexCoord);
    vec4 offset = vec4(0.5,0,0.5,0);
    vec4 shift = vec4(1,0.5,0.5,1);
    oColor = offset + (tex * shift);
}