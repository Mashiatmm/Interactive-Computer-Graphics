#version 330

smooth in vec4 fragNorm;	// Interpolated model-space normal

out vec4 outCol;	// Final pixel color

uniform int uRenderDisc;
uniform float sigma;

void main() {
	// Added by Mashiat
	vec4 color = fragNorm;
	if(uRenderDisc > 0){
		// Get the coordinates of the point within the point sprite (0, 0 to 1, 1)
		vec2 pointCoord = gl_PointCoord * 2.0 - 1.0; // Convert to (-1, -1) to (1, 1)

		// Compute the distance from the center of the point
		float distanceFromCenter = length(pointCoord);

		

		// If the distance is greater than 1, we're outside the circle, discard the fragment
		if (distanceFromCenter > 1.0)
			discard;

		// Gaussian function: f(x) = exp(- (distance^2) / (2 * sigma^2))
		if(uRenderDisc == 2){
			float gaussianFactor = exp(- (distanceFromCenter * distanceFromCenter) / (2.0 * sigma * sigma));
			color.w = gaussianFactor;
		}
    	
	}
	
	

	// Visualize normals as colors
	// outCol = normalize(fragNorm) * 0.5f + vec3(0.5f);
	outCol = color;
}