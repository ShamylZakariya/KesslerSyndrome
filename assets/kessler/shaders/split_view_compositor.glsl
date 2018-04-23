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
uniform vec4 Tint;
uniform vec2 Side;

in vec2 TexCoord;

out vec4 oColor;

void main(void) {
    vec4 tex = texture(ColorTex, TexCoord);
    oColor = Tint * tex;

    vec2 sideDir = TexCoord - vec2(0.5,0.5);
    float d = dot(sideDir, Side);
    oColor.a = step(0, d);
}