#version 330 core
uniform sampler2D Texture0;
uniform int flip;
uniform float MatShine;

in vec3 fragNor;
in vec3 lightDir;
in vec3 EPos;
in vec2 vTexCoord;
out vec4 Outcolor;

void main() {
  vec4 texColor0 = texture(Texture0, vTexCoord);

  vec3 normal = normalize(fragNor);
  if (flip < 1) normal = -normal;

	vec3 light = normalize(lightDir);
	float dC = max(0, dot(normal, light));

  vec3 halfway = normalize(-EPos) + normalize(light);
	float sC = max(pow(dot(normalize(halfway), normal), MatShine), 0.0);

  //to set the out color as the texture color 
  Outcolor = dC * texColor0 + sC * texColor0;
  //if (Outcolor.b > .7) {
    //Outcolor = vec4(1, Outcolor.g * .6, 0, 1);
  //}

  //to set the outcolor as the texture coordinate (for debugging)
  //Outcolor = vec4(vTexCoord.s, vTexCoord.t, 0, 1);
  // Outcolor = vec4(texColor0.r, 0, 0, 1);
  // if (Outcolor.r < .9) Outcolor.r = 0;
  // Outcolor = vec4(0, texColor0.g, 0, 1);
  // if (Outcolor.g < .7) Outcolor.g = 0;
  // Outcolor = vec4(0, 0, texColor0.b, 1);
  // if (Outcolor.b < .6) Outcolor.b = 0;
}

