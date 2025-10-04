#version 330 core
layout(location=0) in vec3 vertPos;
layout(location=1) in vec3 vertNor;
uniform mat4 M, V, P;
out vec3 vNorVS;
void main() {
    mat3 N = mat3(transpose(inverse(M)));
    vec3 nWS = normalize(N * vertNor);
    vNorVS = normalize((mat3(V) * nWS));
    gl_Position = P * V * M * vec4(vertPos,1.0);
}

