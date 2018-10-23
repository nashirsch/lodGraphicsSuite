#version 410

layout (location = 0) in vec4 position;

uniform mat4 viewMat;
uniform mat4 projMat;

uniform vec4 scaling;
uniform vec3 Opos;
uniform vec4 color;

out vec4 vColor;

void main(){

    vec3 CamRelativeWorldSpace = vec3(position.x * scaling.x,
                                      position.y * scaling.y + position.w * scaling.w,
                                      position.z * scaling.z);

    CamRelativeWorldSpace = CamRelativeWorldSpace + Opos;

    gl_Position = projMat * viewMat * vec4(CamRelativeWorldSpace, 1.0);

    vColor = color;
    
}
