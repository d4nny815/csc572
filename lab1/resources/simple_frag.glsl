#version 330 core 

out vec4 color;

uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float MatShine;

//interpolated normal and light vector in camera space
in vec3 fragNor;
in vec3 lightDir;
//position of the vertex in camera space
in vec3 EPos;

void main()
{
	vec3 normal = normalize(fragNor);
	// if (flip < 1) normal = -normal;

	vec3 light = normalize(lightDir);
	float dC = max(0, dot(normal, light));

  	vec3 halfway = normalize(-EPos) + normalize(light);
	float sC = max(pow(dot(normalize(halfway), normal), MatShine), 0.0);

	color = vec4(MatAmb + dC * MatDif + sC * MatSpec, 1.0);
}
