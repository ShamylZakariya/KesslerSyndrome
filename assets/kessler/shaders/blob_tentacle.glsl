vertex:
#version 150
uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec2 ciTexCoord0;

out vec2 TexCoord;

void main(void) {
    gl_Position = ciModelViewProjection * ciPosition;
    TexCoord = ciTexCoord0;
}

fragment:
#version 150

uniform sampler2D Texture;
uniform vec4 Color;

in vec2 TexCoord;

out vec4 oColor;

void main(void) {
    vec4 color = Color * texture(Texture, TexCoord);
    oColor = color;
}
