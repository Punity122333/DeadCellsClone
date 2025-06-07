#version 330

uniform sampler2D texture0;
uniform float aberrationAmount = 0.005; 
in vec2 fragTexCoord;
out vec4 finalColor;

void main()
{
    vec2 uv = fragTexCoord;
    vec3 col;
    col.r = texture(texture0, vec2(uv.x + aberrationAmount, uv.y)).r;
    col.g = texture(texture0, uv).g;
    col.b = texture(texture0, vec2(uv.x - aberrationAmount, uv.y)).b;
    finalColor = vec4(col, 1.0);
}