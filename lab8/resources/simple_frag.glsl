#version 330 core

out vec4 color;

uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float MatShine;

uniform vec3 lightPos;
uniform vec3 lightColor;

//interpolated normal and light vector in camera space
in vec3 fragNor;
// in vec3 lightDir;
//position of the vertex in camera space
in vec3 EPos;

void main() {
    //you will need to work with these for lighting
    vec3 normal = normalize(fragNor);
    vec3 light = normalize(lightPos - EPos);
    float diffColor = max(0.0, dot(normal, light));

    vec3 viewVec = normalize(-EPos);
    vec3 halfVec = normalize(light + viewVec);
    float specColor = pow(max(0.0, dot(normal, halfVec)), MatShine);

    color = vec4(lightColor * (MatDif * diffColor + MatSpec * specColor + MatAmb), 1.0);
}
