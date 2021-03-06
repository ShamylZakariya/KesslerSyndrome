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
uniform sampler2D ColorTex0;
uniform sampler2D ColorTex1;
uniform vec4 Tint0;
uniform vec4 Tint1;
uniform vec2 Side;
uniform float ShadowWidth;
uniform vec4 ShadowColor;

in vec2 TexCoord;

out vec4 oColor;

void main(void) {
    vec4 color0 = Tint0 * texture(ColorTex0, TexCoord);
    vec4 color1 = Tint1 * texture(ColorTex1, TexCoord);

    vec2 sideDir = TexCoord - vec2(0.5,0.5);
    float d = dot(sideDir, Side);

    oColor = mix(color0, color1, step(0, d));
    oColor.a = 1;

    if (d > 0) {
        vec2 line = vec2(-Side.y, Side.x);
        d = dot(sideDir, line);
        vec2 projectedToSplit = vec2(0.5,0.5) + d * line;
        float distanceFromSplit = distance(TexCoord, projectedToSplit) * 2;
        float shadow = 1 - min((distanceFromSplit / ShadowWidth), 1);
        oColor.rgb = mix(oColor.rgb, ShadowColor.rgb, shadow * ShadowColor.a);
    }
}