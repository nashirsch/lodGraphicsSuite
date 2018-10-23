#version 410

layout (location = 0) in vec3 position;

uniform mat4 viewMat;
uniform mat4 projMat;

out vec3 f_cubeCoord;

void main(){

    gl_Position = (projMat * viewMat * vec4(position, 1.0)).xyww;
    f_cubeCoord = position;

}
