#version 330 core
layout(location=0) in vec3 vertPos;
layout(location=1) in vec3 vertNor;

uniform mat4 M, V, P;
uniform float outlineScale; // ~0.01–0.03

void main() {
    // simple uniform “inflate” (works best for centered models)
    vec3 pos = vertPos * (1.0 + outlineScale);

    // alternative for crisper outlines along normals:
    // vec3 pos = vertPos + normalize(vertNor) * outlineScale;

    gl_Position = P * V * (M * vec4(pos, 1.0));
}

