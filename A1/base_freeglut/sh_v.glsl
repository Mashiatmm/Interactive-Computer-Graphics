#version 330

layout(location = 0) in vec2 pos;		// Model-space position
layout(location = 1) in vec2 norm;		// Model-space normal

smooth out vec2 fragNorm;	// Model-space interpolated normal

uniform mat4 xform;			// Model-to-clip space transform

void main() {
	// Transform vertex position
	gl_Position = xform * vec4(pos, 0.0, 1.0);

	// Interpolate normals
	fragNorm = norm;
}