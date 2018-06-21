vertex:
#version 150
uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec4 ciColor;

out vec4 Color;

void main(void) {
    gl_Position = ciModelViewProjection * ciPosition;
    Color = ciColor;
}

fragment:
#version 150

in vec4 Color;

out vec4 oColor;

void main(void) {
    oColor = Color;
}
