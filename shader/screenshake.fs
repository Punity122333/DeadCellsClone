#version 330

uniform sampler2D texture0;
uniform vec2 shakeOffset = vec2(0.0, 0.0);
uniform float shakeIntensity = 0.0;
in vec2 fragTexCoord;
out vec4 finalColor;

void main()
{
    vec2 uv = fragTexCoord;

    uv += shakeOffset * shakeIntensity;

    uv = clamp(uv, 0.0, 1.0);

    vec3 col = texture(texture0, uv).rgb;

    if (shakeIntensity > 0.01) {
        float aberration = shakeIntensity * 0.003;
        col.r = texture(texture0, vec2(uv.x + aberration, uv.y)).r;
        col.b = texture(texture0, vec2(uv.x - aberration, uv.y)).b;
    }
    
    finalColor = vec4(col, 1.0);
}
