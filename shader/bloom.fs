#version 330

uniform sampler2D texture0;
in vec2 fragTexCoord;
out vec4 finalColor;

// Simple 9-sample blur kernel (with optional threshold)
vec3 blur(sampler2D tex, vec2 uv, vec2 texelSize, bool thresholded) {
    vec3 sum = vec3(0.0);
    float kernel[9] = float[](
        1.0, 2.0, 1.0,
        2.0, 4.0, 2.0,
        1.0, 2.0, 1.0
    );
    float ksum = 16.0;
    int idx = 0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++, idx++) {
            vec2 offset = vec2(x, y) * texelSize;
            vec3 color = texture(tex, uv + offset).rgb;
            if (thresholded) {
                float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
                if (brightness > 0.7)
                    sum += color * kernel[idx];
            } else {
                sum += color * kernel[idx];
            }
        }
    }
    return sum / ksum;
}

void main() {
    vec2 texelSize = 1.0 / textureSize(texture0, 0);
    vec3 original = texture(texture0, fragTexCoord).rgb;
    vec3 bloom = blur(texture0, fragTexCoord, texelSize, true) * 3.0;     
    vec3 glow  = blur(texture0, fragTexCoord, texelSize, false) * 0.25;   
    vec3 color = original + bloom + glow;

    
    float scanline = 0.92 + 0.08 * sin(fragTexCoord.y * textureSize(texture0, 0).y * 3.14159);
    color *= scanline;

    
    vec2 center = fragTexCoord - 0.5;
    float vignette = 1.0 - 0.08 * dot(center, center) * 4.0;
    color *= vignette;
    // -------------------------

    finalColor = vec4(color, 1.0);
}