#version 330 core

out vec4 color;

uniform vec3 MatAmb;
uniform vec3 MatDif;

//interpolated normal and light vector in camera space
in vec3 fragNor;
in vec3 lightDir;
//position of the vertex in camera space
in vec3 EPos;

void main() {
    //you will need to work with these for lighting
    vec3 normal = normalize(fragNor);
    vec3 light = normalize(lightDir);
    vec3 dC = max(normal * light, 0);

    color = vec4(MatAmb + dC * MatDif, 1.0);
}
