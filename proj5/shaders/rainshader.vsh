#version 410

layout (location = 0) in vec3 verts;
layout (location = 1) in vec3 position;

uniform mat4 viewMat;
uniform mat4 projMat;

out vec4 vColor;

void main(){

    gl_Position = projMat * viewMat * vec4(position + verts , 1.0);
    vColor = vec4(0.5, 0.5, 1.0, 0.6);

}
