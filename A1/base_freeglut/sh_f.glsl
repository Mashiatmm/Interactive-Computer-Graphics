#version 330

smooth in vec2 fragNorm;	// Interpolated model-space normal

uniform sampler2D tex;

out vec3 outCol;	// Final pixel color

void main() {
	// Visualize normals as colors
	// outCol = normalize(fragNorm) * 0.5f + vec3(0.5f);
	outCol = texture(tex, fragNorm).rgb;
}