#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gBufferTexture;

void main() {
    vec3 color = texture(gBufferTexture, TexCoords).rgb;
    FragColor = vec4(color, 1.0); // Display the texture contents
}
