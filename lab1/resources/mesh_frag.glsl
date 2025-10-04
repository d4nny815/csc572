#version 330 core
in vec3 vNorVS;
out vec4 FragColor;
void main() {
    vec3 L = normalize(vec3(0.6,0.7,0.4));
    float d = clamp(dot(normalize(vNorVS), L), 0.1, 1.0);
    FragColor = vec4(vec3(0.22,0.45,0.9)*d, 1.0);
    // FragColor = vec4(vec3(0.22,0.45,0.9)*d, 1.0);
}

