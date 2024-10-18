#version 330
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

smooth in vec3 fragNorm;	// Interpolated model-space normal
in vec3 fragPos;

out vec3 outCol;	// Final pixel color

void main() {
	// Visualize normals as colors
	gPosition = vec4(fragPos, 1.0);
	gNormal = normalize(fragNorm);
	gAlbedoSpec.rgb = vec3(0.95); // diffuse per-fragment color
}