#version 330 core
uniform sampler2D Texture0;

uniform vec3 lightPos;

in vec2 vTexCoord;
in vec3 fragNor;
in vec3 EPos;
out vec4 Outcolor;

void main() {
    vec4 texColor0 = texture(Texture0, vTexCoord);

    vec3 normal = normalize(fragNor);
    vec3 light = normalize(lightPos - EPos);
    float diffColor = max(0.0, dot(normal, light));

    //to set the out color as the texture color
    Outcolor = texColor0 * diffColor;

    if(Outcolor.x < 0.0001) {
        discard;
    }

    //to set the outcolor as the texture coordinate (for debugging)
    //Outcolor = vec4(vTexCoord.s, vTexCoord.t, 0, 1);
}
