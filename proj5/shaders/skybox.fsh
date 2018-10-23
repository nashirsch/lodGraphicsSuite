#version 410

uniform samplerCube cubeMap;

out vec3 fragColor;

in vec3 f_cubeCoord;

void main(){

    fragColor = vec3(texture(cubeMap, f_cubeCoord));

}
